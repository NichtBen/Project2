#include <GL/glew.h>
#include <SDL.h>
#include <iostream>
#include <SDL.h>
#include <fstream>
#include <sstream>
#include <chrono>
#include <random>
#include <thread>
#include <chrono>


#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_opengl3.h"


std::atomic<bool> stopFlag(false);


//to be done so you can increase decrease amount of agends
    //to be done WATCHOUT same class  vars in simulationWindow class!
int maxSimulationWindowWidth = 1900;
int maxSimulationWindowHeight = 1000;

int simulationWindowWidth = 1900;
int simulationWindowHeight = 1000;

int GUIWindowWidth = 600;
int GUIWindowHeight = 200;

class WindowInterface {
public:
    virtual bool init() = 0;
    virtual void render() = 0;
    virtual void cleanup() = 0;
    virtual ~WindowInterface() {}
    virtual SDL_Window* getWindow() = 0; // Getter for window
    virtual SDL_GLContext getGLContext() = 0; // Getter for GL context
    virtual int getWidth() = 0;
    virtual int getHeight() = 0;
};

class OpenGLSimulationWindow : public WindowInterface{

public:
    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;
    int windowWidth;
    int windowHeight;
    const char* title;
    //window variable
    bool keepUpdating = true;
    bool debugging = false;
    int debugamountX = 3;
    int debugamountY = 2;


    //Simulation variable
    //size of data textures --> amount of paralel agends
    //to be done WATCHOUT same global vars!
    int maxSimulationWidth = 100;
    int maxSimulationHeight = 100;
    int simulationWidth = 1000;
    int simulationHeight = 1000;
    float targetFrameRate = 60;
    float diffusion = 0.2f;
    float evaporation = 0.2f;
    float moveSpeed = 1.0f;
    float steeringangle = 0.2f;
    float randomangle = 0.05f;
    //render variable
//size of world
    int worldWidth = 2000;
    int worldHeight = 1000;

    GLuint CStestProgram;
    GLuint CSanttestProgram;
    GLuint CSblurProgram;

    GLuint* shaderprograms[10];

    bool CSanttest = true;
    bool CStest = false;
    bool CSblur = true;

    GLuint startTexture;
    GLuint resultTexture;

    GLuint currentxyDataTexture;
    GLuint currentAngleDataTexture;
    GLuint nextxyDataTexture;
    GLuint nextAngleDataTexture;

    GLuint* textures[6];

    int previousTime;
    int currentTime;
    const float PI = 3.14159265358979323846;

    bool paused = false;

    int getWidth() override {
        return windowWidth;
    }

    int getHeight() override {
        return windowHeight;
    }

    bool isPaused() {
        return paused;
    }

    SDL_Window* getWindow() override{
        return window;
    }

    SDL_GLContext getGLContext() override {
        return glContext;
    }

    OpenGLSimulationWindow(const char* newTitle, int width, int height) : windowWidth(width), windowHeight(height), title(newTitle)
    {

        init();
    }


    void pauseSimulation() {
        paused = true;
    }

    void continueSimulation() {
        paused = false;
    }


    GLuint LoadComputeShader(const char* shaderPath) {
        // Read the shader source code from the file
        std::ifstream shaderFile(shaderPath);
        if (!shaderFile.is_open()) {
            std::cerr << "Failed to open shader file: " << shaderPath << std::endl;
            return 0;
        }

        std::stringstream shaderStream;
        shaderStream << shaderFile.rdbuf();
        std::string shaderCode = shaderStream.str();
        const GLchar* shaderSource = shaderCode.c_str();

        // Create and compile the shader
        GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(shader, 1, &shaderSource, NULL);
        glCompileShader(shader);

        // Check for compilation errors
        GLint status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status != GL_TRUE) {
            GLchar infoLog[512];
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            std::cerr << "Compute shader compilation error:\n" << infoLog << std::endl;
            glDeleteShader(shader);
            return 0;
        }

        if (shader == 0) {
            std::cerr << "Failed to load compute shader" << std::endl;
            exit(EXIT_FAILURE);
        }

        // Create compute shader program
        GLuint computeShaderProgram = glCreateProgram();
        glAttachShader(computeShaderProgram, shader);
        glLinkProgram(computeShaderProgram);

        GLint linkStatus;
        glGetProgramiv(computeShaderProgram, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLchar infoLog[512];
            glGetProgramInfoLog(computeShaderProgram, 512, NULL, infoLog);
            std::cerr << "Compute shader program linking error:\n" << infoLog << std::endl;
            exit(EXIT_FAILURE);
        }

        glDeleteShader(shader);

        return computeShaderProgram;

    }

    bool stringContainsShaderName(const char* path, const char* shaderName) {
        // Check if the shaderName is present in the path
        return strstr(path, shaderName) != nullptr;
    }

    void initCStestTextures()
    {

        glUseProgram(CStestProgram);

        glBindImageTexture(1, startTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        glBindImageTexture(0, resultTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

        // Create start texture
        glGenTextures(1, &startTexture);
        glBindTexture(GL_TEXTURE_2D, startTexture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, simulationWidth, simulationHeight);

        // Set up the texture as an image in the compute shader
        glBindImageTexture(1, startTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

        // Create result texture
        glGenTextures(1, &resultTexture);
        glBindTexture(GL_TEXTURE_2D, resultTexture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, simulationWidth, simulationHeight);

        // Set up the texture as an image in the compute shader
        glBindImageTexture(0, resultTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

        glUseProgram(0);
    }

    void initCStest()
    {
        glUseProgram(CStest);

        initCStestTextures();

        // Create a random number generator
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(0., simulationWidth - 1);
        std::uniform_int_distribution<int> dis2(0, simulationHeight - 1);
        
        glUseProgram(0);
    }

    void updateCSanttestTextureBindings() {

        // Select shader program as the target
        glUseProgram(CSanttestProgram);

        // Update texture bindings
        glBindImageTexture(6, startTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        glBindImageTexture(7, resultTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindImageTexture(0, currentxyDataTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindImageTexture(2, currentAngleDataTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindImageTexture(3, nextxyDataTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindImageTexture(5, nextAngleDataTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);


        GLint timeUniformLocation = glGetUniformLocation(CSanttestProgram, "deltaTime");
        glUniform1f(timeUniformLocation, (currentTime - previousTime) / (1000 / targetFrameRate));
        //generate random seed
        GLuint randseedUniformLocation = glGetUniformLocation(CSanttestProgram, "randseed");
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<GLuint> dis(0, std::numeric_limits<GLuint>::max());
        GLuint randomSeed = dis(gen);
        glUniform1ui(randseedUniformLocation, randomSeed);


        GLuint moveSpeedUniformLocation = glGetUniformLocation(CSanttestProgram, "moveSpeed");
        glUniform1f(moveSpeedUniformLocation, moveSpeed);


        GLuint steeringangleUniformLocation = glGetUniformLocation(CSanttestProgram, "steeringangle");
        glUniform1f(steeringangleUniformLocation, steeringangle);


        GLuint randomangleUniformLocation = glGetUniformLocation(CSanttestProgram, "randomangle");
        glUniform1f(randomangleUniformLocation, randomangle);


    }

    void initCSanttestDebugTextureBindings() {


        //set up for showing all data for debugging
        textures[0] = &startTexture;
        textures[2] = &currentxyDataTexture;
        textures[4] = &currentAngleDataTexture;
        textures[1] = &resultTexture;
        textures[3] = &nextxyDataTexture;
        textures[5] = &nextAngleDataTexture;

    }


    //creates and binds all textures etc for render call
    void initCSanttestTextures() {
        //select shader programm as target 
        glUseProgram(CSanttestProgram);


        GLint timeUniformLocation = glGetUniformLocation(CSanttestProgram, "deltaTime");
        glUniform1f(timeUniformLocation, (currentTime - previousTime) / (1000.0 / targetFrameRate));

        GLuint randseedUniformLocation = glGetUniformLocation(CSanttestProgram, "randseed");
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<GLuint> dis(0, std::numeric_limits<GLuint>::max());
        GLuint randomSeed = dis(gen);
        glUniform1ui(randseedUniformLocation, randomSeed);


        GLuint moveSpeedUniformLocation = glGetUniformLocation(CSanttestProgram, "moveSpeed");
        std::cout << "  :: " << moveSpeedUniformLocation;
        glUniform1f(moveSpeedUniformLocation, moveSpeed);


        GLuint steeringangleUniformLocation = glGetUniformLocation(CSanttestProgram, "steeringangle");
        glUniform1f(steeringangleUniformLocation, steeringangle);


        GLuint randomangleUniformLocation = glGetUniformLocation(CSanttestProgram, "randomangle");
        glUniform1f(randomangleUniformLocation, randomangle);



        //set up shader constants worldwidth and worldhigh
        GLint widthLocation = glGetUniformLocation(CSanttestProgram, "worldWidth");
        GLint heightLocation = glGetUniformLocation(CSanttestProgram, "worldHeight");

        glUniform1ui(widthLocation, worldWidth);
        glUniform1ui(heightLocation, worldHeight);

        // Update texture bindings
        glBindImageTexture(6, startTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        glBindImageTexture(7, resultTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindImageTexture(0, currentxyDataTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindImageTexture(2, currentAngleDataTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindImageTexture(3, nextxyDataTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindImageTexture(5, nextAngleDataTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);


        //select shader programm as target 
        glUseProgram(CSanttestProgram);

        // Create currendxdata texture holding xy data for agends
        glGenTextures(1, &currentxyDataTexture);
        glBindTexture(GL_TEXTURE_2D, currentxyDataTexture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, simulationWidth, simulationHeight);

        // Set up the texture as an image in the compute shader
        glBindImageTexture(0, currentxyDataTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

        // Create currendxdata texture holding x data for agends
        glGenTextures(1, &currentAngleDataTexture);
        glBindTexture(GL_TEXTURE_2D, currentAngleDataTexture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, simulationWidth, simulationHeight);

        // Set up the texture as an image in the compute shader
        glBindImageTexture(2, currentAngleDataTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

        // Create currendxdata texture holding x data for agends
        glGenTextures(1, &nextxyDataTexture);
        glBindTexture(GL_TEXTURE_2D, nextxyDataTexture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, simulationWidth, simulationHeight);

        // Set up the texture as an image in the compute shader
        glBindImageTexture(3, nextxyDataTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

        // Create currendxdata texture holding x data for agends
        glGenTextures(1, &nextAngleDataTexture);
        glBindTexture(GL_TEXTURE_2D, nextAngleDataTexture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, simulationWidth, simulationHeight);

        // Set up the texture as an image in the compute shader
        glBindImageTexture(5, nextAngleDataTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

        // Graphic textures

        // Create start texture
        glGenTextures(1, &startTexture);
        glBindTexture(GL_TEXTURE_2D, startTexture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, worldWidth, worldHeight);

        // Set up the texture as an image in the compute shader
        glBindImageTexture(6, startTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

        // Create result texture
        glGenTextures(1, &resultTexture);
        glBindTexture(GL_TEXTURE_2D, resultTexture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, worldWidth, worldHeight);

        // Set up the texture as an image in the compute shader
        glBindImageTexture(7, resultTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);




        glUseProgram(0);
    }

    void initCSanttest() {

        //Data textures init moved away to initCSsantestTextures for better overview

        initCSanttestTextures();

        initCSanttestDebugTextureBindings();


        //create initial data

        // Create a random number generator
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(0., simulationWidth - 1);
        std::uniform_int_distribution<int> dis2(0, simulationHeight - 1);
        std::uniform_real_distribution<float> dis3(0, 1);


        srand((unsigned)time(NULL));

        std::cout << "end7";

        // Set an initial xy   for agends
        glBindTexture(GL_TEXTURE_2D, currentxyDataTexture);
        for (int i = 0; i < simulationWidth; i++)
        {
            for (int j = 0; j < simulationHeight; j++)
            {
                float initialColor[4] = { dis3(gen) * worldWidth, dis3(gen) * worldHeight, 0.0f, 1.0f };  // random pos
                glTexSubImage2D(GL_TEXTURE_2D, 0, i, j, 1, 1, GL_RGBA, GL_FLOAT, initialColor);
            }
        }

        std::cout << "end6";
        // Set an initial angle  for agends
        glBindTexture(GL_TEXTURE_2D, currentAngleDataTexture);
        for (int i = 0; i < simulationWidth; i++)
        {
            for (int j = 0; j < simulationHeight; j++)
            {
                float initialColor2[4] = { dis3(gen) * 2 * PI, 0.0f, 0.0f, 1.0f };  // random angle
                glTexSubImage2D(GL_TEXTURE_2D, 0, i, j, 1, 1, GL_RGBA, GL_FLOAT, initialColor2);
            }
        }

        glUseProgram(0);
    }

    void updateBlurTextureBindings() {
        // Update texture bindings
        glBindImageTexture(7, resultTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

        // Select shader program as the target
        glUseProgram(CSblurProgram);
        GLint timeUniformLocation = glGetUniformLocation(CSblurProgram, "deltaTime");
        glUniform1f(timeUniformLocation, (currentTime - previousTime) / (1000 / targetFrameRate));

        GLint diffusionUniformhLocation = glGetUniformLocation(CSblurProgram, "diffusion");
        glUniform1f(diffusionUniformhLocation, diffusion);

        GLuint evaporationUniformLocation = glGetUniformLocation(CSblurProgram, "evaporation");
        std::cout << "  :: " << evaporationUniformLocation;
        glUniform1f(evaporationUniformLocation, evaporation);
    }

    void initCSblurTextures() {
        // Select shader program as the target
        glUseProgram(CSblurProgram);
        GLint timeUniformLocation = glGetUniformLocation(CSblurProgram, "deltaTime");
        glUniform1f(timeUniformLocation, (currentTime - previousTime) / (1000 / targetFrameRate));

        GLint diffusionUniformhLocation = glGetUniformLocation(CSblurProgram, "diffusion");
        glUniform1f(diffusionUniformhLocation, diffusion);

        GLuint evaporationUniformLocation = glGetUniformLocation(CSblurProgram, "evaporation");
        std::cout << "  :: " << evaporationUniformLocation;
        glUniform1f(evaporationUniformLocation, evaporation);

        glGenTextures(1, &resultTexture);
        glBindTexture(GL_TEXTURE_2D, resultTexture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, worldWidth, worldHeight);

        // Set up the texture as an image in the compute shader
        glBindImageTexture(7, resultTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);


        glUseProgram(0);
    }

    void initCSblur()
    {
        initCSblurTextures();
    }

    bool init() override {

        std::cout << " error?:" << glGetError() << "\n";
        std::cout << " error?:" << glGetError() << "\n";
        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
            return false;
        }

        // Adjust window size based on windowWidth and windowHeight
        windowWidth = simulationWindowWidth;
        windowHeight = simulationWindowHeight;

        // Create a window
        window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        if (window == nullptr) {
            std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return false;
        }

        // Create an OpenGL context
        glContext = SDL_GL_CreateContext(window);
        if (glContext == nullptr) {
            std::cerr << "OpenGL context creation failed: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(window);
            SDL_Quit();
            return false;
        }

        // Initialize GLEW
        GLenum glewInitResult = glewInit();
        if (glewInitResult != GLEW_OK) {
            std::cerr << "GLEW initialization failed: " << glewGetErrorString(glewInitResult) << std::endl;
            SDL_GL_DeleteContext(glContext);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return false;
        }


        std::cout << " error?6:" << glGetError() << "\n";

        // Set up OpenGL viewport
        glViewport(0, 0, windowWidth, windowHeight);


        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, simulationWidth, 0, simulationHeight, -1, 1);
        glMatrixMode(GL_MODELVIEW);

        glewInit();





        std::cout << "end3" << glGetError();


        // Load compute shader


        std::cout << "end3.1" << glGetError();

        //only init the shader currently used in computeShaderPath
        if (CStest) {
            initCStest();
        }
        if (CSanttest) {
            CSanttestProgram = LoadComputeShader("CSanttest.glsl");
            initCSanttest();
        }
        if (CSblur) {

            CSblurProgram = LoadComputeShader("CSblur.glsl");
            initCSblur();
        }


        return true;

    }


        void renderTextureToScreen(GLint texture, int startx, int starty, int width, int height) {
            if (texture != NULL) {
                // Bind your texture
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, texture); 

                // Draw a textured quad
                glBegin(GL_QUADS);
                glTexCoord2f(0.0f, 0.0f); glVertex2f(startx, starty);                   // Bottom-left corner
                glTexCoord2f(1.0f, 0.0f); glVertex2f(startx + width, starty);            // Bottom-right corner
                glTexCoord2f(1.0f, 1.0f); glVertex2f(startx + width, starty + height);   // Top-right corner
                glTexCoord2f(0.0f, 1.0f); glVertex2f(startx, starty + height);           // Top-left corner
                glEnd();
            }
        }
    


    void render() {
        if (paused)
            return;


        // Get the current time
        currentTime = clock();

        if (targetFrameRate != 0 && currentTime - previousTime < 1000 / targetFrameRate) {
            return;
        }




        if (CStest)
        {
            //initialise texture context for shader
            initCStestTextures();
            // Dispatch the compute shader
            glUseProgram(CStestProgram);
            glDispatchCompute(simulationWidth / 8, simulationHeight / 8, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        }
        //first blur then CSanttest!!
        if (CSblur)
        {
            //initialise texture context for shader
            updateBlurTextureBindings();
            // Dispatch the compute shader
            glUseProgram(CSblurProgram);
            glDispatchCompute(worldWidth / 8, worldHeight / 8, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        }

        if (CSanttest)
        {
            // update texture bindings
            updateCSanttestTextureBindings();
            // Dispatch the compute shader
            glUseProgram(CSanttestProgram);
            glDispatchCompute(simulationWidth / 8, simulationHeight / 8, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        }







        // Clear the color buffer to black
        glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, windowWidth, 0, windowHeight, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();


        //display the different texture for debugging, or only result:

        glBindTexture(GL_TEXTURE_2D, resultTexture);
        if (!debugging) {
            renderTextureToScreen(resultTexture, 0, 0, windowWidth, windowHeight);
        }
        else {
            for (int i = 0; i < debugamountX; i++) {
                for (int j = 0; j < debugamountY; j++) {
                    int tempx = simulationWidth / debugamountX;
                    int tempy = simulationHeight / debugamountY;
                    renderTextureToScreen(*textures[i * debugamountY + j], tempx * i, tempy * j, tempx, tempy);

                }
            }
        }


        // Swap textures
        GLuint tempTexture = startTexture;
        //startTexture = resultTexture;
        //resultTexture = tempTexture;

        tempTexture = currentxyDataTexture;
        currentxyDataTexture = nextxyDataTexture;
        nextxyDataTexture = tempTexture;

        tempTexture = currentAngleDataTexture;
        currentAngleDataTexture = nextAngleDataTexture;
        nextAngleDataTexture = tempTexture;

        // Swap buffers
        SDL_GL_SwapWindow(window);

        // Check for OpenGL errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "OpenGL error: " << error << std::endl;
        }

        previousTime = currentTime;
    }

    void cleanup() {
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }


};

class OpenGlGuiWindow : public WindowInterface {
    int windowWidth;
    int windowHeight;
    const char* title;
private:
    SDL_Window* window;
    SDL_GLContext glContext;
    OpenGLSimulationWindow* target;

public:
    OpenGlGuiWindow(const char* newTitle, int width, int height, OpenGLSimulationWindow* targetwindow) : windowWidth(width), windowHeight(height), title(newTitle), target(targetwindow) {
        init();
    }

    ~OpenGlGuiWindow() {
        // Cleanup ImGui
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        // Cleanup SDL
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }


    int getWidth() override {
        return windowWidth;
    }

    int getHeight() override {
        return windowHeight;
    }

    SDL_Window* getWindow() override {
        return window;
    }

    SDL_GLContext getGLContext() override {
        return glContext;
    }

    bool init() override {

        // Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        // Initialize SDL2 and create window
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
            return false;
        }

        window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        if (window == nullptr) {
            std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return false;
        }

        glContext = SDL_GL_CreateContext(window);
        if (glContext == nullptr) {
            std::cerr << "OpenGL context creation failed: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(window);
            SDL_Quit();
            return false;
        }

        // Initialize ImGui SDL2 and OpenGL bindings
        ImGui_ImplSDL2_InitForOpenGL(window, glContext);
        ImGui_ImplOpenGL3_Init("#version 130");

        return true;

    }
    

    void render() override {

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        // Create GUI elements
        ImGui::Begin("GUI Window");

        ImGui::Text("Hello, ImGui!");
        if (ImGui::Button("Pause/Unpause")) {
            std::cout << "Button clicked!: Paused" << std::endl;
            if (target->isPaused())
                target->continueSimulation();
            else target->pauseSimulation();
        }

        static float sliderSteeringangle = target->steeringangle;
        ImGui::SliderFloat("Steering Angle", &sliderSteeringangle, -2.0f, 2.0f);

        target->steeringangle = sliderSteeringangle;

        static float sliderDiffusion = target->diffusion;
        ImGui::SliderFloat("Diffusion", &sliderDiffusion, -0.0f, 1.0f);

        target->diffusion = sliderDiffusion;

        static float sliderRandomangle = target->randomangle;
        ImGui::SliderFloat("Random Angle", &sliderRandomangle, -2.0f, 2.0f);

        target->randomangle = sliderRandomangle;

        static float sliderMoveSpeed = target->moveSpeed;
        ImGui::SliderFloat("Move Speed ", &sliderMoveSpeed, -2.0f, 2.0f);

        target->moveSpeed = sliderMoveSpeed;

        static float sliderSimulationSpeed = target->targetFrameRate;
        ImGui::SliderFloat("Frame rate/ simulationspeed ", &sliderSimulationSpeed, -2.0f, 2.0f);

        target->targetFrameRate = sliderSimulationSpeed;

        static float sliderEvaporation = target->evaporation;
        ImGui::SliderFloat("Evaporation", &sliderEvaporation, -2.0f, 2.0f);

        target->evaporation = sliderEvaporation;



        static char textInput[128] = "";
        ImGui::InputText("Input", textInput, sizeof(textInput));

        ImGui::End();




        // Render ImGui frame
        ImGui::Render();
        glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    void cleanup() override {
        // Cleanup ImGui
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        // Cleanup SDL
        if (glContext != nullptr) {
            SDL_GL_DeleteContext(glContext);
        }
        if (window != nullptr) {
            SDL_DestroyWindow(window);
        }
        SDL_Quit();
    }

};

void resizeViewport(int width, int height) {
    // Update the OpenGL viewport to match the window size
    glViewport(0, 0, width, height);
}


int sdltest_main(int argc, char* argv[]) {
    OpenGLSimulationWindow simulationWindow("Simualtion", simulationWindowWidth, simulationWindowHeight);
    OpenGlGuiWindow guiwindow("Controlls",GUIWindowWidth, GUIWindowHeight,&simulationWindow);

    int maxwindows = 2;
    WindowInterface *windows[2];

    for (WindowInterface* w : windows)
        w = 0;

    windows[0] = &simulationWindow;
    windows[1] = &guiwindow;

  

    bool quit = false;
    SDL_Event event;
    while (!quit && !stopFlag) {
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT) {
                quit = true;
                std::cout << "SD_Quit\n";
            }
            else if (event.type == SDL_WINDOWEVENT) {
                // Handle window events
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    std::cout << "SDL_windowevent_close \n";
                    // close all
                    for (int i = 0; i < maxwindows; i++) {

                        if (windows[i] != 0) {
                            // Close all windows
                            quit = true;
                            //windows[i]->cleanup();
                            //windows[i] = 0;
                            break;
                        }
                        
                    }
                }else if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    // If the window size changed, update the viewport
                    int width = event.window.data1;
                    int height = event.window.data2;
                    std::cout << "123xxxxxxxxxxxx3123";
                    resizeViewport(width, height);
                }
            } 
        }
        if(!quit)
        for(WindowInterface *window : windows)
        {
            if (window != 0)
            {
                // Set current OpenGL context
                SDL_GL_MakeCurrent(window->getWindow(), window->getGLContext());

                // Perform OpenGL initialization
                glViewport(0, 0, window->getWidth(), window->getHeight());
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                window->render();
            }
        }
    }
    return 0;
}
