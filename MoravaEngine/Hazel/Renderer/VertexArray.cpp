#include "VertexArray.h"

#include "../Platform/OpenGL/OpenGLVertexArray.h"


namespace Hazel {

	VertexArray* VertexArray::Create()
	{
		return (VertexArray*)new OpenGLVertexArray();
	}

}
