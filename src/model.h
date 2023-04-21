#ifndef MODEL_H
#define MODEL_H

#include "cgltf.h"

#include <ignis/ignis.h>

#include "math/vec3.h"
#include "math/mat4.h"

typedef struct Material
{
    IgnisColorRGBA color;
    IgnisTexture2D base_texture;

    IgnisTexture2D metallic_roughness;
    float roughness;
    float metallic;

    IgnisTexture2D normal;
    IgnisTexture2D occlusion;
    IgnisTexture2D emmisive;
} Material;

int loadDefaultMaterial(Material* material);
int loadMaterialGLTF(const cgltf_material* gltf_material, Material* material, const char* dir);

typedef struct Mesh
{
    IgnisVertexArray vao;

    uint32_t vertex_count;
    uint32_t element_count;

    vec3 min;
    vec3 max;

    // Vertex attributes data
    float* positions;   // Vertex position (XYZ - 3 components per vertex) (shader-location = 0)
    float* texcoords;   // Vertex texture coordinates (UV - 2 components per vertex) (shader-location = 1)
    float* normals;     // Vertex normals (XYZ - 3 components per vertex) (shader-location = 2)
    uint8_t* colors;    // Vertex colors (RGBA - 4 components per vertex) (shader-location = 3)
    float* tangents;    // Vertex tangents (XYZW - 4 components per vertex) (shader-location = 4)

    // joint data
    uint16_t* joints;
    float*    weights;

    uint32_t* indices;  // Vertex indices (in case vertex data comes indexed)
} Mesh;

int loadMeshGLTF(const cgltf_primitive* primitive, Mesh* mesh);

int uploadMesh(Mesh* mesh);


typedef struct JointInfo
{
    uint32_t parent;
    mat4 local;
} JointInfo;

typedef struct Model
{
    Mesh* meshes;
    uint32_t* mesh_materials;
    size_t mesh_count;

    Material* materials;
    size_t material_count;

    JointInfo* joints;
    mat4* joint_inv_transform;
    size_t joint_count;
} Model;

int loadModel(Model* model, const char* dir, const char* filename);

void destroyModel(Model* model);

void renderModel(const Model* model, IgnisShader shader);

typedef struct AnimationFrame
{
    float time;
    mat4 transform;
} AnimationFrame;

#endif // !MODEL_H
