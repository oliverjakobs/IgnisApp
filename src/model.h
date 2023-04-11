#ifndef MODEL_H
#define MODEL_H

#include "cgltf.h"
#include <ignis/ignis.h>

typedef struct
{
    IgnisColorRGBA color;
    IgnisTexture2D base_texture;

    float roughness;
    float metallic;

    IgnisTexture2D normal;
    IgnisTexture2D occlusion;
    IgnisTexture2D emmisive;
} Material;

int loadDefaultMaterial(Material* material);
int loadMaterialGLTF(const cgltf_material* gltf_material, Material* material, const char* dir);

typedef struct
{
    IgnisVertexArray vao;

    uint32_t vertex_count;
    uint32_t element_count;

    // Vertex attributes data
    float* positions;   // Vertex position (XYZ - 3 components per vertex) (shader-location = 0)
    float* texcoords;   // Vertex texture coordinates (UV - 2 components per vertex) (shader-location = 1)
    float* normals;     // Vertex normals (XYZ - 3 components per vertex) (shader-location = 2)
    uint8_t* colors;    // Vertex colors (RGBA - 4 components per vertex) (shader-location = 3)
    float* tangents;    // Vertex tangents (XYZW - 4 components per vertex) (shader-location = 4)
    GLuint* indices;    // Vertex indices (in case vertex data comes indexed)
} Mesh;

int loadMeshGLTF(const cgltf_primitive* primitive, Mesh* mesh);

typedef struct
{
    Mesh* meshes;
    uint32_t* mesh_materials;
    size_t mesh_count;

    Material* materials;
    size_t material_count;
} Model;

#endif // !MODEL_H
