#ifndef MODEL_H
#define MODEL_H

#include "cgltf.h"

#include <ignis/ignis.h>

#include "math/math.h"

typedef struct Model Model;

// ----------------------------------------------------------------
// utility
// ----------------------------------------------------------------
size_t getJointIndex(const cgltf_node* target, const cgltf_skin* skin, size_t fallback);

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

    // joint data
    uint32_t* joints;
    float*    weights;

    uint32_t* indices;  // Vertex indices (in case vertex data comes indexed)
} Mesh;

int  loadMeshGLTF(Mesh* mesh, const cgltf_primitive* primitive);
void destroyMesh(Mesh* mesh);

// ----------------------------------------------------------------
// animation
// ----------------------------------------------------------------
typedef struct AnimationChannel
{
    float* times;
    float* transforms;

    size_t frame_count;
    size_t last_frame;
} AnimationChannel;

int  loadAnimationChannelGLTF(AnimationChannel* channel, cgltf_animation_sampler* sampler);
void destroyAnimationChannel(AnimationChannel* channel);

typedef struct JointAnimation
{
    AnimationChannel translation;
    AnimationChannel rotation;
    AnimationChannel scale;
} JointAnimation;

typedef struct Animation
{
    JointAnimation* joints;
    size_t joint_count;

    float time;
    float duration;
} Animation;

int  loadAnimationGLTF(Animation* animation, cgltf_animation* gltf_animation, cgltf_skin* skin);
void destroyAnimation(Animation* animation);

void getAnimationPose(const Model* model, const Animation* animation, mat4* out);
void getBindPose(const Model* model, mat4* out);

// ----------------------------------------------------------------
// model + skin
// ----------------------------------------------------------------
struct Model
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
};

int  loadSkinGLTF(Model* model, cgltf_skin* skin);
void destroySkin(Model* model);

int  loadModelGLTF(Model* model, Animation* animation, const char* dir, const char* filename);
void destroyModel(Model* model);


int uploadMesh(Mesh* mesh);
void renderModel(const Model* model, const Animation* animation, IgnisShader shader);

#endif // !MODEL_H
