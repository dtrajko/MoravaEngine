#pragma once

#include "H2M/Renderer/CameraH2M.h"


namespace H2M
{

	class SceneCameraH2M : public CameraH2M
	{
	public:
		SceneCameraH2M();
		virtual ~SceneCameraH2M() = default;

		void SetOrthographic(float size, float nearClip, float farClip);
		void SetPerspective(float verticalFOV, float nearClip, float farClip);

		virtual void SetViewportSize(float width, float height) override;

	private:
		virtual void RecalculateProjection() override;

	};

}
