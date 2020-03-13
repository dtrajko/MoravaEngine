#pragma once


class Cube
{
public:
	Cube();
	void Render();
	~Cube();

private:
	unsigned int m_VAO = 0;
	unsigned int m_VBO = 0;

};
