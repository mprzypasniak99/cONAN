#include "conan_communication.h"
#include "conan_state.h"
#include "main.h"

void conanCommunicationThread(void *ptr) {
    MPI_Status status;
    packet_t packet;
    while (conan_state != Exit)
    {        
        debug("Waiting for message");
        MPI_Recv(&packet, MPI_PACKET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, &status);

        setMaxLamport(packet.ts);
        switch (status.MPI_TAG)
        {
        case ERRAND:
            
            
            break;
        
        default:
            break;
        }
    }
    
}