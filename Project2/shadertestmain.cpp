#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <random>
#include <thread>

//window variable
int windowWidth = 1000;
int windowHeight = 800;
bool keepUpdating = true;
bool debugging = false;
int debugamountX = 3;
int debugamountY = 2;


//Simulation variable
//size of data textures --> amount of paralel agends
int simulationWidth = 50;
int simulationHeight = 20;
float targetFrameRate = 15;
float initialLifeAmount = 0.08;
float dispersion = 0.97f;
    //render variable
//size of world
int worldWidth = 250;
int worldHeight = 340;

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
    // Rebind startTexture before the loop
    glBindTexture(GL_TEXTURE_2D, startTexture);

    // Set an initial red pixel in the startTexture
    float initialColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };  // Red color
    for (int i = 0; i < simulationWidth * simulationHeight * initialLifeAmount; i++)
        glTexSubImage2D(GL_TEXTURE_2D, 0, dis(gen), dis2(gen), 1, 1, GL_RGBA, GL_FLOAT, initialColor);

    glUseProgram(0);
}

void updateCSanttestTextureBindings() {
    // Update texture bindings
    glBindImageTexture(6, startTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(7, resultTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(0, currentxyDataTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(2, currentAngleDataTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(3, nextxyDataTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(5, nextAngleDataTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    // Select shader program as the target
    glUseProgram(CSanttestProgram);
    GLint timeUniformLocation = glGetUniformLocation(CSanttestProgram, "deltaTime");
    glUniform1f(timeUniformLocation, currentTime - previousTime);
    //generate random seed
    GLuint randseedUniformLocation = glGetUniformLocation(CSanttestProgram, "randseed");
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<GLuint> dis(0, std::numeric_limits<GLuint>::max());
    GLuint randomSeed = dis(gen);
    glUniform1ui(randseedUniformLocation, randomSeed);
}

void initCSanttestDebugTextureBindings() {
    GLuint randseedUniformLocation = glGetUniformLocation(CSanttestProgram, "randseed");
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<GLuint> dis(0, std::numeric_limits<GLuint>::max());
    GLuint randomSeed = dis(gen);
    glUniform1ui(randseedUniformLocation, randomSeed);

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
            glTexSubImage2D(GL_TEXTURE_2D, 0, i,j, 1, 1, GL_RGBA, GL_FLOAT, initialColor);
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
            glTexSubImage2D(GL_TEXTURE_2D, 0, i,  j, 1, 1, GL_RGBA, GL_FLOAT, initialColor2);
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
    glUniform1f(timeUniformLocation, currentTime - previousTime);
    GLint dispersionUniformhLocation = glGetUniformLocation(CSblurProgram, "dispersion");
    glUniform1f(dispersionUniformhLocation, dispersion);
}

void initCSblurTextures() {
    // Select shader program as the target
    glUseProgram(CSblurProgram);
    GLint timeUniformLocation = glGetUniformLocation(CSblurProgram, "deltaTime");
    glUniform1f(timeUniformLocation, currentTime - previousTime);
    GLint dispersionUniformhLocation = glGetUniformLocation(CSblurProgram, "dispersion");
    glUniform1f(dispersionUniformhLocation, dispersion);
   
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

void init() {
    // Adjust the viewport size to match the desired resolution
    glViewport(0, 0, simulationWidth, simulationHeight);


    // Load compute shader


    std::cout << "end3.1";

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




}


void renderTextureToScreen(GLint texture, int startx, int starty, int width, int height,int i, int j) {

    if (texture != NULL) {

        glUseProgram(0); // Use fixed-function pipeline for rendering quad
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture);

        glBegin(GL_QUADS);
        glTexCoord2f(i, j); glVertex2f(startx, starty);
        glTexCoord2f(i + 1, j); glVertex2f(startx + width, starty);
        glTexCoord2f(i + 1, j + 1); glVertex2f(startx + width, starty + height);
        glTexCoord2f(i, j + 1); glVertex2f(startx, starty + height);
        glEnd();


        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);

    }
}


void renderFunction() {


   
    // Get the current time
    currentTime = glutGet(GLUT_ELAPSED_TIME);
    
    if (targetFrameRate != 0 && currentTime - previousTime < 1000 / targetFrameRate) {
        return;
    }

    // Clear the color buffer to black
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);



    if(CStest)
    {
        //initialise texture context for shader
        initCStestTextures();
        // Dispatch the compute shader
        glUseProgram(CStestProgram);
        glDispatchCompute(simulationWidth / 8, simulationHeight / 8, 1);
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

    if (CSblur)
    {
        //initialise texture context for shader
        updateBlurTextureBindings();
        // Dispatch the compute shader
        glUseProgram(CSblurProgram);
        glDispatchCompute(windowWidth / 8, windowHeight / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    }


    

    //display the different texture for debugging, or only result:
    
    if (!debugging) {
        renderTextureToScreen(resultTexture, 0, 0, simulationWidth, simulationHeight,0,0);
    }
    else{
        for (int i = 0; i < debugamountX; i++) {
            for (int j = 0; j < debugamountY; j++) {
                int tempx = simulationWidth / debugamountX;
                int tempy = simulationHeight / debugamountY;
                renderTextureToScreen(*textures[i*debugamountY + j], tempx*i,tempy*j, tempx,tempy,i,j);

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



    glutSwapBuffers();

    previousTime = currentTime;

    glutPostRedisplay();
}


void cleanup() {
    glDeleteProgram(CSanttestProgram);
    glDeleteTextures(1, &resultTexture);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);

    // Set initial window size
    glutInitWindowSize(windowWidth, windowHeight);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutCreateWindow("Compute Shader Test");

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, simulationWidth, 0, simulationHeight, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glViewport(0, 0, simulationWidth, simulationHeight);

    glewInit();


    std::cout << "end3";
    init();

    std::cout << "end2.5";
    //makes it so it is updated every click?
    glutDisplayFunc(renderFunction); 
    //makes it so it is always updated, pretty fast
    if (keepUpdating) {
        glutIdleFunc(renderFunction);
    }

    std::cout << "end2";
    glutMainLoop();

    cleanup();
    std::cout << "end";

    return 0;
}


