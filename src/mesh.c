#include "model.h"

#define LOAD_ATTRIBUTE(dst, accessor, components, type)             \
size_t offset = accessor->buffer_view->offset + accessor->offset;   \
int8_t* src = accessor->buffer_view->buffer->data;                  \
for (size_t i = 0; i < accessor->count; ++i)                        \
{                                                                   \
    for (uint8_t c = 0; c < components; ++c)                        \
        dst[components * i + c] = ((type*)(src + offset))[c];       \
    offset += accessor->stride;                                     \
}

int loadMeshGLTF(Mesh* mesh, const cgltf_primitive* primitive)
{
    for (size_t i = 0; i < primitive->attributes_count; ++i)
    {
        cgltf_accessor* accessor = primitive->attributes[i].data;
        switch (primitive->attributes[i].type)
        {
        case cgltf_attribute_type_position:
            if (accessor->type == cgltf_type_vec3 && accessor->component_type == cgltf_component_type_r_32f)
            {
                mesh->vertex_count = (uint32_t)accessor->count;
                mesh->positions = malloc(accessor->count * 3 * sizeof(float));
                LOAD_ATTRIBUTE(mesh->positions, accessor, 3, float)

                mesh->min = (vec3){ accessor->min[0], accessor->min[1], accessor->min[2] };
                mesh->max = (vec3){ accessor->max[0], accessor->max[1], accessor->max[2] };
            }
            else IGNIS_WARN("MODEL: Vertices attribute data format not supported, use vec3 float");
            break;
        case cgltf_attribute_type_normal:
            if (accessor->type == cgltf_type_vec3 && accessor->component_type == cgltf_component_type_r_32f)
            {
                mesh->normals = malloc(accessor->count * 3 * sizeof(float));
                LOAD_ATTRIBUTE(mesh->normals, accessor, 3, float)
            }
            else IGNIS_WARN("MODEL: Normal attribute data format not supported, use vec3 float");
            break;
        case cgltf_attribute_type_tangent:
            if (accessor->type == cgltf_type_vec4 && accessor->component_type == cgltf_component_type_r_32f)
            {
                mesh->tangents = malloc(accessor->count * 4 * sizeof(float));
                LOAD_ATTRIBUTE(mesh->tangents, accessor, 4, float)
            }
            else IGNIS_WARN("MODEL: Tangent attribute data format not supported, use vec4 float");
            break;
        case cgltf_attribute_type_texcoord:
            if (accessor->type == cgltf_type_vec2 && accessor->component_type == cgltf_component_type_r_32f)
            {
                mesh->texcoords = malloc(accessor->count * 2 * sizeof(float));
                LOAD_ATTRIBUTE(mesh->texcoords, accessor, 2, float)
            }
            else IGNIS_WARN("MODEL: Texcoords attribute data format not supported, use vec2 float");
            break;
        case cgltf_attribute_type_color:
            IGNIS_WARN("MODEL: Color attribute not supported");
            break;
        case cgltf_attribute_type_joints:
            if (accessor->type == cgltf_type_vec4)
            {
                mesh->joints = malloc(accessor->count * 4 * sizeof(uint16_t));
                if (accessor->component_type == cgltf_component_type_r_16u)
                {
                    LOAD_ATTRIBUTE(mesh->joints, accessor, 4, uint16_t)
                }
                else if (accessor->component_type == cgltf_component_type_r_8u)
                {
                    LOAD_ATTRIBUTE(mesh->joints, accessor, 4, uint8_t)
                }
            }
            else IGNIS_WARN("MODEL: Joint attribute data format not supported, use vec4 u16");
            break;
        case cgltf_attribute_type_weights:
            if (accessor->type == cgltf_type_vec4 && accessor->component_type == cgltf_component_type_r_32f)
            {
                mesh->weights = malloc(accessor->count * 4 * sizeof(float));
                LOAD_ATTRIBUTE(mesh->weights, accessor, 4, float)
            }
            else IGNIS_WARN("MODEL: Joint weight attribute data format not supported, use vec4 float");
            break;
        default:
            IGNIS_WARN("MODEL: Unsupported attribute");

        }
    }

    // Load primitive indices data (if provided)
    if (primitive->indices != NULL)
    {
        cgltf_accessor* accessor = primitive->indices;

        mesh->element_count = (uint32_t)accessor->count;
        mesh->indices = malloc(accessor->count * sizeof(uint32_t));

        if (accessor->component_type == cgltf_component_type_r_32u)
        {
            LOAD_ATTRIBUTE(mesh->indices, accessor, 1, uint32_t)
        }
        else if (accessor->component_type == cgltf_component_type_r_16u)
        {
            LOAD_ATTRIBUTE(mesh->indices, accessor, 1, uint16_t)
        }
        else IGNIS_WARN("MODEL: Indices data format not supported, use u32");
    }

    return IGNIS_SUCCESS;
}

void destroyMesh(Mesh* mesh)
{
    ignisDeleteVertexArray(&mesh->vao);
    if (mesh->positions) free(mesh->positions);
    if (mesh->texcoords) free(mesh->texcoords);
    if (mesh->normals)   free(mesh->normals);
    if (mesh->colors)    free(mesh->colors);
    if (mesh->tangents)  free(mesh->tangents);
    if (mesh->joints)    free(mesh->joints);
    if (mesh->weights)   free(mesh->weights);
    if (mesh->indices)   free(mesh->indices);
}