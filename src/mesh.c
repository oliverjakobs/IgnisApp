#include "model.h"

int  loadMeshGLTF(Mesh* mesh, const cgltf_primitive* primitive, uint32_t group, uint32_t material)
{
    mesh->type = (IgnisPrimitiveType)primitive->type;
    mesh->material = material;
    mesh->group = group;

    mesh->vertex_count = 0;
    mesh->element_count = 0;

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
                cgltf_accessor_unpack_floats(accessor, mesh->positions, accessor->count * 3);

                mesh->min = (vec3){ accessor->min[0], accessor->min[1], accessor->min[2] };
                mesh->max = (vec3){ accessor->max[0], accessor->max[1], accessor->max[2] };
            }
            else IGNIS_WARN("MODEL: Vertices attribute data format not supported, use vec3 float");
            break;
        case cgltf_attribute_type_normal:
            if (accessor->type == cgltf_type_vec3 && accessor->component_type == cgltf_component_type_r_32f)
            {
                mesh->normals = malloc(accessor->count * 3 * sizeof(float));
                cgltf_accessor_unpack_floats(accessor, mesh->normals, accessor->count * 3);
            }
            else IGNIS_WARN("MODEL: Normal attribute data format not supported, use vec3 float");
            break;
        case cgltf_attribute_type_texcoord:
            if (accessor->type == cgltf_type_vec2 && accessor->component_type == cgltf_component_type_r_32f)
            {
                mesh->texcoords = malloc(accessor->count * 2 * sizeof(float));
                cgltf_accessor_unpack_floats(accessor, mesh->texcoords, accessor->count * 2);
            }
            else IGNIS_WARN("MODEL: Texcoords attribute data format not supported, use vec2 float");
            break;
        case cgltf_attribute_type_joints:
            if (accessor->type == cgltf_type_vec4)
            {
                mesh->joints = malloc(accessor->count * 4 * sizeof(uint32_t));
                for (size_t j = 0; j < accessor->count; ++j)
                    cgltf_accessor_read_uint(accessor, j, &mesh->joints[j * 4], 4);
            }
            else IGNIS_WARN("MODEL: Joint attribute data format not supported, use vec4 u16");
            break;
        case cgltf_attribute_type_weights:
            if (accessor->type == cgltf_type_vec4 && accessor->component_type == cgltf_component_type_r_32f)
            {
                mesh->weights = malloc(accessor->count * 4 * sizeof(float));
                cgltf_accessor_unpack_floats(accessor, mesh->weights, accessor->count * 4);
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

        mesh->element_count = accessor->count;
        mesh->indices = malloc(accessor->count * sizeof(uint32_t));

        for (size_t i = 0; i < accessor->count; ++i)
            mesh->indices[i] = cgltf_accessor_read_index(accessor, i);
    }

    return IGNIS_SUCCESS;
}

void destroyMesh(Mesh* mesh)
{
    ignisDeleteVertexArray(&mesh->vao);
    if (mesh->positions) free(mesh->positions);
    if (mesh->texcoords) free(mesh->texcoords);
    if (mesh->normals)   free(mesh->normals);
    if (mesh->joints)    free(mesh->joints);
    if (mesh->weights)   free(mesh->weights);
    if (mesh->indices)   free(mesh->indices);
}