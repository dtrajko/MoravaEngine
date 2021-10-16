cmake_minimum_required(VERSION 3.16)

if(WIN32)
	add_compile_definitions(HZ_PLATFORM_WINDOWS)
elseif(UNIX)
	add_compile_definitions(HZ_PLATFORM_LINUX)
endif()

set(ENV{MORAVA_BUILD_TYPE} Debug)
add_compile_definitions(HZ_DEBUG)

set(MSVC_INCREMENTAL_DEFAULT ON)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

project("MoravaEngine" VERSION 0.0.1)

if(WIN32)
	file(GLOB SOURCES
		./*.cpp
		LearnOpenGL/*.cpp
		SongHo/*.cpp
		../vendor/ImGui/imgui.cpp
		../vendor/ImGui/imgui_demo.cpp
		../vendor/ImGui/imgui_draw.cpp
		../vendor/ImGui/imgui_impl_dx11.cpp
		../vendor/ImGui/imgui_impl_win32.cpp
		../vendor/ImGui/imgui_widgets.cpp
	)
elseif(UNIX)
	file(GLOB SOURCES
		./*.cpp
		./src/Mesh/*.cpp
		./src/SSAO/*.cpp
		./src/Renderer/*.cpp
		./src/Framebuffer/*.cpp
		./src/Misc/*.cpp
		./src/Editor/*.cpp
		./src/SongHo/*.cpp
		./src/Physics/*.cpp
		./src/PostProcessing/*.cpp
		./src/LearnOpenGL/*.cpp
		./src/Water/*.cpp
		./src/Platform/*.cpp
		./src/Platform/Vulkan/*.cpp
		./src/Platform/Windows/*.cpp
		./src/Terrain/*.cpp
		./src/Material/*.cpp
		./src/Skybox/*.cpp
		./src/Texture/*.cpp
		./src/Player/*.cpp
		./src/Deferred/*.cpp
		./src/EnvMap/*.cpp
		./src/Core/*.cpp
		./src/Light/*.cpp
		./src/Camera/*.cpp
		./src/Shader/*.cpp
		./src/Particle/*.cpp
		./src/Scene/*.cpp
		./src/PerlinNoise/*.cpp
		./src/Data/*.cpp
		./src/Hazel/*.cpp
		./src/Hazel/Script/*.cpp
		./src/Hazel/Renderer/*.cpp
		./src/Hazel/Editor/*.cpp
		./src/Hazel/Utils/*.cpp
		./src/Hazel/Physics/*.cpp
		./src/Hazel/Platform/*.cpp
		./src/Hazel/Platform/Vulkan/*.cpp
		./src/Hazel/Platform/OpenGL/*.cpp
		./src/Hazel/Platform/Windows/*.cpp
		./src/Hazel/Core/*.cpp
		./src/Hazel/Core/Math/*.cpp
		./src/Hazel/Events/*.cpp
		./src/Hazel/Hazel-ScriptCore/*.cpp
		./src/Hazel/Hazel-ScriptCore/src/*.cpp
		./src/Hazel/Hazel-ScriptCore/src/Hazel/*.cpp
		./src/Hazel/Hazel-ScriptCore/src/Hazel/Renderer/*.cpp
		./src/Hazel/Hazel-ScriptCore/src/Hazel/Math/*.cpp
		./src/Hazel/Hazel-ScriptCore/src/Hazel/Scene/*.cpp
		./src/Hazel/Hazel-ScriptCore/Properties/*.cpp
		./src/Hazel/Scene/*.cpp
		./src/Hazel/ExampleApp/*.cpp
		./src/Hazel/ExampleApp/src/*.cpp
		./src/Hazel/ExampleApp/Properties/*.cpp
		./src/Hazel/ImGui/*.cpp
		./src/Hazel/Asset/*.cpp
		./src/ImGui/*.cpp

		../vendor/ImGui/imgui.cpp
		../vendor/ImGui/imgui_demo.cpp
		../vendor/ImGui/imgui_draw.cpp
		../vendor/ImGui/imgui_widgets.cpp
	)
endif()

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
	../vendor/cross-platform/assimp/code/Common/BaseImporter.cpp
	../vendor/cross-platform/assimp/code/Common/BaseProcess.cpp
	../vendor/cross-platform/assimp/code/Common/PostStepRegistry.cpp
	../vendor/cross-platform/assimp/code/Common/ImporterRegistry.cpp
	../vendor/cross-platform/assimp/code/Common/DefaultIOStream.cpp
	../vendor/cross-platform/assimp/code/Common/DefaultIOSystem.cpp
	../vendor/cross-platform/assimp/code/Common/ZipArchiveIOSystem.cpp
	../vendor/cross-platform/assimp/code/Common/Importer.cpp
	../vendor/cross-platform/assimp/code/Common/SGSpatialSort.cpp
	../vendor/cross-platform/assimp/code/Common/VertexTriangleAdjacency.cpp
	../vendor/cross-platform/assimp/code/Common/SpatialSort.cpp
	../vendor/cross-platform/assimp/code/Common/SceneCombiner.cpp
	../vendor/cross-platform/assimp/code/Common/ScenePreprocessor.cpp
	../vendor/cross-platform/assimp/code/Common/SkeletonMeshBuilder.cpp
	../vendor/cross-platform/assimp/code/Common/StandardShapes.cpp
	../vendor/cross-platform/assimp/code/Common/TargetAnimation.cpp
	../vendor/cross-platform/assimp/code/Common/RemoveComments.cpp
	../vendor/cross-platform/assimp/code/Common/Subdivision.cpp
	../vendor/cross-platform/assimp/code/Common/scene.cpp
	../vendor/cross-platform/assimp/code/Common/Bitmap.cpp
	../vendor/cross-platform/assimp/code/Common/Version.cpp
	../vendor/cross-platform/assimp/code/Common/CreateAnimMesh.cpp
	../vendor/cross-platform/assimp/code/Common/simd.cpp
	../vendor/cross-platform/assimp/code/Common/material.cpp
	../vendor/cross-platform/assimp/code/Common/AssertHandler.cpp
	../vendor/cross-platform/assimp/code/Common/DefaultLogger.cpp
	../vendor/cross-platform/assimp/code/Common/Exporter.cpp
	../vendor/cross-platform/assimp/code/CApi/AssimpCExport.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/CalcTangentsProcess.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/ComputeUVMappingProcess.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/ConvertToLHProcess.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/EmbedTexturesProcess.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/FindDegenerates.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/FindInstancesProcess.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/FindInvalidDataProcess.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/FixNormalsStep.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/DropFaceNormalsProcess.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/GenFaceNormalsProcess.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/GenVertexNormalsProcess.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/PretransformVertices.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/ImproveCacheLocality.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/JoinVerticesProcess.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/LimitBoneWeightsProcess.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/RemoveRedundantMaterials.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/RemoveVCProcess.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/SortByPTypeProcess.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/SplitLargeMeshes.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/TextureTransform.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/TriangulateProcess.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/ValidateDataStructure.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/OptimizeGraph.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/OptimizeMeshes.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/DeboneProcess.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/ProcessHelper.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/MakeVerboseFormat.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/ScaleProcess.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/ArmaturePopulate.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/GenBoundingBoxesProcess.cpp
	../vendor/cross-platform/assimp/code/PostProcessing/SplitByBoneCountProcess.cpp
	../vendor/cross-platform/assimp/code/Material/MaterialSystem.cpp
	../vendor/cross-platform/assimp/code/AssetLib/STEPParser/STEPFileReader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/STEPParser/STEPFileEncoding.cpp
	../vendor/cross-platform/assimp/code/AssetLib/AMF/AMFImporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/AMF/AMFImporter_Geometry.cpp
	../vendor/cross-platform/assimp/code/AssetLib/AMF/AMFImporter_Material.cpp
	../vendor/cross-platform/assimp/code/AssetLib/AMF/AMFImporter_Postprocess.cpp
	../vendor/cross-platform/assimp/code/AssetLib/3DS/3DSConverter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/3DS/3DSLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/AC/ACLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/ASE/ASELoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/ASE/ASEParser.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Assbin/AssbinLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/B3D/B3DImporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/BVH/BVHLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Collada/ColladaHelper.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Collada/ColladaLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Collada/ColladaParser.cpp
	../vendor/cross-platform/assimp/code/AssetLib/DXF/DXFLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/CSM/CSMLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/HMP/HMPLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Irr/IRRMeshLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Irr/IRRShared.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Irr/IRRLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/LWO/LWOAnimation.cpp
	../vendor/cross-platform/assimp/code/AssetLib/LWO/LWOBLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/LWO/LWOLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/LWO/LWOMaterial.cpp
	../vendor/cross-platform/assimp/code/AssetLib/LWS/LWSLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/M3D/M3DImporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/M3D/M3DWrapper.cpp
	../vendor/cross-platform/assimp/code/AssetLib/MD2/MD2Loader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/MD3/MD3Loader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/MD5/MD5Loader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/MD5/MD5Parser.cpp
	../vendor/cross-platform/assimp/code/AssetLib/MDC/MDCLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/MDL/MDLLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/MDL/MDLMaterialLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/MDL/HalfLife/HL1MDLLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/MDL/HalfLife/UniqueNameGenerator.cpp
	../vendor/cross-platform/assimp/code/AssetLib/NFF/NFFLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/NDO/NDOLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/OFF/OFFLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Obj/ObjFileImporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Obj/ObjFileMtlImporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Obj/ObjFileParser.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Ogre/OgreImporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Ogre/OgreStructs.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Ogre/OgreBinarySerializer.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Ogre/OgreXmlSerializer.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Ogre/OgreMaterial.cpp
	../vendor/cross-platform/assimp/code/AssetLib/OpenGEX/OpenGEXImporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Ply/PlyLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Ply/PlyParser.cpp
	../vendor/cross-platform/assimp/code/AssetLib/MS3D/MS3DLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/COB/COBLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderDNA.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderScene.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderModifier.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderBMesh.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderTessellator.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderCustomData.cpp
	../vendor/cross-platform/assimp/code/AssetLib/IFC/IFCLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/IFC/IFCReaderGen1_2x3.cpp
	../vendor/cross-platform/assimp/code/AssetLib/IFC/IFCReaderGen2_2x3.cpp
	../vendor/cross-platform/assimp/code/AssetLib/IFC/IFCUtil.cpp
	../vendor/cross-platform/assimp/code/AssetLib/IFC/IFCGeometry.cpp
	../vendor/cross-platform/assimp/code/AssetLib/IFC/IFCMaterial.cpp
	../vendor/cross-platform/assimp/code/AssetLib/IFC/IFCProfile.cpp
	../vendor/cross-platform/assimp/code/AssetLib/IFC/IFCCurve.cpp
	../vendor/cross-platform/assimp/code/AssetLib/IFC/IFCBoolean.cpp
	../vendor/cross-platform/assimp/code/AssetLib/IFC/IFCOpenings.cpp
	../vendor/cross-platform/assimp/code/AssetLib/XGL/XGLLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXImporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXParser.cpp
	../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXTokenizer.cpp
	../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXConverter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXUtil.cpp
	../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXDocument.cpp
	../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXProperties.cpp
	../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXMeshGeometry.cpp
	../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXMaterial.cpp
	../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXModel.cpp
	../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXAnimation.cpp
	../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXNodeAttribute.cpp
	../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXDeformer.cpp
	../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXBinaryTokenizer.cpp
	../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXDocumentUtil.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Q3D/Q3DLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Q3BSP/Q3BSPFileParser.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Q3BSP/Q3BSPFileImporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Raw/RawLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/SIB/SIBImporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/SMD/SMDLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/STL/STLLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Terragen/TerragenLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Unreal/UnrealLoader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/X/XFileImporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/X/XFileParser.cpp
	../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Geometry2D.cpp
	../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Geometry3D.cpp
	../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Group.cpp
	../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Light.cpp
	../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Metadata.cpp
	../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Networking.cpp
	../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Postprocess.cpp
	../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Rendering.cpp
	../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Shape.cpp
	../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Texturing.cpp
	../vendor/cross-platform/assimp/code/AssetLib/X3D/FIReader.cpp
	../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DVocabulary.cpp
	../vendor/cross-platform/assimp/code/AssetLib/glTF/glTFCommon.cpp
	../vendor/cross-platform/assimp/code/AssetLib/glTF/glTFImporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/glTF2/glTF2Importer.cpp
	../vendor/cross-platform/assimp/code/AssetLib/3MF/D3MFImporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/3MF/D3MFOpcPackage.cpp
	../vendor/cross-platform/assimp/code/AssetLib/MMD/MMDImporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/MMD/MMDPmxParser.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Obj/ObjExporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/OpenGEX/OpenGEXExporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Ply/PlyExporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/3DS/3DSExporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Assbin/AssbinExporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Assbin/AssbinFileWriter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Assxml/AssxmlExporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Assxml/AssxmlFileWriter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/M3D/M3DExporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Collada/ColladaExporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXExporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXExportNode.cpp
	../vendor/cross-platform/assimp/code/AssetLib/FBX/FBXExportProperty.cpp
	../vendor/cross-platform/assimp/code/AssetLib/STL/STLExporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/X/XFileExporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/X3D/X3DExporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/glTF/glTFExporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/glTF2/glTF2Exporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/3MF/D3MFExporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Assjson/cencode.c
	../vendor/cross-platform/assimp/code/AssetLib/Assjson/json_exporter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Assjson/mesh_splitter.cpp
	../vendor/cross-platform/assimp/code/AssetLib/Step/StepExporter.cpp
	../vendor/cross-platform/assimp/contrib/unzip/ioapi.c
	../vendor/cross-platform/assimp/contrib/unzip/unzip.c
	../vendor/cross-platform/assimp/contrib/poly2tri/poly2tri/common/shapes.cc
	../vendor/cross-platform/assimp/contrib/poly2tri/poly2tri/sweep/advancing_front.cc
	../vendor/cross-platform/assimp/contrib/poly2tri/poly2tri/sweep/cdt.cc
	../vendor/cross-platform/assimp/contrib/poly2tri/poly2tri/sweep/sweep.cc
	../vendor/cross-platform/assimp/contrib/poly2tri/poly2tri/sweep/sweep_context.cc
	../vendor/cross-platform/assimp/contrib/clipper/clipper.cpp
	../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcArithmeticCodec.cpp
	../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcDynamicVectorDecoder.cpp
	../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcDynamicVectorEncoder.cpp
	../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcTools.cpp
	../vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcTriangleFans.cpp
	../vendor/cross-platform/assimp/contrib/zip/src/zip.c
)

if(WIN32)
	SET(glfw3_SRCS	
		../vendor/cross-platform/glfw/src/context.c
		../vendor/cross-platform/glfw/src/init.c
		../vendor/cross-platform/glfw/src/input.c
		../vendor/cross-platform/glfw/src/monitor.c
		../vendor/cross-platform/glfw/src/vulkan.c
		../vendor/cross-platform/glfw/src/window.c
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
elseif(UNIX)
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
	../vendor/cross-platform/assimp/contrib/irrXML/irrXML.cpp
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
)

SET(spirv-cross-core_SRCS
	../vendor/cross-platform/SPIRV-Cross/spirv_cross.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_parser.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_cross_parsed_ir.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_cfg.cpp
)

SET(spirv-cross-cpp_SRCS
	../vendor/cross-platform/SPIRV-Cross/spirv_cross.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_parser.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_cross_parsed_ir.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_cfg.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_glsl.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_cpp.cpp
)

SET(spirv-cross-glsl_SRCS
	../vendor/cross-platform/SPIRV-Cross/spirv_cross.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_parser.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_cross_parsed_ir.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_cfg.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_glsl.cpp
)

SET(spirv-cross-hlsl_SRCS
	../vendor/cross-platform/SPIRV-Cross/spirv_cross.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_parser.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_cross_parsed_ir.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_cfg.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_glsl.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_hlsl.cpp
)

SET(spirv-cross-msl_SRCS
	../vendor/cross-platform/SPIRV-Cross/spirv_cross.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_parser.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_cross_parsed_ir.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_cfg.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_glsl.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_msl.cpp
)

SET(spirv-cross-reflect_SRCS
	../vendor/cross-platform/SPIRV-Cross/spirv_reflect.cpp
)

SET(spirv-cross-util_SRCS
	../vendor/cross-platform/SPIRV-Cross/spirv_cross.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_parser.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_cross_parsed_ir.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_cfg.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_cross_util.cpp
)

SET(SPIRV-Cross_SRCS
	../vendor/cross-platform/SPIRV-Cross/spirv_reflect.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_cross.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_parser.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_cross_parsed_ir.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_cfg.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_glsl.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_cross_util.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_hlsl.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_cpp.cpp
	../vendor/cross-platform/SPIRV-Cross/spirv_msl.cpp
	../vendor/cross-platform/SPIRV-Cross/main.cpp
)

SET(yaml-cpp_SRCS
	../vendor/cross-platform/yaml-cpp/src/binary.cpp
	../vendor/cross-platform/yaml-cpp/src/convert.cpp
	../vendor/cross-platform/yaml-cpp/src/depthguard.cpp
	../vendor/cross-platform/yaml-cpp/src/directives.cpp
	../vendor/cross-platform/yaml-cpp/src/emit.cpp
	../vendor/cross-platform/yaml-cpp/src/emitfromevents.cpp
	../vendor/cross-platform/yaml-cpp/src/emitter.cpp
	../vendor/cross-platform/yaml-cpp/src/emitterstate.cpp
	../vendor/cross-platform/yaml-cpp/src/emitterutils.cpp
	../vendor/cross-platform/yaml-cpp/src/exceptions.cpp
	../vendor/cross-platform/yaml-cpp/src/exp.cpp
	../vendor/cross-platform/yaml-cpp/src/memory.cpp
	../vendor/cross-platform/yaml-cpp/src/node.cpp
	../vendor/cross-platform/yaml-cpp/src/node_data.cpp
	../vendor/cross-platform/yaml-cpp/src/nodebuilder.cpp
	../vendor/cross-platform/yaml-cpp/src/nodeevents.cpp
	../vendor/cross-platform/yaml-cpp/src/null.cpp
	../vendor/cross-platform/yaml-cpp/src/ostream_wrapper.cpp
	../vendor/cross-platform/yaml-cpp/src/parse.cpp
	../vendor/cross-platform/yaml-cpp/src/parser.cpp
	../vendor/cross-platform/yaml-cpp/src/regex_yaml.cpp
	../vendor/cross-platform/yaml-cpp/src/scanner.cpp
	../vendor/cross-platform/yaml-cpp/src/scanscalar.cpp
	../vendor/cross-platform/yaml-cpp/src/scantag.cpp
	../vendor/cross-platform/yaml-cpp/src/scantoken.cpp
	../vendor/cross-platform/yaml-cpp/src/simplekey.cpp
	../vendor/cross-platform/yaml-cpp/src/singledocparser.cpp
	../vendor/cross-platform/yaml-cpp/src/stream.cpp
	../vendor/cross-platform/yaml-cpp/src/tag.cpp
)

SET(box2d_SRCS
	../vendor/box2d/src/collision/b2_broad_phase.cpp
	../vendor/box2d/src/collision/b2_chain_shape.cpp
	../vendor/box2d/src/collision/b2_circle_shape.cpp
	../vendor/box2d/src/collision/b2_collide_circle.cpp
	../vendor/box2d/src/collision/b2_collide_edge.cpp
	../vendor/box2d/src/collision/b2_collide_polygon.cpp
	../vendor/box2d/src/collision/b2_collision.cpp
	../vendor/box2d/src/collision/b2_distance.cpp
	../vendor/box2d/src/collision/b2_dynamic_tree.cpp
	../vendor/box2d/src/collision/b2_edge_shape.cpp
	../vendor/box2d/src/collision/b2_polygon_shape.cpp
	../vendor/box2d/src/collision/b2_time_of_impact.cpp
	../vendor/box2d/src/common/b2_block_allocator.cpp
	../vendor/box2d/src/common/b2_draw.cpp
	../vendor/box2d/src/common/b2_math.cpp
	../vendor/box2d/src/common/b2_settings.cpp
	../vendor/box2d/src/common/b2_stack_allocator.cpp
	../vendor/box2d/src/common/b2_timer.cpp
	../vendor/box2d/src/dynamics/b2_body.cpp
	../vendor/box2d/src/dynamics/b2_chain_circle_contact.cpp
	../vendor/box2d/src/dynamics/b2_chain_circle_contact.h
	../vendor/box2d/src/dynamics/b2_chain_polygon_contact.cpp
	../vendor/box2d/src/dynamics/b2_chain_polygon_contact.h
	../vendor/box2d/src/dynamics/b2_circle_contact.cpp
	../vendor/box2d/src/dynamics/b2_circle_contact.h
	../vendor/box2d/src/dynamics/b2_contact.cpp
	../vendor/box2d/src/dynamics/b2_contact_manager.cpp
	../vendor/box2d/src/dynamics/b2_contact_solver.cpp
	../vendor/box2d/src/dynamics/b2_contact_solver.h
	../vendor/box2d/src/dynamics/b2_distance_joint.cpp
	../vendor/box2d/src/dynamics/b2_edge_circle_contact.cpp
	../vendor/box2d/src/dynamics/b2_edge_circle_contact.h
	../vendor/box2d/src/dynamics/b2_edge_polygon_contact.cpp
	../vendor/box2d/src/dynamics/b2_edge_polygon_contact.h
	../vendor/box2d/src/dynamics/b2_fixture.cpp
	../vendor/box2d/src/dynamics/b2_friction_joint.cpp
	../vendor/box2d/src/dynamics/b2_gear_joint.cpp
	../vendor/box2d/src/dynamics/b2_island.cpp
	../vendor/box2d/src/dynamics/b2_island.h
	../vendor/box2d/src/dynamics/b2_joint.cpp
	../vendor/box2d/src/dynamics/b2_motor_joint.cpp
	../vendor/box2d/src/dynamics/b2_mouse_joint.cpp
	../vendor/box2d/src/dynamics/b2_polygon_circle_contact.cpp
	../vendor/box2d/src/dynamics/b2_polygon_circle_contact.h
	../vendor/box2d/src/dynamics/b2_polygon_contact.cpp
	../vendor/box2d/src/dynamics/b2_polygon_contact.h
	../vendor/box2d/src/dynamics/b2_prismatic_joint.cpp
	../vendor/box2d/src/dynamics/b2_pulley_joint.cpp
	../vendor/box2d/src/dynamics/b2_revolute_joint.cpp
	../vendor/box2d/src/dynamics/b2_weld_joint.cpp
	../vendor/box2d/src/dynamics/b2_wheel_joint.cpp
	../vendor/box2d/src/dynamics/b2_world.cpp
	../vendor/box2d/src/dynamics/b2_world_callbacks.cpp
	../vendor/box2d/src/rope/b2_rope.cpp
)

SET(shaderc_SRCS
	../vendor/cross-platform/shaderc/libshaderc/src/shaderc.cc
	../vendor/cross-platform/shaderc/libshaderc/src/shaderc_cpp_test.cc
	../vendor/cross-platform/shaderc/libshaderc/src/shaderc_private_test.cc
	../vendor/cross-platform/shaderc/libshaderc/src/shaderc_test.cc

	../vendor/cross-platform/shaderc/libshaderc_util/src/args.cc
	../vendor/cross-platform/shaderc/libshaderc_util/src/compiler.cc
	../vendor/cross-platform/shaderc/libshaderc_util/src/compiler_test.cc
	../vendor/cross-platform/shaderc/libshaderc_util/src/counting_includer_test.cc
	../vendor/cross-platform/shaderc/libshaderc_util/src/file_finder.cc
	../vendor/cross-platform/shaderc/libshaderc_util/src/file_finder_test.cc
	../vendor/cross-platform/shaderc/libshaderc_util/src/format_test.cc
	# ../vendor/cross-platform/shaderc/libshaderc_util/src/io.cc
	# ../vendor/cross-platform/shaderc/libshaderc_util/src/io_test.cc
	../vendor/cross-platform/shaderc/libshaderc_util/src/message.cc
	../vendor/cross-platform/shaderc/libshaderc_util/src/message_test.cc
	../vendor/cross-platform/shaderc/libshaderc_util/src/mutex_test.cc
	../vendor/cross-platform/shaderc/libshaderc_util/src/resources.cc
	../vendor/cross-platform/shaderc/libshaderc_util/src/shader_stage.cc
	../vendor/cross-platform/shaderc/libshaderc_util/src/spirv_tools_wrapper.cc
	../vendor/cross-platform/shaderc/libshaderc_util/src/string_piece_test.cc
	../vendor/cross-platform/shaderc/libshaderc_util/src/version_profile.cc
	../vendor/cross-platform/shaderc/libshaderc_util/src/version_profile_test.cc
)

SET(DirectXTex_SRCS
	../vendor/DirectXTex/DirectXTex/BC.cpp
	../vendor/DirectXTex/DirectXTex/BC4BC5.cpp
	../vendor/DirectXTex/DirectXTex/BC6HBC7.cpp
	../vendor/DirectXTex/DirectXTex/BCDirectCompute.cpp
	../vendor/DirectXTex/DirectXTex/DirectXTexCompress.cpp
	../vendor/DirectXTex/DirectXTex/DirectXTexCompressGPU.cpp
	../vendor/DirectXTex/DirectXTex/DirectXTexConvert.cpp
	../vendor/DirectXTex/DirectXTex/DirectXTexD3D11.cpp
	../vendor/DirectXTex/DirectXTex/DirectXTexD3D12.cpp
	../vendor/DirectXTex/DirectXTex/DirectXTexDDS.cpp
	../vendor/DirectXTex/DirectXTex/DirectXTexFlipRotate.cpp
	../vendor/DirectXTex/DirectXTex/DirectXTexHDR.cpp
	../vendor/DirectXTex/DirectXTex/DirectXTexImage.cpp
	../vendor/DirectXTex/DirectXTex/DirectXTexMipmaps.cpp
	../vendor/DirectXTex/DirectXTex/DirectXTexMisc.cpp
	../vendor/DirectXTex/DirectXTex/DirectXTexNormalMaps.cpp
	../vendor/DirectXTex/DirectXTex/DirectXTexPMAlpha.cpp
	../vendor/DirectXTex/DirectXTex/DirectXTexResize.cpp
	../vendor/DirectXTex/DirectXTex/DirectXTexTGA.cpp
	../vendor/DirectXTex/DirectXTex/DirectXTexUtil.cpp
	../vendor/DirectXTex/DirectXTex/DirectXTexWIC.cpp
)

message("Environment variable 'MORAVA_BUILD_TYPE' = '" $ENV{MORAVA_BUILD_TYPE} "'")

option(USE_MSVC_INCREMENTAL_LINKING "Use MSVC Incremental Linking" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../build) # save executable in root directory

set(CMAKE_GENERATOR_PLATFORM, x64)

unset(CMAKE_IMPORT_LIBRARY_SUFFIX)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

add_library(BulletDynamics		STATIC "${BulletDynamics_SRCS}")
add_library(BulletCollision		STATIC "${BulletCollision_SRCS}")
add_library(LinearMath			STATIC "${LinearMath_SRCS}")

add_library(assimp				STATIC "${assimp_SRCS}")
add_library(glfw3				STATIC "${glfw3_SRCS}")
add_library(irrXML				STATIC "${irrXML_SRCS}")
add_library(zlibstatic			STATIC "${zlibstatic_SRCS}")

add_library(spirv-cross-core	STATIC "${spirv-cross-core_SRCS}")
add_library(spirv-cross-cpp		STATIC "${spirv-cross-cpp_SRCS}")
add_library(spirv-cross-glsl	STATIC "${spirv-cross-glsl_SRCS}")
add_library(spirv-cross-hlsl	STATIC "${spirv-cross-hlsl_SRCS}")
add_library(spirv-cross-msl		STATIC "${spirv-cross-msl_SRCS}")
add_library(spirv-cross-reflect	STATIC "${spirv-cross-reflect_SRCS}")
add_library(spirv-cross-util	STATIC "${spirv-cross-util_SRCS}")

add_library(SPIRV-Cross			STATIC "${SPIRV-Cross_SRCS}")
add_library(yaml-cpp			STATIC "${yaml-cpp_SRCS}")
add_library(box2d				STATIC "${box2d_SRCS}")

add_library(shaderc				STATIC "${shaderc_SRCS}")

add_library(DirectXTex          STATIC "${DirectXTex_SRCS}")

#	add_library(Hazel-ScriptCore	SHARED "${Hazel-ScriptCore_SRCS}")
#	add_library(ExampleApp			SHARED "${ExampleApp_SRCS}")

#	set_property(TARGET Hazel-ScriptCore PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.6.1")
#	set_property(TARGET ExampleApp PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.6.1")
#	
#	set(CMAKE_CSharp_FLAGS "/langversion:6")
#	
#	set_property(TARGET Hazel-ScriptCore PROPERTY VS_DOTNET_REFERENCES
#		"Microsoft.CSharp"
#		"System"
#		"System.Core"
#		"System.Data"
#		"System.Deployment"
#		"System.Drawing"
#		"System.Net.Http"
#		"System.Xml"
#	)
#	
#	set_property(TARGET ExampleApp PROPERTY VS_DOTNET_REFERENCES
#		"Microsoft.CSharp"
#		"System"
#		"System.Core"
#		"System.Data"
#		"System.Deployment"
#		"System.Drawing"
#		"System.Net.Http"
#		"System.Xml"
#	)

add_dependencies(assimp irrXML)
add_dependencies(assimp zlibstatic)

add_dependencies(SPIRV-Cross spirv-cross-core)
add_dependencies(SPIRV-Cross spirv-cross-cpp)
add_dependencies(SPIRV-Cross spirv-cross-glsl)
add_dependencies(SPIRV-Cross spirv-cross-hlsl)
add_dependencies(SPIRV-Cross spirv-cross-msl)
add_dependencies(SPIRV-Cross spirv-cross-reflect)
add_dependencies(SPIRV-Cross spirv-cross-util)

add_executable("${PROJECT_NAME}" "${SOURCES}")

add_dependencies("${PROJECT_NAME}" BulletDynamics)
add_dependencies("${PROJECT_NAME}" BulletCollision)
add_dependencies("${PROJECT_NAME}" LinearMath)
add_dependencies("${PROJECT_NAME}" assimp)
add_dependencies("${PROJECT_NAME}" glfw3)
add_dependencies("${PROJECT_NAME}" SPIRV-Cross)
add_dependencies("${PROJECT_NAME}" yaml-cpp)
add_dependencies("${PROJECT_NAME}" box2d)
add_dependencies("${PROJECT_NAME}" DirectXTex)
# add_dependencies("${PROJECT_NAME}" shaderc)
# add_dependencies("${PROJECT_NAME}" Hazel-ScriptCore)
# add_dependencies("${PROJECT_NAME}" ExampleApp)

set_target_properties("${PROJECT_NAME}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)
set_target_properties(BulletDynamics	PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)
set_target_properties(BulletCollision   PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)
set_target_properties(LinearMath		PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)
set_target_properties(assimp			PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)
set_target_properties(glfw3				PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ../build RUNTIME_OUTPUT_DIRECTORY_RELEASE ../build)

target_compile_definitions(glfw3 PRIVATE _GLFW_USE_CONFIG_H)

target_compile_features("${PROJECT_NAME}"	PRIVATE cxx_std_17)
target_compile_features(assimp				PRIVATE cxx_std_17)

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

target_link_directories(
	SPIRV-Cross PUBLIC
	../vendor/cross-platform/SPIRV-Cross
)

target_include_directories(
	yaml-cpp PUBLIC
	../vendor/cross-platform/yaml-cpp/include
	../vendor/cross-platform/yaml-cpp/src
)

target_include_directories(
	box2d PUBLIC
	../vendor/box2d/include
	../vendor/box2d/src
)

target_include_directories(
	shaderc PUBLIC
	../vendor/cross-platform/shaderc/libshaderc/include
	../vendor/cross-platform/shaderc/libshaderc_util/include
)

target_link_directories(
	shaderc PUBLIC
	../vendor/cross-platform/shaderc/libshaderc/$ENV{MORAVA_BUILD_TYPE}
	../vendor/cross-platform/shaderc/libshaderc_util/$ENV{MORAVA_BUILD_TYPE}
)

#	target_include_directories(
#		Hazel-ScriptCore PUBLIC
#		../MoravaEngine/Hazel/ScriptCore
#	)
#	
#	target_include_directories(
#		ExampleApp PUBLIC
#		../MoravaEngine/Hazel/ExampleApp
#	)
#	
#	target_link_directories(
#		Hazel-ScriptCore PUBLIC
#		../MoravaEngine/Hazel/ScriptCore
#	)
#	
#	target_link_directories(
#		ExampleApp PUBLIC
#		../MoravaEngine/Hazel/ExampleApp
#	)

# -- BEGIN Main project

target_include_directories(
	"${PROJECT_NAME}" PUBLIC
	./src
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
    ../vendor/entt/include
	../vendor/cross-platform/yaml-cpp/include
	../vendor/mono/include
	../vendor/box2d/include
	../vendor/cross-platform/shaderc/libshaderc/include
	../vendor/cross-platform/shaderc/libshaderc_util/include
	../vendor/cross-platform/SPIRV-Cross
	../vendor/Vulkan/1.2.148.1/Include
	../vendor/DirectXTex/DirectXTex
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
	../vendor/cross-platform/yaml-cpp
	../vendor/box2d/bin/$ENV{MORAVA_BUILD_TYPE} # Win32
	../vendor/box2d/build/bin # Linux
	../vendor/cross-platform/shaderc/libshaderc/$ENV{MORAVA_BUILD_TYPE}      # Win32
	../vendor/cross-platform/shaderc/libshaderc_util/$ENV{MORAVA_BUILD_TYPE} # Win32
	../vendor/cross-platform/shaderc/libshaderc      # Linux
	../vendor/cross-platform/shaderc/libshaderc_util # Linux
	../vendor/cross-platform/SPIRV-Cross/$ENV{MORAVA_BUILD_TYPE}
	../vendor/Vulkan/1.2.148.1/Lib
	../vendor/DirectXTex/build/bin/CMake/$ENV{MORAVA_BUILD_TYPE}
	../${PROJECT_NAME}/build/$ENV{MORAVA_BUILD_TYPE}
)

if(WIN32)
	target_link_libraries(
		"${PROJECT_NAME}"
		opengl32.lib
		glew32.lib
		glfw3.lib
		assimp-vc142-mt.lib
		BulletDynamics.lib
		BulletCollision.lib
		LinearMath.lib
	if ($ENV{MORAVA_BUILD_TYPE} == 'Debug')
		spirv-cross-cored.lib		# Debug
		spirv-cross-cppd.lib		# Debug
		spirv-cross-glsld.lib		# Debug
		spirv-cross-hlsld.lib		# Debug
		spirv-cross-msld.lib		# Debug
		spirv-cross-reflectd.lib	# Debug
		spirv-cross-utild.lib		# Debug
	elseif($ENV{MORAVA_BUILD_TYPE} == 'Release')
		spirv-cross-core.lib		# Release
		spirv-cross-cpp.lib			# Release
		spirv-cross-glsl.lib		# Release
		spirv-cross-hlsl.lib		# Release
		spirv-cross-msl.lib			# Release
		spirv-cross-reflect.lib		# Release
		spirv-cross-util.lib		# Release
	endif()
	#	yaml-cppd.lib
		yaml-cpp.lib
		mono-2.0-sgen.lib
		box2d.lib
		shaderc_combined.lib
		vulkan-1.lib
		d3d11.lib
		d3dcompiler.lib
		DirectXTex.lib
	)
elseif(UNIX)
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
		libbox2d.a
		libshaderc.a
		libshaderc_util.a
	)
	target_link_libraries("${PROJECT_NAME}" libglfw3.a dl pthread)
endif()

file(COPY ./assimp-vc142-mt.dll	DESTINATION ../build)
file(COPY ./glew32.dll			DESTINATION ../build)
file(COPY ./Models				DESTINATION ../build)
file(COPY ./Shaders				DESTINATION ../build)
file(COPY ./Textures			DESTINATION ../build)

set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}")


if(WIN32)

	project(Sandbox LANGUAGES CSharp)

	# Include CMake utilities for CSharp, for WinForm and WPF application support.
	include(CSharpUtilities)

	SET(ExampleApp_SRCS
		../MoravaEngine/src/Hazel/ExampleApp/src/BasicController.cs
		../MoravaEngine/src/Hazel/ExampleApp/src/MapGenerator.cs
		../MoravaEngine/src/Hazel/ExampleApp/src/Noise.cs
		../MoravaEngine/src/Hazel/ExampleApp/src/PlayerCube.cs
		../MoravaEngine/src/Hazel/ExampleApp/src/RandomColor.cs
		../MoravaEngine/src/Hazel/ExampleApp/src/Script.cs
		../MoravaEngine/src/Hazel/ExampleApp/src/Sink.cs
	)

	SET(Hazel-ScriptCore_SRCS
		../MoravaEngine/src/Hazel/Hazel-ScriptCore/src/Hazel/Math/Matrix4.cs
		../MoravaEngine/src/Hazel/Hazel-ScriptCore/src/Hazel/Math/Noise.cs
		../MoravaEngine/src/Hazel/Hazel-ScriptCore/src/Hazel/Math/Vector2.cs
		../MoravaEngine/src/Hazel/Hazel-ScriptCore/src/Hazel/Math/Vector3.cs
		../MoravaEngine/src/Hazel/Hazel-ScriptCore/src/Hazel/Math/Vector4.cs
		../MoravaEngine/src/Hazel/Hazel-ScriptCore/src/Hazel/Renderer/Color.cs
		../MoravaEngine/src/Hazel/Hazel-ScriptCore/src/Hazel/Renderer/Material.cs
		../MoravaEngine/src/Hazel/Hazel-ScriptCore/src/Hazel/Renderer/Mesh.cs
		../MoravaEngine/src/Hazel/Hazel-ScriptCore/src/Hazel/Renderer/MeshFactory.cs
		../MoravaEngine/src/Hazel/Hazel-ScriptCore/src/Hazel/Renderer/Texture2D.cs
		../MoravaEngine/src/Hazel/Hazel-ScriptCore/src/Hazel/Scene/Component.cs
		../MoravaEngine/src/Hazel/Hazel-ScriptCore/src/Hazel/Entity.cs
		../MoravaEngine/src/Hazel/Hazel-ScriptCore/src/Hazel/Input.cs
		../MoravaEngine/src/Hazel/Hazel-ScriptCore/src/Hazel/KeyCodes.cs
		../MoravaEngine/src/Hazel/Hazel-ScriptCore/src/Hazel/SpriteRenderer.cs
	)

	add_library(ExampleApp			SHARED "${ExampleApp_SRCS}")
	add_library(Hazel-ScriptCore	SHARED "${Hazel-ScriptCore_SRCS}")

endif()
