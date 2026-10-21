#pragma once

#include <cstdint>
#include <string>


/**
 * Who collides with whom (docs/engine/Game_Engine_Runtime_Guide.html, 3.6): a project's setting, for the layers a body is
 * on (PhysicsLayers.h, in the same order). Two layers collide or they don't, both ways. Static with Static and Trigger with
 * Trigger never do (neither moves; sensors don't sense each other), so they can't be turned on.
 */
struct PhysicsCollisionMatrix
{
	enum Layer : uint32_t { Static, Dynamic, Character, Trigger, LayerCount };

	// One bit per layer for each layer: the layers it collides with
	uint32_t Masks[LayerCount] = {};

	// The engine's choice: everything that can collide does, but a trigger senses no static body (that would be every floor
	// it lies on)
	PhysicsCollisionMatrix()
	{
		Set(Static, Dynamic, true);
		Set(Static, Character, true);
		Set(Dynamic, Dynamic, true);
		Set(Dynamic, Character, true);
		Set(Dynamic, Trigger, true);
		Set(Character, Character, true);
		Set(Character, Trigger, true);
	}

	bool Collides(uint32_t a, uint32_t b) const { return (Masks[a] & (1u << b)) != 0; }
	static bool CanCollide(uint32_t a, uint32_t b) { return !(a == b && (a == Static || a == Trigger)); }

	void Set(uint32_t a, uint32_t b, bool collide)
	{
		collide = collide && CanCollide(a, b);
		Masks[a] = collide ? Masks[a] | (1u << b) : Masks[a] & ~(1u << b);
		Masks[b] = collide ? Masks[b] | (1u << a) : Masks[b] & ~(1u << a);
	}

	bool operator==(const PhysicsCollisionMatrix& other) const
	{
		for (uint32_t layer = 0; layer < LayerCount; layer++)
		{
			if (Masks[layer] != other.Masks[layer])
			{
				return false;
			}
		}
		return true;
	}
	bool operator!=(const PhysicsCollisionMatrix& other) const { return !(*this == other); }

	static const char* GetLayerName(uint32_t layer)
	{
		switch (layer)
		{
			case Static:    return "Static";
			case Dynamic:   return "Dynamic";
			case Character: return "Character";
			case Trigger:   return "Trigger";
			default:        return "?";
		}
	}

	// A layer by its name (false: no such layer)
	static bool FindLayer(const std::string& name, uint32_t& layer)
	{
		for (layer = 0; layer < LayerCount; layer++)
		{
			if (name == GetLayerName(layer))
			{
				return true;
			}
		}
		return false;
	}
};
