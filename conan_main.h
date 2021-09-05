#ifndef CONAN_MAIN_H
#define CONAN_MAIN_H

pthread_t conanCommunicationThread;

void initConan();

void finalizeConan();

void conanMainLoop();

#endif