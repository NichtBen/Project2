#include <iostream>
#include <thread>
#include "shadertestmain.h"
#include "sdltest.h"


//strange function i can register to be called on console closure, so console colsed = close other windows etc  currently the flag is inside sdltest ... should be moved here right? duno yet
BOOL WINAPI consoleCtrlHandler(DWORD ctrlType) {
    if (ctrlType == CTRL_CLOSE_EVENT) {
        stopFlag = true;
        return TRUE;
    }
    return FALSE;
}


void shadertestmain_function(int argc, char** argv) {
    std::cout << "shadertestmain is running in a separate thread.\n";
    ShaderTest::shadertest_main(argc, argv);
}

void sdltestmain_function(int argc, char** argv) {
    std::cout << "sdltestmain is running in a separate thread.\n";
    sdltest_main(argc, argv);
}

int main(int argc, char** argv) {
    // Initialize your main program, set up GUI, etc.
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);



    // Create a thread to run the compute shader
    std::thread myThread(sdltestmain_function,argc, argv);

    // Continue with your main program...











    // Wait for the compute thread to finish
    myThread.join();

    // Cleanup and exit
    return EXIT_SUCCESS;
}
