#include "Layer.h"

#include "Core/Log.h"


Layer::Layer(const std::string& debugName)
	: m_DebugName(debugName)
{
	Log::GetLogger()->info("Loading layer '{0}'", m_DebugName);
}

Layer::~Layer()
{

}
