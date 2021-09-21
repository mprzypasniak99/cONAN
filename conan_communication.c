#include "conan_communication.h"
#include "conan_state.h"
#include "main.h"

queue* equipmentQueue;

void clearAcks() {
    for (int i = 0; i < CONANI; i++)
    {   
        zebrane_ack[i] = (i == rank - BIBLIOTEKARZE) ? TRUE : FALSE;
    }
}

void clearEqAcks() {
    for (int i = 0; i < CONANI; i++) {
        zebrane_eq_ack[i] = (i == rank - BIBLIOTEKARZE) ? TRUE : FALSE;    
    }
}

void sendAckErrandPacket(int dest, int errand) {
    packet_t packet;
    packet.data = errand;

    sendPacket(&packet, dest, ACK_ERRAND);
}

void startCollectingEq() {
    clearEqAcks();
    packet_t packet;
    packet.data = zlecenie_dla;
    debug("Started collecting EQ for errand %d", zlecenie_dla);
    for (int i = BIBLIOTEKARZE; i < size; i++)
    {
        if (i != rank) {
            sendPacket(&packet, i, REQ_EQ);
        }
    }
    
}

void tryCompetingForErrandFromList() {
    zlecenia[zlecenie_dla] = TAKEN;
    zlecenie_dla = -1;
    
    clearAcks();
    int flag = TRUE;
    for (int i = 0; i < BIBLIOTEKARZE; i++)
    {
        if (zlecenia[i] == TRUE)
        {
            flag = FALSE;
            debug("Started competing for errand for librarian %d", i);

            packet_t packet;
            for (int j = BIBLIOTEKARZE; j < size; j++)
            {
                if (j != rank)
                {
                    zlecenie_dla = i;
                    packet.data = i;
                    packet.priority = my_priority;
                    sendPacket(&packet, j, REQ_ERRAND);
                    //debug("Sent REQ_ERRAND to %d for errand %d", j, packet.data);
                }
            }
            break;
        }
    }
    if (flag)
        changeState(Ready);
}

void *conanCommunicationThread(void *ptr) {
    MPI_Status status;
    packet_t packet;
    while (stan != Exit)
    {        
        //debug("Waiting for message");
        MPI_Recv(&packet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        setMaxLamport(packet.ts);
        switch(status.MPI_TAG) {
                case FINISH:
                    changeState(Exit);
                    debug("Exitting");
                    break;
                case ERRAND:
                    switch(stan) {
                        case Ready:
                            changeState(CompeteForErrand);
                            
                            debug("Received errand from librarian %d", packet.src);
                            packet.data = packet.src;
                            zlecenie_dla = packet.src;
                            clearAcks();
                            for (int i = BIBLIOTEKARZE; i < size; i++) {
                                if(i != rank) {
                                    zlecenia[zlecenie_dla] = TRUE;
                                    sendPacket(&packet, i, REQ_ERRAND);
                                    //debug("Sent REQ_ERRAND to %d for errand %d", i, packet.data);
                                }
                            }
                            break;
                        case CompeteForErrand:
                            if(zlecenia[packet.src] != TAKEN) {
                                zlecenia[packet.src] = TRUE;
                                debug("Saved errand received from librarian %d", packet.src);
                            } else {
                                zlecenia[packet.src] = FALSE;
                            }
                            // jeszcze trzeba zadbać, że jeśli dostał wcześniej REQa i odpowiedział ACK, a nie dostał ERRANDA to nie zaznaczy jako TRUE
                            break;
                        default:
                            break;
                    }
                    break;
                case REQ_ERRAND:
                    //debug("Received REQ_ERRAND from %d for errand %d", packet.src, packet.data);
                    switch (stan) {
                        case Ready:
                            if((packet.priority < my_priority || (packet.priority == my_priority && packet.src > rank)) &&
                                zlecenia[packet.data] != TAKEN) {
                                changeState(CompeteForErrand);
                                debug("Started competing for errand %d from REQ_ERRAND received from %d", packet.data, packet.src);
                                clearAcks();
                                for (int i = BIBLIOTEKARZE; i < size; i++) {
                                    if(i != rank) {
                                        zlecenia[packet.data] = TRUE;
                                        zlecenie_dla = packet.data;
                                        sendPacket(&packet, i, REQ_ERRAND);
                                        //debug("Sent REQ_ERRAND to %d for errand %d", i, packet.data);
                                    }
                                }
                            } else {
                                zlecenia[packet.data] = TAKEN;
                                sendAckErrandPacket(packet.src, packet.data);
                                //debug("Sent ACK_ERRAND to %d for errand %d due to higher priority REQ", packet.src, packet.data);
                            }
                            break;
                        case CompeteForErrand:
                            if(packet.data == zlecenie_dla) {
                                if(packet.priority > my_priority || (packet.priority == my_priority && packet.src < rank)) {
                                    sendAckErrandPacket(packet.src, packet.data);
                                    //debug("Sent ACK_ERRAND to %d for errand %d", packet.src, packet.data);

                                    tryCompetingForErrandFromList();
                                }
                            } else {
                                if(packet.priority > my_priority || (packet.priority == my_priority && packet.src < rank)) {
                                    if(zlecenia[packet.data] == FALSE) {
                                        zlecenia[packet.data] = TAKEN;
                                    } else {
                                        zlecenia[packet.data] = FALSE;
                                    }
                                    sendAckErrandPacket(packet.src, packet.data);
                                    //debug("Sent ACK_ERRAND to %d for errand %d", packet.src, packet.data);
                                } else {
                                    if (zlecenia[packet.data] != TAKEN) 
                                    {
                                        zlecenia[packet.data] = TRUE;
                                        debug("Saved errand from librarian %d", packet.data);
                                    }
                                }
                                // obawiam się problemów z priorytetem
                            }
                        default:
                            sendAckErrandPacket(packet.src, packet.data);
                            break;
                    }
                    break;
                case ACK_ERRAND:
                    switch (stan) {
                        case CompeteForErrand:
                            debug("Received ACK_ERRAND from %d", packet.src);
                            if (packet.data == zlecenie_dla) {
                                zebrane_ack[packet.src - BIBLIOTEKARZE] = TRUE;
                            }
                            int all_ack_collected = TRUE;
                            int sum = 0;
                            for (int i = 0; i < CONANI; i++) {
                                if(zebrane_ack[i] == FALSE) {
                                    all_ack_collected = FALSE;
                                }
                                sum += zebrane_ack[i];
                            }
                            debug("Collected %d ACKs for errand %d", sum, zlecenie_dla);
                            if(all_ack_collected) {
                                for (int i = 0; i < CONANI; i++) {
                                    zebrane_ack[i] = FALSE;
                                }
                                // changeState(CollectingEq);
                                // for (int i = BIBLIOTEKARZE; i <= size; i++) {
                                //     zlecenie_dla = i;
                                //     packet.data = i;
                                //     packet.priority = my_priority;
                                //     sendPacket(&packet, i, REQ_EQ);
                                // }
                                changeState(CollectingEq);
                                debug("Collected all ACKs for errand %d", zlecenie_dla);
                                startCollectingEq();
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case REQ_EQ:
                    switch (stan) {
                        case CompeteForErrand:
                            if (packet.data == zlecenie_dla) {
                                tryCompetingForErrandFromList();
                            } else {
                                forwardPacket(&packet, rank, ACK_ERRAND);
                            }
                            debug("Sent ACK_EQ to %d", packet.src);
                            sendPacket(0, packet.src, ACK_EQ);
                            break;
                        case CollectingEq:
                            if (equipmentQueue == NULL) {
                                equipmentQueue = malloc(sizeof(queue));
                                if (equipmentQueue == NULL) {
                                    //coś się zepsuło, handling błędów kiedy indziej XD
                                    exit(284829);
                                }
                                equipmentQueue->destination = packet.src;
                                equipmentQueue->priority = packet.priority;
                                equipmentQueue->nextItem = NULL;
                            } else {
                                queue *tmp = equipmentQueue;
                                while (tmp != NULL) {
                                    if(packet.priority < tmp->priority) {
                                        queue* swap = malloc(sizeof(queue));
                                        swap->destination = tmp->destination;
                                        swap->nextItem = tmp->nextItem;
                                        swap->priority = tmp->priority;
                                        tmp->destination = packet.src;
                                        tmp->nextItem = swap;
                                        tmp->priority = packet.priority;
                                        break;
                                    } else tmp = tmp->nextItem;
                                }
                            }
                            zebrane_eq_req[packet.src - BIBLIOTEKARZE] = TRUE;
                            req_check();
                            break;
                        default:
                            sendPacket(0, packet.src, ACK_EQ);
                            break;
                        // nie będę oszukiwał, te priorytety to istotna zabawka, która może nam coś spier............. zepsuć (:
                        // nie wiem też czy wszystko co trzeba
                    }
                case ACK_EQ:
                    switch (stan) {
                        case CollectingEq:
                            debug("Received ACK_EQ from %d", packet.src);
                            zebrane_eq_req[packet.src - BIBLIOTEKARZE] = TRUE;
                            zebrane_eq_ack[packet.src - BIBLIOTEKARZE] = TRUE;
                            req_check();
                            int all_ack_collected = TRUE;
                            for (int i = 0; i < CONANI; i++) {
                                if(zebrane_eq_ack[i] == FALSE) {
                                    all_ack_collected = FALSE;
                                    break;
                                }
                            }
                            if(all_ack_collected) {
                                for (int i = 0; i < CONANI; i++) {
                                    zebrane_eq_req[i] = FALSE;
                                    zebrane_eq_ack[i] = FALSE;
                                }
                                debug("Collected ACK_EQ from everyone. Starting execution");
                                changeState(Executing);
                                sendPacket(0, rank, START_INTERNAL);
                            }
                    }
                    break;
                case ACK_LIB:
                    my_priority -= 1; 
                    changeState(Ready);
                    
                    break;
                case END_INTERNAL:
                    switch (stan)
                    {
                    case Executing:
                        changeState(FinishErrand);
                        debug("Trying to confirm end of errand from librarian %d", zlecenie_dla);
                        sendPacket(0, rank, START_INTERNAL);
                        break;
                    
                    default:
                        break;
                    }
                    break;
                    
            }
        sleep(1);
    }
    
}

void req_check() {
    int all_ack_collected = TRUE;
    for (int i = 0; i < CONANI; i++) {
        if(zebrane_eq_req[i] == FALSE) {
            all_ack_collected = FALSE;
            break;
        }
    }
    if(all_ack_collected) {
        for (int i = 0; i < CONANI; i++) {
            zebrane_eq_req[i] = FALSE;
        }
        int i = 0;
        queue *tmp = equipmentQueue;
        while(i <= STROJE || tmp == NULL) {
            sendPacket(0, tmp->destination, ACK_EQ);
            queue *del = tmp;
            tmp = tmp->nextItem;
            free(del);
        }
        equipmentQueue = tmp;
    }
}

//skoro pralnia ma działać podobnie to czemu nie napisać ogólniejszej funkcji?
void generalized_req_check(queue *q, int *reqs, int resource) {
    int all_ack_collected = TRUE;
    for (int i = 0; i < CONANI; i++) {
        if(reqs[i] == FALSE) {
            all_ack_collected = FALSE;
            break;
        }
    }
    if(all_ack_collected) {
        for (int i = 0; i < CONANI; i++) {
            reqs[i] = FALSE;
        }
        int i = 0;
        while(i <= resource || q == NULL) {
            // w warunku do while nie wiem czy nie trzeba odjąć ilości zasobu, którą poprzednimi plebiscytami zdobyliśmy
            sendPacket(0, q->destination, ACK_EQ);
            queue *del = q;
            q = q->nextItem;
            free(del);
        }
        equipmentQueue = q;
    }
}