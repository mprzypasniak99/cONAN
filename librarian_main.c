#include "librarian_main.h"
#include "main.h"

void librarianMainLoop() {
    MPI_Status status;
    packet_t packet;
    int finishedErrands[CONANI] = { 0 };
    srand(time(NULL));
    while(l_stan != Exit_Librarian) {
        switch (l_stan)
        {
        case Waiting:
            MPI_Recv(&packet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, REQ_LIB, MPI_COMM_WORLD, &status);
            setMaxLamport(packet.ts);

            finishedErrands[packet.src - BIBLIOTEKARZE] += 1;
            sendPacket(0, packet.src, ACK_LIB);
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
                sendPacket(0, i, ERRAND);
            }
            
            changeLibrarianState(Waiting);
            break;
        }
        
        
    }
}