#include "librarian_main.h"
#include "main.h"

void librarianMainLoop() {
    MPI_Status status;
    packet_t packet;
    int* finishedErrands = malloc(sizeof(int) * CONANI);
    for (int i = 0; i < CONANI; i++) {
        finishedErrands[i] = 0;
    }
    int errand = 0;
    srand(time(NULL));
    while(l_stan != Exit_Librarian) {
        switch (l_stan)
        {
        case Waiting:
            MPI_Recv(&packet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, REQ_LIB, MPI_COMM_WORLD, &status);
            setMaxLamport(packet.ts);

            finishedErrands[packet.src - BIBLIOTEKARZE] += 1;
            sendPacket(0, packet.src, ACK_LIB);
            errand++;
            changeLibrarianState(Preparing);
            
            for(int i = BIBLIOTEKARZE; i < size; i++) {
                debug("Conan no. %d completed %d errands", i, finishedErrands[i - BIBLIOTEKARZE]);
            }
            break;
        
        case Preparing:
            sleep(rand() % 5);
            debug("Sent errand to Conans");
            
            for (int i = BIBLIOTEKARZE; i < size; i++)
            {
                sendErrandPacket(0, i, errand, ERRAND);
            }
            
            changeLibrarianState(Waiting);
            break;
        }
        
        
    }
}