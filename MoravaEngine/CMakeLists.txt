cmake_minimum_required(VERSION 3.16)

set(CMAKE_BUILD_TYPE, Release)

set(MSVC_INCREMENTAL_DEFAULT ON)

project("MoravaEngine" VERSION 0.0.1)

if (WIN32)
file(GLOB SOURCES
	./*.h
	./*.cpp
	PerlinNoise/PerlinNoise.hpp
	LearnOpenGL/*.h
	LearnOpenGL/*.cpp
	SongHo/*.h
	SongHo/*.cpp
	../vendor/ImGui/imconfig.h
	../vendor/ImGui/imgui.cpp
	../vendor/ImGui/imgui.h
	../vendor/ImGui/imgui_demo.cpp
	../vendor/ImGui/imgui_draw.cpp
	../vendor/ImGui/imgui_impl_dx11.cpp
	../vendor/ImGui/imgui_impl_dx11.h
	../vendor/ImGui/imgui_impl_glfw.h
	../vendor/ImGui/imgui_impl_opengl3.h
	../vendor/ImGui/imgui_impl_win32.cpp
	../vendor/ImGui/imgui_impl_win32.h
	../vendor/ImGui/imgui_internal.h
	../vendor/ImGui/imgui_widgets.cpp
	../vendor/ImGui/imstb_rectpack.h
	../vendor/ImGui/imstb_textedit.h
	../vendor/ImGui/imstb_truetype.h
)
endif (WIN32)

if (UNIX)
file(GLOB SOURCES
	./*.h
	./*.cpp
	PerlinNoise/PerlinNoise.hpp
	LearnOpenGL/*.h
	LearnOpenGL/*.cpp
	SongHo/*.h
	SongHo/*.cpp
	../vendor/ImGui/imconfig.h
	../vendor/ImGui/imgui.cpp
	../vendor/ImGui/imgui.h
	../vendor/ImGui/imgui_demo.cpp
	../vendor/ImGui/imgui_draw.cpp
	../vendor/ImGui/imgui_impl_glfw.h
	../vendor/ImGui/imgui_impl_opengl3.h
	../vendor/ImGui/imgui_internal.h
	../vendor/ImGui/imgui_widgets.cpp
	../vendor/ImGui/imstb_rectpack.h
	../vendor/ImGui/imstb_textedit.h
	../vendor/ImGui/imstb_truetype.h
)
endif (UNIX)

SET(BulletDynamics_SRCS
	../vendor/bullet3/src/BulletDynamics/Character/btKinematicCharacterController.cpp
	../vendor/bullet3/src/BulletDynamics/ConstraintSolver/btConeTwistConstraint.cpp
	../vendor/bullet3/src/BulletDynamics/ConstraintSolver/btContactConstraint.cpp
	../vendor/bullet3/src/BulletDynamics/ConstraintSolver/btFixedConstraint.cpp
	../vendor/bullet3/src/BulletDynamics/ConstraintSolver/btGearConstraint.cpp
	../vendor/bullet3/src/BulletDynamics/ConstraintSolver/btGeneric6DofConstraint.cpp
	../vendor/bullet3/src/BulletDynamics/ConstraintSolver/btGeneric6DofSpringConstraint.cpp
	../vendor/bullet3/src/BulletDynamics/ConstraintSolver/btGeneric6DofSpring2Constraint.cpp
	../vendor/bullet3/src/BulletDynamics/ConstraintSolver/btHinge2Constraint.cpp
	../vendor/bullet3/src/BulletDynamics/ConstraintSolver/btHingeConstraint.cpp
	../vendor/bullet3/src/BulletDynamics/ConstraintSolver/btPoint2PointConstraint.cpp
	../vendor/bullet3/src/BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.cpp
	../vendor/bullet3/src/BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolverMt.cpp
	../vendor/bullet3/src/BulletDynamics/ConstraintSolver/btBatchedConstraints.cpp
	../vendor/bullet3/src/BulletDynamics/ConstraintSolver/btNNCGConstraintSolver.cpp
	../vendor/bullet3/src/BulletDynamics/ConstraintSolver/btSliderConstraint.cpp
	../vendor/bullet3/src/BulletDynamics/ConstraintSolver/btSolve2LinearConstraint.cpp
	../vendor/bullet3/src/BulletDynamics/ConstraintSolver/btTypedConstraint.cpp
	../vendor/bullet3/src/BulletDynamics/ConstraintSolver/btUniversalConstraint.cpp
	../vendor/bullet3/src/BulletDynamics/Dynamics/btDiscreteDynamicsWorld.cpp
	../vendor/bullet3/src/BulletDynamics/Dynamics/btDiscreteDynamicsWorldMt.cpp
	../vendor/bullet3/src/BulletDynamics/Dynamics/btSimulationIslandManagerMt.cpp
	../vendor/bullet3/src/BulletDynamics/Dynamics/btRigidBody.cpp
	../vendor/bullet3/src/BulletDynamics/Dynamics/btSimpleDynamicsWorld.cpp
	# ../vendor/bullet3/src/BulletDynamics/Dynamics/Bullet-C-API.cpp
	../vendor/bullet3/src/BulletDynamics/Vehicle/btRaycastVehicle.cpp
	../vendor/bullet3/src/BulletDynamics/Vehicle/btWheelInfo.cpp
	../vendor/bullet3/src/BulletDynamics/Featherstone/btMultiBody.cpp
	../vendor/bullet3/src/BulletDynamics/Featherstone/btMultiBodyConstraint.cpp
	../vendor/bullet3/src/BulletDynamics/Featherstone/btMultiBodyConstraintSolver.cpp
	../vendor/bullet3/src/BulletDynamics/Featherstone/btMultiBodyDynamicsWorld.cpp
	../vendor/bullet3/src/BulletDynamics/Featherstone/btMultiBodyFixedConstraint.cpp
	../vendor/bullet3/src/BulletDynamics/Featherstone/btMultiBodyGearConstraint.cpp
	../vendor/bullet3/src/BulletDynamics/Featherstone/btMultiBodyJointLimitConstraint.cpp
	../vendor/bullet3/src/BulletDynamics/Featherstone/btMultiBodyJointMotor.cpp
	../vendor/bullet3/src/BulletDynamics/Featherstone/btMultiBodyMLCPConstraintSolver.cpp
	../vendor/bullet3/src/BulletDynamics/Featherstone/btMultiBodyPoint2Point.cpp
	../vendor/bullet3/src/BulletDynamics/Featherstone/btMultiBodySliderConstraint.cpp
	../vendor/bullet3/src/BulletDynamics/Featherstone/btMultiBodySphericalJointMotor.cpp
	../vendor/bullet3/src/BulletDynamics/MLCPSolvers/btDantzigLCP.cpp
	../vendor/bullet3/src/BulletDynamics/MLCPSolvers/btMLCPSolver.cpp
	../vendor/bullet3/src/BulletDynamics/MLCPSolvers/btLemkeAlgorithm.cpp
)

SET(BulletCollision_SRCS
	../vendor/bullet3/src/BulletCollision/BroadphaseCollision/btAxisSweep3.cpp
	../vendor/bullet3/src/BulletCollision/BroadphaseCollision/btBroadphaseProxy.cpp
	../vendor/bullet3/src/BulletCollision/BroadphaseCollision/btCollisionAlgorithm.cpp
	../vendor/bullet3/src/BulletCollision/BroadphaseCollision/btDbvt.cpp
	../vendor/bullet3/src/BulletCollision/BroadphaseCollision/btDbvtBroadphase.cpp
	../vendor/bullet3/src/BulletCollision/BroadphaseCollision/btDispatcher.cpp
	../vendor/bullet3/src/BulletCollision/BroadphaseCollision/btOverlappingPairCache.cpp
	../vendor/bullet3/src/BulletCollision/BroadphaseCollision/btQuantizedBvh.cpp
	../vendor/bullet3/src/BulletCollision/BroadphaseCollision/btSimpleBroadphase.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btActivatingCollisionAlgorithm.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btBoxBoxCollisionAlgorithm.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btBox2dBox2dCollisionAlgorithm.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btBoxBoxDetector.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btCollisionDispatcher.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btCollisionDispatcherMt.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btCollisionObject.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btCollisionWorld.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btCollisionWorldImporter.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btCompoundCollisionAlgorithm.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btCompoundCompoundCollisionAlgorithm.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btConvexConcaveCollisionAlgorithm.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btConvexConvexAlgorithm.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btConvexPlaneCollisionAlgorithm.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btConvex2dConvex2dAlgorithm.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btEmptyCollisionAlgorithm.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btGhostObject.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btHashedSimplePairCache.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btInternalEdgeUtility.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btInternalEdgeUtility.h
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btManifoldResult.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btSimulationIslandManager.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btSphereBoxCollisionAlgorithm.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btSphereSphereCollisionAlgorithm.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btSphereTriangleCollisionAlgorithm.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/btUnionFind.cpp
	../vendor/bullet3/src/BulletCollision/CollisionDispatch/SphereTriangleDetector.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btBoxShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btBox2dShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btBvhTriangleMeshShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btCapsuleShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btCollisionShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btCompoundShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btConcaveShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btConeShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btConvexHullShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btConvexInternalShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btConvexPointCloudShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btConvexPolyhedron.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btConvexShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btConvex2dShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btConvexTriangleMeshShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btCylinderShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btEmptyShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btMiniSDF.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btMinkowskiSumShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btMultimaterialTriangleMeshShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btMultiSphereShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btOptimizedBvh.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btPolyhedralConvexShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btSdfCollisionShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btShapeHull.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btSphereShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btStaticPlaneShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btStridingMeshInterface.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btTetrahedronShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btTriangleBuffer.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btTriangleCallback.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btTriangleIndexVertexArray.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btTriangleIndexVertexMaterialArray.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btTriangleMesh.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btTriangleMeshShape.cpp
	../vendor/bullet3/src/BulletCollision/CollisionShapes/btUniformScalingShape.cpp
	../vendor/bullet3/src/BulletCollision/Gimpact/btContactProcessing.cpp
	../vendor/bullet3/src/BulletCollision/Gimpact/btGenericPoolAllocator.cpp
	../vendor/bullet3/src/BulletCollision/Gimpact/btGImpactBvh.cpp
	../vendor/bullet3/src/BulletCollision/Gimpact/btGImpactCollisionAlgorithm.cpp
	../vendor/bullet3/src/BulletCollision/Gimpact/btGImpactQuantizedBvh.cpp
	../vendor/bullet3/src/BulletCollision/Gimpact/btGImpactShape.cpp
	../vendor/bullet3/src/BulletCollision/Gimpact/btTriangleShapeEx.cpp
	../vendor/bullet3/src/BulletCollision/Gimpact/gim_box_set.cpp
	../vendor/bullet3/src/BulletCollision/Gimpact/gim_contact.cpp
	../vendor/bullet3/src/BulletCollision/Gimpact/gim_memory.cpp
	../vendor/bullet3/src/BulletCollision/Gimpact/gim_tri_collision.cpp
	../vendor/bullet3/src/BulletCollision/NarrowPhaseCollision/btContinuousConvexCollision.cpp
	../vendor/bullet3/src/BulletCollision/NarrowPhaseCollision/btConvexCast.cpp
	../vendor/bullet3/src/BulletCollision/NarrowPhaseCollision/btGjkConvexCast.cpp
	../vendor/bullet3/src/BulletCollision/NarrowPhaseCollision/btGjkEpa2.cpp
	../vendor/bullet3/src/BulletCollision/NarrowPhaseCollision/btGjkEpaPenetrationDepthSolver.cpp
	../vendor/bullet3/src/BulletCollision/NarrowPhaseCollision/btGjkPairDetector.cpp
	../vendor/bullet3/src/BulletCollision/NarrowPhaseCollision/btMinkowskiPenetrationDepthSolver.cpp
	../vendor/bullet3/src/BulletCollision/NarrowPhaseCollision/btPersistentManifold.cpp
	../vendor/bullet3/src/BulletCollision/NarrowPhaseCollision/btRaycastCallback.cpp
	../vendor/bullet3/src/BulletCollision/NarrowPhaseCollision/btSubSimplexConvexCast.cpp
	../vendor/bullet3/src/BulletCollision/NarrowPhaseCollision/btVoronoiSimplexSolver.cpp
	../vendor/bullet3/src/BulletCollision/NarrowPhaseCollision/btPolyhedralContactClipping.cpp
)

SET(LinearMath_SRCS
	../vendor/bullet3/src/LinearMath/btAlignedAllocator.cpp
	../vendor/bullet3/src/LinearMath/btConvexHull.cpp
	../vendor/bullet3/src/LinearMath/btConvexHullComputer.cpp
	../vendor/bullet3/src/LinearMath/btGeometryUtil.cpp
	../vendor/bullet3/src/LinearMath/btPolarDecomposition.cpp
	../vendor/bullet3/src/LinearMath/btQuickprof.cpp
	../vendor/bullet3/src/LinearMath/btSerializer.cpp
	../vendor/bullet3/src/LinearMath/btSerializer64.cpp
	../vendor/bullet3/src/LinearMath/btThreads.cpp
	../vendor/bullet3/src/LinearMath/btVector3.cpp
	../vendor/bullet3/src/LinearMath/TaskScheduler/btTaskScheduler.cpp
	../vendor/bullet3/src/LinearMath/TaskScheduler/btThreadSupportPosix.cpp
	../vendor/bullet3/src/LinearMath/TaskScheduler/btThreadSupportWin32.cpp
)

option(USE_MSVC_INCREMENTAL_LINKING "Use MSVC Incremental Linking" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../build) # save executable in root directory

set(CMAKE_GENERATOR_PLATFORM, x64)

unset(CMAKE_IMPORT_LIBRARY_SUFFIX)

add_library(BulletDynamics  STATIC "${BulletDynamics_SRCS}")
add_library(BulletCollision STATIC "${BulletCollision_SRCS}")
add_library(LinearMath      STATIC "${LinearMath_SRCS}")

add_executable("${PROJECT_NAME}" "${SOURCES}")

add_dependencies("${PROJECT_NAME}" BulletDynamics)
add_dependencies("${PROJECT_NAME}" BulletCollision)
add_dependencies("${PROJECT_NAME}" LinearMath)

set_target_properties("${PROJECT_NAME}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)
set_target_properties(BulletDynamics  PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)
set_target_properties(BulletCollision PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)
set_target_properties(LinearMath      PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)

# set_property(TARGET "${PROJECT_NAME}" PROPERTY CXX_STANDARD 14)
# set_property(TARGET BulletDynamics    PROPERTY CXX_STANDARD 14)
# set_property(TARGET BulletCollision   PROPERTY CXX_STANDARD 14)
# set_property(TARGET LinearMath        PROPERTY CXX_STANDARD 14)

target_compile_features("${PROJECT_NAME}" PRIVATE cxx_std_17)

include_directories(
	"${PROJECT_NAME}" PUBLIC
	../vendor/GLFW/include
	../vendor/GLEW/include
	../vendor/ASSIMP/include
	../vendor/ImGui
	../vendor/GLM
	../vendor/bullet3/src
	../vendor/tinyobjloader
	../vendor/spdlog/include
)

target_link_directories(
	"${PROJECT_NAME}" PUBLIC
	../vendor/GLFW/lib-vc2019
	../vendor/ASSIMP/lib
	../vendor/GLEW/lib
	../vendor/bullet3/lib
	../build/Release
	../build/Debug
)

target_link_libraries(
	"${PROJECT_NAME}"
	opengl32.lib
	glew32.lib
	glfw3.lib
	assimp-vc140-mt.lib
	BulletDynamics.lib
	BulletCollision.lib
	LinearMath.lib
)

file(COPY ./assimp-vc140-mt.dll DESTINATION ../build)
file(COPY ./glew32.dll          DESTINATION ../build)
file(COPY ./Models              DESTINATION ../build)
file(COPY ./Shaders             DESTINATION ../build)
file(COPY ./Textures            DESTINATION ../build)

set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}")
