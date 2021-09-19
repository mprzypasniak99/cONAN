#include "librarian_communication.h"
#include "main.h"

void* librarianCommunicationThread(void* ptr) {
    MPI_Status status;
    packet_t packet;
    while (l_stan != Exit) {
        debug("Waiting for message");
        MPI_Recv(&packet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        setMaxLamport(packet.ts);

        switch (status.MPI_TAG)
        {
        case FINISH:
            if(l_stan == Waiting) {
                changeLibrarianState(Preparing);
                sendPacket(0, rank, ERRAND);
            }
            break;
        
        default:
            break;
        }
    }
}