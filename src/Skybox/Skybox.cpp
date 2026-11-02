#include "Skybox.h"


Skybox::Skybox()
{
}

Skybox::Skybox(std::vector<std::string> faceLocations, bool flipVert)
{
	// Shader setup
	MoravaShaderSpecification moravaShaderSpecification;
	moravaShaderSpecification.ShaderType = MoravaShaderSpecification::ShaderType::MoravaShader;
	moravaShaderSpecification.VertexShaderPath = "Shaders/skybox.vert";
	moravaShaderSpecification.FragmentShaderPath = "Shaders/skybox.frag";
	m_SkyShader = MoravaShader::Create(moravaShaderSpecification);

	// Texture setup
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width;
	int height;
	int bitDepth;
	GLenum internalFormat = 0;
	GLenum dataFormat = 0;

	stbi_set_flip_vertically_on_load(flipVert ? 1 : 0);

	for (unsigned int i = 0; i < 6; i++)
	{
		unsigned char* texData = stbi_load(faceLocations[i].c_str(), &width, &height, &bitDepth, 0);
		if (!texData)
		{
			printf("Failed to find: '%s'\n", faceLocations[i].c_str());
			return;
		}

		if (bitDepth == 4)
		{
			internalFormat = GL_RGBA8;
			dataFormat = GL_RGBA;
		}
		else if (bitDepth == 3)
		{
			internalFormat = GL_RGB8;
			dataFormat = GL_RGB;
		}

		printf("Loading cube texture [SlotID=%d] '%s'\n", GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, faceLocations[i].c_str());

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, texData);
		stbi_image_free(texData);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	float skyboxVertices[] = {
		-1.0f,  1.0f, -1.0f,    0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,
		-1.0f, -1.0f, -1.0f,    0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, -1.0f,    0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,
		 1.0f, -1.0f, -1.0f,    0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,

		-1.0f,  1.0f,  1.0f,    0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,
		 1.0f,  1.0f,  1.0f,    0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,
		-1.0f, -1.0f,  1.0f,    0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f,    0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 0.0f,
	};

	// Mesh Setup
	unsigned int skyboxIndices[] = {
		// front
		0, 1, 2,
		2, 1, 3,
		// right
		2, 3, 5,
		5, 3, 7,
		// back
		5, 7, 4,
		4, 7, 6,
		// left
		4, 6, 0,
		0, 6, 1,
		// top
		4, 0, 5,
		5, 0, 2,
		// bottom
		1, 6, 3,
		3, 6, 7,
	};

	skyMesh = new Mesh();
	skyMesh->Create(skyboxVertices, skyboxIndices, 14 * 8, 6 * 6);
}

void Skybox::Draw(glm::mat4 modelMatrix, glm::mat4 viewMatrix, glm::mat4 projectionMatrix)
{
	viewMatrix = glm::mat4(glm::mat3(viewMatrix));

	glDepthMask(GL_FALSE);

	m_SkyShader->Bind();

	m_SkyShader->SetMat4("model", modelMatrix);
	m_SkyShader->SetMat4("view", viewMatrix);
	m_SkyShader->SetMat4("projection", projectionMatrix);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	m_SkyShader->Validate();

	skyMesh->Render();

	glDepthMask(GL_TRUE);
}

Skybox::~Skybox()
{
}
