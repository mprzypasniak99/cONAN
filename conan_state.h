#ifndef CONAN_STATE_H
#define CONAN_STATE_H

#include "queue.h"

typedef enum {  Ready, CompeteForErrand, CollectingEq, Executing, 
                FinishErrand, Laundry, ReturnEq, Exit} conan_state;

extern conan_state conanState;

//extern queue* errandQueue;
extern queue* equipmentQueue;
extern queue* laundryQueue;

#endif