#pragma once

#include "Hazel/Renderer/HazelCamera.h"


namespace Hazel {

	class SceneCamera : public HazelCamera
	{
	public:
		SceneCamera();
		virtual ~SceneCamera() = default;

		void SetOrthographic(float size, float nearClip, float farClip);
		void SetPerspective(float verticalFOV, float nearClip, float farClip);

		virtual void SetViewportSize(float width, float height) override;

	private:
		virtual void RecalculateProjection() override;

	};

}
