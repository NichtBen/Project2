#ifndef SHADER_TEST_MAIN_H
#define SHADER_TEST_MAIN_H

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <random>
#include <thread>


namespace ShaderTest {

	extern int windowWidth;
	extern int windowHeight;
	extern bool keepUpdating;
	extern bool debugging;
	extern int debugamountX;
	extern int debugamountY;

	extern int simulationWidth;
	extern int simulationHeight;
	extern float targetFrameRate;
	extern float initialLifeAmount;
	extern float diffusion;
	extern float moveSpeed;
	extern float steeringangle;
	extern float randomangle;

	extern int worldWidth;
	extern int worldHeight;

	extern GLuint CStestProgram;
	extern GLuint CSanttestProgram;
	extern GLuint CSblurProgram;

	extern GLuint* shaderprograms[10];

	extern bool CSanttest;
	extern bool CStest;
	extern bool CSblur;

	extern GLuint startTexture;
	extern GLuint resultTexture;

	extern GLuint currentxyDataTexture;
	extern GLuint currentAngleDataTexture;
	extern GLuint nextxyDataTexture;
	extern GLuint nextAngleDataTexture;

	extern GLuint* textures[6];

	extern int previousTime;
	extern int currentTime;

	extern const float PI;

	int shadertest_main(int argc, char** argv);

	GLuint LoadComputeShader(const char* shaderPath);
	bool stringContainsShaderName(const char* path, const char* shaderName);

	void initCStestTextures();
	void initCStest();
	void updateCSanttestTextureBindings();
	void initCSanttestDebugTextureBindings();
	void initCSanttestTextures();
	void initCSanttest();
	void updateBlurTextureBindings();
	void initCSblurTextures();
	void initCSblur();
	void init();

	void renderTextureToScreen(GLint texture, int startx, int starty, int width, int height, int i, int j);
	void renderFunction();
	void cleanup();

#endif // COMPUTE_SHADER_MAIN_TEST_H

}