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


get_filename_component(ROOT_PATH_ABS ".." ABSOLUTE)
message("ROOT_PATH_ABS: ${ROOT_PATH_ABS}")


SET(assimp_SRCS
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/Assimp.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/CApi/CInterfaceIOWrapper.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/BaseImporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/BaseProcess.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/PostStepRegistry.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/ImporterRegistry.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/DefaultIOStream.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/DefaultIOSystem.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/ZipArchiveIOSystem.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/Importer.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/SGSpatialSort.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/VertexTriangleAdjacency.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/SpatialSort.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/SceneCombiner.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/ScenePreprocessor.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/SkeletonMeshBuilder.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/StandardShapes.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/TargetAnimation.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/RemoveComments.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/Subdivision.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/scene.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/Bitmap.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/Version.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/CreateAnimMesh.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/simd.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/material.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/AssertHandler.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/DefaultLogger.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Common/Exporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/CApi/AssimpCExport.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/CalcTangentsProcess.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/ComputeUVMappingProcess.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/ConvertToLHProcess.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/EmbedTexturesProcess.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/FindDegenerates.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/FindInstancesProcess.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/FindInvalidDataProcess.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/FixNormalsStep.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/DropFaceNormalsProcess.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/GenFaceNormalsProcess.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/GenVertexNormalsProcess.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/PretransformVertices.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/ImproveCacheLocality.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/JoinVerticesProcess.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/LimitBoneWeightsProcess.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/RemoveRedundantMaterials.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/RemoveVCProcess.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/SortByPTypeProcess.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/SplitLargeMeshes.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/TextureTransform.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/TriangulateProcess.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/ValidateDataStructure.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/OptimizeGraph.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/OptimizeMeshes.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/DeboneProcess.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/ProcessHelper.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/MakeVerboseFormat.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/ScaleProcess.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/ArmaturePopulate.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/GenBoundingBoxesProcess.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/PostProcessing/SplitByBoneCountProcess.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/Material/MaterialSystem.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/STEPParser/STEPFileReader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/STEPParser/STEPFileEncoding.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/AMF/AMFImporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/AMF/AMFImporter_Geometry.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/AMF/AMFImporter_Material.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/AMF/AMFImporter_Postprocess.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/3DS/3DSConverter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/3DS/3DSLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/AC/ACLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/ASE/ASELoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/ASE/ASEParser.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Assbin/AssbinLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/B3D/B3DImporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/BVH/BVHLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Collada/ColladaHelper.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Collada/ColladaLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Collada/ColladaParser.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/DXF/DXFLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/CSM/CSMLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/HMP/HMPLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Irr/IRRMeshLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Irr/IRRShared.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Irr/IRRLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/LWO/LWOAnimation.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/LWO/LWOBLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/LWO/LWOLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/LWO/LWOMaterial.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/LWS/LWSLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/M3D/M3DImporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/M3D/M3DWrapper.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/MD2/MD2Loader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/MD3/MD3Loader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/MD5/MD5Loader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/MD5/MD5Parser.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/MDC/MDCLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/MDL/MDLLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/MDL/MDLMaterialLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/MDL/HalfLife/HL1MDLLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/MDL/HalfLife/UniqueNameGenerator.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/NFF/NFFLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/NDO/NDOLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/OFF/OFFLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Obj/ObjFileImporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Obj/ObjFileMtlImporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Obj/ObjFileParser.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Ogre/OgreImporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Ogre/OgreStructs.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Ogre/OgreBinarySerializer.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Ogre/OgreXmlSerializer.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Ogre/OgreMaterial.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/OpenGEX/OpenGEXImporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Ply/PlyLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Ply/PlyParser.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/MS3D/MS3DLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/COB/COBLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderDNA.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderScene.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderModifier.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderBMesh.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderTessellator.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Blender/BlenderCustomData.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/IFC/IFCLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/IFC/IFCReaderGen1_2x3.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/IFC/IFCReaderGen2_2x3.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/IFC/IFCUtil.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/IFC/IFCGeometry.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/IFC/IFCMaterial.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/IFC/IFCProfile.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/IFC/IFCCurve.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/IFC/IFCBoolean.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/IFC/IFCOpenings.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/XGL/XGLLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/FBX/FBXImporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/FBX/FBXParser.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/FBX/FBXTokenizer.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/FBX/FBXConverter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/FBX/FBXUtil.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/FBX/FBXDocument.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/FBX/FBXProperties.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/FBX/FBXMeshGeometry.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/FBX/FBXMaterial.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/FBX/FBXModel.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/FBX/FBXAnimation.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/FBX/FBXNodeAttribute.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/FBX/FBXDeformer.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/FBX/FBXBinaryTokenizer.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/FBX/FBXDocumentUtil.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Q3D/Q3DLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Q3BSP/Q3BSPFileParser.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Q3BSP/Q3BSPFileImporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Raw/RawLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/SIB/SIBImporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/SMD/SMDLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/STL/STLLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Terragen/TerragenLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Unreal/UnrealLoader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/X/XFileImporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/X/XFileParser.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Geometry2D.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Geometry3D.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Group.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Light.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Metadata.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Networking.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Postprocess.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Rendering.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Shape.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/X3D/X3DImporter_Texturing.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/X3D/FIReader.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/X3D/X3DVocabulary.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/glTF/glTFCommon.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/glTF/glTFImporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/glTF2/glTF2Importer.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/3MF/D3MFImporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/3MF/D3MFOpcPackage.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/MMD/MMDImporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/MMD/MMDPmxParser.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Obj/ObjExporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/OpenGEX/OpenGEXExporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Ply/PlyExporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/3DS/3DSExporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Assbin/AssbinExporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Assbin/AssbinFileWriter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Assxml/AssxmlExporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Assxml/AssxmlFileWriter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/M3D/M3DExporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Collada/ColladaExporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/FBX/FBXExporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/FBX/FBXExportNode.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/FBX/FBXExportProperty.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/STL/STLExporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/X/XFileExporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/X3D/X3DExporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/glTF/glTFExporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/glTF2/glTF2Exporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/3MF/D3MFExporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Assjson/cencode.c
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Assjson/json_exporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Assjson/mesh_splitter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code/AssetLib/Step/StepExporter.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/unzip/ioapi.c
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/unzip/unzip.c
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/poly2tri/poly2tri/common/shapes.cc
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/poly2tri/poly2tri/sweep/advancing_front.cc
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/poly2tri/poly2tri/sweep/cdt.cc
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/poly2tri/poly2tri/sweep/sweep.cc
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/poly2tri/poly2tri/sweep/sweep_context.cc
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/clipper/clipper.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcArithmeticCodec.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcDynamicVectorDecoder.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcDynamicVectorEncoder.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcTools.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/Open3DGC/o3dgcTriangleFans.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/zip/src/zip.c
)


file(GLOB_RECURSE SOURCES ./src/*.cpp ./src/*.h)
source_group(TREE ../src PREFIX src FILES ${SOURCES})

# src/Resources
file(GLOB_RECURSE RESOURCES ./Resources/Shaders/*.glsl)
source_group(TREE ../Resources PREFIX src/Resources FILES ${RESOURCES})
set_property(SOURCE ${RESOURCES} PROPERTY VS_SETTINGS "ExcludedFromBuild=true")

# src/Vendors/ImGui ../vendor/ImGui
file(GLOB_RECURSE SOURCES_IMGUI ../vendor/ImGui/*.cpp)
source_group(TREE ../../vendor/ImGui PREFIX src/Vendor/ImGui FILES ${SOURCES_IMGUI})

# Vendors/ImGuizmo ../vendor/cross-platform/ImGuizmo
file(GLOB_RECURSE SOURCES_IMGUIZMO ../vendor/cross-platform/ImGuizmo/*.cpp ../vendor/cross-platform/ImGuizmo/*.h)
source_group(TREE ../../vendor/cross-platform/ImGuizmo PREFIX src/Vendor/ImGuizmo FILES ${SOURCES_IMGUIZMO})

# Vendors/miniaudio ../vendor/miniaudio/include
file(GLOB_RECURSE SOURCES_MINIAUDIO ../vendor/miniaudio/include/*.c ../vendor/miniaudio/include/*.h)
source_group(TREE ../../vendor/miniaudio PREFIX src/Vendor/miniaudio/include FILES ${SOURCES_MINIAUDIO})

# Vendors/VulkanMemoryAllocator vendor/VulkanMemoryAllocator
file(GLOB_RECURSE SOURCES_VMA ../vendor/VulkanMemoryAllocator/*.cpp vendor/VulkanMemoryAllocator/*.h)
source_group(TREE ../../vendor/VulkanMemoryAllocator PREFIX src/Vendor/VulkanMemoryAllocator FILES ${SOURCES_VMA})


set_property(SOURCE ../../vendor/ImGui/backends/imgui_impl_dx12.cpp PROPERTY VS_SETTINGS "ExcludedFromBuild=true")
set_property(SOURCE ../../vendor/ImGui/backends/imgui_impl_vulkan.cpp PROPERTY VS_SETTINGS "ExcludedFromBuild=true")


file(GLOB_RECURSE BulletDynamics_SRCS		${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src/BulletDynamics/*.cpp)
file(GLOB_RECURSE BulletCollision_SRCS		${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src/BulletCollision/*.cpp)
file(GLOB_RECURSE LinearMath_SRCS			${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src/LinearMath/*.cpp)
#	file(GLOB_RECURSE assimp_SRCS
#		${ROOT_PATH_ABS}/vendor/cross-platform/assimp/*.cpp
#		${ROOT_PATH_ABS}/vendor/cross-platform/assimp/*.c
#		${ROOT_PATH_ABS}/vendor/cross-platform/assimp/*.cc
#	)
file(GLOB_RECURSE yaml-cpp_SRCS				${ROOT_PATH_ABS}/vendor/cross-platform/yaml-cpp/src/*.cpp)
file(GLOB_RECURSE box2d_SRCS
	${ROOT_PATH_ABS}/vendor/box2d/src/*.cpp
	${ROOT_PATH_ABS}/vendor/box2d/src/*.h
)
file(GLOB_RECURSE DirectXTex_SRCS			${ROOT_PATH_ABS}/vendor/DirectXTex/DirectXTex/*.cpp)
file(GLOB_RECURSE zlibstatic_SRCS			${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/zlib/*.c)
file(GLOB_RECURSE SPIRV-Cross_SRCS			${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/*.cpp)
file(GLOB_RECURSE irrXML_SRCS				${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/irrXML/*.cpp)
#	file(GLOB_RECURSE spirv-cross-core_SRCS		${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/*.cpp)
#	file(GLOB_RECURSE spirv-cross-cpp_SRCS		${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/*.cpp)
#	file(GLOB_RECURSE spirv-cross-glsl_SRCS		${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/*.cpp)
#	file(GLOB_RECURSE spirv-cross-hlsl_SRCS		${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/*.cpp)
#	file(GLOB_RECURSE spirv-cross-msl_SRCS		${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/*.cpp)
#	file(GLOB_RECURSE spirv-cross-reflect_SRCS	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/*.cpp)
#	file(GLOB_RECURSE spirv-cross-util_SRCS		${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/*.cpp)

if(WIN32)
	SET(glfw3_SRCS	
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/context.c
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/init.c
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/input.c
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/monitor.c
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/vulkan.c
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/window.c
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_init.c
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_joystick.c
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_monitor.c
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_time.c
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_thread.c
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_window.c
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/wgl_context.c
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/egl_context.c
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/osmesa_context.c
	)
elseif(UNIX)
	SET(glfw3_SRCS
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/glfw_config.h
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/include/GLFW/glfw3.h
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/include/GLFW/glfw3native.h
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/internal.h
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/mappings.h
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/context.c
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/init.c
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/input.c
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/monitor.c
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/vulkan.c
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/window.c
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_platform.h
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_joystick.h
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/wgl_context.h
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/egl_context.h
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/osmesa_context.h
		#${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_init.c
		#${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_joystick.c
		#${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_monitor.c
		#${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_time.c
		#${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_thread.c
		#${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/win32_window.c
		#${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/wgl_context.c
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/egl_context.c
		${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src/osmesa_context.c
	)
endif()

SET(zlibstatic_SRCS	
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/zlib/adler32.c
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/zlib/compress.c
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/zlib/crc32.c
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/zlib/deflate.c
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/zlib/gzclose.c
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/zlib/gzlib.c
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/zlib/gzread.c
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/zlib/gzwrite.c
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/zlib/inflate.c
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/zlib/infback.c
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/zlib/inftrees.c
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/zlib/inffast.c
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/zlib/trees.c
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/zlib/uncompr.c
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/zlib/zutil.c
)
SET(spirv-cross-core_SRCS
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_parser.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross_parsed_ir.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cfg.cpp
)

SET(spirv-cross-cpp_SRCS
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_parser.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross_parsed_ir.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cfg.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_glsl.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cpp.cpp
)

SET(spirv-cross-glsl_SRCS
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_parser.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross_parsed_ir.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cfg.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_glsl.cpp
)

SET(spirv-cross-hlsl_SRCS
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_parser.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross_parsed_ir.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cfg.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_glsl.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_hlsl.cpp
)

SET(spirv-cross-msl_SRCS
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_parser.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross_parsed_ir.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cfg.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_glsl.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_msl.cpp
)

SET(spirv-cross-reflect_SRCS
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_reflect.cpp
)

SET(spirv-cross-util_SRCS
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_parser.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross_parsed_ir.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cfg.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross_util.cpp
)

SET(SPIRV-Cross_SRCS
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_reflect.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_parser.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross_parsed_ir.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cfg.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_glsl.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cross_util.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_hlsl.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_cpp.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/spirv_msl.cpp
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/main.cpp
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
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/zlib
)

target_link_directories(
	zlibstatic PUBLIC
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/zlib/$ENV{MORAVA_BUILD_TYPE}
)

target_include_directories(
	assimp PUBLIC
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/code
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/include
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/irrXML
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/zlib
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/unzip
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/openddlparser/include
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/contrib/rapidjson/include
)

target_link_directories(
	assimp PUBLIC
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/lib/$ENV{MORAVA_BUILD_TYPE} # Win32
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/bin                         # Linux
)

target_include_directories(
	glfw3 PUBLIC
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/include
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src
)

target_link_directories(
	glfw3 PUBLIC
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src
)

target_include_directories(
	BulletDynamics PUBLIC
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/examples/ThirdPartyLibs/zlib
)

target_link_directories(
	BulletDynamics PUBLIC
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/lib                # Win32
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src/BulletDynamics # Linux
)

target_include_directories(
	BulletCollision PUBLIC
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/examples/ThirdPartyLibs/zlib
)

target_link_directories(
	BulletCollision PUBLIC
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/lib                 # Win32
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src/BulletCollision # Linux
)

target_include_directories(
	LinearMath PUBLIC
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/examples/ThirdPartyLibs/zlib
)

target_link_directories(
	LinearMath PUBLIC
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/lib            # Win32
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src/LinearMath # Linux
)

target_link_directories(
	SPIRV-Cross PUBLIC
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross
)

target_include_directories(
	yaml-cpp PUBLIC
	${ROOT_PATH_ABS}/vendor/cross-platform/yaml-cpp/include
	${ROOT_PATH_ABS}/vendor/cross-platform/yaml-cpp/src
)

target_include_directories(
	box2d PUBLIC
	${ROOT_PATH_ABS}/vendor/box2d/include
	${ROOT_PATH_ABS}/vendor/box2d/src
)

#	target_include_directories(
#		shaderc PUBLIC
#		${ROOT_PATH_ABS}/vendor/cross-platform/shaderc/libshaderc/include
#		${ROOT_PATH_ABS}/vendor/cross-platform/shaderc/libshaderc_util/include
#	)

#	target_link_directories(
#		shaderc PUBLIC
#		${ROOT_PATH_ABS}/vendor/cross-platform/shaderc/libshaderc/$ENV{MORAVA_BUILD_TYPE}
#		${ROOT_PATH_ABS}/vendor/cross-platform/shaderc/libshaderc_util/$ENV{MORAVA_BUILD_TYPE}
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
	${ROOT_PATH_ABS}/${PROJECT_NAME}/src
	${ROOT_PATH_ABS}/vendor
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/include
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/include
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/examples/ThirdPartyLibs/zlib
	${ROOT_PATH_ABS}/vendor/cross-platform/ImGuizmo
	${ROOT_PATH_ABS}/vendor/GLEW/include
	${ROOT_PATH_ABS}/vendor/ImGui
	${ROOT_PATH_ABS}/vendor/GLM
	${ROOT_PATH_ABS}/vendor/tinyobjloader
	${ROOT_PATH_ABS}/vendor/spdlog/include
    ${ROOT_PATH_ABS}/vendor/entt/include
	${ROOT_PATH_ABS}/vendor/cross-platform/yaml-cpp/include
	${ROOT_PATH_ABS}/vendor/mono/include
	${ROOT_PATH_ABS}/vendor/box2d/include
	${ROOT_PATH_ABS}/vendor/cross-platform/shaderc/libshaderc/include
	${ROOT_PATH_ABS}/vendor/cross-platform/shaderc/libshaderc_util/include
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross
	${ROOT_PATH_ABS}/vendor/Vulkan/1.2.148.1/Include
	${ROOT_PATH_ABS}/vendor/DirectXTex/DirectXTex
	${ROOT_PATH_ABS}/vendor/Optick/src
	${ROOT_PATH_ABS}/vendor/miniaudio/include
	${ROOT_PATH_ABS}/vendor/farbot/include
	${ROOT_PATH_ABS}/vendor/PhysX/include
	${ROOT_PATH_ABS}/vendor/PhysX/include/PhysX
	${ROOT_PATH_ABS}/vendor/NvidiaAftermath/include
)

target_link_directories(
	"${PROJECT_NAME}" PUBLIC
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/lib                 # Win32
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src/BulletDynamics  # Linux
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src/BulletCollision # Linux
	${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/src/LinearMath      # Linux
	# ${ROOT_PATH_ABS}/vendor/cross-platform/glew/build/lib
	${ROOT_PATH_ABS}/vendor/GLEW/lib
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/lib/$ENV{MORAVA_BUILD_TYPE} # Win32
	${ROOT_PATH_ABS}/vendor/cross-platform/assimp/bin                         # Linux
	${ROOT_PATH_ABS}/vendor/cross-platform/glfw/src
	${ROOT_PATH_ABS}/vendor/cross-platform/yaml-cpp
	${ROOT_PATH_ABS}/vendor/box2d/bin/$ENV{MORAVA_BUILD_TYPE} # Win32
	${ROOT_PATH_ABS}/vendor/box2d/build/bin # Linux
	${ROOT_PATH_ABS}/vendor/cross-platform/shaderc/libshaderc/$ENV{MORAVA_BUILD_TYPE}      # Win32
	${ROOT_PATH_ABS}/vendor/cross-platform/shaderc/libshaderc_util/$ENV{MORAVA_BUILD_TYPE} # Win32
	${ROOT_PATH_ABS}/vendor/cross-platform/shaderc/libshaderc      # Linux
	${ROOT_PATH_ABS}/vendor/cross-platform/shaderc/libshaderc_util # Linux
	${ROOT_PATH_ABS}/vendor/cross-platform/SPIRV-Cross/$ENV{MORAVA_BUILD_TYPE}
	${ROOT_PATH_ABS}/vendor/Vulkan/1.2.148.1/Lib
	#${ROOT_PATH_ABS}/vendor/DirectXTex/build/bin/CMake/$ENV{MORAVA_BUILD_TYPE}
	${ROOT_PATH_ABS}/vendor/DirectXTex/bin/CMake/$ENV{MORAVA_BUILD_TYPE}
	${ROOT_PATH_ABS}/vendor/mono/lib/$ENV{MORAVA_BUILD_TYPE}
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
execute_process(COMMAND cmake . WORKING_DIRECTORY ${ROOT_PATH_ABS}/vendor/cross-platform/assimp)

# Build GLFW with CMake
execute_process(COMMAND cmake . WORKING_DIRECTORY ${ROOT_PATH_ABS}/vendor/cross-platform/glfw)

# Build shaderc - use python to run git-sync-deps
execute_process(COMMAND python git-sync-deps WORKING_DIRECTORY ${ROOT_PATH_ABS}/vendor/cross-platform/shaderc/utils)
# Build shaderc with CMake
execute_process(COMMAND cmake . WORKING_DIRECTORY ${ROOT_PATH_ABS}/vendor/cross-platform/shaderc)

# Build DirectXTex
execute_process(COMMAND cmake . WORKING_DIRECTORY ${ROOT_PATH_ABS}/vendor/DirectXTex)

# Build yaml-cpp
execute_process(COMMAND cmake . WORKING_DIRECTORY ${ROOT_PATH_ABS}/vendor/cross-platform/yaml-cpp)

# Delete directory bullet3/examples/ThirdPartyLibs/imgui to avoid ImGui conflicts in compilation
file(REMOVE_RECURSE ${ROOT_PATH_ABS}/vendor/cross-platform/bullet3/examples/ThirdPartyLibs/imgui)


set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}")

set_property(TARGET "${PROJECT_NAME}" PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "..")

if(WIN32)

set_property(TARGET	"${PROJECT_NAME}"		PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

endif()
