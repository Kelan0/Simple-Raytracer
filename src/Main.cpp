#define GLEW_NO_GLU
#include "Main.h"
#include <GL/glew.h>
#include <SDL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <stdarg.h>
#include <vector>
#include <string>
#include <fstream>
#include <chrono>
#include <ft2build.h>
#include FT_FREETYPE_H  
#include <map>

#define BUFFER_OFFSET(i) ((char*)NULL + (i))
#define DEFAULT_FONT_DIRECTORY "C:/Windows/Fonts/"

unsigned const int WINDOW_WIDTH = 1024;
unsigned const int WINDOW_HEIGHT = 768;
unsigned int FOV = 70;
float secondsSinceStart = 0.0F;
int workgroupWidth;
int workgroupHeight;
int numWorkgroupsX;
int numWorkgroupsY;
bool useCPU = false;
bool doBlend = true;
int numShaderOutput = 64 * 64;

//uniforms
GLint UNIFORM_MODEL_MATRIX = -1;
GLint UNIFORM_VIEW_MATRIX = -1;
GLint UNIFORM_PROJECTION_MATRIX = -1;

GLint UNIFORM_SAMPLER = -1;
GLint UNIFORM_FONT = -1;

GLint UNIFORM_EYE = -1;
GLint UNIFORM_F00 = -1;
GLint UNIFORM_F01 = -1;
GLint UNIFORM_F10 = -1;
GLint UNIFORM_F11 = -1;
GLint UNIFORM_TIME = -1;
GLint UNIFORM_RAND = -1;
GLint UNIFORM_BLEND_FACTOR = -1;

GLuint SHADER_SBO = -1;

glm::mat4x4 projectionMatrix;
glm::mat4x4 viewMatrix;
float data[WINDOW_WIDTH][WINDOW_HEIGHT][4];
std::vector<float> shaderData;

//models
Model* room;
Model* sphere1;
Mesh* screenQuad;

Texture* scene;

std::map<Font*, std::vector<Text>*> allText;
Font* font_verdana;
GLuint Font_VAO;
GLuint Font_VBO;

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texture;
	glm::vec4 colour;
};

struct Transformation	
{
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;

	Transformation(glm::vec3 position = glm::vec3(), glm::vec3 rotation = glm::vec3(), glm::vec3 scale = glm::vec3(1.0F)) : position(position), rotation(rotation), scale(scale) {}

	glm::mat4x4 getTransformationMatrix() const
	{
		glm::mat4x4 mat(1.0F);

		mat = glm::translate(mat, position);
		mat = glm::rotate(mat, rotation.x, glm::vec3(1.0F, 0.0F, 0.0F));
		mat = glm::rotate(mat, rotation.y, glm::vec3(0.0F, 1.0F, 0.0F));
		mat = glm::rotate(mat, rotation.z, glm::vec3(0.0F, 0.0F, 1.0F));
		mat = glm::scale(mat, scale);

		return mat;
	}
};

class Mesh				
{
private:
	GLuint VAO;
	GLuint VBO;
	GLuint IBO;
	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;
	std::vector<float> vData;

	Mesh::Mesh(GLuint VAO, GLuint VBO, GLuint IBO) : VAO(VAO), VBO(VBO), IBO(IBO) {}

public:
	int getNumVertices() const
	{
		return vertices.size();
	}

	int getNumIndices() const
	{
		return indices.size();
	}

	GLuint addVertex(glm::vec3 position, glm::vec3 normal = glm::vec3(), glm::vec2 texture = glm::vec2(), glm::vec4 colour = glm::vec4(1.0F))
	{
		Vertex v;
		v.position = position;
		v.normal = normal;
		v.texture = texture;
		v.colour = colour;

		vertices.push_back(v);

		vData.push_back(position[0]);
		vData.push_back(position[1]);
		vData.push_back(position[2]);
		vData.push_back(normal[0]);
		vData.push_back(normal[1]);
		vData.push_back(normal[2]);
		vData.push_back(texture[0]);
		vData.push_back(texture[1]);
		vData.push_back(colour[0]);
		vData.push_back(colour[1]);
		vData.push_back(colour[2]);
		vData.push_back(colour[3]);

		return vertices.size() - 1;
	}

	void createTriangle(GLuint v0, GLuint v1, GLuint v2)
	{
		indices.push_back(v0);
		indices.push_back(v1);
		indices.push_back(v2);
	}

	void createQuad(GLuint v0, GLuint v1, GLuint v2, GLuint v3)
	{
		createTriangle(v0, v1, v2);
		createTriangle(v0, v3, v2);
	}

	void uploadMesh() const
	{
		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), &vData[0], GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), &indices[0], GL_DYNAMIC_DRAW);
	}

	void render(double delta, GLuint shader) const
	{
		glBindVertexArray(VAO);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);

		glDrawElements(GL_TRIANGLES, getNumIndices(), GL_UNSIGNED_INT, nullptr);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		glDisableVertexAttribArray(3);

		glBindVertexArray(0);
	}

	static Mesh* createMesh()
	{
		GLuint VAO, VBO, IBO;

		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);

		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), BUFFER_OFFSET(offsetof(Vertex, position)));
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), BUFFER_OFFSET(offsetof(Vertex, normal)));
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), BUFFER_OFFSET(offsetof(Vertex, texture)));
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), BUFFER_OFFSET(offsetof(Vertex, colour)));

		glGenBuffers(1, &IBO);
		glBindVertexArray(0);

		return new Mesh(VAO, VBO, IBO);
	}

	static Mesh* createCuboid(float size = 1.0F, float width = 1.0F, float height = 1.0F, float depth = 1.0F)
	{
		Mesh* mesh = createMesh();

		width *= size;
		height *= size;
		depth *= size;

		int v00 = mesh->addVertex(glm::vec3(+0.5F * width, +0.5F * height, -0.5F * depth), glm::vec3(0.0F, +1.0F, 0.0F), glm::vec2(1.0F, 0.0F));
		int v01 = mesh->addVertex(glm::vec3(-0.5F * width, +0.5F * height, -0.5F * depth), glm::vec3(0.0F, +1.0F, 0.0F), glm::vec2(0.0F, 0.0F));
		int v02 = mesh->addVertex(glm::vec3(-0.5F * width, +0.5F * height, +0.5F * depth), glm::vec3(0.0F, +1.0F, 0.0F), glm::vec2(0.0F, 1.0F));
		int v03 = mesh->addVertex(glm::vec3(+0.5F * width, +0.5F * height, +0.5F * depth), glm::vec3(0.0F, +1.0F, 0.0F), glm::vec2(1.0F, 1.0F));
		int v04 = mesh->addVertex(glm::vec3(+0.5F * width, -0.5F * height, -0.5F * depth), glm::vec3(0.0F, -1.0F, 0.0F), glm::vec2(1.0F, 0.0F));
		int v05 = mesh->addVertex(glm::vec3(-0.5F * width, -0.5F * height, -0.5F * depth), glm::vec3(0.0F, -1.0F, 0.0F), glm::vec2(0.0F, 0.0F));
		int v06 = mesh->addVertex(glm::vec3(-0.5F * width, -0.5F * height, +0.5F * depth), glm::vec3(0.0F, -1.0F, 0.0F), glm::vec2(0.0F, 1.0F));
		int v07 = mesh->addVertex(glm::vec3(+0.5F * width, -0.5F * height, +0.5F * depth), glm::vec3(0.0F, -1.0F, 0.0F), glm::vec2(1.0F, 1.0F));
		int v08 = mesh->addVertex(glm::vec3(+0.5F * width, +0.5F * height, +0.5F * depth), glm::vec3(0.0F, 0.0F, +1.0F), glm::vec2(1.0F, 0.0F));
		int v09 = mesh->addVertex(glm::vec3(-0.5F * width, +0.5F * height, +0.5F * depth), glm::vec3(0.0F, 0.0F, +1.0F), glm::vec2(0.0F, 0.0F));
		int v10 = mesh->addVertex(glm::vec3(-0.5F * width, -0.5F * height, +0.5F * depth), glm::vec3(0.0F, 0.0F, +1.0F), glm::vec2(0.0F, 1.0F));
		int v11 = mesh->addVertex(glm::vec3(+0.5F * width, -0.5F * height, +0.5F * depth), glm::vec3(0.0F, 0.0F, +1.0F), glm::vec2(1.0F, 1.0F));
		int v12 = mesh->addVertex(glm::vec3(+0.5F * width, -0.5F * height, -0.5F * depth), glm::vec3(0.0F, 0.0F, -1.0F), glm::vec2(1.0F, 0.0F));
		int v13 = mesh->addVertex(glm::vec3(-0.5F * width, -0.5F * height, -0.5F * depth), glm::vec3(0.0F, 0.0F, -1.0F), glm::vec2(0.0F, 0.0F));
		int v14 = mesh->addVertex(glm::vec3(-0.5F * width, +0.5F * height, -0.5F * depth), glm::vec3(0.0F, 0.0F, -1.0F), glm::vec2(0.0F, 1.0F));
		int v15 = mesh->addVertex(glm::vec3(+0.5F * width, +0.5F * height, -0.5F * depth), glm::vec3(0.0F, 0.0F, -1.0F), glm::vec2(1.0F, 1.0F));
		int v16 = mesh->addVertex(glm::vec3(-0.5F * width, +0.5F * height, +0.5F * depth), glm::vec3(-1.0F, 0.0F, 0.0F), glm::vec2(1.0F, 0.0F));
		int v17 = mesh->addVertex(glm::vec3(-0.5F * width, +0.5F * height, -0.5F * depth), glm::vec3(-1.0F, 0.0F, 0.0F), glm::vec2(0.0F, 0.0F));
		int v18 = mesh->addVertex(glm::vec3(-0.5F * width, -0.5F * height, -0.5F * depth), glm::vec3(-1.0F, 0.0F, 0.0F), glm::vec2(0.0F, 1.0F));
		int v19 = mesh->addVertex(glm::vec3(-0.5F * width, -0.5F * height, +0.5F * depth), glm::vec3(-1.0F, 0.0F, 0.0F), glm::vec2(1.0F, 1.0F));
		int v20 = mesh->addVertex(glm::vec3(+0.5F * width, +0.5F * height, +0.5F * depth), glm::vec3(+1.0F, 0.0F, 0.0F), glm::vec2(1.0F, 0.0F));
		int v21 = mesh->addVertex(glm::vec3(+0.5F * width, +0.5F * height, -0.5F * depth), glm::vec3(+1.0F, 0.0F, 0.0F), glm::vec2(0.0F, 0.0F));
		int v22 = mesh->addVertex(glm::vec3(+0.5F * width, -0.5F * height, -0.5F * depth), glm::vec3(+1.0F, 0.0F, 0.0F), glm::vec2(0.0F, 1.0F));
		int v23 = mesh->addVertex(glm::vec3(+0.5F * width, -0.5F * height, +0.5F * depth), glm::vec3(+1.0F, 0.0F, 0.0F), glm::vec2(1.0F, 1.0F));

		mesh->createQuad(v00, v03, v02, v01);
		mesh->createQuad(v04, v05, v06, v07);
		mesh->createQuad(v08, v11, v10, v09);
		mesh->createQuad(v12, v15, v14, v13);
		mesh->createQuad(v16, v19, v18, v17);
		mesh->createQuad(v20, v21, v22, v23);
		mesh->uploadMesh();

		return mesh;
	}

	static Mesh* createSphere(float radius = 1.0F, unsigned int rings = 30.0F, unsigned int sectors = 30.0F)
	{
		float const R = 1.0F / (float)(rings - 1);
		float const S = 1.0F / (float)(sectors - 1);
		int r, s;

		Mesh* mesh = createMesh();

		std::vector<GLuint> indices;
		for (r = 0; r < rings; r++)
		{
			for (s = 0; s < sectors; s++)
			{
				float const x = cos(2.0F * glm::pi<float>() * s * S) * sin(glm::pi<float>() * r * R);
				float const y = sin(-glm::half_pi<float>() + glm::pi<float>() * r * R);
				float const z = sin(2.0F * glm::pi<float>() * s * S) * sin(glm::pi<float>() * r * R);

				mesh->addVertex(glm::vec3(x * radius, y * radius, z * radius), glm::vec3(x, y, z), glm::vec2(1.0f - s * S, 1.0F - r * R));
			}
		}

		for (r = 0; r < rings; r++)
		{
			for (s = 0; s < sectors; s++)
			{
				int v0 = r * sectors + s;
				int v1 = r * sectors + (s + 1);
				int v2 = (r + 1) * sectors + (s + 1);
				int v3 = (r + 1) * sectors + s;

				mesh->createQuad(v0, v1, v2, v3);
			}
		}


		mesh->uploadMesh();
		return mesh;
	}
};

class Texture			
{
	Texture::Texture(GLuint id) : id(id) {}

public:
	GLuint id;

	void bind(GLuint sampler) const
	{
		glUniform1i(UNIFORM_SAMPLER, sampler);
		glActiveTexture(GL_TEXTURE0 + sampler);
		glBindTexture(GL_TEXTURE_2D, id);
	}

	static Texture* createTexture(GLuint width, GLuint height)
	{
		GLuint id;

		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WINDOW_WIDTH, WINDOW_WIDTH, 0, GL_RGBA, GL_FLOAT, nullptr);
		glBindTexture(GL_TEXTURE_2D, 0);

		return new Texture(id);
	}
};

class Material
{
private:
	float reflect;
	float refract;
	float specular;

	Material::Material(float reflect, float refract, float specular) : reflect(reflect), refract(refract), specular(specular) {}
public:
	void bind(GLuint shader)
	{

	}

	static Material* createMaterial()
	{
		Material* material = new Material(0, 0, 0);

		return material;
	}
};

class Model
{
private:
	Mesh* mesh;
	Texture* texture;
	Material* material;
	Transformation transform;

public:
	Model::Model(Transformation transform, Mesh* mesh, Texture* texture, Material* material) : mesh(mesh), texture(texture), material(material), transform(transform) {}

	void render(double delta, GLuint shader) const
	{
		glUniformMatrix4fv(UNIFORM_MODEL_MATRIX, 1, GL_FALSE, glm::value_ptr(transform.getTransformationMatrix()));

		texture->bind(0);
		material->bind(shader);
		mesh->render(delta, shader);
	}

	const Mesh* getMesh() const
	{
		return mesh;
	}

	const Texture* getTexture() const
	{
		return texture;
	}

	const Material* getMaterial() const
	{
		return material;
	}
};

class Camera
{
	glm::vec3 position;
	glm::vec3 X;
	glm::vec3 Y;
	glm::vec3 Z;
	glm::quat rotation;
	float roll;

	void setRotation(glm::quat rotation)
	{
		glm::mat4x4 rotationMatrix = glm::mat4_cast(rotation);
		X = glm::vec3(glm::vec4(0.0F) * rotationMatrix);
		Y = glm::vec3(glm::vec4(0.0F) * rotationMatrix);
		Z = glm::vec3(glm::vec4(0.0F) * rotationMatrix);
		//Z = glm::cross(X, Y);
	}

public:
	explicit Camera::Camera(glm::vec3 position) : position(position), roll(0.0F)
	{
		X = glm::vec3(1.0F, 0.0F, 0.0F);
		Y = glm::vec3(0.0F, 1.0F, 0.0F);
		Z = glm::vec3(0.0F, 0.0F, 1.0F);

		this->position.y = 1.8F;
		this->position.x = 2.0F;
		this->position.z = 2.0F;
	}

	void render(double delta, int shader)
	{
		//look at gold sphere
		//position = glm::vec3(3.769200, 0.984392, 1.134096);
		//Z = glm::vec3(0.855785, -0.274518, -0.438488);

		//look at cubeadd
		//position = glm::vec3(1.142840, 1.222499, 0.966323);
		//Z = glm::vec3(-0.644135, -0.548983, -0.532643);
		//Y = glm::vec3(0.0F, 1.0F, 0.0F);

		viewMatrix = glm::lookAt(position, position + Z, Y);

		glm::vec3 look = getEyeRay(0, 0);
		//
		//printf("Look: glm::vec3(%f, %f, %f), position: glm::vec3(%f, %f, %f)\n", look.x, look.y, look.z, position.x, position.y, position.z);
		glm::vec3 d_position;

		float moveSpeed = float(delta) * 2.71828; //move 2.7m/s

		{
			const Uint8* keystate = SDL_GetKeyboardState(nullptr);

			if (keystate[SDL_SCANCODE_W])
			{
				d_position += Z;
			}

			if (keystate[SDL_SCANCODE_S])
			{
				d_position -= Z;
			}

			if (keystate[SDL_SCANCODE_A])
			{
				d_position += X;
			}

			if (keystate[SDL_SCANCODE_D])
			{
				d_position -= X;
			}

			float roll = 0;

			if (keystate[SDL_SCANCODE_Q])
			{
				roll -= 1;
			}

			if (keystate[SDL_SCANCODE_E])
			{
				roll += 1;
			}

			this->roll = roll;

			if (d_position.x + d_position.y + d_position.z != 0.0F)
			{
				d_position = glm::normalize(d_position);
				doBlend = false;
			}

			position += d_position * moveSpeed;
		}
	}

	void input(SDL_Event event)
	{
		float mouseSens = 0.002F;

		float x = (Main::getMousePosition().x - WINDOW_WIDTH / 2.0F) * mouseSens;
		float y = -(Main::getMousePosition().y - WINDOW_HEIGHT / 2.0F) * mouseSens;
		float z = 0.0F;

		if (x * x + y * y + z * z > 0.0F)
		{
			doBlend = false;
		}

		if (event.type == SDL_KEYDOWN)
		{
			if (event.key.keysym.sym == SDLK_q)
			{
				z += mouseSens * 2.71828;
			}

			if (event.key.keysym.sym == SDLK_e)
			{
				z -= mouseSens * 2.71828;
			}
		}

		glm::mat4x4 rotation;

		rotation = glm::rotate(rotation, x, Y);
		rotation = glm::rotate(rotation, y, X);
		rotation = glm::rotate(rotation, z, Z);

		X = glm::vec3(glm::vec4(X, 0.0F) * rotation);
		Y = glm::vec3(glm::vec4(Y, 0.0F) * rotation);

		Z = glm::cross(X, Y);
	}

	glm::vec3 getEyeRay(float x, float y) const
	{

		glm::vec4 screenRay(x, y, -1.0F, 1.0F);
		glm::vec4 eyeRay = glm::vec4(glm::vec2(glm::inverse(Main::getProjectionMatrix()) * screenRay), -1.0F, 0.0F);
		glm::vec3 worldRay = glm::vec3(glm::inverse(Main::getViewMatrix()) * eyeRay);

		return glm::normalize(worldRay);
	}

	glm::vec3 getPosition() const
	{
		return position;
	}
};

class Text
{
public:
	std::string string;
	glm::vec2 position;
	glm::vec4 colour;
	float size;

	Text::Text(std::string string, glm::vec2 position, glm::vec4 colour, float size) : string(string), position(position), colour(colour), size(size) {}
};

class Font
{
public:
	struct Character
	{
		GLuint texture;
		int x_size;
		int y_size;
		int x_offset;
		int y_offset;
		int advance;
		unsigned char character;
	};

	std::map<char, Character> characters;
	std::string name;
	FT_Library* ft;
	FT_Face face;
	float maxSize;

	Font::Font(FT_Library* ft, std::string name, float maxSize)
	{
		this->name = name;
		this->ft = ft;
		this->maxSize = maxSize;

		int n = strlen(DEFAULT_FONT_DIRECTORY) + strlen(name.c_str()) + strlen(".ttf");

		std::string directory = std::string("C:/Windows/Fonts/" + name + ".ttf");

		if (FT_New_Face(*ft, directory.c_str(), 0, &face))
		{
			printf("An error occurred while loading the font %s from the directory \"%s\" Are you sure the file path is correct?", name.c_str(), directory);
		}
		else
		{
			printf("Creating font \"%s\"\n", name.c_str());
			FT_Set_Pixel_Sizes(face, 0.0F, maxSize);

			for (unsigned char c = 0; c < 128; c++)
			{
				if (FT_Load_Char(face, c, FT_LOAD_RENDER))
				{
					printf("An error occurred while loading the character \'%c\' Skipping this character", c);
					continue;
				}

				Character character;

				glGenTextures(1, &character.texture);
				glBindTexture(GL_TEXTURE_2D, character.texture);
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				character.x_size = face->glyph->bitmap.width;
				character.y_size = face->glyph->bitmap.rows;
				character.x_offset = face->glyph->bitmap_left;
				character.y_offset = face->glyph->bitmap_top;
				character.advance = face->glyph->advance.x;
				character.character = c;

				characters.insert(std::pair<char, Character>(c, character));
			}
			printf("\n");
		}

		FT_Done_Face(face);
	}

	void renderText(GLuint shader, std::string string, glm::vec2 position, glm::vec4 colour, float scale)
	{
		glUseProgram(shader);
		glActiveTexture(GL_TEXTURE0);
		glBindVertexArray(Font_VAO);

		float curx = position.x;
		float cury = position.y;

		std::string::const_iterator c;

		for (c = string.begin(); c != string.end(); ++c)
		{
			Character ch = characters[*c];
			float r = colour.r, g = colour.g, b = colour.b, a = colour.a;

			float advance = (ch.advance >> 6) * scale;
//
//			if (ch.character == '\t')
//			{
//				r = g = b = a = 0.0F;
//
//				int columnWidth = 24;
//				int numColumns = WINDOW_WIDTH / columnWidth;
//				int curColumn = floor(curx / numColumns);
//
//				float distanceToNext = columnWidth - (curx - columnWidth * curColumn);
//
//				advance = ((int)distanceToNext >> 6) * scale;
//			}

			float x = curx + ch.x_offset * scale;
			float y = cury - (ch.y_size - ch.y_offset) * scale;
			float w = ch.x_size * scale;
			float h = ch.y_size * scale;

			const int numFloats = sizeof(Vertex) / sizeof(float);

			float font_z = 0.0F;
			float vertices[6][numFloats] = {
				{x + 0, y + h, font_z, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, r, g, b, a},
				{x + 0, y + 0, font_z, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, r, g, b, a},
				{x + w, y + 0, font_z, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, r, g, b, a},
				{x + 0, y + h, font_z, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, r, g, b, a},
				{x + w, y + 0, font_z, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, r, g, b, a},
				{x + w, y + h, font_z, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, r, g, b, a}
			};
			glBindTexture(GL_TEXTURE_2D, ch.texture);
			glBindBuffer(GL_ARRAY_BUFFER, Font_VBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			curx += advance;
		}
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	static void Font::initializeFonts()
	{
		glGenVertexArrays(1, &Font_VAO);
		glBindVertexArray(Font_VAO);

		glGenBuffers(1, &Font_VBO);
		glBindBuffer(GL_ARRAY_BUFFER, Font_VBO);
		
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 6, nullptr, GL_DYNAMIC_DRAW);
		
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), BUFFER_OFFSET(offsetof(Vertex, position)));
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), BUFFER_OFFSET(offsetof(Vertex, normal)));
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), BUFFER_OFFSET(offsetof(Vertex, texture)));
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), BUFFER_OFFSET(offsetof(Vertex, colour)));
		
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(0);
	}
};

char* Main::string_format(const std::string fmt, ...)
{
	int size = ((int)fmt.size()) * 2 + 50;
	std::string str;
	va_list ap;

	while (true)
	{
		str.resize(size);
		va_start(ap, fmt);
		int n = vsnprintf((char*)str.data(), size, fmt.c_str(), ap);
		va_end(ap);
		
		if (n > -1 && n < size)
		{
			str.resize(n);
			break;
		}

		size = n > -1 ? n + 1 : size * 2;
	}

	char* cstr = new char[str.size() + 1];
	memcpy(cstr, str.c_str(), str.size() + 1);

	return cstr;
}

int Main::nextPowerOfTwo(int i)
{
	i--;
	i |= i >> 1; 
	i |= i >> 2; 
	i |= i >> 4; 
	i |= i >> 8; 
	i |= i >> 16;

	i++;
	return i;
}

int Main::start()
{
	printf("Running application\n");

	Camera* camera;
	SDL_Window* window;
	SDL_GLContext ctxt;
	SDL_Event event;
	GLuint rasterizeShader;
	GLuint raytraceShader;

	// ------------============ INITIALIZATION ============------------\\

	{
		// --------======== SETUP GL & SDL ========--------\\

		{
			int gl_major_version = 1;
			int gl_minor_version = 3;

			printf("Setting up OpenGL version %i.%i context with SDL\n", gl_major_version, gl_minor_version);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, gl_major_version);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, gl_minor_version);
			SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

			SDL_Init(SDL_INIT_EVERYTHING);

			window = SDL_CreateWindow("GPU Raytracer Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
			ctxt = SDL_GL_CreateContext(window);
			SDL_GL_MakeCurrent(window, ctxt);

			if (window == nullptr)
			{
				return exit("A fatal error occurred while initializing an SDL window\nApplication will exit\n", 1);
			}

			glewExperimental = GL_TRUE;
			GLenum err = glewInit();

			if (err != GLEW_OK)
			{
				return exit(string_format("A fatal error occurred while imitializing GLEW: %s\nApplication will exit", (char*)glewGetErrorString(err)), 2);
			}
			printf("Using OpenGL Shading Language version %s\n", (char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
			rasterizeShader = createProgram_Rasterizer("res/shaders/raytrace/vertex.glsl", "res/shaders/raytrace/fragment.glsl");
			{
				UNIFORM_MODEL_MATRIX = getUniform(rasterizeShader, "modelMatrix");
				UNIFORM_VIEW_MATRIX = getUniform(rasterizeShader, "viewMatrix");
				UNIFORM_PROJECTION_MATRIX = getUniform(rasterizeShader, "projectionMatrix");
				UNIFORM_SAMPLER = getUniform(rasterizeShader, "sampler");
				UNIFORM_FONT = getUniform(rasterizeShader, "font");

				screenQuad = Mesh::createMesh();
				glm::vec2 p0(0.0F, 0.0F);
				glm::vec2 p1(0.0F, 1.0F);
				glm::vec2 p2(1.0F, 0.0F);
				glm::vec2 p3(1.0F, 1.0F);

				int v0 = screenQuad->addVertex(glm::vec3(p0.x * WINDOW_WIDTH, p0.y * WINDOW_HEIGHT, 0.0F), glm::vec3(), p0, glm::vec4(1.0F, 1.0F, 1.0F, 1.0F));
				int v1 = screenQuad->addVertex(glm::vec3(p1.x * WINDOW_WIDTH, p1.y * WINDOW_HEIGHT, 0.0F), glm::vec3(), p1, glm::vec4(1.0F, 1.0F, 1.0F, 1.0F));
				int v2 = screenQuad->addVertex(glm::vec3(p2.x * WINDOW_WIDTH, p2.y * WINDOW_HEIGHT, 0.0F), glm::vec3(), p2, glm::vec4(1.0F, 1.0F, 1.0F, 1.0F));
				int v3 = screenQuad->addVertex(glm::vec3(p3.x * WINDOW_WIDTH, p3.y * WINDOW_HEIGHT, 0.0F), glm::vec3(), p3, glm::vec4(1.0F, 1.0F, 1.0F, 1.0F));

				//position.xy * 0.5 + vec2(0.5, 0.5);
				screenQuad->createTriangle(v0, v1, v2);
				screenQuad->createTriangle(v1, v3, v2);
				screenQuad->uploadMesh();

				glUseProgram(rasterizeShader);
				glUniform1i(UNIFORM_SAMPLER, 0);
				glUseProgram(0);
			}

			raytraceShader = createProgram_Raytracer("res/shaders/raytrace/compute.glsl");
			{
				GLint workgroupSize[3];
				glUseProgram(raytraceShader);
				glGetProgramiv(raytraceShader, GL_COMPUTE_WORK_GROUP_SIZE, workgroupSize);
				workgroupWidth = workgroupSize[0];
				workgroupHeight = workgroupSize[1];
				glUseProgram(0);

				scene = Texture::createTexture(WINDOW_WIDTH, WINDOW_HEIGHT);

				UNIFORM_EYE = getUniform(raytraceShader, "eye");
				UNIFORM_F00 = getUniform(raytraceShader, "ray00");
				UNIFORM_F01 = getUniform(raytraceShader, "ray01");
				UNIFORM_F10 = getUniform(raytraceShader, "ray10");
				UNIFORM_F11 = getUniform(raytraceShader, "ray11");
				UNIFORM_TIME = getUniform(raytraceShader, "time");
				UNIFORM_RAND = getUniform(raytraceShader, "rndNum");
				UNIFORM_BLEND_FACTOR = getUniform(raytraceShader, "blendFactor");

				glGenBuffers(1, &SHADER_SBO);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, SHADER_SBO);
				glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * numShaderOutput, nullptr, GL_DYNAMIC_DRAW);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
			}

			camera = new Camera(glm::vec3());

			FT_Library ft;
			FT_Init_FreeType(&ft);

			font_verdana = new Font(&ft, "Verdana", 72);

			Font::initializeFonts();
		}

		//--------======== CREATING SCENE ========--------\\

		{
			room = new Model(Transformation(), Mesh::createCuboid(1.0F, 8.0F, 3.0F, 5.0F), Texture::createTexture(0, 0), Material::createMaterial());
			sphere1 = new Model(Transformation(), Mesh::createSphere(1.0F), Texture::createTexture(0, 0), Material::createMaterial());
		}

		SDL_GL_SetSwapInterval(0);
	}

	// ------------============ GAME LOOP ============------------\\
	
	{
		bool doRun = true;
		bool mouseGrabbed = true;
		bool wireframeMode = false;

		const unsigned long long int resolution = 1000000000000000; //femtoseocnd
		double max_fps = 500;

		double delta = 0.0;
		using namespace std::chrono;

		typedef duration<long long, std::ratio<1, resolution>> time;
		typedef high_resolution_clock clock;

		long long lastTime = duration_cast<time>(clock::now().time_since_epoch()).count();
		long long lastFrame = duration_cast<time>(clock::now().time_since_epoch()).count();

		while (doRun)
		{
			long long now = duration_cast<time>(clock::now().time_since_epoch()).count();

			delta += (now - lastTime) / (static_cast<double>(resolution) / max_fps);
			lastTime = now;

			// --------======== RENDERING ========--------\\

			if (delta >= 1.0)
			{
				double d_fps = (now - lastFrame) / static_cast<double>(resolution);

				secondsSinceStart += d_fps;
				glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				glPolygonMode(GL_FRONT_AND_BACK, wireframeMode ? GL_LINE : GL_FILL);
				camera->render(d_fps, raytraceShader);
				raytraceScene(d_fps, raytraceShader, camera);
				rasterizeScene(d_fps, rasterizeShader, camera);

				float fontSize = 24.0F;
				float y = 10.0F;

				char fps[100], res[100], ray[100], wgs[100];

				float numRays = 0;

//				printf("%i\n", shaderData.size());
				for (int i = 0; i < shaderData.size(); i++)
				{
					numRays += shaderData[i];
				}

//				numRays /= (WINDOW_WIDTH * WINDOW_HEIGHT); //number of rays per pixel

				sprintf_s(fps, "%.2f", 1.0F / d_fps);
				sprintf_s(res, "%ix%i", WINDOW_WIDTH, WINDOW_HEIGHT);
				sprintf_s(ray, "%.0f", numRays);
				sprintf_s(wgs, "%i", numWorkgroupsX * numWorkgroupsY);

				addText(font_verdana, Text(std::string("FPS: ") + std::string(fps),	glm::vec2(10.0F, WINDOW_HEIGHT - (y += fontSize)), glm::vec4(1.0F, 1.0F, 1.0F, 1.0F), fontSize));
				addText(font_verdana, Text(std::string("Resolution:") + std::string(res), glm::vec2(10.0F, WINDOW_HEIGHT - (y += fontSize)), glm::vec4(1.0F, 1.0F, 1.0F, 1.0F), fontSize));
				addText(font_verdana, Text(std::string("Ray Count:") + std::string(ray), glm::vec2(10.0F, WINDOW_HEIGHT - (y += fontSize)), glm::vec4(1.0F, 1.0F, 1.0F, 1.0F), fontSize));
				addText(font_verdana, Text(std::string("Workgroup Count:") + std::string(wgs), glm::vec2(10.0F, WINDOW_HEIGHT - (y += fontSize)), glm::vec4(1.0F, 1.0F, 1.0F, 1.0F), fontSize));

				renderText(d_fps, rasterizeShader, camera);

				SDL_GL_SwapWindow(window);

				delta--;

				lastFrame = now;
			}

			doBlend = true;
			// --------======== INPUT ========-------- \\

			while (SDL_PollEvent(&event))
			{
				if (event.type == SDL_QUIT)
				{
					doRun = false;
					break;
				}

				if (event.type == SDL_KEYDOWN)
				{
					if (event.key.keysym.sym == SDLK_ESCAPE)
					{
						mouseGrabbed = !mouseGrabbed;
					}

					if (event.key.keysym.sym == SDLK_F1)
					{
						wireframeMode = !wireframeMode;
					}

					if (event.key.keysym.sym == SDLK_F2)
					{
						FOV = 70.0F;
					}
				}

				if (event.type == SDL_MOUSEWHEEL)
				{
					float f = 0.02F;
					if (event.wheel.y < 0)
					{
						FOV *= 1.0 + f;
					} else
					{
						FOV *= 1.0 - f;
					}

					if (FOV > 90.0F)
					{
						FOV = 90.0;
					} if (FOV < 2.0F)
					{
						FOV = 2.0F;
					}

				}

				camera->input(event);

				if (mouseGrabbed)
				{
					SDL_WarpMouseInWindow(window, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
					SDL_ShowCursor(SDL_FALSE);
				} else
				{
					SDL_ShowCursor(SDL_TRUE);
				}
			}
		}
	}

	SDL_GL_DeleteContext(ctxt);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return exit();
}

void Main::raytraceScene(double delta, GLuint shader, Camera* camera)
{
	setProjectionMatrixPrespective(FOV, 0.1F, 1000.0F, WINDOW_WIDTH, WINDOW_HEIGHT);
	glm::vec3 f00 = camera->getEyeRay(-1.0F, -1.0F);
	glm::vec3 f01 = camera->getEyeRay(-1.0F, +1.0F);
	glm::vec3 f10 = camera->getEyeRay(+1.0F, -1.0F);
	glm::vec3 f11 = camera->getEyeRay(+1.0F, +1.0F);

	if (!useCPU)
	{
		glUseProgram(shader);

		glUniform3f(UNIFORM_EYE, camera->getPosition().x, camera->getPosition().y, camera->getPosition().z);

		glUniform3f(UNIFORM_F00, f00.x, f00.y, f00.z);
		glUniform3f(UNIFORM_F01, f01.x, f01.y, f01.z);
		glUniform3f(UNIFORM_F10, f10.x, f10.y, f10.z);
		glUniform3f(UNIFORM_F11, f11.x, f11.y, f11.z);

		glUniform1f(UNIFORM_TIME, secondsSinceStart);
		glUniform1f(UNIFORM_RAND, (float)std::rand() / (float)RAND_MAX);
		glUniform1f(UNIFORM_BLEND_FACTOR, doBlend ? 0.99F : 0.0F);

		glBindImageTexture(0, scene->id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, SHADER_SBO);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * numShaderOutput, &shaderData, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, SHADER_SBO);

		int width = nextPowerOfTwo(WINDOW_WIDTH);
		int height = nextPowerOfTwo(WINDOW_HEIGHT);
		numWorkgroupsX = width / workgroupWidth;
		numWorkgroupsY = height / workgroupHeight;

		glDispatchCompute(numWorkgroupsX, numWorkgroupsY, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		float* ptr;
		ptr = static_cast<float*>(glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY));
		shaderData.clear();

		for (int i = 0; i < numShaderOutput; i++)
		{
			shaderData.push_back(ptr[i]);
		}

		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		glUseProgram(0);
	} else
	{
//		float data[WINDOW_WIDTH][WINDOW_HEIGHT][4];
//
//		for (int y = 0; y < WINDOW_HEIGHT; y++)
//		{
//			for (int x = 0; x < WINDOW_WIDTH; x++)
//			{
//				data[x][y][0] = 1.0F;
//				data[x][y][1] = 1.0F;
//				data[x][y][2] = 1.0F;
//				data[x][y][3] = 1.0F;
//			}
//		}
//
//		if (scene != nullptr)
//		{
//			scene->bind(0);
//			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
//			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_RGBA32F, GL_FLOAT, &data[0]);
//			delete[] data;
//		}
	}
}

void Main::rasterizeScene(double delta, GLuint shader, Camera* camera)
{
	setProjectionMatrixOrthographic(0.0F, WINDOW_HEIGHT, 0.0F, WINDOW_WIDTH);
	
	glUseProgram(shader);
	glUniformMatrix4fv(UNIFORM_PROJECTION_MATRIX, 1, GL_FALSE, glm::value_ptr(getProjectionMatrix()));
	glUniform1i(UNIFORM_FONT, false);

	glClearColor(0.3F, 0.3F, 1.0F, 1.0F);
	scene->bind(0);
	screenQuad->render(delta, shader);
}

void Main::renderText(double delta, GLuint shader, Camera* camera)
{
	glUseProgram(true);
	glUniform1i(UNIFORM_FONT, true);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	for (auto it = allText.begin(); it != allText.end(); ++it)
	{
		Font* font = it->first;
		std::vector<Text> texts = *it->second;

		for (int i = 0; i < texts.size(); i++)
		{
			Text text = texts[i];

			font->renderText(shader, text.string, text.position, text.colour, text.size / font->maxSize);

		}

		it->second->clear();
	}
}

void Main::addText(Font* font, Text text)
{
	if (font != nullptr)
	{
		if (allText.count(font) <= 0)
		{
			allText[font] = new std::vector<Text>();
		}

		allText[font]->push_back(text);
	}
}

void Main::setProjectionMatrixOrthographic(float left, float top, float bottom, float right)
{
	projectionMatrix = glm::ortho(left, right, bottom, top, -1.0F, 1.0F);
}

void Main::setProjectionMatrixPrespective(float FOV, float near, float far, unsigned width, unsigned height)
{
	//right-handed projection matrix (flip the signs of [2][3] and [2][2] to change)
	float halfFOV = tan(glm::radians(FOV / 2.0F));
	float aspect = static_cast<float>(width) / static_cast<float>(height);
	glm::mat4x4 mat(0.0F);

	mat[0][0] = 1.0F / (aspect * halfFOV);
	mat[1][1] = 1.0F / halfFOV;
	mat[2][3] = -1.0F;
	mat[2][2] = -(far + near) / (far - near);
	mat[3][2] = -(2.0F * far * near) / (far - near);

	projectionMatrix = mat;
}

glm::mat4x4 Main::getProjectionMatrix()
{
	return projectionMatrix;
}

glm::mat4x4 Main::getViewMatrix()
{
	return viewMatrix;
}

glm::mat4x4 Main::getModelMatrix(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale)
{
	glm::mat4x4 mat(1.0F);
	mat = glm::translate(mat, position);
	mat = glm::rotate(mat, rotation.x, glm::vec3(1.0F, 0.0F, 0.0F));
	mat = glm::rotate(mat, rotation.y, glm::vec3(0.0F, 1.0F, 0.0F));
	mat = glm::rotate(mat, rotation.z, glm::vec3(0.0F, 0.0F, 1.0F));
	mat = glm::scale(mat, scale);
	return mat;
}

glm::vec2 Main::getMousePosition()
{
	int x, y;
	SDL_GetMouseState(&x, &y);
	return glm::vec2(x, y);
}

int Main::exit(char* message, int error)
{
	printf(message);
	system("PAUSE");
	return error;
}

int Main::createProgram_Rasterizer(const char* vertex_file_path, const char* fragment_file_path)
{
	GLuint vertexID = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentID = glCreateShader(GL_FRAGMENT_SHADER);

	std::string vertex_src;
	std::string fragment_src;

	std::ifstream vertex_stream(vertex_file_path, std::ios::in);
	std::ifstream fragment_stream(fragment_file_path, std::ios::in);

	if (vertex_stream.is_open())
	{
		std::string line = "";
		while (getline(vertex_stream, line))
		{
			vertex_src += "\n" + line;
		}

		vertex_stream.close();
	} else
	{
		printf("An error occurred while opening the file \"%s\". Is the file path correct?\n", vertex_file_path);
		getchar();
		return 0;
	}

	if (fragment_stream.is_open())
	{
		std::string line = "";
		while (getline(fragment_stream, line))
		{
			fragment_src += "\n" + line;
		}
		fragment_stream.close();
	} else
	{
		printf("An error occurred while opening the file \"%s\". Is the file path correct?\n", fragment_file_path);
		getchar();
		return 0;
	}

	GLint result = GL_FALSE;
	int logLength;

	printf("Compiling vertex shader from file \"%s\"\n", vertex_file_path);
	const char* vertex_src_ptr = vertex_src.c_str();
	glShaderSource(vertexID, 1, &vertex_src_ptr, nullptr);
	glCompileShader(vertexID);

	glGetShaderiv(vertexID, GL_COMPILE_STATUS, &result);
	glGetShaderiv(vertexID, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0)
	{
		std::vector<char> v(logLength + 1);
		glGetShaderInfoLog(vertexID, logLength, nullptr, &v[0]);
		printf("%s\n", &v[0]);
	}

	printf("Compiling fragment shader from file \"%s\"\n", fragment_file_path);
	const char* fragment_src_ptr = fragment_src.c_str();
	glShaderSource(fragmentID, 1, &fragment_src_ptr, nullptr);
	glCompileShader(fragmentID);

	glGetShaderiv(fragmentID, GL_COMPILE_STATUS, &result);
	glGetShaderiv(fragmentID, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0)
	{
		std::vector<char> v(logLength + 1);
		glGetShaderInfoLog(fragmentID, logLength, nullptr, &v[0]);
		printf("%s\n", &v[0]);
	}

	int program = glCreateProgram();
	printf("Linking shaders to program with ID %i\n", program);
	glAttachShader(program, vertexID);
	glAttachShader(program, fragmentID);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &result);
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0)
	{
		std::vector<char> v(logLength + 1);
		glGetProgramInfoLog(program, logLength, nullptr, &v[0]);
		printf("%s\n", &v[0]);
	}

	glDetachShader(program, vertexID);
	glDetachShader(program, fragmentID);

	glDeleteShader(vertexID);
	glDeleteShader(fragmentID);

	glBindAttribLocation(program, 0, "position");
	glBindAttribLocation(program, 1, "normal");
	glBindAttribLocation(program, 2, "texture");
	glBindAttribLocation(program, 3, "colour");
	return program;
}

int Main::createProgram_Raytracer(const char* compute_file_path)
{
	GLuint computeID = glCreateShader(GL_COMPUTE_SHADER);

	std::string compute_src;

	std::ifstream compute_stream(compute_file_path, std::ios::in);

	if (compute_stream.is_open())
	{
		std::string line = "";
		while (getline(compute_stream, line))
		{
			compute_src += "\n" + line;
		}

		compute_stream.close();
	}
	else
	{
		printf("An error occurred while opening the file \"%s\". Is the file path correct?\n", compute_file_path);
		getchar();
		return 0;
	}

	GLint result = GL_FALSE;
	int logLength;

	printf("Compiling compute shader from file \"%s\"\n", compute_file_path);
	const char* compute_src_ptr = compute_src.c_str();
	glShaderSource(computeID, 1, &compute_src_ptr, nullptr);
	glCompileShader(computeID);

	glGetShaderiv(computeID, GL_COMPILE_STATUS, &result);
	glGetShaderiv(computeID, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0)
	{
		std::vector<char> v(logLength + 1);
		glGetShaderInfoLog(computeID, logLength, nullptr, &v[0]);
		printf("%s\n", &v[0]);
	}

	int program = glCreateProgram();
	printf("Linking shaders to program with ID %i\n", program);
	glAttachShader(program, computeID);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &result);
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0)
	{
		std::vector<char> v(logLength + 1);
		glGetProgramInfoLog(program, logLength, nullptr, &v[0]);
		printf("%s\n", &v[0]);
	}

	glDetachShader(program, computeID);
	glDeleteShader(computeID);

	return program;
}

int Main::getUniform(int program, char* uniform)
{
	int i = glGetUniformLocation(program, uniform);
	printf("Uniform location \"%s\" loaded to location %i\n", uniform, i);
	return i;
}

int main(int argc, char* argv[])
{
	return Main::start();
}
