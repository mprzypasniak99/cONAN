#include "conan_main.h"
#include "conan_state.h"
#include "main.h"

queue* equipmentQueue;
queue* laundryQueue;

void clearAcks() {
    for (int i = 0; i < CONANI; i++)
    {   
        zebrane_ack[i] = (i == rank - BIBLIOTEKARZE) ? TRUE : FALSE;
    }
}

void generalizedClearAcks(int *array_to_clear) {
    for (int i = 0; i < CONANI; i++) {
        array_to_clear[i] = FALSE;  
    }
}

void sendAckErrandPacket(int dest, int errand) {
    packet_t packet;
    packet.data = errand;

    sendPacket(&packet, dest, ACK_ERRAND);
}

void startCollectingEq() {
    generalizedClearAcks(zebrane_eq_ack);
    packet_t packet;
    packet.data = zlecenie_dla;
    debug("Started collecting EQ for errand %d", zlecenie_dla);
    for (int i = BIBLIOTEKARZE; i < size; i++)
    {
        sendPacket(&packet, i, REQ_EQ);
    }
    
}

void startLaundry() {
    generalizedClearAcks(zebrane_laundry_ack);
    debug("Started laundry");
    for (int i = BIBLIOTEKARZE; i < size; i++)
    {
        sendPacket(0, i, REQ_LAUNDRY);
    }
}
void tryCompetingForErrandFromList() {
    if(zlecenie_dla != -1) {
    errandQueue[zlecenie_dla].available = FALSE;
    zlecenie_dla = -1;
    }
    clearAcks();
    int flag = TRUE;
    for (int i =0; i < BIBLIOTEKARZE; i++) {
        if (errandQueue[i].available != FALSE) {
            if (errandQueue[i].ack_destination == 0 || (my_priority > errandQueue[i].priority || (my_priority == errandQueue[i].priority && rank < errandQueue[i].ack_destination))) {
                flag = FALSE;
                debug("Started competing for errand for librarian %d", i);

                packet_t packet;
                for (int j = BIBLIOTEKARZE; j < size; j++)
                {
                    zlecenie_dla = i;
                    packet.data = i;
                    packet.errandNum = errandQueue[i].errandNum;
                    if (j != rank)
                    {
                        sendPacket(&packet, j, REQ_ERRAND);
                    }
                }
                break;
            }
        }
    }
    if (flag)
        changeConanState(Ready);
}

queue* generalizedReqCheck(queue *q, int *reqs, int *acks, int resource, int tag) {
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
        int sent = 0;
        int flag = FALSE;
        while(q != NULL && sent < resource) {
            if(q->destination == rank) flag = TRUE;
            sendMutedAck(q->destination, tag, acks);
            if(flag) ++sent;
            q = q->nextItem;
        }
        *acks = sent;
    }
    return q;
}

void conanMainLoop() {
    MPI_Status status;
    packet_t packet;
    while (stan != Exit)
    {        
        MPI_Recv(&packet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        setMaxLamport(packet.ts);
        switch(status.MPI_TAG) {
                case FINISH:
                    changeConanState(Exit);
                    debug("Exitting");
                    break;
                case ERRAND:
                    //debug("Received errand from librarian %d", packet.src);
                    if(errandQueue[packet.src].errandNum < packet.errandNum) {
                        debug("Received new errand from librarian %d", packet.src);
                        errandQueue[packet.src] = (errand){packet.errandNum, TRUE, 0, 0};
                    }
                    if (stan == Ready) {
                        if(errandQueue[packet.src].available == TRUE) {
                            debug("Started competing for errand for librarian %d", packet.src);
                            errandQueue[packet.src].ack_destination = rank;
                            errandQueue[packet.src].priority = my_priority;
                            clearAcks();
                            zlecenie_dla = packet.src;
                            packet.data = packet.src;
                            changeConanState(CompeteForErrand);
                            for (int i = BIBLIOTEKARZE; i < size; i++) {
                                if(i != rank) {
                                    sendErrandPacket(&packet, i, packet.errandNum, REQ_ERRAND);
                                }
                            }
                        }
                    }
                    break;
                case REQ_ERRAND:
                    if(errandQueue[packet.data].errandNum < packet.errandNum) {
                        debug("Received new errand from REQ for librarian %d", packet.data);
                        errandQueue[packet.data] = (errand){packet.errandNum, TRUE, 0, 0};
                    }
                    switch (stan){
                        case Ready:
                            if ((packet.priority < my_priority || (packet.priority == my_priority && packet.src > rank)) && errandQueue[packet.data].available == TRUE) {
                                debug("Competeing for ERRAND, my priority is higher.");
                                errandQueue[packet.data].ack_destination = rank;
                                errandQueue[packet.data].priority = my_priority;
                                clearAcks();
                                zlecenie_dla = packet.data;
                                changeConanState(CompeteForErrand);
                                for (int i = BIBLIOTEKARZE; i < size; i++) {
                                    if(i != rank) {
                                        sendErrandPacket(&packet, i, packet.errandNum, REQ_ERRAND);
                                    }
                                }
                            } else {
                                if (errandQueue[packet.data].ack_destination == 0 || (packet.priority > errandQueue[packet.data].priority || (packet.priority == errandQueue[packet.data].priority && packet.src < errandQueue[packet.data].ack_destination))) {
                                    debug("Higher priority, sendind ACK without trying to compete.")
                                    errandQueue[packet.data].ack_destination = packet.src;
                                    errandQueue[packet.data].priority = packet.priority;
                                    errandQueue[packet.data].available = FALSE;
                                    sendAckErrandPacket(packet.src, packet.data);
                                }
                            }
                            break;
                        case CompeteForErrand:
                            if((packet.priority < my_priority || (packet.priority == my_priority && packet.src > rank))) {
                                if(zlecenie_dla != packet.data || errandQueue[packet.data].available != FALSE) {
                                    if (errandQueue[packet.data].ack_destination == 0 || (packet.priority > errandQueue[packet.data].priority || (packet.priority == errandQueue[packet.data].priority && packet.src < errandQueue[packet.data].ack_destination))) {
                                        errandQueue[packet.data].ack_destination = packet.src;
                                        errandQueue[packet.data].priority = packet.priority;
                                        errandQueue[packet.data].available = TAKEN;
                                        debug("Saved errand received from librarian %d", packet.data);
                                    }
                                }
                            } else {
                                if (errandQueue[packet.data].ack_destination == 0 || (packet.priority > errandQueue[packet.data].priority || (packet.priority == errandQueue[packet.data].priority && packet.src < errandQueue[packet.data].ack_destination))) {
                                    errandQueue[packet.data].ack_destination = packet.src;
                                    errandQueue[packet.data].priority = packet.priority;
                                    errandQueue[packet.data].available = FALSE;
                                    sendAckErrandPacket(packet.src, packet.data);
                                    debug("Sent ACK_ERRAND to %d for errand %d and gave up on competing for this errand", packet.src, packet.data);
                                    clearAcks();
                                    tryCompetingForErrandFromList();
                                }
                            }
                            break;
                        default:
                            if (errandQueue[packet.data].ack_destination == 0 || (packet.priority > errandQueue[packet.data].priority || (packet.priority == errandQueue[packet.data].priority && packet.src < errandQueue[packet.data].ack_destination))) {
                                errandQueue[packet.data].ack_destination = packet.src;
                                errandQueue[packet.data].priority = packet.priority;
                                errandQueue[packet.data].available = FALSE;
                                sendAckErrandPacket(packet.src, packet.data);
                            }
                            break;
                    }
                    break;
                case ACK_ERRAND:
                    if(stan == CompeteForErrand) {
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
                        if(all_ack_collected) {
                            clearAcks();
                            for(int i = 0; i < BIBLIOTEKARZE; i++) {
                                if(errandQueue[i].available == TAKEN) {
                                    sendAckErrandPacket(errandQueue[i].ack_destination, i);
                                    errandQueue[i].available = FALSE;
                                }
                            }
                            changeConanState(CollectingEq);
                            debug("Collected all ACKs for errand %d", zlecenie_dla);
                            startCollectingEq();
                        }
                    }
                    break;
                case REQ_EQ:
                    switch (stan) {
                        case CompeteForErrand:
                            if (packet.data == zlecenie_dla) {
                                 tryCompetingForErrandFromList();
                            }
                            if (sent_eq_acks) {
                                if(sent_eq_acks < STROJE) {
                                    sent_eq_acks++;
                                    sendMutedAck(packet.src, ACK_EQ, &sent_eq_acks);
                                } else {
                                    addToQueue(&equipmentQueue, packet.src, packet.priority);
                                }
                            } else {
                                sendMutedAck(packet.src, ACK_EQ, &sent_eq_acks);
                            }
                            break;
                        case CollectingEq:
                            addToQueue(&equipmentQueue, packet.src, packet.priority);
                            zebrane_eq_req[packet.src - BIBLIOTEKARZE] = TRUE;
                            equipmentQueue = generalizedReqCheck(equipmentQueue, zebrane_eq_req, &sent_eq_acks, STROJE, ACK_EQ);
                            break;
                        case Ready:
                            zlecenia[packet.data] == TAKEN;
                        default:
                            if (sent_eq_acks) {
                                if(sent_eq_acks < STROJE) {
                                    sent_eq_acks++;
                                    sendMutedAck(packet.src, ACK_EQ, &sent_eq_acks);
                                } else {
                                    addToQueue(&equipmentQueue, packet.src, packet.priority);
                                }
                            } else {
                                sendMutedAck(packet.src, ACK_EQ, &sent_eq_acks);
                            }
                            break;
                    }
                    break;
                case ACK_EQ:
                    switch (stan) {
                        case CollectingEq:
                            zebrane_eq_req[packet.src - BIBLIOTEKARZE] = TRUE;
                            zebrane_eq_ack[packet.src - BIBLIOTEKARZE] = TRUE;
                            equipmentQueue = generalizedReqCheck(equipmentQueue, zebrane_eq_req, &sent_eq_acks, STROJE, ACK_EQ);
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
                                changeConanState(Executing);
                                sendPacket(0, rank, START_INTERNAL);
                            }
                    }
                    break;
                case REQ_LAUNDRY:
                    switch(stan) {
                        case Laundry:
                            addToQueue(&laundryQueue, packet.src, packet.priority);
                            zebrane_laundry_req[packet.src - BIBLIOTEKARZE] = TRUE;
                            laundryQueue = generalizedReqCheck(laundryQueue, zebrane_laundry_req, &sent_laundry_acks, PRALNIA, ACK_LAUNDRY);
                            break;
                        default:
                            if (sent_laundry_acks) {
                                if(sent_laundry_acks < PRALNIA) {
                                    sent_laundry_acks++;
                                    sendMutedAck(packet.src, ACK_LAUNDRY, &sent_laundry_acks);
                                } else {
                                    addToQueue(&laundryQueue,packet.src, packet.priority);
                                }
                            } else {
                                sendMutedAck(packet.src, ACK_LAUNDRY, &sent_laundry_acks);
                            }
                            break;
                    }
                    break;
                case ACK_LAUNDRY:
                    switch (stan) {
                        case Laundry:
                            zebrane_laundry_req[packet.src - BIBLIOTEKARZE] = TRUE;
                            zebrane_laundry_ack[packet.src - BIBLIOTEKARZE] = TRUE;
                            laundryQueue = generalizedReqCheck(laundryQueue, zebrane_laundry_req, &sent_laundry_acks, PRALNIA, ACK_LAUNDRY);
                            int all_ack_collected = TRUE;
                            for (int i = 0; i < CONANI; i++) {
                                if(zebrane_laundry_ack[i] == FALSE) {
                                    all_ack_collected = FALSE;
                                    break;
                                }
                            }
                            if(all_ack_collected) {
                                for (int i = 0; i < CONANI; i++) {
                                    zebrane_laundry_req[i] = FALSE;
                                    zebrane_laundry_ack[i] = FALSE;
                                }
                                debug("Collected ACK_LAUNDRY from everyone.");
                                pthread_t washing;
                                pthread_create(&washing, NULL, wash, 0);
                                changeConanState(Ready);
                                tryCompetingForErrandFromList();
                            }
                    }
                    break;
                case ACK_LIB:
                    my_priority -= 1; 
                    changeConanState(Laundry);
                    errandQueue[zlecenie_dla].available = FALSE;
                    zlecenie_dla = -1;

                    startLaundry();
                    break;
                case END_INTERNAL:
                    switch (stan)
                    {
                    case Executing:
                        changeConanState(FinishErrand);
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