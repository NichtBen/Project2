#include <iostream>
#include <thread>
#include "shadertestmain.h"
#include "sldtest.h"

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

    // Create a thread to run the compute shader
    std::thread myThread(sdltestmain_function,argc, argv);

    // Continue with your main program...











    // Wait for the compute thread to finish
    myThread.join();

    // Cleanup and exit
    return EXIT_SUCCESS;
}
