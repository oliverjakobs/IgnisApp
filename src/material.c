#include "model.h"

int loadDefaultMaterial(Material* material)
{
    material->color = IGNIS_WHITE;
    material->base_texture = IGNIS_DEFAULT_TEXTURE2D;
    material->normal = IGNIS_DEFAULT_TEXTURE2D;
    material->occlusion = IGNIS_DEFAULT_TEXTURE2D;
    material->emmisive = IGNIS_DEFAULT_TEXTURE2D;
    return IGNIS_SUCCESS;
}

int loadMaterialGLTF(Material* material, const cgltf_material* gltf_material,const char* dir)
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
            ignisCreateTexture2D(&material->base_texture, ignisTextFormat("%s/%s", dir, image->uri), 0, NULL);

        }
        // Load base color factor
        material->color.r = gltf_material->pbr_metallic_roughness.base_color_factor[0];
        material->color.g = gltf_material->pbr_metallic_roughness.base_color_factor[1];
        material->color.b = gltf_material->pbr_metallic_roughness.base_color_factor[2];
        material->color.a = gltf_material->pbr_metallic_roughness.base_color_factor[3];

        // Load metallic/roughness texture
        if (gltf_material->pbr_metallic_roughness.metallic_roughness_texture.texture)
        {
            cgltf_image* image = gltf_material->pbr_metallic_roughness.metallic_roughness_texture.texture->image;
            ignisCreateTexture2D(&material->metallic_roughness, ignisTextFormat("%s/%s", dir, image->uri), 0, NULL);

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
        cgltf_image* image = gltf_material->normal_texture.texture->image;
        ignisCreateTexture2D(&material->normal, ignisTextFormat("%s/%s", dir, image->uri), 0, NULL);
    }

    // Load ambient occlusion texture
    if (gltf_material->occlusion_texture.texture)
    {
        cgltf_image* image = gltf_material->occlusion_texture.texture->image;
        ignisCreateTexture2D(&material->occlusion, ignisTextFormat("%s/%s", dir, image->uri), 0, NULL);
    }

    // Load emissive texture
    if (gltf_material->emissive_texture.texture)
    {
        cgltf_image* image = gltf_material->emissive_texture.texture->image;
        ignisCreateTexture2D(&material->emmisive, ignisTextFormat("%s/%s", dir, image->uri), 0, NULL);
    }

    // Other possible materials not supported by raylib pipeline:
    // has_clearcoat, has_transmission, has_volume, has_ior, has specular, has_sheen
    return IGNIS_SUCCESS;
}

void destroyMaterial(Material* material)
{
    if (!ignisIsDefaultTexture2D(material->base_texture)) ignisDeleteTexture2D(&material->base_texture);
    if (!ignisIsDefaultTexture2D(material->normal))       ignisDeleteTexture2D(&material->normal);
    if (!ignisIsDefaultTexture2D(material->occlusion))    ignisDeleteTexture2D(&material->occlusion);
    if (!ignisIsDefaultTexture2D(material->emmisive))     ignisDeleteTexture2D(&material->emmisive);
}