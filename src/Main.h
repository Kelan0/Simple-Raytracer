#pragma once
#include <string>
#include <glm/glm.hpp>
#include <SDL.h>

struct Vertex;

struct Transformation;

class Mesh;

class Texture;

class Material;

class Model;

class Camera;

class Text;

class Font;

typedef unsigned int GLuint;

class Main
{
public:
	static char* string_format(const std::string fmt, ...);

	static int nextPowerOfTwo(int i);

	static int start();

	static int exit(char* message = "", int error = 0);

	static int createProgram_Rasterizer(const char* vertex_file_path, const char* fragment_file_path);
	
	static int createProgram_Raytracer(const char* compute_file_path);

	static int getUniform(int program, char* uniform);

	static void raytraceScene(double delta, GLuint shader, Camera* camera);

	static void rasterizeScene(double delta, GLuint shader, Camera* camera);

	static void renderText(double delta, GLuint shader, Camera* camera);

	static void addText(Font* font, Text text);
	
	static void setProjectionMatrixOrthographic(float left, float top, float bottom, float right);
	
	static void setProjectionMatrixPrespective(float FOV, float near, float far, unsigned width, unsigned height);

	static glm::mat4x4 getProjectionMatrix();

	static glm::mat4x4 getViewMatrix();

	static glm::mat4x4 getModelMatrix(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale);

	static glm::vec2 getMousePosition();
};

class Raytracer
{
public:
	static glm::vec3 trace(glm::vec3 eye, glm::vec3 dir);
};
