#pragma once

#include "btBulletDynamicsCommon.h"


class BulletDebugDrawer : public btIDebugDraw
{
public:
	BulletDebugDrawer();

	inline virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override {};
	inline virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override {};
	inline virtual void reportErrorWarning(const char* warningString) override {};
	inline virtual void draw3dText(const btVector3& location, const char* textString) override {};
	inline virtual void setDebugMode(int debugMode) override {};
	inline virtual int getDebugMode() const override { return 0; };

	~BulletDebugDrawer();
};
