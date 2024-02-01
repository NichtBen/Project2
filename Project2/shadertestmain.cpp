#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <random>

GLuint computeShaderProgram;
GLuint startTexture;
GLuint resultTexture;

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

    return shader;
}

void init() {
    // Load compute shader
    const char* computeShaderPath = "CStest.glsl";  // Replace with your actual path
    GLuint computeShader = LoadComputeShader(computeShaderPath);
    if (computeShader == 0) {
        std::cerr << "Failed to load compute shader" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Create compute shader program
    computeShaderProgram = glCreateProgram();
    glAttachShader(computeShaderProgram, computeShader);
    glLinkProgram(computeShaderProgram);

    GLint linkStatus;
    glGetProgramiv(computeShaderProgram, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
        GLchar infoLog[512];
        glGetProgramInfoLog(computeShaderProgram, 512, NULL, infoLog);
        std::cerr << "Compute shader program linking error:\n" << infoLog << std::endl;
        exit(EXIT_FAILURE);
    }

    glDeleteShader(computeShader);

    // Create start texture
    glGenTextures(1, &startTexture);
    glBindTexture(GL_TEXTURE_2D, startTexture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 800, 600);

    // Set up the texture as an image in the compute shader
    glBindImageTexture(1, startTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

    // Create result texture
    glGenTextures(1, &resultTexture);
    glBindTexture(GL_TEXTURE_2D, resultTexture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 800, 600);

    // Set up the texture as an image in the compute shader
    glBindImageTexture(0, resultTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);


    // Create a random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0., 799);
    std::uniform_int_distribution<int> dis2(0,599);


    // Rebind startTexture before the loop
    glBindTexture(GL_TEXTURE_2D, startTexture);

    // Set an initial red pixel in the startTexture
    float initialColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };  // Red color
    for(int i = 0; i <70000; i ++)
        glTexSubImage2D(GL_TEXTURE_2D, 0, dis(gen), dis2(gen), 1, 1, GL_RGBA, GL_FLOAT, initialColor);
}

void renderFunction() {


    GLint timeUniformLocation = glGetUniformLocation(computeShaderProgram, "time");

    // Get the current time
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto duration = currentTime.time_since_epoch();
    float time = std::chrono::duration<float>(duration).count();

    // Pass time to the compute shader
    glUseProgram(computeShaderProgram);
    glUniform1f(timeUniformLocation, time);


    // Clear the color buffer to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Dispatch the compute shader
    glUseProgram(computeShaderProgram);
    glDispatchCompute(800 / 8, 600 / 8, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Use the resultTexture to render a quad
    glUseProgram(0); // Use fixed-function pipeline for rendering quad
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, resultTexture);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, 1.0f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    // Swap textures
    GLuint tempTexture = startTexture;
    startTexture = resultTexture;
    resultTexture = tempTexture;

    // Update texture bindings
    glBindImageTexture(1, startTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(0, resultTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);


    glutSwapBuffers();
}


void cleanup() {
    glDeleteProgram(computeShaderProgram);
    glDeleteTextures(1, &resultTexture);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutCreateWindow("Compute Shader Test");

    glewInit();

    init();

    //makes it so it is updated every click?
    glutDisplayFunc(renderFunction); 
    //makes it so it is always updated, pretty fast
    //glutIdleFunc(renderFunction);

    glutMainLoop();

    cleanup();

    return 0;
}
