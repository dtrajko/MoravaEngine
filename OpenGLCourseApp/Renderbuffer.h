#pragma once


class Renderbuffer
{
public:
	Renderbuffer();
	Renderbuffer(unsigned int width, unsigned int height);
	~Renderbuffer();

private:
	unsigned int bufferID;
	unsigned int m_Width;
	unsigned int m_Height;

};
