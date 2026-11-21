#include "Effects.h"

#include <algorithm>


float FlashLight::CurrentIntensity() const
{
	if (Seconds <= 0.0f || Age >= Seconds)
	{
		return 0.0f;
	}
	const float left = 1.0f - Age / Seconds;
	return Intensity * left * left;
}

float SpawnedDecal::Opacity() const
{
	float opacity = 1.0f;
	if (FadeFrom >= 0.0f)
	{
		opacity = std::min(opacity, 1.0f - (Age - FadeFrom) / SceneEffects::FadeSeconds);
	}
	if (Lifetime > 0.0f)
	{
		// The last of its lifetime (at most a fade's worth): fading out
		const float fade = std::min(SceneEffects::FadeSeconds, Lifetime * 0.5f);
		opacity = std::min(opacity, (Lifetime - Age) / fade);
	}
	return std::clamp(opacity, 0.0f, 1.0f);
}

void SceneEffects::AddFlash(const FlashLight& flash)
{
	if (Flashes.size() >= MaxFlashes)
	{
		auto nearestEnd = std::min_element(Flashes.begin(), Flashes.end(), [](const FlashLight& a, const FlashLight& b)
		{
			return a.Seconds - a.Age < b.Seconds - b.Age;
		});
		*nearestEnd = flash;
		return;
	}
	Flashes.push_back(flash);
}

void SceneEffects::SpawnDecal(const SpawnedDecal& decal)
{
	// Over the limit (those already fading not counted): the oldest steady one starts to fade
	size_t steady = 0;
	for (const SpawnedDecal& d : Decals)
	{
		steady += d.FadeFrom < 0.0f ? 1 : 0;
	}
	if (steady >= MaxDecals)
	{
		for (SpawnedDecal& d : Decals)
		{
			if (d.FadeFrom < 0.0f)
			{
				d.FadeFrom = d.Age;
				break;
			}
		}
	}
	// A hard limit: more fading than a fade's worth of fast fire
	while (Decals.size() >= MaxDecals + MaxDecals / 4)
	{
		Decals.pop_front();
	}
	Decals.push_back(decal);
}

void SceneEffects::Update(float deltaTime)
{
	const float dt = std::max(deltaTime, 0.0f);
	for (FlashLight& flash : Flashes)
	{
		flash.Age += dt;
	}
	Flashes.erase(std::remove_if(Flashes.begin(), Flashes.end(), [](const FlashLight& flash) { return flash.Age >= flash.Seconds; }), Flashes.end());
	for (SpawnedDecal& decal : Decals)
	{
		decal.Age += dt;
	}
	Decals.erase(std::remove_if(Decals.begin(), Decals.end(), [](const SpawnedDecal& decal) { return decal.Opacity() <= 0.0f; }), Decals.end());
}

void SceneEffects::Clear()
{
	Flashes.clear();
	Decals.clear();
}
