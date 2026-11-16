#pragma once

#include "EnvMapVulkan/EnvMapVulkanComponents.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <vector>

class EnvMapVulkanScene;
class NavMesh;
struct NavMeshData;
class ScenePhysics;
class dtCrowd;
class dtNavMeshQuery;


/**
 * The running game's walking agents (docs/games/Shooter_Plan.html, E1.4): Detour's crowd on the scene's baked navmesh (its
 * Nav Mesh's), with an agent for each character that has a NavAgentComponent. Made when the game starts and gone when it
 * stops, as the physics bodies are.
 *
 * Each fixed step, before physics (Update): each agent takes its character's position after the last step (physics
 * decides where it is: a wall, a step, another character), the crowd plans and steers (its path's corners, the others
 * around it, its acceleration), and its velocity goes to the character (with the world's gravity when it isn't on the
 * ground). The characters without a Nav Agent (the player) are in the crowd too, moved by their own velocity: the agents
 * see where they go and walk around them.
 *
 * The game sends an agent somewhere (SetDestination: to the navmesh's nearest point; a place it can't reach: as near as
 * the navmesh goes) and stops it (Stop). It arrives within its StoppingDistance, and stops there. One that stands (arrived,
 * stopped, without a destination) makes way for a character that comes at it: it steps aside from that one's line, then
 * stands again. An agent pushed off its path is put back on the navmesh where it is; one that can't move on for a second
 * while it should is stuck.
 */
class SceneNavigation
{
public:
	SceneNavigation();
	~SceneNavigation();
	SceneNavigation(const SceneNavigation&) = delete;
	SceneNavigation& operator=(const SceneNavigation&) = delete;

	// The agents of the scene's characters (physics started: their characters exist). No baked navmesh: none (a Nav
	// Agent's character stands, logged once)
	void Start(EnvMapVulkanScene& scene, ScenePhysics& physics);
	void Stop();
	bool IsRunning() const { return m_Crowd != nullptr; }
	// Entities new in the running game (a prefab): the agents of their characters
	void AddEntities(const std::vector<EnvMapVulkanEntityID>& entities);

	// Before the physics step: the agents where their characters are, the crowd's step, its velocities to the characters
	// (or, on a Nav Link, where the engine carries them)
	void Update(float fixedStep);

	// The scene's Nav Areas' and Nav Links' states (a Door's Open, a Costly area's Cost, a link's Enabled; E1.5) into its
	// navmesh: by the game's scene while it runs (each step, and when a script changes one), by the edited scene in Edit
	static void SyncStates(const EnvMapVulkanScene& scene);

	// ---- The game's (C#: NavAgent). False: the entity isn't a running agent

	// Walk to the navmesh's point nearest destination (within a few meters); *found false (and nothing changes) when there
	// is none
	bool SetDestination(EnvMapVulkanEntityID entity, const glm::vec3& destination, bool* found = nullptr);
	// Stands where it is, without a destination
	bool StopAgent(EnvMapVulkanEntityID entity);
	// Speed, Acceleration and StoppingDistance as the component has them (the component is changed too)
	bool SetSettings(EnvMapVulkanEntityID entity, float speed, float acceleration, float stoppingDistance);

	struct AgentState
	{
		bool OnNavMesh = false;      // it stands on the navmesh (false: off it, or not placed yet)
		bool HasDestination = false; // it walks somewhere: its path found, or being found
		bool PathPending = false;    // being found (the crowd finds paths over a few steps when many ask)
		bool PathPartial = false;    // the destination can't be reached: it walks to the navmesh's point nearest it
		bool Arrived = false;        // within StoppingDistance of its destination (or the partial path's end); stands there
		bool Stuck = false;          // it should walk, but hasn't moved on for a second (a closed door, the others in the way)
		bool Yielding = false;       // it stands, and steps aside for a character that comes at it
		bool OnLink = false;         // the engine carries it along a Nav Link (E1.5): up a ladder, in a jump
		int LinkType = -1;           // NavLinkComponent::LinkType while OnLink (-1: none)
		EnvMapVulkanEntityID LinkEntity = NoEntity; // the Nav Link's entity while OnLink
		glm::vec3 Destination = glm::vec3(0.0f); // on the navmesh
		glm::vec3 Position = glm::vec3(0.0f);    // its feet, on the navmesh
		glm::vec3 Velocity = glm::vec3(0.0f);    // the velocity it asks its character for (across: the vertical is the gravity's;
		                                         // on a link, how it is carried: up a ladder, up)
		glm::vec3 DesiredVelocity = glm::vec3(0.0f); // along its path, before steering around the others
		float RemainingDistance = 0.0f;          // along its path to the destination (0: arrived, or none)
		float Radius = 0.0f, Height = 0.0f;
		NavAgentComponent Settings;
	};
	bool GetAgentState(EnvMapVulkanEntityID entity, AgentState& state) const;
	// The path it walks, from its feet to the destination (its corners; empty: no destination, or arrived). *complete:
	// it reaches the destination
	bool GetAgentPath(EnvMapVulkanEntityID entity, std::vector<glm::vec3>& corners, bool* complete = nullptr) const;

	// ---- The debug view (the editor's Navigation view while the game runs): every agent, and the characters it avoids
	struct DebugAgent
	{
		EnvMapVulkanEntityID Entity = NoEntity;
		bool Passive = false;        // a character without a Nav Agent: in the crowd to be avoided
		AgentState State;
		std::vector<glm::vec3> Path; // the corners of its path (none for a passive one)
	};
	void GetDebugAgents(std::vector<DebugAgent>& agents) const;

	struct Stats
	{
		uint32_t Agents = 0, Passive = 0, Walking = 0, Stuck = 0;
		float LastUpdateMs = 0.0f;
	};
	Stats GetStats() const;

private:
	struct Agent
	{
		EnvMapVulkanEntityID Entity = NoEntity;
		int Index = -1;               // in the crowd
		bool Passive = false;
		float Radius = 0.3f, Height = 1.8f;
		float FeetOffset = 0.0f;      // the character's feet below (negative) or above the entity's position
		NavAgentComponent Applied;    // its component's settings as the crowd has them
		bool HasDestination = false;
		bool Arrived = false;
		bool ArrivedPartial = false;  // at the end of a partial path: as near as it gets
		glm::vec3 Destination = glm::vec3(0.0f);
		uint64_t DestinationPolygon = 0;
		float StuckTime = 0.0f;
		bool Yielding = false;        // making way (the crowd moves it by a velocity: MakeWay)

		// A Nav Link (E1.5): the engine carries it along the link's way (points and each segment's speed), the crowd
		// leaves it alone (Detour's off-mesh state) until it is at the far end
		bool OnLink = false;
		uint8_t LinkArea = 0;
		uint32_t LinkIndex = 0;
		std::vector<glm::vec3> LinkPoints; // feet positions: from where it got on to the link's far end
		std::vector<float> LinkSpeeds;     // per segment
		size_t LinkSegment = 0;
		float LinkAlong = 0.0f;            // meters along the segment
		glm::vec3 LinkFeet = glm::vec3(0.0f); // where the way has it now
		glm::vec3 LinkVelocity = glm::vec3(0.0f);
		uint64_t PendingLink = 0;          // the link its corners end at before the crowd's step (Detour may take it on itself)
	};

	void AddAgent(EnvMapVulkanEntityID entity);
	void RemoveAgent(Agent& agent);
	// The crowd's settings of an agent, from its component (a passive one: moved by its own velocity, avoiding nothing)
	void ApplyParameters(const Agent& agent);
	// The agent where its character's feet are: along its corridor, or put on the navmesh again where it is (pushed off
	// its path, teleported). False: nowhere near the navmesh
	bool PlaceAgent(Agent& agent, const glm::vec3& feet);
	// A move request to the agent's destination (again: after it was put back on the navmesh)
	void RequestMove(Agent& agent);
	// The agents that stand: aside from the characters coming at them, or still again once they have passed
	void MakeWay();
	// The walking agents near the start of a link on their paths get on it (before the crowd's step, so Detour doesn't
	// take them over itself); after it, the ones Detour took on anyway. Then each one on a link carried on along it
	void StartLinks(float fixedStep);
	void StartLink(Agent& agent, uint64_t linkPolygon, const glm::vec3& from, const glm::vec3& start, const glm::vec3& end);
	void UpdateLinks(float fixedStep);
	// An area costs more than the ground (a Costly one): the agents don't straighten their paths by looking ahead
	bool HasCostlyAreas() const;
	Agent* Find(EnvMapVulkanEntityID entity);
	const Agent* Find(EnvMapVulkanEntityID entity) const;
	void FillState(const Agent& agent, AgentState& state) const;

	EnvMapVulkanScene* m_Scene = nullptr;
	ScenePhysics* m_Physics = nullptr;
	std::shared_ptr<const NavMesh> m_Mesh; // kept while the crowd uses it
	std::shared_ptr<const NavMeshData> m_Data;
	uint32_t m_FilterVersion = 0;         // the mesh's state version the crowd's filter is of
	bool m_Shortcuts = true;              // the agents straighten their paths by looking ahead (no Costly areas)
	dtCrowd* m_Crowd = nullptr;
	dtNavMeshQuery* m_Query = nullptr;     // the agents' placing and their paths (the crowd's own is its)
	std::vector<Agent> m_Agents;
	int m_MaxAgents = 0;
	float m_MaxRadius = 0.0f;
	bool m_WarnedFull = false;
	float m_LastUpdateMs = 0.0f;
	double m_UpdateSeconds = 0.0;
	uint64_t m_UpdateCount = 0;
};
