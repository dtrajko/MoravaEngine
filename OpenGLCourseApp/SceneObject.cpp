#include "SceneObject.h"


SceneObject::SceneObject()
{
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
    // delete mesh;
    // delete model;
    delete AABB;
    // delete pivot;
}
