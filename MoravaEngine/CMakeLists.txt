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
	../vendor/cross-platform/bullet3/src/BulletDynamics/Character/btKinematicCharacterController.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/ConstraintSolver/btConeTwistConstraint.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/ConstraintSolver/btContactConstraint.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/ConstraintSolver/btFixedConstraint.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/ConstraintSolver/btGearConstraint.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/ConstraintSolver/btGeneric6DofConstraint.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/ConstraintSolver/btGeneric6DofSpringConstraint.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/ConstraintSolver/btGeneric6DofSpring2Constraint.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/ConstraintSolver/btHinge2Constraint.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/ConstraintSolver/btHingeConstraint.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/ConstraintSolver/btPoint2PointConstraint.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolverMt.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/ConstraintSolver/btBatchedConstraints.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/ConstraintSolver/btNNCGConstraintSolver.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/ConstraintSolver/btSliderConstraint.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/ConstraintSolver/btSolve2LinearConstraint.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/ConstraintSolver/btTypedConstraint.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/ConstraintSolver/btUniversalConstraint.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/Dynamics/btDiscreteDynamicsWorld.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/Dynamics/btDiscreteDynamicsWorldMt.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/Dynamics/btSimulationIslandManagerMt.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/Dynamics/btRigidBody.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/Dynamics/btSimpleDynamicsWorld.cpp
	# ../vendor/cross-platform/bullet3/src/BulletDynamics/Dynamics/Bullet-C-API.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/Vehicle/btRaycastVehicle.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/Vehicle/btWheelInfo.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/Featherstone/btMultiBody.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/Featherstone/btMultiBodyConstraint.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/Featherstone/btMultiBodyConstraintSolver.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/Featherstone/btMultiBodyDynamicsWorld.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/Featherstone/btMultiBodyFixedConstraint.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/Featherstone/btMultiBodyGearConstraint.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/Featherstone/btMultiBodyJointLimitConstraint.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/Featherstone/btMultiBodyJointMotor.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/Featherstone/btMultiBodyMLCPConstraintSolver.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/Featherstone/btMultiBodyPoint2Point.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/Featherstone/btMultiBodySliderConstraint.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/Featherstone/btMultiBodySphericalJointMotor.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/MLCPSolvers/btDantzigLCP.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/MLCPSolvers/btMLCPSolver.cpp
	../vendor/cross-platform/bullet3/src/BulletDynamics/MLCPSolvers/btLemkeAlgorithm.cpp
)

SET(BulletCollision_SRCS
	../vendor/cross-platform/bullet3/src/BulletCollision/BroadphaseCollision/btAxisSweep3.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/BroadphaseCollision/btBroadphaseProxy.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/BroadphaseCollision/btCollisionAlgorithm.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/BroadphaseCollision/btDbvt.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/BroadphaseCollision/btDbvtBroadphase.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/BroadphaseCollision/btDispatcher.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/BroadphaseCollision/btOverlappingPairCache.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/BroadphaseCollision/btQuantizedBvh.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/BroadphaseCollision/btSimpleBroadphase.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btActivatingCollisionAlgorithm.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btBoxBoxCollisionAlgorithm.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btBox2dBox2dCollisionAlgorithm.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btBoxBoxDetector.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btCollisionDispatcher.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btCollisionDispatcherMt.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btCollisionObject.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btCollisionWorld.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btCollisionWorldImporter.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btCompoundCollisionAlgorithm.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btCompoundCompoundCollisionAlgorithm.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btConvexConcaveCollisionAlgorithm.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btConvexConvexAlgorithm.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btConvexPlaneCollisionAlgorithm.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btConvex2dConvex2dAlgorithm.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btEmptyCollisionAlgorithm.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btGhostObject.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btHashedSimplePairCache.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btInternalEdgeUtility.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btInternalEdgeUtility.h
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btManifoldResult.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btSimulationIslandManager.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btSphereBoxCollisionAlgorithm.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btSphereSphereCollisionAlgorithm.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btSphereTriangleCollisionAlgorithm.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/btUnionFind.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionDispatch/SphereTriangleDetector.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btBoxShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btBox2dShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btBvhTriangleMeshShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btCapsuleShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btCollisionShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btCompoundShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btConcaveShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btConeShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btConvexHullShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btConvexInternalShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btConvexPointCloudShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btConvexPolyhedron.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btConvexShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btConvex2dShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btConvexTriangleMeshShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btCylinderShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btEmptyShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btMiniSDF.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btMinkowskiSumShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btMultimaterialTriangleMeshShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btMultiSphereShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btOptimizedBvh.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btPolyhedralConvexShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btSdfCollisionShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btShapeHull.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btSphereShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btStaticPlaneShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btStridingMeshInterface.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btTetrahedronShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btTriangleBuffer.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btTriangleCallback.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btTriangleIndexVertexArray.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btTriangleIndexVertexMaterialArray.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btTriangleMesh.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btTriangleMeshShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/CollisionShapes/btUniformScalingShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/Gimpact/btContactProcessing.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/Gimpact/btGenericPoolAllocator.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/Gimpact/btGImpactBvh.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/Gimpact/btGImpactCollisionAlgorithm.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/Gimpact/btGImpactQuantizedBvh.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/Gimpact/btGImpactShape.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/Gimpact/btTriangleShapeEx.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/Gimpact/gim_box_set.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/Gimpact/gim_contact.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/Gimpact/gim_memory.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/Gimpact/gim_tri_collision.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/NarrowPhaseCollision/btContinuousConvexCollision.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/NarrowPhaseCollision/btConvexCast.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/NarrowPhaseCollision/btGjkConvexCast.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/NarrowPhaseCollision/btGjkEpa2.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/NarrowPhaseCollision/btGjkEpaPenetrationDepthSolver.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/NarrowPhaseCollision/btGjkPairDetector.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/NarrowPhaseCollision/btMinkowskiPenetrationDepthSolver.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/NarrowPhaseCollision/btPersistentManifold.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/NarrowPhaseCollision/btRaycastCallback.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/NarrowPhaseCollision/btSubSimplexConvexCast.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/NarrowPhaseCollision/btVoronoiSimplexSolver.cpp
	../vendor/cross-platform/bullet3/src/BulletCollision/NarrowPhaseCollision/btPolyhedralContactClipping.cpp
)

SET(LinearMath_SRCS
	../vendor/cross-platform/bullet3/src/LinearMath/btAlignedAllocator.cpp
	../vendor/cross-platform/bullet3/src/LinearMath/btConvexHull.cpp
	../vendor/cross-platform/bullet3/src/LinearMath/btConvexHullComputer.cpp
	../vendor/cross-platform/bullet3/src/LinearMath/btGeometryUtil.cpp
	../vendor/cross-platform/bullet3/src/LinearMath/btPolarDecomposition.cpp
	../vendor/cross-platform/bullet3/src/LinearMath/btQuickprof.cpp
	../vendor/cross-platform/bullet3/src/LinearMath/btSerializer.cpp
	../vendor/cross-platform/bullet3/src/LinearMath/btSerializer64.cpp
	../vendor/cross-platform/bullet3/src/LinearMath/btThreads.cpp
	../vendor/cross-platform/bullet3/src/LinearMath/btVector3.cpp
	../vendor/cross-platform/bullet3/src/LinearMath/TaskScheduler/btTaskScheduler.cpp
	../vendor/cross-platform/bullet3/src/LinearMath/TaskScheduler/btThreadSupportPosix.cpp
	../vendor/cross-platform/bullet3/src/LinearMath/TaskScheduler/btThreadSupportWin32.cpp
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
	../vendor/cross-platform/bullet3/src
	../vendor/tinyobjloader
	../vendor/spdlog/include
)

target_link_directories(
	"${PROJECT_NAME}" PUBLIC
	../vendor/GLFW/lib-vc2019
	../vendor/ASSIMP/lib
	../vendor/GLEW/lib
	../vendor/cross-platform/bullet3/lib
	../build/Release
	../build/Debug
)

if (WIN32)
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
endif (WIN32)

if (UNIX)
target_link_libraries(
	"${PROJECT_NAME}"
	libGLEW.so # -lGLEW
	libGLU.so  # -lGLU
	libGL.so   # -lGL

	glfw3.lib
	assimp-vc140-mt.lib
	BulletDynamics.lib
	BulletCollision.lib
	LinearMath.lib
)
endif (UNIX)

file(COPY ./assimp-vc140-mt.dll DESTINATION ../build)
file(COPY ./glew32.dll          DESTINATION ../build)
file(COPY ./Models              DESTINATION ../build)
file(COPY ./Shaders             DESTINATION ../build)
file(COPY ./Textures            DESTINATION ../build)

set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}")
