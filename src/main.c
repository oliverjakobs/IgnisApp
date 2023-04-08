#include <ignis/ignis.h>

#include <minimal/application.h>

#include "math/math.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"


static void ignisErrorCallback(ignisErrorLevel level, const char *desc);
static void printVersionInfo();

IgnisFont font;

float width, height;
mat4 screen_projection;

IgnisShader shader;
IgnisVertexArray vao;

int view_mode = 0;
int poly_mode = 0;

vec3 camera_pos = { 0.0f, 0.0f, 10.0f };
float cameraspeed = 1.0f;

typedef struct
{
    IgnisVertexArray vao;

    size_t vertexCount;        // Number of vertices stored in arrays
    size_t elementCount;      // Number of triangles stored (indexed or not)

    // Vertex attributes data
    float* positions;        // Vertex position (XYZ - 3 components per vertex) (shader-location = 0)
    float* texcoords;       // Vertex texture coordinates (UV - 2 components per vertex) (shader-location = 1)
    float* texcoords2;      // Vertex texture second coordinates (UV - 2 components per vertex) (shader-location = 5)
    float* normals;         // Vertex normals (XYZ - 3 components per vertex) (shader-location = 2)
    float* tangents;        // Vertex tangents (XYZW - 4 components per vertex) (shader-location = 4)
    unsigned char* colors;      // Vertex colors (RGBA - 4 components per vertex) (shader-location = 3)
    GLuint* indices;    // Vertex indices (in case vertex data comes indexed)
} Mesh;

int uploadMesh(Mesh* mesh)
{
    ignisGenerateVertexArray(&mesh->vao);
    
    GLsizeiptr vertex_size = ignisGetOpenGLTypeSize(GL_FLOAT) * 3;
    ignisAddArrayBuffer(&mesh->vao, mesh->vertexCount * vertex_size, mesh->positions, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


    if (mesh->indices != NULL)
    {
        ignisLoadElementBuffer(&mesh->vao, mesh->indices, mesh->elementCount, GL_STATIC_DRAW);
    }
    return 0;
}

typedef struct 
{
    Mesh* meshes;
    size_t mesh_count;
} Model;

float* loadAttributef(cgltf_accessor *accessor, uint8_t components)
{
    if (accessor->count <= 0 || components <= 0) return NULL;

    float* data = (float*)malloc(accessor->count * components * sizeof(float));

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

int loadModel(Model* model, const char* filename)
{
    size_t size = 0;
    char* filedata = ignisReadFile(filename, &size);

    if (!filedata) return IGNIS_FAILURE;

    cgltf_options options = { 0 };
    cgltf_data* data = NULL;
    cgltf_result result = cgltf_parse(&options, filedata, size, &data);
    if (result != cgltf_result_success)
    {
        MINIMAL_ERROR("MODEL: [%s] Failed to load glTF data", filename);
        return IGNIS_FAILURE;
    }

    if (data->file_type == cgltf_file_type_glb)       MINIMAL_TRACE("MODEL: [%s] Model basic data (glb) loaded successfully", filename);
    else if (data->file_type == cgltf_file_type_gltf) MINIMAL_TRACE("MODEL: [%s] Model basic data (glTF) loaded successfully", filename);
    else MINIMAL_TRACE("MODEL: [%s] Model format not recognized", filename);

    MINIMAL_TRACE("    > Meshes count: %i", data->meshes_count);
    MINIMAL_TRACE("    > Materials count: %i (+1 default)", data->materials_count);
    MINIMAL_TRACE("    > Buffers count: %i", data->buffers_count);
    MINIMAL_TRACE("    > Images count: %i", data->images_count);
    MINIMAL_TRACE("    > Textures count: %i", data->textures_count);

    result = cgltf_load_buffers(&options, data, filename);
    if (result != cgltf_result_success)
    {
        MINIMAL_ERROR("MODEL: [%s] Failed to load mesh/material buffers", filename);
        return IGNIS_FAILURE;
    }

    size_t primitivesCount = 0;
    for (size_t i = 0; i < data->meshes_count; i++) primitivesCount += data->meshes[i].primitives_count;

    // Load our model data: meshes and materials
    model->mesh_count = primitivesCount;
    model->meshes = calloc(model->mesh_count, sizeof(Mesh));

    // Load meshes data
    //----------------------------------------------------------------------------------------------------
    for (unsigned int i = 0, meshIndex = 0; i < data->meshes_count; i++)
    {
        // NOTE: meshIndex accumulates primitives
        for (unsigned int p = 0; p < data->meshes[i].primitives_count; p++)
        {
            // NOTE: We only support primitives defined by triangles
            // Other alternatives: points, lines, line_strip, triangle_strip
            if (data->meshes[i].primitives[p].type != cgltf_primitive_type_triangles) continue;

            // NOTE: Attributes data could be provided in several data formats (8, 8u, 16u, 32...),
            // Only some formats for each attribute type are supported, read info at the top of this function!
            for (unsigned int j = 0; j < data->meshes[i].primitives[p].attributes_count; j++)
            {
                cgltf_accessor* accessor = data->meshes[i].primitives[p].attributes[j].data;
                switch (data->meshes[i].primitives[p].attributes[j].type)
                {
                case cgltf_attribute_type_position:
                    if (accessor->type == cgltf_type_vec3 && accessor->component_type == cgltf_component_type_r_32f)
                    {
                        model->meshes[meshIndex].vertexCount = accessor->count;
                        model->meshes[meshIndex].positions = loadAttributef(accessor, 3);
                    }
                    else MINIMAL_WARN("MODEL: [%s] Vertices attribute data format not supported, use vec3 float", filename);
                    break;
                case cgltf_attribute_type_normal:
                    if (accessor->type == cgltf_type_vec3 && accessor->component_type == cgltf_component_type_r_32f)
                    {
                        model->meshes[meshIndex].normals = loadAttributef(accessor, 3);
                    }
                    else MINIMAL_WARN("MODEL: [%s] Normal attribute data format not supported, use vec3 float", filename);
                    break;
                case cgltf_attribute_type_texcoord:
                    if (accessor->type == cgltf_type_vec2 && accessor->component_type == cgltf_component_type_r_32f)
                    {
                        model->meshes[meshIndex].texcoords = loadAttributef(accessor, 2);
                    }
                    else MINIMAL_WARN("MODEL: [%s] Texcoords attribute data format not supported, use vec2 float", filename);
                    break;
                case cgltf_attribute_type_tangent:
                    if (accessor->type == cgltf_type_vec4 && accessor->component_type == cgltf_component_type_r_32f)
                    {
                        model->meshes[meshIndex].tangents = loadAttributef(accessor, 4);
                    }
                    else MINIMAL_WARN("MODEL: [%s] Tangent attribute data format not supported, use vec4 float", filename);
                    break;
                case cgltf_attribute_type_color:
                    MINIMAL_WARN("MODEL: [%s] Color attribute not supported", filename);
                    break;
                }
            }

            // Load primitive indices data (if provided)
            if (data->meshes[i].primitives[p].indices != NULL)
            {
                cgltf_accessor* attribute = data->meshes[i].primitives[p].indices;

                model->meshes[meshIndex].elementCount = attribute->count;
                model->meshes[meshIndex].indices = malloc(attribute->count * sizeof(GLuint));

                if (attribute->component_type == cgltf_component_type_r_32u)
                {
                    size_t offset = attribute->buffer_view->offset + attribute->offset;
                    int8_t* buffer = attribute->buffer_view->buffer->data;
                    for (size_t k = 0; k < attribute->count; k++)
                    {
                        model->meshes[meshIndex].indices[k] = ((uint32_t*)(buffer + offset))[0];
                        offset += attribute->stride;
                    }
                }
                else if (attribute->component_type == cgltf_component_type_r_16u)
                {
                    size_t offset = attribute->buffer_view->offset + attribute->offset;
                    int8_t* buffer = attribute->buffer_view->buffer->data;
                    for (size_t k = 0; k < attribute->count; k++)
                    {
                        model->meshes[meshIndex].indices[k] = ((uint16_t*)(buffer + offset))[0];
                        offset += attribute->stride;
                    }
                }
                else MINIMAL_WARN("MODEL: [%s] Indices data format not supported, use u32", filename);
            }
            else model->meshes[meshIndex].elementCount = model->meshes[meshIndex].vertexCount;    // Unindexed mesh

            meshIndex++;       // Move to next mesh
        }
    }

    MINIMAL_INFO("Model loaded");
    cgltf_free(data);

    return IGNIS_SUCCESS;
}

Model robot;

static void setViewport(float w, float h)
{
    width = w;
    height = h;
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    screen_projection = mat4_ortho(0.0f, w, h, 0.0f, -1.0f, 1.0f);
}

int onLoad(MinimalApp *app, uint32_t w, uint32_t h)
{
    /* ingis initialization */
    // ignisSetAllocator(FrostMemoryGetAllocator(), tb_mem_malloc, tb_mem_realloc, tb_mem_free);
    ignisSetErrorCallback(ignisErrorCallback);

#ifdef _DEBUG
    int debug = 1;
    minimalEnableDebug(app, debug);
#else
    int debug = 0;
#endif

    if (!ignisInit(debug))
    {
        MINIMAL_ERROR("[IGNIS] Failed to initialize Ignis");
        return MINIMAL_FAIL;
    }

    ignisEnableBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ignisSetClearColor(IGNIS_DARK_GREY);

    glEnable(GL_DEPTH_TEST);

    /* font renderer */
    ignisCreateFont(&font, "res/fonts/ProggyTiny.ttf", 24.0);
    ignisFontRendererInit();
    ignisFontRendererBindFontColor(&font, IGNIS_WHITE);

    setViewport((float)w, (float)h);

    /* shader */
    shader = ignisCreateShadervf("res/shaders/shader.vert", "res/shaders/shader.frag");

    /* gltf model */
    loadModel(&robot, "res/models/box/Box.gltf");
    //loadModel(&robot, "res/models/walking_robot/Scene.gltf");

    for (int i = 0; i < robot.mesh_count; i++) uploadMesh(&robot.meshes[i]);

    printVersionInfo();

    return MINIMAL_OK;
}

void onDestroy(MinimalApp *app)
{
    ignisDeleteVertexArray(&vao);
    ignisDeleteShader(shader);
    
    ignisDeleteFont(&font);

    ignisFontRendererDestroy();
}

int onEvent(MinimalApp *app, const MinimalEvent* e)
{
    float w, h;
    if (minimalEventWindowSize(e, &w, &h))
    {
        setViewport(w, h);
        glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    }

    switch (minimalEventKeyPressed(e))
    {
    case GLFW_KEY_ESCAPE:    minimalClose(app); break;
    case GLFW_KEY_F6:        minimalToggleVsync(app); break;
    case GLFW_KEY_F7:        minimalToggleDebug(app); break;
    case GLFW_KEY_F9:        view_mode = !view_mode; break;
    case GLFW_KEY_F10:       poly_mode = !poly_mode; break;
    }

    return MINIMAL_OK;
}

void onUpdate(MinimalApp *app, float deltatime)
{
    // clear screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    if (minimalKeyDown(GLFW_KEY_LEFT))  camera_pos.x -= cameraspeed * deltatime;
    if (minimalKeyDown(GLFW_KEY_RIGHT)) camera_pos.x += cameraspeed * deltatime;
    if (minimalKeyDown(GLFW_KEY_DOWN))  camera_pos.y += cameraspeed * deltatime;
    if (minimalKeyDown(GLFW_KEY_UP))    camera_pos.y -= cameraspeed * deltatime;

    mat4 model = model = mat4_rotation((vec3) { 0.5f, 1.0f, 0.0f }, (float)glfwGetTime());
    mat4 view = view = mat4_translation(vec3_negate(camera_pos));
    mat4 proj = proj = mat4_perspective(degToRad(45.0f), (float)width / (float)height, 0.1f, 100.0f);

    ignisSetUniformMat4(shader, "proj", proj.v[0]);
    ignisSetUniformMat4(shader, "view", view.v[0]);
    ignisSetUniformMat4(shader, "model", model.v[0]);

    ignisUseShader(shader);
    glPolygonMode(GL_FRONT_AND_BACK, poly_mode ? GL_LINE : GL_FILL);

    for (int i = 0; i < robot.mesh_count; i++)
    {
        Mesh* mesh = &robot.meshes[i];
        ignisBindVertexArray(&mesh->vao);
        glDrawElements(GL_TRIANGLES, mesh->elementCount, GL_UNSIGNED_INT, NULL);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // render debug info
    ignisFontRendererSetProjection(screen_projection.v[0]);

    /* fps */
    ignisFontRendererRenderTextFormat(8.0f, 8.0f, "FPS: %d", minimalGetFps(app));

    if (app->debug)
    {
        /* Settings */
        ignisFontRendererTextFieldBegin(width - 220.0f, 8.0f, 8.0f);

        ignisFontRendererTextFieldLine("F6:  Toggle Vsync");
        ignisFontRendererTextFieldLine("F7:  Toggle debug mode");
        ignisFontRendererTextFieldLine("F9:  Toggle view mode");
        ignisFontRendererTextFieldLine("F10: Toggle polygon mode");
    }

    ignisFontRendererFlush();
}

int main()
{
    MinimalApp app = {
        .on_load =    onLoad,
        .on_destroy = onDestroy,
        .on_event =   onEvent,
        .on_update =  onUpdate
    };

    if (minimalLoad(&app, "IgnisApp", 1024, 800, "4.4"))
        minimalRun(&app);

    minimalDestroy(&app);

    return 0;
}

void ignisErrorCallback(ignisErrorLevel level, const char *desc)
{
    switch (level)
    {
    case IGNIS_LVL_WARN:     MINIMAL_WARN("%s", desc); break;
    case IGNIS_LVL_ERROR:    MINIMAL_ERROR("%s", desc); break;
    case IGNIS_LVL_CRITICAL: MINIMAL_CRITICAL("%s", desc); break;
    }
}

void printVersionInfo()
{
    MINIMAL_INFO("[GLFW]   Version:      %s", glfwGetVersionString());
    MINIMAL_INFO("[OpenGL] Version:      %s", ignisGetGLVersion());
    MINIMAL_INFO("[OpenGL] Vendor:       %s", ignisGetGLVendor());
    MINIMAL_INFO("[OpenGL] Renderer:     %s", ignisGetGLRenderer());
    MINIMAL_INFO("[OpenGL] GLSL Version: %s", ignisGetGLSLVersion());
    MINIMAL_INFO("[Ignis]  Version:      %s", ignisGetVersionString());
}