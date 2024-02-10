#ifndef SLDTEST_MAIN_H
#define SLDTEST_MAIN_H

#ifndef STOP_FLAG_H
#define STOP_FLAG_H

#include <atomic>

// Declaration of the stop flag variable
extern std::atomic<bool> stopFlag;

#endif



int sdltest_main(int argc, char* argv[]);
void puseSimulation();
void continueSimulation(); 
bool isPaused();


#endif 