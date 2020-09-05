cmake_minimum_required(VERSION 3.16)

set(ENV{MORAVA_BUILD_TYPE} Release)

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
elseif (UNIX)
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
endif ()

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

SET(assimp_SRCS
	../vendor/cross-platform/assimp/code/Common/Assimp.cpp
	../vendor/cross-platform/assimp/code/CApi/CInterfaceIOWrapper.cpp
	../vendor/cross-platform/assimp/code/CApi/CInterfaceIOWrapper.h
	../vendor/cross-platform/assimp/code/Common/BaseImporter.cpp
	../vendor/cross-platform/assimp/code/Common/BaseProcess.cpp
	../vendor/cross-platform/assimp/code/Common/BaseProcess.h
	../vendor/cross-platform/assimp/code/Common/Importer.h
	../vendor/cross-platform/assimp/code/Common/ScenePrivate.h
	../vendor/cross-platform/assimp/code/Common/PostStepRegistry.cpp
	../vendor/cross-platform/assimp/code/Common/ImporterRegistry.cpp
	../vendor/cross-platform/assimp/code/Common/DefaultProgressHandler.h
	../vendor/cross-platform/assimp/code/Common/DefaultIOStream.cpp
	../vendor/cross-platform/assimp/code/Common/DefaultIOSystem.cpp
	../vendor/cross-platform/assimp/code/Common/ZipArchiveIOSystem.cpp
	../vendor/cross-platform/assimp/code/Common/PolyTools.h
	../vendor/cross-platform/assimp/code/Common/Importer.cpp
	../vendor/cross-platform/assimp/code/Common/IFF.h
	../vendor/cross-platform/assimp/code/Common/SGSpatialSort.cpp
	../vendor/cross-platform/assimp/code/Common/VertexTriangleAdjacency.cpp
	../vendor/cross-platform/assimp/code/Common/VertexTriangleAdjacency.h
	../vendor/cross-platform/assimp/code/Common/SpatialSort.cpp
	../vendor/cross-platform/assimp/code/Common/SceneCombiner.cpp
	../vendor/cross-platform/assimp/code/Common/ScenePreprocessor.cpp
	../vendor/cross-platform/assimp/code/Common/ScenePreprocessor.h
	../vendor/cross-platform/assimp/code/Common/SkeletonMeshBuilder.cpp
	../vendor/cross-platform/assimp/code/Common/StandardShapes.cpp
	../vendor/cross-platform/assimp/code/Common/TargetAnimation.cpp
	../vendor/cross-platform/assimp/code/Common/TargetAnimation.h
	../vendor/cross-platform/assimp/code/Common/RemoveComments.cpp
	../vendor/cross-platform/assimp/code/Common/Subdivision.cpp
	../vendor/cross-platform/assimp/code/Common/scene.cpp
	../vendor/cross-platform/assimp/code/Common/Bitmap.cpp
	../vendor/cross-platform/assimp/code/Common/Version.cpp
	../vendor/cross-platform/assimp/code/Common/CreateAnimMesh.cpp
	../vendor/cross-platform/assimp/code/Common/simd.h
	../vendor/cross-platform/assimp/code/Common/simd.cpp
	../vendor/cross-platform/assimp/code/Common/material.cpp
	../vendor/cross-platform/assimp/code/Common/AssertHandler.cpp
	../vendor/cross-platform/assimp/include/assimp/DefaultLogger.hpp
	../vendor/cross-platform/assimp/include/assimp/LogStream.hpp
    ../vendor/cross-platform/assimp/include/assimp/Logger.hpp
    ../vendor/cross-platform/assimp/include/assimp/NullLogger.hpp
    ../vendor/cross-platform/assimp/code/Common/Win32DebugLogStream.h
    ../vendor/cross-platform/assimp/code/Common/DefaultLogger.cpp
    ../vendor/cross-platform/assimp/code/Common/FileLogStream.h
    ../vendor/cross-platform/assimp/code/Common/StdOStreamLogStream.h
    ../vendor/cross-platform/assimp/code/Common/Exporter.cpp
    ../vendor/cross-platform/assimp/code/CApi/AssimpCExport.cpp
    ../vendor/cross-platform/assimp/include/assimp/BlobIOSystem.h
    ../vendor/cross-platform/assimp/code/PostProcessing/CalcTangentsProcess.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/CalcTangentsProcess.h
    ../vendor/cross-platform/assimp/code/PostProcessing/ComputeUVMappingProcess.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/ComputeUVMappingProcess.h
    ../vendor/cross-platform/assimp/code/PostProcessing/ConvertToLHProcess.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/ConvertToLHProcess.h
    ../vendor/cross-platform/assimp/code/PostProcessing/EmbedTexturesProcess.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/EmbedTexturesProcess.h
    ../vendor/cross-platform/assimp/code/PostProcessing/FindDegenerates.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/FindDegenerates.h
    ../vendor/cross-platform/assimp/code/PostProcessing/FindInstancesProcess.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/FindInstancesProcess.h
    ../vendor/cross-platform/assimp/code/PostProcessing/FindInvalidDataProcess.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/FindInvalidDataProcess.h
    ../vendor/cross-platform/assimp/code/PostProcessing/FixNormalsStep.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/FixNormalsStep.h
    ../vendor/cross-platform/assimp/code/PostProcessing/DropFaceNormalsProcess.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/DropFaceNormalsProcess.h
    ../vendor/cross-platform/assimp/code/PostProcessing/GenFaceNormalsProcess.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/GenFaceNormalsProcess.h
    ../vendor/cross-platform/assimp/code/PostProcessing/GenVertexNormalsProcess.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/GenVertexNormalsProcess.h
    ../vendor/cross-platform/assimp/code/PostProcessing/PretransformVertices.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/PretransformVertices.h
    ../vendor/cross-platform/assimp/code/PostProcessing/ImproveCacheLocality.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/ImproveCacheLocality.h
    ../vendor/cross-platform/assimp/code/PostProcessing/JoinVerticesProcess.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/JoinVerticesProcess.h
    ../vendor/cross-platform/assimp/code/PostProcessing/LimitBoneWeightsProcess.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/LimitBoneWeightsProcess.h
    ../vendor/cross-platform/assimp/code/PostProcessing/RemoveRedundantMaterials.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/RemoveRedundantMaterials.h
    ../vendor/cross-platform/assimp/code/PostProcessing/RemoveVCProcess.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/RemoveVCProcess.h
    ../vendor/cross-platform/assimp/code/PostProcessing/SortByPTypeProcess.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/SortByPTypeProcess.h
    ../vendor/cross-platform/assimp/code/PostProcessing/SplitLargeMeshes.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/SplitLargeMeshes.h
    ../vendor/cross-platform/assimp/code/PostProcessing/TextureTransform.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/TextureTransform.h
    ../vendor/cross-platform/assimp/code/PostProcessing/TriangulateProcess.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/TriangulateProcess.h
    ../vendor/cross-platform/assimp/code/PostProcessing/ValidateDataStructure.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/ValidateDataStructure.h
    ../vendor/cross-platform/assimp/code/PostProcessing/OptimizeGraph.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/OptimizeGraph.h
    ../vendor/cross-platform/assimp/code/PostProcessing/OptimizeMeshes.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/OptimizeMeshes.h
    ../vendor/cross-platform/assimp/code/PostProcessing/DeboneProcess.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/DeboneProcess.h
    ../vendor/cross-platform/assimp/code/PostProcessing/ProcessHelper.h
    ../vendor/cross-platform/assimp/code/PostProcessing/ProcessHelper.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/MakeVerboseFormat.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/MakeVerboseFormat.h
    ../vendor/cross-platform/assimp/code/PostProcessing/ScaleProcess.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/ScaleProcess.h
    ../vendor/cross-platform/assimp/code/PostProcessing/ArmaturePopulate.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/ArmaturePopulate.h
    ../vendor/cross-platform/assimp/code/PostProcessing/GenBoundingBoxesProcess.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/GenBoundingBoxesProcess.h
    ../vendor/cross-platform/assimp/code/PostProcessing/SplitByBoneCountProcess.cpp
    ../vendor/cross-platform/assimp/code/PostProcessing/SplitByBoneCountProcess.h
    ../vendor/cross-platform/assimp/code/Material/MaterialSystem.cpp
    ../vendor/cross-platform/assimp/code/Material/MaterialSystem.h
    ../vendor/cross-platform/assimp/code/AssetLib/STEPParser/STEPFileReader.h
    ../vendor/cross-platform/assimp/code/AssetLib/STEPParser/STEPFileReader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/STEPParser/STEPFileEncoding.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/STEPParser/STEPFileEncoding.h
    ../vendor/cross-platform/assimp/code/AssetLib/AMF/AMFImporter.hpp
    ../vendor/cross-platform/assimp/code/AssetLib/AMF/AMFImporter_Macro.hpp
    ../vendor/cross-platform/assimp/code/AssetLib/AMF/AMFImporter_Node.hpp
    ../vendor/cross-platform/assimp/code/AssetLib/AMF/AMFImporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/AMF/AMFImporter_Geometry.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/AMF/AMFImporter_Material.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/AMF/AMFImporter_Postprocess.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/3DS/3DSConverter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/3DS/3DSHelper.h
    ../vendor/cross-platform/assimp/code/AssetLib/3DS/3DSLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/3DS/3DSLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/AC/ACLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/AC/ACLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/ASE/ASELoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/ASE/ASELoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/ASE/ASEParser.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/ASE/ASEParser.h
    ../vendor/cross-platform/assimp/code/AssetLib/Assbin/AssbinLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/Assbin/AssbinLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/B3D/B3DImporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/B3D/B3DImporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/BVH/BVHLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/BVH/BVHLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/Collada/ColladaHelper.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Collada/ColladaHelper.h
    ../vendor/cross-platform/assimp/code/AssetLib/Collada/ColladaLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Collada/ColladaLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/Collada/ColladaParser.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Collada/ColladaParser.h
    ../vendor/cross-platform/assimp/code/AssetLib/DXF/DXFLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/DXF/DXFLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/DXF/DXFHelper.h
    ../vendor/cross-platform/assimp/code/AssetLib/CSM/CSMLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/CSM/CSMLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/HMP/HMPFileData.h
    ../vendor/cross-platform/assimp/code/AssetLib/HMP/HMPLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/HMP/HMPLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/HMP/HalfLifeFileData.h
    ../vendor/cross-platform/assimp/code/AssetLib/Irr/IRRMeshLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Irr/IRRMeshLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/Irr/IRRShared.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Irr/IRRShared.h
    ../vendor/cross-platform/assimp/code/AssetLib/Irr/IRRLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Irr/IRRLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/LWO/LWOAnimation.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/LWO/LWOAnimation.h
    ../vendor/cross-platform/assimp/code/AssetLib/LWO/LWOBLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/LWO/LWOFileData.h
    ../vendor/cross-platform/assimp/code/AssetLib/LWO/LWOLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/LWO/LWOLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/LWO/LWOMaterial.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/LWS/LWSLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/LWS/LWSLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/M3D/M3DMaterials.h
    ../vendor/cross-platform/assimp/code/AssetLib/M3D/M3DImporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/M3D/M3DImporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/M3D/M3DWrapper.h
    ../vendor/cross-platform/assimp/code/AssetLib/M3D/M3DWrapper.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/M3D/m3d.h
    ../vendor/cross-platform/assimp/code/AssetLib/MD2/MD2FileData.h
    ../vendor/cross-platform/assimp/code/AssetLib/MD2/MD2Loader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/MD2/MD2Loader.h
    ../vendor/cross-platform/assimp/code/AssetLib/MD2/MD2NormalTable.h
    ../vendor/cross-platform/assimp/code/AssetLib/MD3/MD3FileData.h
    ../vendor/cross-platform/assimp/code/AssetLib/MD3/MD3Loader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/MD3/MD3Loader.h
    ../vendor/cross-platform/assimp/code/AssetLib/MD5/MD5Loader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/MD5/MD5Loader.h
    ../vendor/cross-platform/assimp/code/AssetLib/MD5/MD5Parser.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/MD5/MD5Parser.h
    ../vendor/cross-platform/assimp/code/AssetLib/MDC/MDCFileData.h
    ../vendor/cross-platform/assimp/code/AssetLib/MDC/MDCLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/MDC/MDCLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/MDC/MDCNormalTable.h
    ../vendor/cross-platform/assimp/code/AssetLib/MDL/MDLDefaultColorMap.h
    ../vendor/cross-platform/assimp/code/AssetLib/MDL/MDLFileData.h
    ../vendor/cross-platform/assimp/code/AssetLib/MDL/MDLLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/MDL/MDLLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/MDL/MDLMaterialLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/MDL/HalfLife/HalfLifeMDLBaseHeader.h
    ../vendor/cross-platform/assimp/code/AssetLib/MDL/HalfLife/HL1FileData.h
    ../vendor/cross-platform/assimp/code/AssetLib/MDL/HalfLife/HL1MDLLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/MDL/HalfLife/HL1MDLLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/MDL/HalfLife/HL1ImportDefinitions.h
    ../vendor/cross-platform/assimp/code/AssetLib/MDL/HalfLife/HL1ImportSettings.h
    ../vendor/cross-platform/assimp/code/AssetLib/MDL/HalfLife/HL1MeshTrivert.h
    ../vendor/cross-platform/assimp/code/AssetLib/MDL/HalfLife/LogFunctions.h
    ../vendor/cross-platform/assimp/code/AssetLib/MDL/HalfLife/UniqueNameGenerator.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/MDL/HalfLife/UniqueNameGenerator.h
    ../vendor/cross-platform/assimp/code/AssetLib/NFF/NFFLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/NFF/NFFLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/NDO/NDOLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/NDO/NDOLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/OFF/OFFLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/OFF/OFFLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/Obj/ObjFileData.h
    ../vendor/cross-platform/assimp/code/AssetLib/Obj/ObjFileImporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Obj/ObjFileImporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/Obj/ObjFileMtlImporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Obj/ObjFileMtlImporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/Obj/ObjFileParser.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Obj/ObjFileParser.h
    ../vendor/cross-platform/assimp/code/AssetLib/Obj/ObjTools.h
    ../vendor/cross-platform/assimp/code/AssetLib/Ogre/OgreImporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/Ogre/OgreStructs.h
    ../vendor/cross-platform/assimp/code/AssetLib/Ogre/OgreParsingUtils.h
    ../vendor/cross-platform/assimp/code/AssetLib/Ogre/OgreBinarySerializer.h
    ../vendor/cross-platform/assimp/code/AssetLib/Ogre/OgreXmlSerializer.h
    ../vendor/cross-platform/assimp/code/AssetLib/Ogre/OgreImporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Ogre/OgreStructs.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Ogre/OgreBinarySerializer.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Ogre/OgreXmlSerializer.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Ogre/OgreMaterial.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/OpenGEX/OpenGEXImporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/OpenGEX/OpenGEXImporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/OpenGEX/OpenGEXStructs.h
    ../vendor/cross-platform/assimp/code/AssetLib/Ply/PlyLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Ply/PlyLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/Ply/PlyParser.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Ply/PlyParser.h
    ../vendor/cross-platform/assimp/code/AssetLib/MS3D/MS3DLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/MS3D/MS3DLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/COB/COBLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/COB/COBLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/COB/COBScene.h
    ../vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderDNA.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderDNA.h
    ../vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderDNA.inl
    ../vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderScene.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderScene.h
    ../vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderSceneGen.h
    ../vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderIntermediate.h
    ../vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderModifier.h
    ../vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderModifier.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderBMesh.h
    ../vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderBMesh.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderTessellator.h
    ../vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderTessellator.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderCustomData.h
    ../vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderCustomData.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/IFC/IFCLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/IFC/IFCLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/IFC/IFCReaderGen1_2x3.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/IFC/IFCReaderGen2_2x3.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/IFC/IFCReaderGen_2x3.h
    ../vendor/cross-platform/assimp/code/AssetLib/IFC/IFCUtil.h
    ../vendor/cross-platform/assimp/code/AssetLib/IFC/IFCUtil.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/IFC/IFCGeometry.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/IFC/IFCMaterial.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/IFC/IFCProfile.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/IFC/IFCCurve.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/IFC/IFCBoolean.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/IFC/IFCOpenings.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/XGL/XGLLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/XGL/XGLLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXImporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXCompileConfig.h
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXImporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXParser.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXParser.h
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXTokenizer.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXTokenizer.h
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXImportSettings.h
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXConverter.h
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXConverter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXUtil.h
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXUtil.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXDocument.h
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXDocument.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXProperties.h
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXProperties.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXMeshGeometry.h
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXMeshGeometry.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXMaterial.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXModel.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXAnimation.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXNodeAttribute.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXDeformer.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXBinaryTokenizer.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXDocumentUtil.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXCommon.h
    ../vendor/cross-platform/assimp/code/AssetLib/Q3D/Q3DLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Q3D/Q3DLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/Q3BSP/Q3BSPFileData.h
    ../vendor/cross-platform/assimp/code/AssetLib/Q3BSP/Q3BSPFileParser.h
    ../vendor/cross-platform/assimp/code/AssetLib/Q3BSP/Q3BSPFileParser.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Q3BSP/Q3BSPFileImporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/Q3BSP/Q3BSPFileImporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Raw/RawLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Raw/RawLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/SIB/SIBImporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/SIB/SIBImporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/SMD/SMDLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/SMD/SMDLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/STL/STLLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/STL/STLLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/Terragen/TerragenLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Terragen/TerragenLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/Unreal/UnrealLoader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Unreal/UnrealLoader.h
    ../vendor/cross-platform/assimp/code/AssetLib/X/XFileHelper.h
    ../vendor/cross-platform/assimp/code/AssetLib/X/XFileImporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/X/XFileImporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/X/XFileParser.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/X/XFileParser.h
    ../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter.hpp
    ../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Geometry2D.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Geometry3D.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Group.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Light.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Macro.hpp
    ../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Metadata.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Networking.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Node.hpp
    ../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Postprocess.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Rendering.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Shape.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Texturing.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/X3D/FIReader.hpp
    ../vendor/cross-platform/assimp/code/AssetLib/X3D/FIReader.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DVocabulary.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/glTF/glTFCommon.h
    ../vendor/cross-platform/assimp/code/AssetLib/glTF/glTFCommon.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/glTF/glTFAsset.h
    ../vendor/cross-platform/assimp/code/AssetLib/glTF/glTFAsset.inl
    ../vendor/cross-platform/assimp/code/AssetLib/glTF/glTFAssetWriter.h
    ../vendor/cross-platform/assimp/code/AssetLib/glTF/glTFAssetWriter.inl
    ../vendor/cross-platform/assimp/code/AssetLib/glTF/glTFImporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/glTF/glTFImporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/glTF2/glTF2Asset.h
    ../vendor/cross-platform/assimp/code/AssetLib/glTF2/glTF2Asset.inl
    ../vendor/cross-platform/assimp/code/AssetLib/glTF2/glTF2AssetWriter.h
    ../vendor/cross-platform/assimp/code/AssetLib/glTF2/glTF2AssetWriter.inl
    ../vendor/cross-platform/assimp/code/AssetLib/glTF2/glTF2Importer.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/glTF2/glTF2Importer.h
    ../vendor/cross-platform/assimp/code/AssetLib/3MF/D3MFImporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/3MF/D3MFImporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/3MF/D3MFOpcPackage.h
    ../vendor/cross-platform/assimp/code/AssetLib/3MF/D3MFOpcPackage.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/3MF/3MFXmlTags.h
    ../vendor/cross-platform/assimp/code/AssetLib/MMD/MMDCpp14.h
    ../vendor/cross-platform/assimp/code/AssetLib/MMD/MMDImporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/MMD/MMDImporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/MMD/MMDPmdParser.h
    ../vendor/cross-platform/assimp/code/AssetLib/MMD/MMDPmxParser.h
    ../vendor/cross-platform/assimp/code/AssetLib/MMD/MMDPmxParser.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/MMD/MMDVmdParser.h
    ../vendor/cross-platform/assimp/code/AssetLib/Obj/ObjExporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/Obj/ObjExporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/OpenGEX/OpenGEXExporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/OpenGEX/OpenGEXExporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/Ply/PlyExporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Ply/PlyExporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/3DS/3DSExporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/3DS/3DSExporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Assbin/AssbinExporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/Assbin/AssbinExporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Assbin/AssbinFileWriter.h
    ../vendor/cross-platform/assimp/code/AssetLib/Assbin/AssbinFileWriter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Assxml/AssxmlExporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/Assxml/AssxmlExporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Assxml/AssxmlFileWriter.h
    ../vendor/cross-platform/assimp/code/AssetLib/Assxml/AssxmlFileWriter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/M3D/M3DExporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/M3D/M3DExporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Collada/ColladaExporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/Collada/ColladaExporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXExporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXExporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXExportNode.h
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXExportNode.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXExportProperty.h
    ../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXExportProperty.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/STL/STLExporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/STL/STLExporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/X/XFileExporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/X/XFileExporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DExporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DExporter.hpp
    ../vendor/cross-platform/assimp/code/AssetLib/glTF/glTFExporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/glTF/glTFExporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/glTF2/glTF2Exporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/glTF2/glTF2Exporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/3MF/D3MFExporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/3MF/D3MFExporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Assjson/cencode.c
    ../vendor/cross-platform/assimp/code/AssetLib/Assjson/cencode.h
    ../vendor/cross-platform/assimp/code/AssetLib/Assjson/json_exporter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Assjson/mesh_splitter.cpp
    ../vendor/cross-platform/assimp/code/AssetLib/Assjson/mesh_splitter.h
    ../vendor/cross-platform/assimp/code/AssetLib/Step/StepExporter.h
    ../vendor/cross-platform/assimp/code/AssetLib/Step/StepExporter.cpp
    ../vendor/cross-platform/assimp/include/assimp/irrXMLWrapper.h
    ../vendor/cross-platform/assimp/contrib/unzip/crypt.h
    ../vendor/cross-platform/assimp/contrib/unzip/ioapi.c
    ../vendor/cross-platform/assimp/contrib/unzip/ioapi.h
    ../vendor/cross-platform/assimp/contrib/unzip/unzip.c
    ../vendor/cross-platform/assimp/contrib/unzip/unzip.h
    ../vendor/cross-platform/assimp/contrib/poly2tri/poly2tri/common/shapes.cc
    ../vendor/cross-platform/assimp/contrib/poly2tri/poly2tri/common/shapes.h
    ../vendor/cross-platform/assimp/contrib/poly2tri/poly2tri/common/utils.h
    ../vendor/cross-platform/assimp/contrib/poly2tri/poly2tri/sweep/advancing_front.h
    ../vendor/cross-platform/assimp/contrib/poly2tri/poly2tri/sweep/advancing_front.cc
    ../vendor/cross-platform/assimp/contrib/poly2tri/poly2tri/sweep/cdt.cc
    ../vendor/cross-platform/assimp/contrib/poly2tri/poly2tri/sweep/cdt.h
    ../vendor/cross-platform/assimp/contrib/poly2tri/poly2tri/sweep/sweep.cc
    ../vendor/cross-platform/assimp/contrib/poly2tri/poly2tri/sweep/sweep.h
    ../vendor/cross-platform/assimp/contrib/poly2tri/poly2tri/sweep/sweep_context.cc
    ../vendor/cross-platform/assimp/contrib/poly2tri/poly2tri/sweep/sweep_context.h
    ../vendor/cross-platform/assimp/contrib/clipper/clipper.hpp
    ../vendor/cross-platform/assimp/contrib/clipper/clipper.cpp
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcAdjacencyInfo.h
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcArithmeticCodec.cpp
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcArithmeticCodec.h
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcBinaryStream.h
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcCommon.h
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcDVEncodeParams.h
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcDynamicVectorDecoder.cpp
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcDynamicVectorDecoder.h
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcDynamicVectorEncoder.cpp
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcDynamicVectorEncoder.h
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcDynamicVector.h
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcFIFO.h
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcIndexedFaceSet.h
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcIndexedFaceSet.inl
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcSC3DMCDecoder.h
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcSC3DMCDecoder.inl
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcSC3DMCEncodeParams.h
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcSC3DMCEncoder.h
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcSC3DMCEncoder.inl
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcTimer.h
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcTools.cpp
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcTriangleFans.cpp
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcTriangleFans.h
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcTriangleListDecoder.h
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcTriangleListDecoder.inl
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcTriangleListEncoder.h
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcTriangleListEncoder.inl
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcVector.h
    ../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcVector.inl
    ../vendor/cross-platform/assimp/contrib/zip/src/miniz.h
    ../vendor/cross-platform/assimp/contrib/zip/src/zip.c
    ../vendor/cross-platform/assimp/contrib/zip/src/zip.h
    ../vendor/cross-platform/assimp/include/assimp/anim.h
    ../vendor/cross-platform/assimp/include/assimp/aabb.h
    ../vendor/cross-platform/assimp/include/assimp/ai_assert.h
    ../vendor/cross-platform/assimp/include/assimp/camera.h
    ../vendor/cross-platform/assimp/include/assimp/color4.h
    ../vendor/cross-platform/assimp/include/assimp/color4.inl
    ../vendor/cross-platform/assimp/include/assimp/config.h
    ../vendor/cross-platform/assimp/include/assimp/ColladaMetaData.h
    ../vendor/cross-platform/assimp/include/assimp/commonMetaData.h
    ../vendor/cross-platform/assimp/include/assimp/defs.h
    ../vendor/cross-platform/assimp/include/assimp/Defines.h
    ../vendor/cross-platform/assimp/include/assimp/cfileio.h
    ../vendor/cross-platform/assimp/include/assimp/light.h
    ../vendor/cross-platform/assimp/include/assimp/material.h
    ../vendor/cross-platform/assimp/include/assimp/material.inl
    ../vendor/cross-platform/assimp/include/assimp/matrix3x3.h
    ../vendor/cross-platform/assimp/include/assimp/matrix3x3.inl
    ../vendor/cross-platform/assimp/include/assimp/matrix4x4.h
    ../vendor/cross-platform/assimp/include/assimp/matrix4x4.inl
    ../vendor/cross-platform/assimp/include/assimp/mesh.h
    ../vendor/cross-platform/assimp/include/assimp/pbrmaterial.h
    ../vendor/cross-platform/assimp/include/assimp/postprocess.h
    ../vendor/cross-platform/assimp/include/assimp/quaternion.h
    ../vendor/cross-platform/assimp/include/assimp/quaternion.inl
    ../vendor/cross-platform/assimp/include/assimp/scene.h
    ../vendor/cross-platform/assimp/include/assimp/metadata.h
    ../vendor/cross-platform/assimp/include/assimp/texture.h
    ../vendor/cross-platform/assimp/include/assimp/types.h
    ../vendor/cross-platform/assimp/include/assimp/vector2.h
    ../vendor/cross-platform/assimp/include/assimp/vector2.inl
    ../vendor/cross-platform/assimp/include/assimp/vector3.h
    ../vendor/cross-platform/assimp/include/assimp/vector3.inl
    ../vendor/cross-platform/assimp/include/assimp/version.h
    ../vendor/cross-platform/assimp/include/assimp/cimport.h
    ../vendor/cross-platform/assimp/include/assimp/importerdesc.h
    ../vendor/cross-platform/assimp/include/assimp/Importer.hpp
    ../vendor/cross-platform/assimp/include/assimp/ProgressHandler.hpp
    ../vendor/cross-platform/assimp/include/assimp/IOStream.hpp
    ../vendor/cross-platform/assimp/include/assimp/IOSystem.hpp
    ../vendor/cross-platform/assimp/include/assimp/cexport.h
    ../vendor/cross-platform/assimp/include/assimp/Exporter.hpp
    ../vendor/cross-platform/assimp/include/assimp/DefaultIOStream.h
    ../vendor/cross-platform/assimp/include/assimp/DefaultIOSystem.h
    ../vendor/cross-platform/assimp/include/assimp/ZipArchiveIOSystem.h
    ../vendor/cross-platform/assimp/include/assimp/SceneCombiner.h
    ../vendor/cross-platform/assimp/include/assimp/fast_atof.h
    ../vendor/cross-platform/assimp/include/assimp/qnan.h
    ../vendor/cross-platform/assimp/include/assimp/BaseImporter.h
    ../vendor/cross-platform/assimp/include/assimp/Hash.h
    ../vendor/cross-platform/assimp/include/assimp/MemoryIOWrapper.h
    ../vendor/cross-platform/assimp/include/assimp/ParsingUtils.h
    ../vendor/cross-platform/assimp/include/assimp/StreamReader.h
    ../vendor/cross-platform/assimp/include/assimp/StreamWriter.h
    ../vendor/cross-platform/assimp/include/assimp/StringComparison.h
    ../vendor/cross-platform/assimp/include/assimp/StringUtils.h
    ../vendor/cross-platform/assimp/include/assimp/SGSpatialSort.h
    ../vendor/cross-platform/assimp/include/assimp/GenericProperty.h
    ../vendor/cross-platform/assimp/include/assimp/SpatialSort.h
    ../vendor/cross-platform/assimp/include/assimp/SkeletonMeshBuilder.h
    ../vendor/cross-platform/assimp/include/assimp/SmallVector.h
    ../vendor/cross-platform/assimp/include/assimp/SmoothingGroups.h
    ../vendor/cross-platform/assimp/include/assimp/SmoothingGroups.inl
    ../vendor/cross-platform/assimp/include/assimp/StandardShapes.h
    ../vendor/cross-platform/assimp/include/assimp/RemoveComments.h
    ../vendor/cross-platform/assimp/include/assimp/Subdivision.h
    ../vendor/cross-platform/assimp/include/assimp/Vertex.h
    ../vendor/cross-platform/assimp/include/assimp/LineSplitter.h
    ../vendor/cross-platform/assimp/include/assimp/TinyFormatter.h
    ../vendor/cross-platform/assimp/include/assimp/Profiler.h
    ../vendor/cross-platform/assimp/include/assimp/LogAux.h
    ../vendor/cross-platform/assimp/include/assimp/Bitmap.h
    ../vendor/cross-platform/assimp/include/assimp/XMLTools.h
    ../vendor/cross-platform/assimp/include/assimp/IOStreamBuffer.h
    ../vendor/cross-platform/assimp/include/assimp/CreateAnimMesh.h
    ../vendor/cross-platform/assimp/include/assimp/MathFunctions.h
    ../vendor/cross-platform/assimp/include/assimp/Exceptional.h
    ../vendor/cross-platform/assimp/include/assimp/ByteSwapper.h
    ../vendor/cross-platform/assimp/include/assimp/Compiler/pushpack1.h
    ../vendor/cross-platform/assimp/include/assimp/Compiler/poppack1.h
    ../vendor/cross-platform/assimp/include/assimp/Compiler/pstdint.h
)

if (WIN32)
SET(glfw3_SRCS
    ../vendor/cross-platform/glfw/src/glfw_config.h
    ../vendor/cross-platform/glfw/include/GLFW/glfw3.h
    ../vendor/cross-platform/glfw/include/GLFW/glfw3native.h
    ../vendor/cross-platform/glfw/src/internal.h
    ../vendor/cross-platform/glfw/src/mappings.h
    ../vendor/cross-platform/glfw/src/context.c
    ../vendor/cross-platform/glfw/src/init.c
    ../vendor/cross-platform/glfw/src/input.c
    ../vendor/cross-platform/glfw/src/monitor.c
    ../vendor/cross-platform/glfw/src/vulkan.c
    ../vendor/cross-platform/glfw/src/window.c
    ../vendor/cross-platform/glfw/src/win32_platform.h
    ../vendor/cross-platform/glfw/src/win32_joystick.h
    ../vendor/cross-platform/glfw/src/wgl_context.h
    ../vendor/cross-platform/glfw/src/egl_context.h
    ../vendor/cross-platform/glfw/src/osmesa_context.h
    ../vendor/cross-platform/glfw/src/win32_init.c
    ../vendor/cross-platform/glfw/src/win32_joystick.c
    ../vendor/cross-platform/glfw/src/win32_monitor.c
    ../vendor/cross-platform/glfw/src/win32_time.c
    ../vendor/cross-platform/glfw/src/win32_thread.c
    ../vendor/cross-platform/glfw/src/win32_window.c
    ../vendor/cross-platform/glfw/src/wgl_context.c
    ../vendor/cross-platform/glfw/src/egl_context.c
    ../vendor/cross-platform/glfw/src/osmesa_context.c
)
elseif (UNIX)
SET(glfw3_SRCS
    ../vendor/cross-platform/glfw/src/glfw_config.h
    ../vendor/cross-platform/glfw/include/GLFW/glfw3.h
    ../vendor/cross-platform/glfw/include/GLFW/glfw3native.h
    ../vendor/cross-platform/glfw/src/internal.h
    ../vendor/cross-platform/glfw/src/mappings.h
    ../vendor/cross-platform/glfw/src/context.c
    ../vendor/cross-platform/glfw/src/init.c
    ../vendor/cross-platform/glfw/src/input.c
    ../vendor/cross-platform/glfw/src/monitor.c
    ../vendor/cross-platform/glfw/src/vulkan.c
    ../vendor/cross-platform/glfw/src/window.c
    ../vendor/cross-platform/glfw/src/win32_platform.h
    ../vendor/cross-platform/glfw/src/win32_joystick.h
    ../vendor/cross-platform/glfw/src/wgl_context.h
    ../vendor/cross-platform/glfw/src/egl_context.h
    ../vendor/cross-platform/glfw/src/osmesa_context.h
    #../vendor/cross-platform/glfw/src/win32_init.c
    #../vendor/cross-platform/glfw/src/win32_joystick.c
    #../vendor/cross-platform/glfw/src/win32_monitor.c
    #../vendor/cross-platform/glfw/src/win32_time.c
    #../vendor/cross-platform/glfw/src/win32_thread.c
    #../vendor/cross-platform/glfw/src/win32_window.c
    #../vendor/cross-platform/glfw/src/wgl_context.c
    ../vendor/cross-platform/glfw/src/egl_context.c
    ../vendor/cross-platform/glfw/src/osmesa_context.c
)
endif()

SET(irrXML_SRCS
    ../vendor/cross-platform/assimp/contrib/irrXML/CXMLReaderImpl.h
    ../vendor/cross-platform/assimp/contrib/irrXML/heapsort.h
    ../vendor/cross-platform/assimp/contrib/irrXML/irrArray.h
    ../vendor/cross-platform/assimp/contrib/irrXML/irrString.h
    ../vendor/cross-platform/assimp/contrib/irrXML/irrTypes.h
    ../vendor/cross-platform/assimp/contrib/irrXML/irrXML.cpp
    ../vendor/cross-platform/assimp/contrib/irrXML/irrXML.h
)

SET(zlibstatic_SRCS
	../vendor/cross-platform/assimp/contrib/zlib/adler32.c
	../vendor/cross-platform/assimp/contrib/zlib/compress.c
	../vendor/cross-platform/assimp/contrib/zlib/crc32.c
	../vendor/cross-platform/assimp/contrib/zlib/deflate.c
	../vendor/cross-platform/assimp/contrib/zlib/gzclose.c
	../vendor/cross-platform/assimp/contrib/zlib/gzlib.c
	../vendor/cross-platform/assimp/contrib/zlib/gzread.c
	../vendor/cross-platform/assimp/contrib/zlib/gzwrite.c
	../vendor/cross-platform/assimp/contrib/zlib/inflate.c
	../vendor/cross-platform/assimp/contrib/zlib/infback.c
	../vendor/cross-platform/assimp/contrib/zlib/inftrees.c
	../vendor/cross-platform/assimp/contrib/zlib/inffast.c
	../vendor/cross-platform/assimp/contrib/zlib/trees.c
	../vendor/cross-platform/assimp/contrib/zlib/uncompr.c
	../vendor/cross-platform/assimp/contrib/zlib/zutil.c
	../vendor/cross-platform/assimp/contrib/zlib/zconf.h
	../vendor/cross-platform/assimp/contrib/zlib/zlib.h
	../vendor/cross-platform/assimp/contrib/zlib/crc32.h
	../vendor/cross-platform/assimp/contrib/zlib/deflate.h
	../vendor/cross-platform/assimp/contrib/zlib/gzguts.h
	../vendor/cross-platform/assimp/contrib/zlib/inffast.h
	../vendor/cross-platform/assimp/contrib/zlib/inffixed.h
	../vendor/cross-platform/assimp/contrib/zlib/inflate.h
	../vendor/cross-platform/assimp/contrib/zlib/inftrees.h
	../vendor/cross-platform/assimp/contrib/zlib/trees.h
	../vendor/cross-platform/assimp/contrib/zlib/zutil.h
)

message("Environment variable 'MORAVA_BUILD_TYPE' = '" $ENV{MORAVA_BUILD_TYPE} "'")

option(USE_MSVC_INCREMENTAL_LINKING "Use MSVC Incremental Linking" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../build) # save executable in root directory

set(CMAKE_GENERATOR_PLATFORM, x64)

unset(CMAKE_IMPORT_LIBRARY_SUFFIX)

add_library(BulletDynamics  STATIC "${BulletDynamics_SRCS}")
add_library(BulletCollision STATIC "${BulletCollision_SRCS}")
add_library(LinearMath      STATIC "${LinearMath_SRCS}")
add_library(assimp          STATIC "${assimp_SRCS}")
add_library(glfw3           STATIC "${glfw3_SRCS}")
add_library(irrXML          STATIC "${irrXML_SRCS}")
add_library(zlibstatic      STATIC "${zlibstatic_SRCS}")

add_dependencies(assimp irrXML)
add_dependencies(assimp zlibstatic)

add_executable("${PROJECT_NAME}" "${SOURCES}")

add_dependencies("${PROJECT_NAME}" BulletDynamics)
add_dependencies("${PROJECT_NAME}" BulletCollision)
add_dependencies("${PROJECT_NAME}" LinearMath)
add_dependencies("${PROJECT_NAME}" assimp)
add_dependencies("${PROJECT_NAME}" glfw3)

set_target_properties("${PROJECT_NAME}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)
set_target_properties(BulletDynamics    PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)
set_target_properties(BulletCollision   PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)
set_target_properties(LinearMath        PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)
set_target_properties(assimp            PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)
set_target_properties(glfw3             PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)

target_compile_definitions(glfw3 PRIVATE _GLFW_USE_CONFIG_H)


target_compile_features("${PROJECT_NAME}" PRIVATE cxx_std_17)
target_compile_features(assimp            PRIVATE cxx_std_17)

target_include_directories(
	zlibstatic PUBLIC
	../vendor/cross-platform/assimp/contrib/zlib
)

target_link_directories(
	zlibstatic PUBLIC
	../vendor/cross-platform/assimp/contrib/zlib/$ENV{MORAVA_BUILD_TYPE}
)

target_include_directories(
	assimp PUBLIC
	../vendor/cross-platform/assimp
	../vendor/cross-platform/assimp/code
	../vendor/cross-platform/assimp/include
	../vendor/cross-platform/assimp/contrib/irrXML
	../vendor/cross-platform/assimp/contrib/zlib
	../vendor/cross-platform/assimp/contrib/unzip
	../vendor/cross-platform/assimp/contrib/openddlparser/include
	../vendor/cross-platform/assimp/contrib/rapidjson/include
)

target_link_directories(
	assimp PUBLIC
	../vendor/cross-platform/assimp/lib/$ENV{MORAVA_BUILD_TYPE} # Win32
	../vendor/cross-platform/assimp/bin                         # Linux
)

target_include_directories(
	glfw3 PUBLIC
	../vendor/cross-platform/glfw/include
	../vendor/cross-platform/glfw/src
)

target_link_directories(
	glfw3 PUBLIC
	../vendor/cross-platform/glfw/src
)

target_include_directories(
	BulletDynamics PUBLIC
	../vendor/cross-platform/bullet3/src
	../vendor/cross-platform/bullet3/examples/ThirdPartyLibs/zlib
)

target_link_directories(
	BulletDynamics PUBLIC
	../vendor/cross-platform/bullet3/lib                # Win32
	../vendor/cross-platform/bullet3/src/BulletDynamics # Linux
)

target_include_directories(
	BulletCollision PUBLIC
	../vendor/cross-platform/bullet3/src
	../vendor/cross-platform/bullet3/examples/ThirdPartyLibs/zlib
)

target_link_directories(
	BulletCollision PUBLIC
	../vendor/cross-platform/bullet3/lib                 # Win32
	../vendor/cross-platform/bullet3/src/BulletCollision # Linux
)

target_include_directories(
	LinearMath PUBLIC
	../vendor/cross-platform/bullet3/src
	../vendor/cross-platform/bullet3/examples/ThirdPartyLibs/zlib
)

target_link_directories(
	LinearMath PUBLIC
	../vendor/cross-platform/bullet3/lib            # Win32
	../vendor/cross-platform/bullet3/src/LinearMath # Linux
)

target_include_directories(
	"${PROJECT_NAME}" PUBLIC
	../vendor/cross-platform/assimp/include
	../vendor/cross-platform/glfw/include
	../vendor/cross-platform/glfw/src
	../vendor/cross-platform/bullet3/src
	../vendor/cross-platform/bullet3/examples/ThirdPartyLibs/zlib
	../vendor/cross-platform/ImGuizmo
	../vendor/GLEW/include
	../vendor/ImGui
	../vendor/GLM
	../vendor/tinyobjloader
	../vendor/spdlog/include
)

target_link_directories(
	"${PROJECT_NAME}" PUBLIC
	../vendor/cross-platform/bullet3/lib                 # Win32
	../vendor/cross-platform/bullet3/src/BulletDynamics  # Linux
	../vendor/cross-platform/bullet3/src/BulletCollision # Linux
	../vendor/cross-platform/bullet3/src/LinearMath      # Linux
	../vendor/cross-platform/glew/build/lib
	../vendor/cross-platform/assimp/lib/$ENV{MORAVA_BUILD_TYPE} # Win32
	../vendor/cross-platform/assimp/bin                         # Linux
	../vendor/cross-platform/glfw/src
	../vendor/GLEW/lib
	../${PROJECT_NAME}/build/$ENV{MORAVA_BUILD_TYPE}
)

if (WIN32)
target_link_libraries(
	"${PROJECT_NAME}"
	opengl32.lib
	glew32.lib
	glfw3.lib
	assimp-vc142-mt.lib
	BulletDynamics.lib
	BulletCollision.lib
	LinearMath.lib
)
elseif (UNIX)
target_link_libraries(
	"${PROJECT_NAME}"
	libGLEW.so # -lGLEW
	libGLU.so  # -lGLU
	libGL.so   # -lGL
	#libglfw3.a
	libassimp.so
	libBulletDynamics.a
	libBulletCollision.a
	libLinearMath.a
)
target_link_libraries("${PROJECT_NAME}" libglfw3.a dl pthread)
endif ()

file(COPY ./assimp-vc142-mt.dll DESTINATION ../build)
file(COPY ./glew32.dll          DESTINATION ../build)
file(COPY ./Models              DESTINATION ../build)
file(COPY ./Shaders             DESTINATION ../build)
file(COPY ./Textures            DESTINATION ../build)

set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}")
