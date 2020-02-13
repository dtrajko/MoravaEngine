#include <stdio.h>
#include <string.h>
#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>



// Window dimensions
const GLint WIDTH = 1280;
const GLint HEIGHT = 720;

GLuint VAO;
GLuint VBO;
GLuint programID;
GLuint uniformModel;
bool direction = true;
float triangleOffset = 0.0f;
float triangleMaxOffset = 1.0f;
float triangleIncrement = 0.0005f;



// Vertex shader
static const char* vShader = R"(
#version 330

layout (location = 0) in vec3 pos;

uniform float xMove;

out vec3 v_Position;

void main()
{
	v_Position = pos;
	gl_Position = vec4(0.5 * pos.x + xMove, 0.5 * pos.y, pos.z, 1.0);
}
)";

// Fragment shader
static const char* fShader = R"(
#version 330

in vec3 v_Position;

out vec4 color;

void main()
{
	color = vec4((v_Position + 1.0) / 2.0, 1.0);
}
)";

void CreateTriangle()
{
	GLfloat vertices[] = {
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		 0.0f,  1.0f, 0.0f,
	};

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	{
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		{
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(0);
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	glBindVertexArray(0);
}

void AddShader(GLuint programID, const char* shaderCode, GLenum shaderType)
{
	const char* shaderTypeName = "Unknown";
	if (shaderType == GL_VERTEX_SHADER)               shaderTypeName = "Vertex";
	else if (shaderType == GL_FRAGMENT_SHADER)        shaderTypeName = "Fragment";
	else if (shaderType == GL_TESS_CONTROL_SHADER)    shaderTypeName = "Tessellation Control";
	else if (shaderType == GL_TESS_EVALUATION_SHADER) shaderTypeName = "Tessellation Evaluation";
	else if (shaderType == GL_GEOMETRY_SHADER)        shaderTypeName = "Geometry";
	else if (shaderType == GL_COMPUTE_SHADER)         shaderTypeName = "Compute";

	GLuint shaderID = glCreateShader(shaderType);

	const GLchar* theCode[1];
	theCode[0] = shaderCode;

	GLint codeLength[1];
	codeLength[0] = strlen(shaderCode);

	glShaderSource(shaderID, 1, theCode, codeLength);
	glCompileShader(shaderID);

	GLint result = 0;
	GLchar eLog[1024] = { 0 };

	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(shaderID, sizeof(eLog), NULL, eLog);
		printf("%s shader compilation error: '%s'\n", shaderTypeName, eLog);
		return;
	}

	printf("%s shader compiled.\n", shaderTypeName);

	glAttachShader(programID, shaderID);
	return;
}

void CompileShaders()
{
	programID = glCreateProgram();

	if (!programID)
	{
		printf("Error creating shader program!");
		return;
	}

	AddShader(programID, vShader, GL_VERTEX_SHADER);
	AddShader(programID, fShader, GL_FRAGMENT_SHADER);

	GLint result = 0;
	GLchar eLog[1024] = { 0 };

	glLinkProgram(programID);
	glGetProgramiv(programID, GL_LINK_STATUS, &result);
	if (!result)
	{
		glGetProgramInfoLog(programID, sizeof(eLog), NULL, eLog);
		printf("Shader program linking error: '%s'\n", eLog);
		return;
	}

	printf("Shader program linking complete.\n");

	glValidateProgram(programID);
	glGetProgramiv(programID, GL_VALIDATE_STATUS, &result);
	if (!result)
	{
		glGetProgramInfoLog(programID, sizeof(eLog), NULL, eLog);
		printf("Shader program validation error: '%s'\n", eLog);
		return;
	}

	uniformModel = glGetUniformLocation(programID, "xMove");

	printf("Shader program validation complete.\n");
}


int main()
{
	// Initialize GLFW
	if (!glfwInit())
	{
		printf("GLFW initialization failed!\n");
		glfwTerminate();
		return 1;
	}

	// Setup GLFW window properties
	// OpenGL version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// Core profile = No backwards compatibility
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// Allow forward compatibility
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	GLFWwindow* mainWindow = glfwCreateWindow(WIDTH, HEIGHT, "Test Window", NULL, NULL);
	if (!mainWindow)
	{
		printf("GLFW Window creation failed!\n");
		glfwTerminate();
		return 1;
	}

	// Get Buffer size information
	int bufferWidth;
	int bufferHeight;
	glfwGetFramebufferSize(mainWindow, &bufferWidth, &bufferHeight);

	// Set context for GLEW to use
	glfwMakeContextCurrent(mainWindow);

	// Allow modern extension features
	glewExperimental = GL_TRUE;

	if (glewInit() != GLEW_OK)
	{
		printf("GLEW initialization failed!\n");
		glfwDestroyWindow(mainWindow);
		glfwTerminate();
		return 1;
	}

	// Setup Viewport size
	glViewport(0, 0, bufferWidth, bufferHeight);

	printf("GLFW and GLEW initialized.\n");

	CreateTriangle();
	CompileShaders();

	// Loop until window closed
	while (!glfwWindowShouldClose(mainWindow))
	{
		// Get and handle user input events
		glfwPollEvents();

		if (direction)
		{
			triangleOffset += triangleIncrement;
		}
		else
		{
			triangleOffset -= triangleIncrement;
		}

		if (abs(triangleOffset) > triangleMaxOffset)
		{
			direction = !direction;
		}

		// Clear window
		glClearColor(0.2f, 0.0f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(programID);
		{
			glUniform1f(uniformModel, triangleOffset);

			glBindVertexArray(VAO);
			{
				glDrawArrays(GL_TRIANGLES, 0, 3);
			}
			glBindVertexArray(0);
		}
		glUseProgram(0);

		glfwSwapBuffers(mainWindow);
	}

	return 0;
}
