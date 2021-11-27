#include "Editor/SceneObject.h"


SceneObject::SceneObject()
{
    castShadow = true;
    receiveShadows = true;
}

void SceneObject::Render()
{
    if (mesh && m_Type == "mesh") // is it a mesh?
    {
        mesh->Render(); // Render by 'editor_object' OR 'editor_object_pbr' OR 'skinning'
    }
    else if (model && m_Type == "model") // is it a model?
    {
        model->RenderPBR();
    }
}

SceneObject::~SceneObject()
{
    //  delete m_AABB;
    //  delete pivot;
    //  if (mesh != nullptr) {
    //      delete mesh;
    //  }
    //  if (model != nullptr && model->GetMeshList().size() > 0) {
    //      delete model;
    //  }
}
