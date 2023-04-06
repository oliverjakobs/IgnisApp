#include <ignis/ignis.h>

#include <minimal/application.h>

#include "math/math.h"

static void ignisErrorCallback(ignisErrorLevel level, const char *desc);
static void printVersionInfo();

IgnisFont font;

float width, height;
mat4 screen_projection;


float vertices[] = {
    // neg x, red
    -0.5f,  0.5f,  0.5f,  0.1f, 0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.1f, 0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.1f, 0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.1f, 0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.1f, 0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.1f, 0.0f, 0.0f, 1.0f,
    // pos x, red
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 1.0f,
    // neg y, green
    -0.5f, -0.5f, -0.5f,  0.0f, 0.1f, 0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 0.1f, 0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 0.1f, 0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 0.1f, 0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.1f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.1f, 0.0f, 1.0f,
    // pos y, green
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f, 1.0f,
    // neg z, blue
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.1f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.1f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 0.1f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 0.1f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 0.1f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.1f, 1.0f,
    // pos z, blue
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f, 1.0f,
};

IgnisShader shader;
IgnisVertexArray vao;

int view_mode = 0;
int poly_mode = 0;

vec3 camera_pos = { 0.0f, 0.0f, 10.0f };
float cameraspeed = 1.0f;

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

    /* cube */
    ignisGenerateVertexArray(&vao);

    IgnisBufferElement layout[] = {
        { GL_FLOAT, 3, GL_FALSE },
        { GL_FLOAT, 4, GL_FALSE }
    };
    ignisAddArrayBufferLayout(&vao, sizeof(vertices), vertices, GL_STATIC_DRAW, 0, layout, 2);


    /* shader */
    ignisCreateShadervf(&shader, "res/shaders/shader.vert", "res/shaders/shader.frag");

    printVersionInfo();

    return MINIMAL_OK;
}

void onDestroy(MinimalApp *app)
{
    ignisDeleteVertexArray(&vao);
    ignisDeleteShader(&shader);
    
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

const vec3 tiles[] = {
    { 0.0f, 0.0f, 0.0f, },
    { 1.0f, 0.0f, 0.0f, },
    { 2.0f, 0.0f, 0.0f, },
    { 3.0f, 0.0f, 0.0f, }
};

void onUpdate(MinimalApp *app, float deltatime)
{
    // clear screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    if (minimalKeyDown(GLFW_KEY_LEFT))  camera_pos.x -= cameraspeed * deltatime;
    if (minimalKeyDown(GLFW_KEY_RIGHT)) camera_pos.x += cameraspeed * deltatime;
    if (minimalKeyDown(GLFW_KEY_DOWN))  camera_pos.y += cameraspeed * deltatime;
    if (minimalKeyDown(GLFW_KEY_UP))    camera_pos.y -= cameraspeed * deltatime;

    mat4 model = mat4_identity();
    mat4 view = mat4_identity();
    mat4 proj = mat4_identity();

    // create transformations
    if (!view_mode)
    {
        model = mat4_rotation((vec3) { 0.5f, 1.0f, 0.0f }, (float)glfwGetTime());
        view = mat4_translation(vec3_negate(camera_pos));
        proj = mat4_perspective(degToRad(45.0f), (float)width / (float)height, 0.1f, 100.0f);
    }
    else
    {
        //camera_pos.z = 0.0f;
        view = mat4_translation(vec3_negate(camera_pos));
        view = mat4_rotate_x(view, degToRad(-30.0f));
        view = mat4_rotate_z(view, degToRad(-45.0f));

        float w = width / 100.0f;
        float h = height / 100.0f;
        proj = mat4_ortho(-w / 2, w / 2, -h / 2, h / 2, -1.0f, h + camera_pos.z);
        //proj = mat4_perspective(degToRad(45.0f), (float)width / (float)height, 0.1f, 100.0f);
    }

    ignisSetUniformMat4(&shader, "proj", proj.v[0]);
    ignisSetUniformMat4(&shader, "view", view.v[0]);

    ignisUseShader(&shader);
    ignisBindVertexArray(&vao);

    glPolygonMode(GL_FRONT_AND_BACK, poly_mode ? GL_LINE : GL_FILL);

    /*
    for (int i = 0; i < 4; ++i)
    {
        model = mat4_translation(tiles[i]);
        ignisSetUniformMat4(&shader, "model", model.v[0]);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    */
    ignisSetUniformMat4(&shader, "model", model.v[0]);
    glDrawArrays(GL_TRIANGLES, 0, 36);

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