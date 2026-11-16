#include "SceneNavigation.h"

#include "NavMesh.h"

#include "Core/Log.h"

#include "EnvMapVulkan/EnvMapVulkanScene.h"

#include "Physics/ScenePhysics.h"

#include <DetourCommon.h>
#include <DetourCrowd.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>

#include <algorithm>
#include <chrono>
#include <cmath>


static constexpr int MaxPathCorners = 256;   // a path's corners for its remaining distance and the debug view
static constexpr int MaxQueryNodes = 2048;
static constexpr float StuckSeconds = 1.0f;  // wanting to walk without getting on for this long: stuck
static constexpr float TeleportDistance = 1.0f; // the character this far from where its corridor got it to: put again
static constexpr float LadderClimbSpeed = 1.5f; // meters per second up and down a ladder (E1.5)
static constexpr float LinkTriggerMargin = 0.3f; // an agent gets on a link this much earlier than Detour's crowd would


SceneNavigation::SceneNavigation() = default;

SceneNavigation::~SceneNavigation()
{
	Stop();
}

void SceneNavigation::Stop()
{
	if (m_Crowd && m_UpdateCount > 0)
	{
		Log::GetLogger()->info("Navigation: {0} agent step(s), {1:.3f} ms each on average", m_UpdateCount,
			m_UpdateSeconds * 1000.0 / (double)m_UpdateCount);
	}
	if (m_Crowd)
	{
		dtFreeCrowd(m_Crowd);
		m_Crowd = nullptr;
	}
	if (m_Query)
	{
		dtFreeNavMeshQuery(m_Query);
		m_Query = nullptr;
	}
	m_Mesh.reset();
	m_Data.reset();
	m_FilterVersion = 0;
	m_Shortcuts = true;
	m_Agents.clear();
	m_Scene = nullptr;
	m_Physics = nullptr;
	m_MaxAgents = 0;
	m_MaxRadius = 0.0f;
	m_WarnedFull = false;
	m_LastUpdateMs = 0.0f;
	m_UpdateSeconds = 0.0;
	m_UpdateCount = 0;
}

void SceneNavigation::SyncStates(const EnvMapVulkanScene& scene)
{
	const EnvMapVulkanEntityID navMeshEntity = scene.FindFirst<NavMeshComponent>();
	if (navMeshEntity == NoEntity)
	{
		return;
	}
	const NavMeshComponent& component = scene.Get<NavMeshComponent>(navMeshEntity);
	if (!component.Mesh || !component.Data)
	{
		return;
	}
	NavMesh& mesh = *component.Mesh;
	const NavMeshData& data = *component.Data;
	// An entity gone since the bake (deleted, another scene's): its polygons stay as baked
	for (size_t i = 0; i < data.AreaEntities.size(); i++)
	{
		const EnvMapVulkanEntityID entity = data.AreaEntities[i];
		if (const NavAreaComponent* area = scene.Exists(entity) ? scene.TryGet<NavAreaComponent>(entity) : nullptr)
		{
			const uint8_t id = (uint8_t)(NavAreaIds::FirstArea + i);
			mesh.SetAreaWalkable(id, area->Type != NavAreaComponent::AreaType::Door || area->Open);
			mesh.SetAreaCost(id, area->Cost);
		}
	}
	for (size_t i = 0; i < data.LinkEntities.size(); i++)
	{
		const EnvMapVulkanEntityID entity = data.LinkEntities[i];
		if (const NavLinkComponent* link = scene.Exists(entity) ? scene.TryGet<NavLinkComponent>(entity) : nullptr)
		{
			mesh.SetLinkEnabled((uint32_t)i, link->Enabled);
		}
	}
}

void SceneNavigation::Start(EnvMapVulkanScene& scene, ScenePhysics& physics)
{
	Stop();
	SyncStates(scene); // the queries' too (a script's Navigation.FindPath), also without agents

	// The characters: the agents (with a Nav Agent) and the others they walk around
	uint32_t characters = 0, agents = 0;
	float maxRadius = 0.0f;
	scene.Each<CharacterControllerComponent>([&](EnvMapVulkanEntityID entity, const CharacterControllerComponent& character)
	{
		characters++;
		agents += scene.Has<NavAgentComponent>(entity) ? 1 : 0;
		maxRadius = std::max(maxRadius, character.Radius);
	});

	const EnvMapVulkanEntityID navMeshEntity = scene.FindFirst<NavMeshComponent>();
	std::shared_ptr<const NavMesh> mesh = navMeshEntity != NoEntity ? scene.Get<NavMeshComponent>(navMeshEntity).Mesh : nullptr;
	if (!mesh || !mesh->IsReady())
	{
		if (agents > 0)
		{
			Log::GetLogger()->warn("Navigation: {0} Nav Agent(s), but the scene has no baked navmesh: they stand (the Nav Mesh's Bake)", agents);
		}
		return;
	}

	m_Scene = &scene;
	m_Physics = &physics;
	m_Mesh = mesh;
	m_Data = scene.Get<NavMeshComponent>(navMeshEntity).Data;
	m_MaxAgents = (int)characters * 2 + 32; // room for the ones a game places (prefabs)
	m_MaxRadius = std::max(maxRadius, mesh->GetSettings().AgentRadius);
	m_Crowd = dtAllocCrowd();
	m_Query = dtAllocNavMeshQuery();
	if (!m_Crowd || !m_Crowd->init(m_MaxAgents, m_MaxRadius, mesh->GetDetourMesh()) || !m_Query ||
		dtStatusFailed(m_Query->init(mesh->GetDetourMesh(), MaxQueryNodes)))
	{
		Log::GetLogger()->error("Navigation: the crowd couldn't be made: the Nav Agents stand");
		Stop();
		return;
	}

	// The crowd walks by the mesh's filter: the areas' costs, the closed doors and the links that are off
	*m_Crowd->getEditableFilter(0) = *mesh->GetFilter();
	m_FilterVersion = mesh->GetStateVersion();
	m_Shortcuts = !HasCostlyAreas();

	// The avoidance's quality (NavAgentComponent::AvoidanceQuality, Low to High): how many velocities it tries around the
	// others, as Detour's demo has them
	const struct { unsigned char Divisions, Rings, Depth; } qualities[] = { { 5, 2, 1 }, { 7, 2, 3 }, { 7, 3, 3 } };
	for (int i = 0; i < 3; i++)
	{
		dtObstacleAvoidanceParams params = *m_Crowd->getObstacleAvoidanceParams(0);
		params.velBias = 0.5f;
		params.adaptiveDivs = qualities[i].Divisions;
		params.adaptiveRings = qualities[i].Rings;
		params.adaptiveDepth = qualities[i].Depth;
		m_Crowd->setObstacleAvoidanceParams(i, &params);
	}

	scene.Each<CharacterControllerComponent>([&](EnvMapVulkanEntityID entity, const CharacterControllerComponent&)
	{
		AddAgent(entity);
	});
	uint32_t passive = 0;
	for (const Agent& agent : m_Agents)
	{
		passive += agent.Passive ? 1 : 0;
	}
	if (m_Agents.size() > passive)
	{
		Log::GetLogger()->info("Navigation: {0} agent(s) on the navmesh, {1} other character(s) they walk around", m_Agents.size() - passive, passive);
	}
}

void SceneNavigation::AddEntities(const std::vector<EnvMapVulkanEntityID>& entities)
{
	if (!m_Crowd)
	{
		return;
	}
	for (EnvMapVulkanEntityID entity : entities)
	{
		if (m_Scene->Exists(entity) && m_Scene->Has<CharacterControllerComponent>(entity))
		{
			AddAgent(entity);
		}
	}
}

// The crowd's settings of an agent: its component's, or a passive one's (moved by its own velocity: as fast as it wants,
// avoiding nothing, so the others see where it really goes)
// shortcuts: Detour's straightening of the path by looking ahead (raycasts), which doesn't see the areas' costs: off while
// the navmesh has a Costly area (an agent would cut through it; the path found is the cheapest already)
static dtCrowdAgentParams MakeParameters(float radius, float height, const NavAgentComponent* settings, bool shortcuts)
{
	dtCrowdAgentParams params = {};
	params.radius = radius;
	params.height = height;
	params.collisionQueryRange = radius * 12.0f;
	params.pathOptimizationRange = radius * 30.0f;
	params.separationWeight = 2.0f;
	if (!settings)
	{
		params.maxSpeed = 100.0f;
		params.maxAcceleration = 10000.0f;
		return params;
	}
	params.maxSpeed = std::max(settings->Speed, 0.0f);
	params.maxAcceleration = std::max(settings->Acceleration, 0.01f);
	params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OPTIMIZE_TOPO | (shortcuts ? DT_CROWD_OPTIMIZE_VIS : 0);
	if (settings->Avoidance != NavAgentComponent::AvoidanceQuality::None)
	{
		params.updateFlags |= DT_CROWD_OBSTACLE_AVOIDANCE | DT_CROWD_SEPARATION;
		params.obstacleAvoidanceType = (unsigned char)((int)settings->Avoidance - 1);
	}
	return params;
}

void SceneNavigation::ApplyParameters(const Agent& agent)
{
	const NavAgentComponent* settings = agent.Passive ? nullptr : m_Scene->TryGet<NavAgentComponent>(agent.Entity);
	const dtCrowdAgentParams params = MakeParameters(agent.Radius, agent.Height, settings, m_Shortcuts);
	m_Crowd->updateAgentParameters(agent.Index, &params);
}

void SceneNavigation::AddAgent(EnvMapVulkanEntityID entity)
{
	ScenePhysics::CharacterState state;
	if (Find(entity) || !m_Physics->GetCharacterState(entity, state))
	{
		return;
	}
	if ((int)m_Agents.size() >= m_MaxAgents)
	{
		if (!m_WarnedFull)
		{
			Log::GetLogger()->warn("Navigation: the crowd is full ({0} characters): '{1}' and those after it walk without it", m_MaxAgents,
				m_Scene->Get<NameComponent>(entity).Name);
			m_WarnedFull = true;
		}
		return;
	}

	const CharacterControllerComponent& character = m_Scene->Get<CharacterControllerComponent>(entity);
	Agent agent;
	agent.Entity = entity;
	agent.Passive = !m_Scene->Has<NavAgentComponent>(entity);
	agent.Radius = std::max(character.Radius, 0.01f);
	agent.Height = std::max(character.Height, agent.Radius * 2.0f);
	agent.FeetOffset = character.Offset.y - agent.Height * 0.5f; // the capsule's bottom (CreateCharacter: around position + offset)
	if (!agent.Passive)
	{
		agent.Applied = m_Scene->Get<NavAgentComponent>(entity);
		const float baked = m_Mesh->GetSettings().AgentRadius;
		if (agent.Radius > baked + 0.01f)
		{
			Log::GetLogger()->warn("Navigation: '{0}' is wider ({1:.2f} m) than the navmesh was baked for ({2:.2f} m): it may scrape the walls",
				m_Scene->Get<NameComponent>(entity).Name, agent.Radius, baked);
		}
	}

	// On the navmesh where its feet are (not near it: the crowd keeps it aside until it gets there, PlaceAgent)
	const glm::vec3 feet = state.Position + glm::vec3(0.0f, agent.FeetOffset, 0.0f);
	glm::vec3 position = feet;
	const glm::vec3 extents(std::max(agent.Radius * 2.0f, 0.5f), std::max(agent.Height, 1.0f), std::max(agent.Radius * 2.0f, 0.5f));
	dtPolyRef polygon = 0;
	m_Query->findNearestPoly(&feet.x, &extents.x, m_Crowd->getFilter(0), &polygon, &position.x);
	const dtCrowdAgentParams params = MakeParameters(agent.Radius, agent.Height, agent.Passive ? nullptr : &agent.Applied, m_Shortcuts);
	agent.Index = m_Crowd->addAgent(&position.x, &params);
	if (agent.Index < 0)
	{
		return;
	}
	m_Agents.push_back(agent);
}

void SceneNavigation::RemoveAgent(Agent& agent)
{
	if (m_Crowd && agent.Index >= 0)
	{
		m_Crowd->removeAgent(agent.Index);
	}
	agent.Index = -1;
}

SceneNavigation::Agent* SceneNavigation::Find(EnvMapVulkanEntityID entity)
{
	for (Agent& agent : m_Agents)
	{
		if (agent.Entity == entity)
		{
			return &agent;
		}
	}
	return nullptr;
}

const SceneNavigation::Agent* SceneNavigation::Find(EnvMapVulkanEntityID entity) const
{
	return const_cast<SceneNavigation*>(this)->Find(entity);
}

bool SceneNavigation::PlaceAgent(Agent& agent, const glm::vec3& feet)
{
	dtCrowdAgent* crowdAgent = m_Crowd->getEditableAgent(agent.Index);
	const dtQueryFilter* filter = m_Crowd->getFilter(crowdAgent->params.queryFilterType);
	if (crowdAgent->state == DT_CROWDAGENT_STATE_WALKING)
	{
		// Along its corridor to where physics put it (a step aside from its path: still on it)
		crowdAgent->corridor.movePosition(&feet.x, m_Query, filter);
		dtVcopy(crowdAgent->npos, crowdAgent->corridor.getPos());
		const float across = glm::length(glm::vec2(feet.x - crowdAgent->npos[0], feet.z - crowdAgent->npos[2]));
		if (across < TeleportDistance && std::abs(feet.y - crowdAgent->npos[1]) < agent.Height)
		{
			return true;
		}
	}

	// Off the navmesh, or put somewhere else (a script, a fall): on the navmesh where it is now, its path found again
	const glm::vec3 extents(std::max(agent.Radius * 2.0f, 0.5f), std::max(agent.Height, 1.0f), std::max(agent.Radius * 2.0f, 0.5f));
	glm::vec3 nearest = feet;
	dtPolyRef polygon = 0;
	if (dtStatusFailed(m_Query->findNearestPoly(&feet.x, &extents.x, filter, &polygon, &nearest.x)) || polygon == 0)
	{
		crowdAgent->state = DT_CROWDAGENT_STATE_INVALID;
		dtVcopy(crowdAgent->npos, &feet.x);
		dtVset(crowdAgent->vel, 0.0f, 0.0f, 0.0f);
		dtVset(crowdAgent->dvel, 0.0f, 0.0f, 0.0f);
		dtVset(crowdAgent->nvel, 0.0f, 0.0f, 0.0f);
		return false;
	}
	crowdAgent->corridor.reset(polygon, &nearest.x);
	crowdAgent->boundary.reset();
	crowdAgent->state = DT_CROWDAGENT_STATE_WALKING;
	crowdAgent->partial = false;
	dtVcopy(crowdAgent->npos, &nearest.x);
	if (!agent.Passive && agent.HasDestination && !agent.Arrived)
	{
		RequestMove(agent);
	}
	return true;
}

void SceneNavigation::RequestMove(Agent& agent)
{
	m_Crowd->requestMoveTarget(agent.Index, agent.DestinationPolygon, &agent.Destination.x);
}

void SceneNavigation::Update(float fixedStep)
{
	if (!m_Crowd || fixedStep <= 0.0f)
	{
		return;
	}
	const auto start = std::chrono::steady_clock::now();

	// The agents of characters that are gone (destroyed by the game, deleted in the editor) go
	for (auto agent = m_Agents.begin(); agent != m_Agents.end();)
	{
		ScenePhysics::CharacterState state;
		if (!m_Scene->Exists(agent->Entity) || !m_Physics->GetCharacterState(agent->Entity, state))
		{
			RemoveAgent(*agent);
			agent = m_Agents.erase(agent);
			continue;
		}
		++agent;
	}
	// Only the characters without a Nav Agent: nothing to steer
	if (std::none_of(m_Agents.begin(), m_Agents.end(), [](const Agent& agent) { return !agent.Passive; }))
	{
		return;
	}

	// The doors, areas and links as the game has them now (a script may have locked one)
	SyncStates(*m_Scene);
	if (m_FilterVersion != m_Mesh->GetStateVersion())
	{
		*m_Crowd->getEditableFilter(0) = *m_Mesh->GetFilter();
		m_FilterVersion = m_Mesh->GetStateVersion();
		if (m_Shortcuts == HasCostlyAreas())
		{
			m_Shortcuts = !m_Shortcuts;
			for (const Agent& agent : m_Agents)
			{
				ApplyParameters(agent);
			}
		}
	}

	// Each where its character is after the last step; the passive ones go on as they move, the agents with the settings
	// their components have now (the Inspector, a script)
	for (Agent& agent : m_Agents)
	{
		ScenePhysics::CharacterState state;
		m_Physics->GetCharacterState(agent.Entity, state);
		if (agent.OnLink)
		{
			continue; // carried: where the link's way has it
		}
		PlaceAgent(agent, state.Position + glm::vec3(0.0f, agent.FeetOffset, 0.0f));
		if (agent.Passive)
		{
			const glm::vec3 velocity(state.Velocity.x, 0.0f, state.Velocity.z);
			m_Crowd->requestMoveVelocity(agent.Index, &velocity.x);
			continue;
		}
		const NavAgentComponent& settings = m_Scene->Get<NavAgentComponent>(agent.Entity);
		if (settings.Speed != agent.Applied.Speed || settings.Acceleration != agent.Applied.Acceleration ||
			settings.Avoidance != agent.Applied.Avoidance)
		{
			agent.Applied = settings;
			ApplyParameters(agent);
		}
		agent.Applied.StoppingDistance = settings.StoppingDistance;
	}

	// The agents that stand make way for the characters that come at them (the crowd steers only the walking ones)
	MakeWay();

	StartLinks(fixedStep);
	m_Crowd->update(fixedStep, nullptr);
	UpdateLinks(fixedStep);

	// The agents' velocities to their characters: across from the crowd, down from the world's gravity off the ground
	const float gravity = m_Physics->GetWorld() ? m_Physics->GetWorld()->GetGravity().y : -9.81f;
	for (Agent& agent : m_Agents)
	{
		if (agent.Passive || agent.OnLink)
		{
			continue;
		}
		dtCrowdAgent* crowdAgent = m_Crowd->getEditableAgent(agent.Index);
		ScenePhysics::CharacterState state;
		m_Physics->GetCharacterState(agent.Entity, state);

		// Arrived: within its stopping distance of its path's end (the destination, or the nearest it can get to it)
		if (agent.HasDestination && !agent.Arrived && crowdAgent->state == DT_CROWDAGENT_STATE_WALKING &&
			crowdAgent->targetState == DT_CROWDAGENT_TARGET_VALID && crowdAgent->ncorners > 0 &&
			(crowdAgent->cornerFlags[crowdAgent->ncorners - 1] & DT_STRAIGHTPATH_END) != 0)
		{
			const float* end = &crowdAgent->cornerVerts[(crowdAgent->ncorners - 1) * 3];
			if (dtVdist2D(crowdAgent->npos, end) <= std::max(agent.Applied.StoppingDistance, 0.01f) &&
				std::abs(crowdAgent->npos[1] - end[1]) < agent.Height)
			{
				agent.Arrived = true;
				agent.ArrivedPartial = crowdAgent->partial;
				agent.StuckTime = 0.0f;
				m_Crowd->resetMoveTarget(agent.Index);
			}
		}

		glm::vec3 velocity(0.0f);
		if (crowdAgent->state == DT_CROWDAGENT_STATE_WALKING && ((agent.HasDestination && !agent.Arrived) || agent.Yielding))
		{
			velocity = glm::vec3(crowdAgent->vel[0], 0.0f, crowdAgent->vel[2]);
		}

		// Stuck: it wants to walk, but physics doesn't get it on (a wall the navmesh doesn't know, the others in the way)
		const float wanted = glm::length(glm::vec2(crowdAgent->dvel[0], crowdAgent->dvel[2]));
		const float moved = glm::length(glm::vec2(state.Velocity.x, state.Velocity.z));
		if (agent.HasDestination && !agent.Arrived && wanted > 0.1f && moved < wanted * 0.2f)
		{
			agent.StuckTime += fixedStep;
		}
		else
		{
			agent.StuckTime = 0.0f;
		}

		// Down: as it really moved, plus the gravity's, while it isn't on the ground (in the air, or sliding down a slope)
		velocity.y = state.Ground == ScenePhysics::CharacterGround::OnGround ? 0.0f : state.Velocity.y + gravity * fixedStep;
		m_Physics->SetCharacterVelocity(agent.Entity, velocity);
	}

	const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
	m_LastUpdateMs = (float)(seconds * 1000.0);
	m_UpdateSeconds += seconds;
	m_UpdateCount++;
}

bool SceneNavigation::HasCostlyAreas() const
{
	for (size_t i = 0; m_Data && i < m_Data->AreaEntities.size(); i++)
	{
		if (m_Mesh->GetAreaCost((uint8_t)(NavAreaIds::FirstArea + i)) > 1.001f)
		{
			return true;
		}
	}
	return false;
}

void SceneNavigation::StartLinks(float fixedStep)
{
	for (Agent& agent : m_Agents)
	{
		agent.PendingLink = 0;
		if (agent.Passive || agent.OnLink || !agent.HasDestination || agent.Arrived)
		{
			continue;
		}
		dtCrowdAgent* crowdAgent = m_Crowd->getEditableAgent(agent.Index);
		if (crowdAgent->state != DT_CROWDAGENT_STATE_WALKING || crowdAgent->ncorners == 0 ||
			(crowdAgent->cornerFlags[crowdAgent->ncorners - 1] & DT_STRAIGHTPATH_OFFMESH_CONNECTION) == 0)
		{
			continue;
		}
		const dtPolyRef link = crowdAgent->cornerPolys[crowdAgent->ncorners - 1];
		agent.PendingLink = link;
		const float* corner = &crowdAgent->cornerVerts[(crowdAgent->ncorners - 1) * 3];
		const float trigger = agent.Radius * 2.25f + agent.Applied.Speed * fixedStep + LinkTriggerMargin;
		if (dtVdist2D(crowdAgent->npos, corner) > trigger || std::abs(crowdAgent->npos[1] - corner[1]) > agent.Height)
		{
			continue;
		}
		dtPolyRef refs[2];
		glm::vec3 start, end;
		const glm::vec3 from(crowdAgent->npos[0], crowdAgent->npos[1], crowdAgent->npos[2]);
		if (crowdAgent->corridor.moveOverOffmeshConnection(link, refs, &start.x, &end.x, m_Query))
		{
			StartLink(agent, refs[1], from, start, end);
		}
	}
}

// The way along a link, as feet positions and each segment's speed: from where the agent is to the link's start, then a
// ladder straight up and onto the top (or over the edge and straight down), a jump in an arc, a walk in a line
static void MakeLinkWay(uint8_t area, const glm::vec3& from, const glm::vec3& start, const glm::vec3& end, float speed,
	std::vector<glm::vec3>& points, std::vector<float>& speeds)
{
	speed = std::max(speed, 0.5f);
	points = { from, start };
	speeds = { speed };
	if (area == NavAreaIds::LadderLink)
	{
		const bool up = end.y >= start.y;
		points.push_back(up ? glm::vec3(start.x, end.y + 0.05f, start.z) : glm::vec3(end.x, start.y + 0.05f, end.z));
		speeds.push_back(up ? LadderClimbSpeed : speed * 0.5f);
		points.push_back(end);
		speeds.push_back(up ? speed * 0.5f : LadderClimbSpeed);
	}
	else if (area == NavAreaIds::JumpLink)
	{
		// An arc half a meter above the line, in as long as walking across would take (at least 0.4 s)
		const int Segments = 10;
		const float across = glm::length(glm::vec2(end.x - start.x, end.z - start.z));
		const float seconds = std::max(across / speed, 0.4f);
		glm::vec3 previous = start;
		for (int i = 1; i <= Segments; i++)
		{
			const float u = (float)i / Segments;
			const glm::vec3 point = glm::mix(start, end, u) + glm::vec3(0.0f, 2.0f * u * (1.0f - u), 0.0f);
			speeds.push_back(std::max(glm::distance(previous, point) / (seconds / Segments), 0.01f));
			points.push_back(point);
			previous = point;
		}
	}
	else
	{
		points.push_back(end);
		speeds.push_back(speed);
	}
}

void SceneNavigation::StartLink(Agent& agent, uint64_t linkPolygon, const glm::vec3& from, const glm::vec3& start, const glm::vec3& end)
{
	dtCrowdAgent* crowdAgent = m_Crowd->getEditableAgent(agent.Index);
	NavMesh::LinkInfo link;
	agent.LinkArea = m_Mesh->GetLink(linkPolygon, link) ? link.Area : NavAreaIds::WalkLink;
	agent.LinkIndex = link.Link;
	MakeLinkWay(agent.LinkArea, from, start, end, agent.Applied.Speed, agent.LinkPoints, agent.LinkSpeeds);
	agent.LinkSegment = 0;
	agent.LinkAlong = 0.0f;
	agent.LinkFeet = from;
	agent.LinkVelocity = glm::vec3(0.0f);
	agent.OnLink = true;
	agent.StuckTime = 0.0f;
	crowdAgent->state = DT_CROWDAGENT_STATE_OFFMESH;
	crowdAgent->ncorners = 0;
	crowdAgent->nneis = 0;
	dtVset(crowdAgent->vel, 0.0f, 0.0f, 0.0f);
	dtVset(crowdAgent->dvel, 0.0f, 0.0f, 0.0f);
	dtVset(crowdAgent->nvel, 0.0f, 0.0f, 0.0f);
}

void SceneNavigation::UpdateLinks(float fixedStep)
{
	for (Agent& agent : m_Agents)
	{
		if (agent.Passive)
		{
			continue;
		}
		dtCrowdAgent* crowdAgent = m_Crowd->getEditableAgent(agent.Index);
		// Detour's crowd took it onto the link itself (it got near before StartLinks saw it): ours from here. The corridor
		// is past the link already: its far end is where the corridor is
		if (!agent.OnLink && crowdAgent->state == DT_CROWDAGENT_STATE_OFFMESH && agent.PendingLink != 0)
		{
			NavMesh::LinkInfo link;
			ScenePhysics::CharacterState state;
			m_Physics->GetCharacterState(agent.Entity, state);
			const glm::vec3 end(crowdAgent->corridor.getPos()[0], crowdAgent->corridor.getPos()[1], crowdAgent->corridor.getPos()[2]);
			if (m_Mesh->GetLink(agent.PendingLink, link))
			{
				const glm::vec3 start = glm::distance(link.Start, end) > glm::distance(link.End, end) ? link.Start : link.End;
				StartLink(agent, agent.PendingLink, state.Position + glm::vec3(0.0f, agent.FeetOffset, 0.0f), start, end);
			}
		}
		if (!agent.OnLink)
		{
			continue;
		}

		// Along the way by the segments' speeds; at its end on the navmesh again, walking on along the corridor
		const glm::vec3 before = agent.LinkFeet;
		float time = fixedStep;
		glm::vec3 feet = before;
		while (agent.LinkSegment + 1 < agent.LinkPoints.size() && time > 0.0f)
		{
			const glm::vec3 a = agent.LinkPoints[agent.LinkSegment], b = agent.LinkPoints[agent.LinkSegment + 1];
			const float length = glm::distance(a, b);
			const float speed = agent.LinkSpeeds[agent.LinkSegment];
			const float left = length - agent.LinkAlong;
			if (left <= speed * time)
			{
				time -= left / speed;
				agent.LinkSegment++;
				agent.LinkAlong = 0.0f;
				feet = b;
				continue;
			}
			agent.LinkAlong += speed * time;
			time = 0.0f;
			feet = a + (b - a) * (agent.LinkAlong / std::max(length, 1e-6f));
		}
		agent.LinkFeet = feet;
		agent.LinkVelocity = (feet - before) / fixedStep;
		m_Physics->CarryCharacter(agent.Entity, feet - glm::vec3(0.0f, agent.FeetOffset, 0.0f));
		dtVcopy(crowdAgent->npos, &feet.x);
		dtVset(crowdAgent->vel, 0.0f, 0.0f, 0.0f);
		crowdAgent->state = DT_CROWDAGENT_STATE_OFFMESH; // Detour's own link animation (if it took the link) may have ended

		if (agent.LinkSegment + 1 >= agent.LinkPoints.size())
		{
			agent.OnLink = false;
			agent.LinkVelocity = glm::vec3(0.0f);
			crowdAgent->state = DT_CROWDAGENT_STATE_WALKING;
			crowdAgent->boundary.reset();
			dtVcopy(crowdAgent->npos, crowdAgent->corridor.getPos());
		}
	}
}

void SceneNavigation::MakeWay()
{
	// Where each character is and how it moves (across), from the crowd: the passive ones as physics moved them, the
	// agents as they walk
	struct Mover { glm::vec2 Position, Velocity; float Radius; };
	std::vector<Mover> movers;
	movers.reserve(m_Agents.size());
	for (const Agent& agent : m_Agents)
	{
		const dtCrowdAgent* crowdAgent = m_Crowd->getAgent(agent.Index);
		const bool walking = agent.Passive || (agent.HasDestination && !agent.Arrived);
		movers.push_back({ glm::vec2(crowdAgent->npos[0], crowdAgent->npos[2]),
			walking ? glm::vec2(agent.Passive ? crowdAgent->targetPos[0] : crowdAgent->vel[0], agent.Passive ? crowdAgent->targetPos[2] : crowdAgent->vel[2])
			: glm::vec2(0.0f), agent.Radius });
	}

	for (size_t i = 0; i < m_Agents.size(); i++)
	{
		Agent& agent = m_Agents[i];
		if (agent.Passive || (agent.HasDestination && !agent.Arrived))
		{
			continue;
		}
		// Aside from the line each one walks along, the more the nearer it is: one that will be here within about half a
		// second, coming this way (not one walking away or past)
		glm::vec2 push(0.0f);
		for (size_t j = 0; j < movers.size(); j++)
		{
			const float speed = glm::length(movers[j].Velocity);
			if (j == i || speed < 0.3f)
			{
				continue;
			}
			const glm::vec2 offset = movers[i].Position - movers[j].Position;
			const float distance = glm::length(offset);
			const float contact = movers[i].Radius + movers[j].Radius;
			const float reach = contact + 0.4f + speed * 0.6f;
			const glm::vec2 direction = movers[j].Velocity / speed;
			const float ahead = glm::dot(offset, direction);
			const glm::vec2 lateral = offset - direction * ahead;
			const float across = glm::length(lateral);
			if (distance >= reach || ahead < -contact || across > contact + 0.4f)
			{
				continue;
			}
			const glm::vec2 side = across > 0.05f ? lateral / across : glm::vec2(-direction.y, direction.x);
			push += side * (1.0f - glm::clamp((distance - contact) / (reach - contact), 0.0f, 1.0f));
		}

		const float strength = glm::length(push);
		if (strength > 0.05f)
		{
			const glm::vec2 velocity = push / strength * agent.Applied.Speed * glm::clamp(strength, 0.3f, 1.0f);
			const glm::vec3 request(velocity.x, 0.0f, velocity.y);
			m_Crowd->requestMoveVelocity(agent.Index, &request.x);
			agent.Yielding = true;
		}
		else if (agent.Yielding)
		{
			m_Crowd->resetMoveTarget(agent.Index);
			agent.Yielding = false;
		}
	}
}

bool SceneNavigation::SetDestination(EnvMapVulkanEntityID entity, const glm::vec3& destination, bool* found)
{
	Agent* agent = Find(entity);
	if (found)
	{
		*found = false;
	}
	if (!agent || agent->Passive)
	{
		return false;
	}
	const glm::vec3 extents(2.0f, std::max(agent->Height, 2.0f), 2.0f);
	glm::vec3 nearest = destination;
	dtPolyRef polygon = 0;
	if (dtStatusFailed(m_Query->findNearestPoly(&destination.x, &extents.x, m_Crowd->getFilter(0), &polygon, &nearest.x)) || polygon == 0)
	{
		return true;
	}
	agent->Destination = nearest;
	agent->DestinationPolygon = polygon;
	agent->HasDestination = true;
	agent->Arrived = false;
	agent->ArrivedPartial = false;
	agent->Yielding = false;
	agent->StuckTime = 0.0f;
	RequestMove(*agent);
	if (found)
	{
		*found = true;
	}
	return true;
}

bool SceneNavigation::StopAgent(EnvMapVulkanEntityID entity)
{
	Agent* agent = Find(entity);
	if (!agent || agent->Passive)
	{
		return false;
	}
	agent->HasDestination = false;
	agent->Arrived = false;
	agent->ArrivedPartial = false;
	agent->Yielding = false;
	agent->StuckTime = 0.0f;
	m_Crowd->resetMoveTarget(agent->Index);
	return true;
}

bool SceneNavigation::SetSettings(EnvMapVulkanEntityID entity, float speed, float acceleration, float stoppingDistance)
{
	Agent* agent = Find(entity);
	NavAgentComponent* settings = m_Scene ? m_Scene->TryGet<NavAgentComponent>(entity) : nullptr;
	if (!agent || agent->Passive || !settings)
	{
		return false;
	}
	settings->Speed = std::max(speed, 0.0f);
	settings->Acceleration = std::max(acceleration, 0.01f);
	settings->StoppingDistance = std::max(stoppingDistance, 0.0f);
	agent->Applied = *settings;
	ApplyParameters(*agent);
	return true;
}

bool SceneNavigation::GetAgentPath(EnvMapVulkanEntityID entity, std::vector<glm::vec3>& corners, bool* complete) const
{
	corners.clear();
	const Agent* agent = Find(entity);
	if (!agent || agent->Passive)
	{
		return false;
	}
	const dtCrowdAgent* crowdAgent = m_Crowd->getAgent(agent->Index);
	if (complete)
	{
		*complete = !crowdAgent->partial;
	}
	if (!agent->HasDestination || agent->Arrived || crowdAgent->state != DT_CROWDAGENT_STATE_WALKING || crowdAgent->corridor.getPathCount() == 0)
	{
		return true;
	}
	float points[MaxPathCorners * 3];
	int count = 0;
	m_Query->findStraightPath(crowdAgent->npos, crowdAgent->corridor.getTarget(), crowdAgent->corridor.getPath(), crowdAgent->corridor.getPathCount(),
		points, nullptr, nullptr, &count, MaxPathCorners);
	corners.reserve(count);
	for (int i = 0; i < count; i++)
	{
		corners.emplace_back(points[i * 3], points[i * 3 + 1], points[i * 3 + 2]);
	}
	return true;
}

void SceneNavigation::FillState(const Agent& agent, AgentState& state) const
{
	const dtCrowdAgent* crowdAgent = m_Crowd->getAgent(agent.Index);
	state.OnNavMesh = crowdAgent->state == DT_CROWDAGENT_STATE_WALKING || agent.OnLink;
	state.HasDestination = agent.HasDestination && !agent.Arrived;
	state.PathPending = state.HasDestination && (crowdAgent->targetState == DT_CROWDAGENT_TARGET_REQUESTING ||
		crowdAgent->targetState == DT_CROWDAGENT_TARGET_WAITING_FOR_QUEUE || crowdAgent->targetState == DT_CROWDAGENT_TARGET_WAITING_FOR_PATH);
	state.PathPartial = agent.Arrived ? agent.ArrivedPartial : (state.HasDestination && crowdAgent->partial);
	state.Arrived = agent.HasDestination && agent.Arrived;
	state.Stuck = agent.StuckTime >= StuckSeconds;
	state.Yielding = agent.Yielding;
	state.OnLink = agent.OnLink;
	state.LinkType = !agent.OnLink ? -1 : agent.LinkArea == NavAreaIds::LadderLink ? (int)NavLinkComponent::LinkType::Ladder :
		agent.LinkArea == NavAreaIds::JumpLink ? (int)NavLinkComponent::LinkType::Jump : (int)NavLinkComponent::LinkType::Walk;
	state.LinkEntity = agent.OnLink && m_Data && agent.LinkIndex < m_Data->LinkEntities.size() ? m_Data->LinkEntities[agent.LinkIndex] : NoEntity;
	state.Destination = agent.Destination;
	state.Position = glm::vec3(crowdAgent->npos[0], crowdAgent->npos[1], crowdAgent->npos[2]);
	state.Velocity = agent.OnLink ? agent.LinkVelocity : agent.Passive || state.HasDestination || agent.Yielding ?
		glm::vec3(crowdAgent->vel[0], 0.0f, crowdAgent->vel[2]) : glm::vec3(0.0f);
	state.DesiredVelocity = state.HasDestination ? glm::vec3(crowdAgent->dvel[0], 0.0f, crowdAgent->dvel[2]) : glm::vec3(0.0f);
	state.Radius = agent.Radius;
	state.Height = agent.Height;
	state.Settings = agent.Applied;
	state.RemainingDistance = 0.0f;
	if (state.HasDestination && !agent.Passive)
	{
		std::vector<glm::vec3> corners;
		GetAgentPath(agent.Entity, corners);
		for (size_t i = 1; i < corners.size(); i++)
		{
			state.RemainingDistance += glm::distance(corners[i - 1], corners[i]);
		}
	}
}

bool SceneNavigation::GetAgentState(EnvMapVulkanEntityID entity, AgentState& state) const
{
	const Agent* agent = Find(entity);
	if (!agent || agent->Passive)
	{
		return false;
	}
	FillState(*agent, state);
	return true;
}

void SceneNavigation::GetDebugAgents(std::vector<DebugAgent>& agents) const
{
	agents.clear();
	agents.reserve(m_Agents.size());
	for (const Agent& agent : m_Agents)
	{
		DebugAgent debug;
		debug.Entity = agent.Entity;
		debug.Passive = agent.Passive;
		FillState(agent, debug.State);
		if (!agent.Passive)
		{
			GetAgentPath(agent.Entity, debug.Path);
		}
		agents.push_back(std::move(debug));
	}
}

SceneNavigation::Stats SceneNavigation::GetStats() const
{
	Stats stats;
	for (const Agent& agent : m_Agents)
	{
		if (agent.Passive)
		{
			stats.Passive++;
			continue;
		}
		stats.Agents++;
		stats.Walking += agent.HasDestination && !agent.Arrived ? 1 : 0;
		stats.Stuck += agent.StuckTime >= StuckSeconds ? 1 : 0;
	}
	stats.LastUpdateMs = m_LastUpdateMs;
	return stats;
}
