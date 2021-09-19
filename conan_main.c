#include "main.h"
#include "conan_main.h"
#include "conan_communication.h"
#include "conan_state.h"

void conanMainLoop() {
    MPI_Status status;
    packet_t packet;
    srand(time(NULL));
    while(stan != Exit) {
        MPI_Recv(&packet, 1, MPI_PAKIET_T, rank, START_INTERNAL, MPI_COMM_WORLD, &status);
        
        switch (stan)
        {
        case Executing:
            sleep(rand() % 5);
            debug("Started executing errand");
            sendPacket(0, rank, END_INTERNAL);
            break;
        
        case FinishErrand:
            sendPacket(0, zlecenie_dla, REQ_LIB);
            break;
        default:
            break;
        }
    }
}