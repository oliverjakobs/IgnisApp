#include "model.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "minimal/utils.h"

int loadDefaultMaterial(Material* material)
{
    material->color = IGNIS_WHITE;
    material->base_texture = IGNIS_DEFAULT_TEXTURE2D;
    material->normal = IGNIS_DEFAULT_TEXTURE2D;
    material->occlusion = IGNIS_DEFAULT_TEXTURE2D;
    material->emmisive = IGNIS_DEFAULT_TEXTURE2D;
    return IGNIS_SUCCESS;
}

int loadMaterialGLTF(const cgltf_material* gltf_material, Material* material, const char* dir)
{
    // set defaults
    loadDefaultMaterial(material);

    // Check glTF material flow: PBR metallic/roughness flow
    // NOTE: Alternatively, materials can follow PBR specular/glossiness flow
    if (gltf_material->has_pbr_metallic_roughness)
    {
        // Load base color texture
        if (gltf_material->pbr_metallic_roughness.base_color_texture.texture)
        {
            cgltf_image* image = gltf_material->pbr_metallic_roughness.base_color_texture.texture->image;
            MINIMAL_TRACE("MODEL: Material has base color texture");
            MINIMAL_TRACE("MODEL:     > URI: %s", image->uri);

        }
        // Load base color factor
        material->color.r = gltf_material->pbr_metallic_roughness.base_color_factor[0];
        material->color.g = gltf_material->pbr_metallic_roughness.base_color_factor[1];
        material->color.b = gltf_material->pbr_metallic_roughness.base_color_factor[2];
        material->color.a = gltf_material->pbr_metallic_roughness.base_color_factor[3];

        // Load metallic/roughness texture
        if (gltf_material->pbr_metallic_roughness.metallic_roughness_texture.texture)
        {
            MINIMAL_TRACE("MODEL: Material has metallic/roughness texture");

            // Load metallic/roughness material properties
            material->roughness = gltf_material->pbr_metallic_roughness.roughness_factor;
            material->metallic = gltf_material->pbr_metallic_roughness.metallic_factor;
        }
    }
    else if (gltf_material->has_pbr_specular_glossiness)
    {
        // Load diffuse texture
        if (gltf_material->pbr_specular_glossiness.diffuse_texture.texture)
        {
            cgltf_image* image = gltf_material->pbr_specular_glossiness.diffuse_texture.texture->image;
            MINIMAL_TRACE("MODEL: Material has diffuse texture");
            MINIMAL_TRACE("MODEL:     > URI: %s", image->uri);

            ignisCreateTexture2D(&material->base_texture, ignisTextFormat("%s/%s", dir, image->uri), 0, NULL);
        }
        // Load diffuse factor
        material->color.r = gltf_material->pbr_specular_glossiness.diffuse_factor[0];
        material->color.g = gltf_material->pbr_specular_glossiness.diffuse_factor[1];
        material->color.b = gltf_material->pbr_specular_glossiness.diffuse_factor[2];
        material->color.a = gltf_material->pbr_specular_glossiness.diffuse_factor[3];
    }

    // Load normal texture
    if (gltf_material->normal_texture.texture)
    {
        MINIMAL_TRACE("MODEL: Material has normal texture");
    }

    // Load ambient occlusion texture
    if (gltf_material->occlusion_texture.texture)
    {
        MINIMAL_TRACE("MODEL: Material has ambient occlusion texture");
    }

    // Load emissive texture
    if (gltf_material->emissive_texture.texture)
    {
        MINIMAL_TRACE("MODEL: Material has emissive texture");
    }

    // Other possible materials not supported by raylib pipeline:
    // has_clearcoat, has_transmission, has_volume, has_ior, has specular, has_sheen
    return IGNIS_SUCCESS;
}


float* loadAttributef(cgltf_accessor* accessor, uint8_t components)
{
    if (accessor->count <= 0 || components <= 0) return NULL;

    float* data = malloc(accessor->count * components * sizeof(float));

    if (!data) return NULL;

    size_t offset = accessor->buffer_view->offset + accessor->offset;
    int8_t* buffer = accessor->buffer_view->buffer->data;
    for (size_t i = 0; i < accessor->count; ++i)
    {
        for (uint8_t c = 0; c < components; ++c)
        {
            data[components * i + c] = ((float*)(buffer + offset))[c];
        }
        offset += accessor->stride;
    }
    return data;
}

int loadMeshGLTF(const cgltf_primitive* primitive, Mesh* mesh)
{
    // NOTE: Attributes data could be provided in several data formats (8, 8u, 16u, 32...),
            // Only some formats for each attribute type are supported, read info at the top of this function!
    for (unsigned int j = 0; j < primitive->attributes_count; j++)
    {
        cgltf_accessor* accessor = primitive->attributes[j].data;
        switch (primitive->attributes[j].type)
        {
        case cgltf_attribute_type_position:
            if (accessor->type == cgltf_type_vec3 && accessor->component_type == cgltf_component_type_r_32f)
            {
                mesh->vertex_count = (uint32_t)accessor->count;
                mesh->positions = loadAttributef(accessor, 3);
            }
            else MINIMAL_WARN("MODEL: Vertices attribute data format not supported, use vec3 float");
            break;
        case cgltf_attribute_type_normal:
            if (accessor->type == cgltf_type_vec3 && accessor->component_type == cgltf_component_type_r_32f)
            {
                mesh->normals = loadAttributef(accessor, 3);
            }
            else MINIMAL_WARN("MODEL: Normal attribute data format not supported, use vec3 float");
            break;
        case cgltf_attribute_type_texcoord:
            if (accessor->type == cgltf_type_vec2 && accessor->component_type == cgltf_component_type_r_32f)
            {
                mesh->texcoords = loadAttributef(accessor, 2);
            }
            else MINIMAL_WARN("MODEL: Texcoords attribute data format not supported, use vec2 float");
            break;
        case cgltf_attribute_type_tangent:
            if (accessor->type == cgltf_type_vec4 && accessor->component_type == cgltf_component_type_r_32f)
            {
                mesh->tangents = loadAttributef(accessor, 4);
            }
            else MINIMAL_WARN("MODEL: Tangent attribute data format not supported, use vec4 float");
            break;
        case cgltf_attribute_type_color:
            MINIMAL_WARN("MODEL: Color attribute not supported");
            break;
        }
    }

    // Load primitive indices data (if provided)
    if (primitive->indices != NULL)
    {
        cgltf_accessor* accessor = primitive->indices;

        mesh->element_count = (uint32_t)accessor->count;
        mesh->indices = malloc(accessor->count * sizeof(GLuint));

        if (accessor->component_type == cgltf_component_type_r_32u)
        {
            size_t offset = accessor->buffer_view->offset + accessor->offset;
            int8_t* buffer = accessor->buffer_view->buffer->data;
            for (size_t k = 0; k < accessor->count; k++)
            {
                mesh->indices[k] = ((uint32_t*)(buffer + offset))[0];
                offset += accessor->stride;
            }
        }
        else if (accessor->component_type == cgltf_component_type_r_16u)
        {
            size_t offset = accessor->buffer_view->offset + accessor->offset;
            int8_t* buffer = accessor->buffer_view->buffer->data;
            for (size_t k = 0; k < accessor->count; k++)
            {
                mesh->indices[k] = ((uint16_t*)(buffer + offset))[0];
                offset += accessor->stride;
            }
        }
        else MINIMAL_WARN("MODEL: Indices data format not supported, use u32");
    }
    else mesh->element_count = mesh->vertex_count;    // Unindexed mesh

    return IGNIS_SUCCESS;
}