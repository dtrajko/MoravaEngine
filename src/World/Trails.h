#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>


/**
 * A trail's settings (docs/engine/Game_Engine_Runtime_Guide.html, 12.2; the racing game's skid marks): plain data without a
 * graphics API, saved in the scene (TrailComponent). Marks left behind by a moving entity: while Emitting, the entity leaves
 * a point every MinDistance meters; each two points make a piece of a ribbon, Width wide, lying across the entity's up (a
 * script puts the entity where the wheel touches the road and turns its up to the road's). Stopping and starting again
 * begins a new ribbon, and so does a jump of more than BreakDistance (a car put back on the track).
 *
 * Color: the mark's color and how much it covers (alpha). It darkens what's under it, as a darker surface would be: the
 * light the road gets (the sun, its shadows, the lamps, the sky) stays on the mark. A script may change it while driving
 * (a harder slide, a darker mark): each point keeps the color it was left with. Texture: its color times the mark's, along
 * the ribbon every TileLength meters (empty: a plain band with soft edges).
 *
 * MaxPoints: kept at once (a ring: the oldest marks go first, faded out over the last tenth). Lifetime: seconds a point
 * stays, fading over the second half of it (0: until the ring reuses it). The points are in the running scene only: not
 * saved, and a copy of the scene (Play) starts without any.
 */
struct TrailSettings
{
	bool Emitting = true;
	float Width = 0.25f;             // meters
	float MinDistance = 0.2f;        // meters between two points
	uint32_t MaxPoints = 1000;
	float Lifetime = 0.0f;           // seconds (0: no end)
	glm::vec4 Color = glm::vec4(0.05f, 0.05f, 0.05f, 0.7f);
	std::string Texture;             // absolute here; in the scene file an asset reference. Empty: a plain band
	float TileLength = 1.0f;         // meters along the ribbon a texture covers once

	static constexpr uint32_t MaxPointsLimit = 65536; // per trail
	static constexpr float BreakDistance = 10.0f;     // meters: a jump this long starts a new ribbon
	static constexpr float Lift = 0.015f;             // meters above the surface, along the entity's up (no flicker on it)

	void Clamp();
};

// A point a trail left
struct TrailPoint
{
	glm::vec3 Position = glm::vec3(0.0f); // where the entity was
	glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec4 Color = glm::vec4(1.0f);
	float Width = 0.0f;
	double Time = 0.0;                    // when (the trails' clock)
	float Along = 0.0f;                   // meters along its ribbon (the texture's coordinate)
	bool Connected = false;               // joined to the point before it (false: a ribbon starts here)
};

// A trail's points while the scene runs: a ring, MaxPoints long. Not copied with the component: a copy of the scene (Play),
// copy and paste, start without marks
class TrailPoints
{
public:
	TrailPoints() = default;
	TrailPoints(const TrailPoints&) {}
	TrailPoints& operator=(const TrailPoints&) { return *this; }
	TrailPoints(TrailPoints&&) = default;
	TrailPoints& operator=(TrailPoints&&) = default;

	void Clear();
	void SetCapacity(uint32_t capacity); // a change clears the ring
	uint32_t GetCapacity() const { return (uint32_t)m_Ring.size(); }
	uint32_t GetCount() const { return m_Count; }
	void Push(const TrailPoint& point);
	void PopOldest();
	// i = 0: the oldest
	const TrailPoint& Get(uint32_t i) const { return m_Ring[(m_First + i) % m_Ring.size()]; }
	const TrailPoint& Newest() const { return Get(m_Count - 1); }

	bool Open = false; // a ribbon goes on: the next point joins the last

private:
	std::vector<TrailPoint> m_Ring;
	uint32_t m_First = 0, m_Count = 0;
};
