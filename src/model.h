#ifndef MODEL_H
#define MODEL_H

#include "cgltf.h"

#include <ignis/ignis.h>

#include "math/math.h"

typedef struct Model Model;

// ----------------------------------------------------------------
// utility
// ----------------------------------------------------------------
size_t getMeshIndex(const cgltf_mesh* target, const cgltf_mesh* meshes, size_t count);
uint32_t getJointIndex(const cgltf_node* target, const cgltf_skin* skin, uint32_t fallback);

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
    IgnisPrimitiveType type;

    size_t vertex_count;
    size_t element_count;

    vec3 min;
    vec3 max;

    uint32_t material;
    uint32_t group;

    // Vertex attributes data
    float* positions;   // Vertex position (XYZ - 3 components per vertex) (shader-location = 0)
    float* texcoords;   // Vertex texture coordinates (UV - 2 components per vertex) (shader-location = 1)
    float* normals;     // Vertex normals (XYZ - 3 components per vertex) (shader-location = 2)

    // joint data
    uint32_t* joints;
    float*    weights;

    uint32_t* indices;  // Vertex indices (in case vertex data comes indexed)
} Mesh;

int  loadMeshGLTF(Mesh* mesh, const cgltf_primitive* primitive, uint32_t group, uint32_t material);
void destroyMesh(Mesh* mesh);

// ----------------------------------------------------------------
// animation
// ----------------------------------------------------------------
typedef struct AnimationChannel
{
    float* times;
    float* transforms;

    size_t frame_count;
} AnimationChannel;

int  loadAnimationChannelGLTF(AnimationChannel* channel, cgltf_animation_sampler* sampler);
void destroyAnimationChannel(AnimationChannel* channel);

typedef struct Animation
{
    AnimationChannel* translations;
    AnimationChannel* rotations;
    AnimationChannel* scales;
    size_t channel_count;

    float time;
    float duration;
} Animation;

int  loadAnimationGLTF(Animation* animation, cgltf_animation* gltf_animation, const cgltf_data* data);
void destroyAnimation(Animation* animation);

Animation* loadAnimationsGLTF(cgltf_data* data, size_t* count);

int  getAnimationTransform(const Animation* animation, size_t index, mat4* transform);
void getAnimationJointTransforms(const Model* model, const Animation* animation, mat4* transforms);
void getBindPose(const Model* model, mat4* out);

void resetAnimation(Animation* animation);
void tickAnimation(Animation* animation, float deltatime);

// ----------------------------------------------------------------
// model + skin
// ----------------------------------------------------------------
struct Model
{
    Mesh* meshes;
    size_t mesh_count;

    Material* materials;
    size_t material_count;

    // instances
    uint32_t* instances;
    mat4* transforms;
    size_t instance_count;

    // skin
    uint32_t* joints;
    mat4* joint_locals;
    mat4* joint_inv_transforms;
    size_t joint_count;
};

int  loadSkinGLTF(Model* model, cgltf_skin* skin);
void destroySkin(Model* model);

int  loadModelGLTF(Model* model, cgltf_data* data, const char* dir);
void destroyModel(Model* model);


int uploadMesh(Mesh* mesh);
void renderModel(const Model* model, const Animation* animation, IgnisShader shader);
void renderModelSkinned(const Model* model, const Animation* animation, IgnisShader shader);

#endif // !MODEL_H
