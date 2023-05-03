#ifndef MODEL_H
#define MODEL_H

#include "cgltf.h"

#include <ignis/ignis.h>

#include "math/math.h"

// ----------------------------------------------------------------
// material
// ----------------------------------------------------------------
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

int  loadDefaultMaterial(Material* material);
int  loadMaterialGLTF(Material* material, const cgltf_material* gltf_material, const char* dir);
void destroyMaterial(Material* material);

// ----------------------------------------------------------------
// mesh
// ----------------------------------------------------------------
typedef struct Mesh
{
    IgnisVertexArray vao;

    size_t vertex_count;
    size_t element_count;

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

int  loadMeshGLTF(Mesh* mesh, const cgltf_primitive* primitive);
void destroyMesh(Mesh* mesh);

// ----------------------------------------------------------------
// model + skin
// ----------------------------------------------------------------
typedef struct Model
{
    Mesh* meshes;
    uint32_t* mesh_materials;
    size_t mesh_count;

    Material* materials;
    size_t material_count;

    // skin
    uint32_t* joints;
    mat4* joint_locals;
    mat4* joint_inv_transforms;
    size_t joint_count;
} Model;

int  loadSkinGLTF(Model* model, cgltf_skin* skin);
void destroySkin(Model* model);

int loadModelGLTF(Model* model, const cgltf_data* data, const char* dir);

// ----------------------------------------------------------------
// animation
// ----------------------------------------------------------------
typedef struct Animation
{
    float* times;
    mat4** transforms;

    size_t frame_count;
    size_t joint_count;

    uint32_t current_frame;

    float duration;
} Animation;

typedef struct TransformSampler
{
    cgltf_accessor* translation;
    cgltf_accessor* rotation;
    cgltf_accessor* scale;
} TransformSampler;

int loadAnimation(Animation* animation, cgltf_accessor* times, TransformSampler* transforms);
void destroyAnimation(Animation* animation);

void getAnimationPoses(const Model* model, const mat4* in, mat4* out);




int loadModel(Model* model, Animation* animation, const char* dir, const char* filename);
void destroyModel(Model* model);

int uploadMesh(Mesh* mesh);
void renderModel(const Model* model, const Animation* animation, IgnisShader shader);

#endif // !MODEL_H
