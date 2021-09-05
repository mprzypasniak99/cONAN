#include "main.h"
#include "watek_komunikacyjny.h"

/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */
void *startKomWatek(void *ptr)
{
    MPI_Status status;
    int is_message = FALSE;
    packet_t pakiet;
    /* Obrazuje pętlę odbierającą pakiety o różnych typach */
    //while ( stan!=InFinish ) {
    while (TRUE) {
	debug("czekam na recv");
        MPI_Recv( &pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	
	    setMaxLamport(pakiet.ts);
        if (rank < BIBLIOTEKARZE) {
            switch(status.MPI_TAG) {
                case FINISH:
                    changeState(InFinish);
                    break;
                case REQ_LIB:
                    if(stan==Waiting) {
                        sendPacket(0, pakiet.src, ACK_LIB);
                        changeState(Preparing);
                    }
                    break;
                default:
                    break;
            }
        } else {
            switch(status.MPI_TAG) {
                case FINISH:
                    changeState(InFinish);
                    break;
                case ERRAND:
                    switch(stan) {
                        case Ready:
                            changeState(CompeteForErrand);
                            for (int i = BIBLIOTEKARZE; i <= size; i++) {
                                zlecenia[pakiet.src] = TRUE;
                                zlecenie_dla = pakiet.src;
                                pakiet.data = pakiet.src;
                                pakiet.priority = my_priority;
                                sendPacket(&pakiet, i, REQ_ERRAND);
                            }
                            break;
                        case CompeteForErrand:
                            if(zlecenia[pakiet.src] != TAKEN) {
                                zlecenia[pakiet.src] = TRUE;
                            } else {
                                zlecenia[pakiet.src] = FALSE;
                            }
                            // jeszcze trzeba zadbać, że jeśli dostał wcześniej REQa i odpowiedział ACK, a nie dostał ERRANDA to nie zaznaczy jako TRUE
                            break;
                        default:
                            break;
                    }
                case REQ_ERRAND:
                    switch (stan) {
                    case Ready:
                        if(pakiet.priority < my_priority || (pakiet.priority == my_priority && rank < pakiet.src)) {
                            changeState(CompeteForErrand);
                            for (int i = BIBLIOTEKARZE; i <= size; i++) {
                                zlecenia[pakiet.data] = TRUE;
                                zlecenie_dla = pakiet.data;
                                pakiet.data = pakiet.data;
                                pakiet.priority = my_priority;
                                sendPacket(&pakiet, i, REQ_ERRAND);
                            }
                        } else {
                            zlecenia[pakiet.data] = TAKEN;
                            sendPacket(0, pakiet.src, ACK_ERRAND);
                        }
                        break;
                    case CompeteForErrand:
                        if(pakiet.data == zlecenie_dla) {
                            if(pakiet.priority > my_priority || (pakiet.priority == my_priority && rank > pakiet.src)) {
                                zlecenie_dla = -1;
                                zlecenia[pakiet.data] = FALSE;
                                sendPacket(0, pakiet.src, ACK_ERRAND);
                                for(int i = 0; i < BIBLIOTEKARZE; i++) {
                                    int flag = TRUE;
                                    if(zlecenia[i]) {
                                        flag = FALSE;
                                        for (int i = BIBLIOTEKARZE; i <= size; i++) {
                                            zlecenie_dla = i;
                                            pakiet.data = i;
                                            pakiet.priority = my_priority;
                                            sendPacket(&pakiet, i, REQ_ERRAND);
                                        }
                                    }
                                    if(flag) changeState(Ready);
                                }
                            }
                        } else {
                            if(zlecenia[pakiet.data] == FALSE) {
                                zlecenia[pakiet.data] = TAKEN;
                            } else {
                                zlecenia[pakiet.data] = FALSE;
                            }
                            sendPacket(0, pakiet.src, ACK_ERRAND);
                            // obawiam się problemów z priorytetem
                        }
                    default:
                        sendPacket(0, pakiet.src, ACK_ERRAND);
                        break;
                    }
                case ACK_ERRAND:
                    switch (stan)
                    {
                    case CompeteForErrand:
                        // no coś tu napisać jeszcze trza
                        break;
                    
                    default:
                        break;
                    }
            }
        }
        /*switch ( status.MPI_TAG ) {
	    case FINISH: 
                changeState(InFinish);
	    break;
	    case TALLOWTRANSPORT: 
                changeTallow( pakiet.data);
                debug("Dostałem wiadomość od %d z danymi %d",pakiet.src, pakiet.data);
	    break;
	    case GIVEMESTATE: 
                pakiet.data = tallow;
                sendPacket(&pakiet, ROOT, STATE);
                debug("Wysyłam mój stan do monitora: %d funtów łoju na składzie!", tallow);
	    break;
            case STATE:
                numberReceived++;
                globalState += pakiet.data;
                if (numberReceived > size-1) {
                    debug("W magazynach mamy %d funtów łoju.", globalState);
                } 
            break;
	    case INMONITOR: 
                changeState( InMonitor );
                debug("Od tej chwili czekam na polecenia od monitora");
	    break;
	    case INRUN: 
                changeState( InRun );
                debug("Od tej chwili decyzję podejmuję autonomicznie i losowo");
	    break;
	    default:
	    break;
        }*/
    }
}
