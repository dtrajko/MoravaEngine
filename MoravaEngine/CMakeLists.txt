cmake_minimum_required(VERSION 3.8)

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
file(GLOB_RECURSE SOURCES ./src/*.cpp)
#	source_group(src FILES ${SOURCES})
source_group(TREE ../src PREFIX src FILES ${SOURCES})

file(GLOB_RECURSE BulletDynamics_SRCS		../vendor/cross-platform/bullet3/src/BulletDynamics/*.cpp)
file(GLOB_RECURSE BulletCollision_SRCS		../vendor/cross-platform/bullet3/src/BulletCollision/*.cpp)
file(GLOB_RECURSE LinearMath_SRCS			../vendor/cross-platform/bullet3/src/LinearMath/*.cpp)
#	file(GLOB_RECURSE assimp_SRCS
#		../vendor/cross-platform/assimp/*.cpp
#		../vendor/cross-platform/assimp/*.c
#		../vendor/cross-platform/assimp/*.cc
#	)
file(GLOB_RECURSE yaml-cpp_SRCS				../vendor/cross-platform/yaml-cpp/src/*.cpp)
file(GLOB_RECURSE box2d_SRCS
	../vendor/box2d/src/*.cpp
	../vendor/box2d/src/*.h
)
file(GLOB_RECURSE DirectXTex_SRCS			../vendor/DirectXTex/DirectXTex/*.cpp)
file(GLOB_RECURSE zlibstatic_SRCS			../vendor/cross-platform/assimp/contrib/zlib/*.c)
file(GLOB_RECURSE SPIRV-Cross_SRCS			../vendor/cross-platform/SPIRV-Cross/*.cpp)
file(GLOB_RECURSE irrXML_SRCS				../vendor/cross-platform/assimp/contrib/irrXML/*.cpp)
#	file(GLOB_RECURSE spirv-cross-core_SRCS		../vendor/cross-platform/SPIRV-Cross/*.cpp)
#	file(GLOB_RECURSE spirv-cross-cpp_SRCS		../vendor/cross-platform/SPIRV-Cross/*.cpp)
#	file(GLOB_RECURSE spirv-cross-glsl_SRCS		../vendor/cross-platform/SPIRV-Cross/*.cpp)
#	file(GLOB_RECURSE spirv-cross-hlsl_SRCS		../vendor/cross-platform/SPIRV-Cross/*.cpp)
#	file(GLOB_RECURSE spirv-cross-msl_SRCS		../vendor/cross-platform/SPIRV-Cross/*.cpp)
#	file(GLOB_RECURSE spirv-cross-reflect_SRCS	../vendor/cross-platform/SPIRV-Cross/*.cpp)
#	file(GLOB_RECURSE spirv-cross-util_SRCS		../vendor/cross-platform/SPIRV-Cross/*.cpp)

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

#	add_library(shaderc				STATIC "${shaderc_SRCS}")

#	add_library(DirectXTex          STATIC "${DirectXTex_SRCS}")

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


if(WIN32)

set_property(TARGET assimp					PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")			
set_property(TARGET box2d					PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET BulletCollision			PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET BulletDynamics			PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET LinearMath				PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET glfw3					PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET irrXML					PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
#set_property(TARGET shaderc					PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET SPIRV-Cross				PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET spirv-cross-core		PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET spirv-cross-cpp			PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET spirv-cross-glsl		PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET spirv-cross-hlsl		PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET spirv-cross-msl			PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET spirv-cross-reflect		PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET spirv-cross-util		PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET yaml-cpp				PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET zlibstatic				PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

endif()

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
#	add_dependencies("${PROJECT_NAME}" DirectXTex)
#	add_dependencies("${PROJECT_NAME}" shaderc)
#	add_dependencies("${PROJECT_NAME}" Hazel-ScriptCore)
#	add_dependencies("${PROJECT_NAME}" ExampleApp)

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

#	target_include_directories(
#		shaderc PUBLIC
#		../vendor/cross-platform/shaderc/libshaderc/include
#		../vendor/cross-platform/shaderc/libshaderc_util/include
#	)

#	target_link_directories(
#		shaderc PUBLIC
#		../vendor/cross-platform/shaderc/libshaderc/$ENV{MORAVA_BUILD_TYPE}
#		../vendor/cross-platform/shaderc/libshaderc_util/$ENV{MORAVA_BUILD_TYPE}
#	)

#	target_include_directories(
#		Hazel-ScriptCore PUBLIC
#		../MoravaEngine/src/H2M/Hazel-ScriptCore
#	)
#	
#	target_include_directories(
#		ExampleApp PUBLIC
#		../MoravaEngine/src/H2M/ExampleApp
#	)
#	
#	target_link_directories(
#		Hazel-ScriptCore PUBLIC
#		../MoravaEngine/src/H2M/Hazel-ScriptCore
#	)
#	
#	target_link_directories(
#		ExampleApp PUBLIC
#		../MoravaEngine/src/H2M/ExampleApp
#	)

# -- BEGIN Main project

target_include_directories(
	"${PROJECT_NAME}" PUBLIC
	./src
	../vendor
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
	../vendor/Optick/src
	../vendor/miniaudio/include
	../vendor/farbot/include
	../vendor/PhysX/include
	../vendor/PhysX/include/PhysX
	../vendor/NvidiaAftermath/include
)

target_link_directories(
	"${PROJECT_NAME}" PUBLIC
	../vendor/cross-platform/bullet3/lib                 # Win32
	../vendor/cross-platform/bullet3/src/BulletDynamics  # Linux
	../vendor/cross-platform/bullet3/src/BulletCollision # Linux
	../vendor/cross-platform/bullet3/src/LinearMath      # Linux
	# ../vendor/cross-platform/glew/build/lib
	../vendor/GLEW/lib
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
	#../vendor/DirectXTex/build/bin/CMake/$ENV{MORAVA_BUILD_TYPE}
	../vendor/DirectXTex/bin/CMake/$ENV{MORAVA_BUILD_TYPE}
	../vendor/mono/lib/$ENV{MORAVA_BUILD_TYPE}
	../${PROJECT_NAME}/build/$ENV{MORAVA_BUILD_TYPE}
)

if(WIN32)
	target_link_libraries(
		"${PROJECT_NAME}"
		opengl32.lib
		glew32.lib
		glfw3.lib
		#assimp-vc142-mt.lib
		assimp.lib
		BulletDynamics.lib
		BulletCollision.lib
		LinearMath.lib
	#	yaml-cppd.lib
		yaml-cpp.lib
		mono-2.0-sgen.lib
		box2d.lib
		shaderc_combined.lib
		vulkan-1.lib
		d3d11.lib
		d3dcompiler.lib
		DirectXTex.lib
		spirv-cross-core.lib
		spirv-cross-cpp.lib
		spirv-cross-glsl.lib
		spirv-cross-hlsl.lib
		spirv-cross-msl.lib
		spirv-cross-reflect.lib
		spirv-cross-util.lib
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

#	if ($ENV{MORAVA_BUILD_TYPE} == 'Debug')
#		target_link_libraries(
#			"${PROJECT_NAME}"
#			spirv-cross-cored.lib		# Debug
#			spirv-cross-cppd.lib		# Debug
#			spirv-cross-glsld.lib		# Debug
#			spirv-cross-hlsld.lib		# Debug
#			spirv-cross-msld.lib		# Debug
#			spirv-cross-reflectd.lib	# Debug
#			spirv-cross-utild.lib		# Debug
#		)
#	elseif($ENV{MORAVA_BUILD_TYPE} == 'Release')
#		target_link_libraries(
#			"${PROJECT_NAME}"
#			spirv-cross-core.lib		# Release
#			spirv-cross-cpp.lib			# Release
#			spirv-cross-glsl.lib		# Release
#			spirv-cross-hlsl.lib		# Release
#			spirv-cross-msl.lib			# Release
#			spirv-cross-reflect.lib		# Release
#			spirv-cross-util.lib		# Release
#		)
#	endif()

#	file(COPY ./assimp-vc142-mt.dll		DESTINATION ../build)
#	file(COPY ./glew32.dll				DESTINATION ../build)
#	file(COPY ./Models					DESTINATION ../build)
#	file(COPY ./Shaders					DESTINATION ../build)
#	file(COPY ./Textures				DESTINATION ../build)


# Build assimp with CMake
execute_process(COMMAND cmake . WORKING_DIRECTORY ../vendor/cross-platform/assimp)

# Build GLFW with CMake
execute_process(COMMAND cmake . WORKING_DIRECTORY ../vendor/cross-platform/glfw)

# Build shaderc - use python to run git-sync-deps
execute_process(COMMAND python git-sync-deps WORKING_DIRECTORY ../vendor/cross-platform/shaderc/utils)
# Build shaderc with CMake
execute_process(COMMAND cmake . WORKING_DIRECTORY ../vendor/cross-platform/shaderc)

# Build DirectXTex
execute_process(COMMAND cmake . WORKING_DIRECTORY ../vendor/DirectXTex)

# Build yaml-cpp
execute_process(COMMAND cmake . WORKING_DIRECTORY ../vendor/cross-platform/yaml-cpp)

# Delete directory bullet3/examples/ThirdPartyLibs to avoid ImGui conflicts in compilation
file(REMOVE_RECURSE ../vendor/cross-platform/bullet3/examples/ThirdPartyLibs)


set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}")

set_property(TARGET "${PROJECT_NAME}" PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "..")

if(WIN32)

set_property(TARGET	"${PROJECT_NAME}"		PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

endif()
