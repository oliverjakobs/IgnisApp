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

static int loadTextureBase64(IgnisTexture2D* texture, char* buffer, size_t len)
{
    if (len % 4 != 0)
    {
        IGNIS_WARN("IMAGE: data is not a multiple of 4");
        return IGNIS_FAILURE;
    }

    size_t output_len = (len / 4) * 3;
    output_len -= (buffer[len - 1] == '=');
    output_len -= (buffer[len - 2] == '=');

    void* data = NULL;
    cgltf_options options = { 0 };
    if (cgltf_load_buffer_base64(&options, output_len, buffer, &data) != cgltf_result_success)
    {
        IGNIS_WARN("IMAGE: Failed to load base64 buffer");
        return IGNIS_FAILURE;
    }

    int result = ignisCreateTexture2DSrc(texture, data, output_len, NULL);
    free(data);
    return result;
}

int loadTextureGLTF(IgnisTexture2D* texture, cgltf_texture* gltf_texture, const char* dir)
{
    cgltf_image* image = gltf_texture->image;
    if (image->uri)
    {
        char* uri = image->uri;
        size_t uri_len = cgltf_decode_uri(uri);
        // Check if image is provided as base64 text data
        if ((uri_len > 5) && (uri[0] == 'd') && (uri[1] == 'a') && (uri[2] == 't') && (uri[3] == 'a') && (uri[4] == ':'))
        {
            // Data URI Format: data:<mediatype>;base64,<data>

            // Find the comma
            int i = 5;
            while ((uri[i] != ',') && (uri[i] != '\0')) i++;

            if (uri[i++] == '\0')
            {
                IGNIS_WARN("IMAGE: glTF data URI is not a valid image");
                return IGNIS_FAILURE;
            }
            return loadTextureBase64(texture, uri + i, uri_len - i);
        }
        else     // Check if image is provided as image path
        {
            return ignisCreateTexture2D(texture, ignisTextFormat("%s/%s", dir, uri), NULL);
        }
    }
    else if (image->buffer_view->buffer->data != NULL)
    {
        uint8_t* data = malloc(image->buffer_view->size);
        size_t offset = image->buffer_view->offset;
        size_t stride = image->buffer_view->stride ? image->buffer_view->stride : 1;

        // Copy buffer data to memory for loading
        for (size_t i = 0; i < image->buffer_view->size; i++)
        {
            data[i] = ((uint8_t*)image->buffer_view->buffer->data)[offset];
            offset += stride;
        }

        int result = ignisCreateTexture2DSrc(texture, data, image->buffer_view->size, NULL);
        free(data);
        return result;
    }
    return IGNIS_SUCCESS;
}

int loadMaterialGLTF(Material* material, const cgltf_material* gltf_material, const char* dir)
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
            cgltf_texture* texture = gltf_material->pbr_metallic_roughness.base_color_texture.texture;
            loadTextureGLTF(&material->base_texture, texture, dir);
        }
        // Load base color factor
        material->color.r = gltf_material->pbr_metallic_roughness.base_color_factor[0];
        material->color.g = gltf_material->pbr_metallic_roughness.base_color_factor[1];
        material->color.b = gltf_material->pbr_metallic_roughness.base_color_factor[2];
        material->color.a = gltf_material->pbr_metallic_roughness.base_color_factor[3];

        // Load metallic/roughness texture
        if (gltf_material->pbr_metallic_roughness.metallic_roughness_texture.texture)
        {
            cgltf_texture* texture = gltf_material->pbr_metallic_roughness.metallic_roughness_texture.texture;
            loadTextureGLTF(&material->metallic_roughness, texture, dir);

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
            cgltf_texture* texture = gltf_material->pbr_specular_glossiness.diffuse_texture.texture;
            loadTextureGLTF(&material->base_texture, texture, dir);
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
        cgltf_texture* texture = gltf_material->normal_texture.texture;
        loadTextureGLTF(&material->normal, texture, dir);
    }

    // Load ambient occlusion texture
    if (gltf_material->occlusion_texture.texture)
    {
        cgltf_texture* texture = gltf_material->occlusion_texture.texture;
        loadTextureGLTF(&material->occlusion, texture, dir);
    }

    // Load emissive texture
    if (gltf_material->emissive_texture.texture)
    {
        cgltf_texture* texture = gltf_material->emissive_texture.texture;
        loadTextureGLTF(&material->emmisive, texture, dir);
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