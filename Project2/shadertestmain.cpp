#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <fstream>
#include <sstream>

GLuint computeShaderProgram;
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

    // Create result texture
    glGenTextures(1, &resultTexture);
    glBindTexture(GL_TEXTURE_2D, resultTexture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 800, 600);

    // Set up the texture as an image in the compute shader
    glBindImageTexture(0, resultTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    // Use the compute shader program
    glUseProgram(computeShaderProgram);
}

void renderFunction() {
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

    glutDisplayFunc(renderFunction);

    glutMainLoop();

    cleanup();

    return 0;
}
