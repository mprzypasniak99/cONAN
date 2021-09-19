#include "conan_communication.h"
#include "conan_state.h"
#include "main.h"

void clearAcks() {
    for (int i = 0; i < CONANI; i++)
    {   
        zebrane_ack[i] = (i == rank - BIBLIOTEKARZE) ? TRUE : FALSE;
    }
}

void sendAckErrandPacket(int dest, int errand) {
    packet_t packet;
    packet.data = errand;

    sendPacket(&packet, dest, ACK_ERRAND);
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
                                    debug("Sent REQ_ERRAND to %d for errand %d", i, packet.data);
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
                    debug("Received REQ_ERRAND from %d for errand %d", packet.src, packet.data);
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
                                        debug("Sent REQ_ERRAND to %d for errand %d", i, packet.data);
                                    }
                                }
                            } else {
                                zlecenia[packet.data] = TAKEN;
                                sendAckErrandPacket(packet.src, packet.data);
                                debug("Sent ACK_ERRAND to %d for errand %d due to higher priority REQ", packet.src, packet.data);
                            }
                            break;
                        case CompeteForErrand:
                            if(packet.data == zlecenie_dla) {
                                if(packet.priority > my_priority || (packet.priority == my_priority && packet.src < rank)) {
                                    zlecenie_dla = -1;
                                    zlecenia[packet.data] = TAKEN;
                                    sendAckErrandPacket(packet.src, packet.data);

                                    debug("Sent ACK_ERRAND to %d for errand %d", packet.src, packet.data);
                                    clearAcks();
                                    int flag = TRUE;
                                    for(int i = 0; i < BIBLIOTEKARZE; i++) {
                                        if(zlecenia[i] == TRUE) {
                                            flag = FALSE;
                                            debug("Started competing for errand for librarian %d", i);
                                            for (int j = BIBLIOTEKARZE; j < size; j++) {
                                                if(j != rank) {
                                                    zlecenie_dla = i;
                                                    packet.data = i;
                                                    packet.priority = my_priority;
                                                    sendPacket(&packet, j, REQ_ERRAND);
                                                    debug("Sent REQ_ERRAND to %d for errand %d", j, packet.data);
                                                }
                                            }
                                            break;

                                        }
                                    }
                                    if(flag) changeState(Ready);
                                }
                            } else {
                                if(packet.priority > my_priority || (packet.priority == my_priority && packet.src < rank)) {
                                    if(zlecenia[packet.data] == FALSE) {
                                        zlecenia[packet.data] = TAKEN;
                                    } else {
                                        zlecenia[packet.data] = FALSE;
                                    }
                                    sendAckErrandPacket(packet.src, packet.data);
                                    debug("Sent ACK_ERRAND to %d for errand %d", packet.src, packet.data);
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
                                changeState(Executing);
                                debug("Collected all ACKs for errand %d", zlecenie_dla);
                                sendPacket(0, rank, START_INTERNAL);
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case REQ_EQ:
                    switch (stan) {
                        default:
                            sendPacket(0, packet.src, ACK_EQ);
                            break;
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
        sleep(5);
    }
}