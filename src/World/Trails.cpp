#include "Trails.h"

#include <algorithm>


void TrailSettings::Clamp()
{
	Width = std::clamp(Width, 0.001f, 100.0f);
	MinDistance = std::clamp(MinDistance, 0.01f, 100.0f);
	MaxPoints = std::clamp(MaxPoints, 2u, MaxPointsLimit);
	Lifetime = std::max(Lifetime, 0.0f);
	TileLength = std::clamp(TileLength, 0.01f, 1000.0f);
}

void TrailPoints::Clear()
{
	m_First = 0;
	m_Count = 0;
	Open = false;
}

void TrailPoints::SetCapacity(uint32_t capacity)
{
	if (capacity != m_Ring.size())
	{
		m_Ring.assign(capacity, TrailPoint());
		Clear();
	}
}

void TrailPoints::Push(const TrailPoint& point)
{
	if (m_Ring.empty())
	{
		return;
	}
	if (m_Count == m_Ring.size())
	{
		PopOldest();
	}
	m_Ring[(m_First + m_Count) % m_Ring.size()] = point;
	m_Count++;
}

void TrailPoints::PopOldest()
{
	if (m_Count == 0)
	{
		return;
	}
	m_First = (m_First + 1) % (uint32_t)m_Ring.size();
	m_Count--;
}
