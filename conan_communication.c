#include "conan_communication.h"
#include "conan_state.h"
#include "main.h"

void *conanCommunicationThread(void *ptr) {
    MPI_Status status;
    packet_t packet;
    while (conanState != Exit)
    {        
        debug("Waiting for message");
        MPI_Recv(&packet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        setMaxLamport(packet.ts);
        switch(status.MPI_TAG) {
                case FINISH:
                    changeState(Exit);
                    break;
                case ERRAND:
                    switch(stan) {
                        case Ready:
                            changeState(CompeteForErrand);
                            for (int i = BIBLIOTEKARZE; i <= size; i++) {
                                zlecenia[packet.src] = TRUE;
                                zlecenie_dla = packet.src;
                                packet.data = packet.src;
                                packet.priority = my_priority;
                                sendPacket(&packet, i, REQ_ERRAND);
                            }
                            break;
                        case CompeteForErrand:
                            if(zlecenia[packet.src] != TAKEN) {
                                zlecenia[packet.src] = TRUE;
                            } else {
                                zlecenia[packet.src] = FALSE;
                            }
                            // jeszcze trzeba zadbać, że jeśli dostał wcześniej REQa i odpowiedział ACK, a nie dostał ERRANDA to nie zaznaczy jako TRUE
                            break;
                        default:
                            break;
                    }
                case REQ_ERRAND:
                    switch (stan) {
                        case Ready:
                            if(packet.priority < my_priority || (packet.priority == my_priority && rank < packet.src)) {
                                changeState(CompeteForErrand);
                                for (int i = BIBLIOTEKARZE; i <= size; i++) {
                                    zlecenia[packet.data] = TRUE;
                                    zlecenie_dla = packet.data;
                                    packet.data = packet.data;
                                    packet.priority = my_priority;
                                    sendPacket(&packet, i, REQ_ERRAND);
                                }
                            } else {
                                zlecenia[packet.data] = TAKEN;
                                sendPacket(0, packet.src, ACK_ERRAND);
                            }
                            break;
                        case CompeteForErrand:
                            if(packet.data == zlecenie_dla) {
                                if(packet.priority > my_priority || (packet.priority == my_priority && rank > packet.src)) {
                                    zlecenie_dla = -1;
                                    zlecenia[packet.data] = FALSE;
                                    sendPacket(0, packet.src, ACK_ERRAND);
                                    for(int i = 0; i < CONANI; i++) {
                                        zebrane_ack[i] = FALSE;
                                    }
                                    int flag = TRUE;
                                    for(int i = 0; i < BIBLIOTEKARZE; i++) {
                                        if(zlecenia[i]) {
                                            flag = FALSE;
                                            for (int j = BIBLIOTEKARZE; j <= size; j++) {
                                                zlecenie_dla = i;
                                                packet.data = i;
                                                packet.priority = my_priority;
                                                sendPacket(&packet, j, REQ_ERRAND);
                                            }
                                            break;

                                        }
                                    }
                                    if(flag) changeState(Ready);
                                }
                            } else {
                                if(zlecenia[packet.data] == FALSE) {
                                    zlecenia[packet.data] = TAKEN;
                                } else {
                                    zlecenia[packet.data] = FALSE;
                                }
                                sendPacket(0, packet.src, ACK_ERRAND);
                                // obawiam się problemów z priorytetem
                            }
                        default:
                            sendPacket(0, packet.src, ACK_ERRAND);
                            break;
                    }
                case ACK_ERRAND:
                    switch (stan) {
                        case CompeteForErrand:
                            zebrane_ack[packet.src - BIBLIOTEKARZE] = TRUE;
                            int all_ack_collected = TRUE;
                            for (int i = 0; i < CONANI; i++) {
                                if(zebrane_ack[i] == FALSE) {
                                    all_ack_collected = FALSE;
                                    break;
                                }
                            }
                            if(all_ack_collected) {
                                for (int i = 0; i < CONANI; i++) {
                                    zebrane_ack[i] = FALSE;
                                }
                                changeState(CollectingEq);
                                for (int i = BIBLIOTEKARZE; i <= size; i++) {
                                    zlecenie_dla = i;
                                    packet.data = i;
                                    packet.priority = my_priority;
                                    sendPacket(&packet, i, REQ_EQ);
                                }
                            }
                            break;
                        default:
                            break;
                    }
                case REQ_EQ:
                    switch (stan) {
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
                                changeState(Executing);
                            }
                    }
            }
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
            tmp = tmp->nextItem;
        }
        equipmentQueue = tmp;
    }
}