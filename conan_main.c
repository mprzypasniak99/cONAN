#include "main.h"
#include "conan_main.h"
#include "conan_communication.h"
#include "conan_state.h"

conan_state conanState;

queue* errandQueue;
queue* equipmentQueue;
queue* laundryQueue;

void initConan() {
    conanState = Ready;

    errandQueue = NULL;
    equipmentQueue = NULL;
    laundryQueue = NULL;

    pthread_create(&conanCommunicationThread, NULL, conanCommunicationThread, 0);
}

void finalizeConan() {
    pthread_join(conanCommunicationThread, NULL);

    deleteQueue(errandQueue);
    deleteQueue(equipmentQueue);
    deleteQueue(laundryQueue);
}

void conanMainLoop() {
    while(conanState != Exit) {
        switch (conanState)
        {
        case Ready:
            if (errandQueue != NULL)
            {
                
            }
            
            break;
        
        default:
            break;
        }
    }
}