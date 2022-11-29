#include "Application.h"

#include <string.h>

/* --------------------------| minimal app |----------------------------- */
void MinimalGLFWErrorCallback(int error, const char* desc)
{
    MINIMAL_ERROR("[GLFW] (%d) %s", error, desc);
}

void MinimalGLFWWindowSizeCallback(GLFWwindow* window, int width, int height);

void MinimalGLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void MinimalGLFWCharCallback(GLFWwindow* window, unsigned int keycode);
void MinimalGLFWMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void MinimalGLFWCursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void MinimalGLFWScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

static void MinimalGetGLVersion(const char* version_str, int* major, int* minor)
{
    const char* sep = ".";
    const char* major_str = version_str;
    const char* minor_str = version_str + strcspn(version_str, sep) + 1;

    if (major_str && major) *major = atoi(major_str);
    if (minor_str && minor) *minor = atoi(minor_str);
}

static int MinimalInitGlfw(MinimalApp* app, const char* title, uint32_t w, uint32_t h, const char* gl_version)
{
    if (!glfwInit()) return MINIMAL_FAIL;

    int gl_major, gl_minor;
    MinimalGetGLVersion(gl_version, &gl_major, &gl_minor);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, gl_major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, gl_minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef _DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    glfwSetErrorCallback(MinimalGLFWErrorCallback);

    /* creating the window */
    app->window = glfwCreateWindow(w, h, title, NULL, NULL);
    if (!app->window) return MINIMAL_FAIL;

    glfwMakeContextCurrent(app->window);
    glfwSetWindowUserPointer(app->window, app);

    /* set GLFW callbacks */
    glfwSetWindowSizeCallback(app->window,  MinimalGLFWWindowSizeCallback);
    glfwSetKeyCallback(app->window,         MinimalGLFWKeyCallback);
    glfwSetCharCallback(app->window,        MinimalGLFWCharCallback);
    glfwSetMouseButtonCallback(app->window, MinimalGLFWMouseButtonCallback);
    glfwSetCursorPosCallback(app->window,   MinimalGLFWCursorPosCallback);
    glfwSetScrollCallback(app->window,      MinimalGLFWScrollCallback);

    return MINIMAL_OK;
}

int MinimalLoad(MinimalApp* app, const char* title, uint32_t w, uint32_t h, const char* gl_version)
{
    if (MinimalInitGlfw(app, title, w, h, gl_version) != MINIMAL_OK)
    {
        MINIMAL_ERROR("[GLFW] Failed to initialize GLFW.");
        glfwTerminate();
        return MINIMAL_FAIL;
    }

    /* apply settings */
    MinimalEnableDebug(app, app->debug);
    MinimalEnableVsync(app, app->vsync);

    MinimalTimerReset(&app->timer);

    return (app->on_load) ? app->on_load(app, w, h) : MINIMAL_OK;
}

void MinimalDestroy(MinimalApp* app)
{
    if (app->on_destroy) app->on_destroy(app);
    glfwDestroyWindow(app->window);
    glfwTerminate();
}

void MinimalRun(MinimalApp* app)
{
    MINIMAL_ASSERT(app, "");
    MINIMAL_ASSERT(app->on_update, "Update callback missing!");

    while (!glfwWindowShouldClose(app->window)) {
        MinimalTimerStart(&app->timer, glfwGetTime());
        MinimalUpdateInput(app->window);

        app->on_update(app, (float)app->timer.deltatime);

        glfwPollEvents();
        glfwSwapBuffers(app->window);

        MinimalTimerEnd(&app->timer, glfwGetTime());
    }
}

void MinimalClose(MinimalApp* app) { glfwSetWindowShouldClose(app->window, GLFW_TRUE); }

/* --------------------------| settings |-------------------------------- */
void MinimalSetWindowTitle(MinimalApp* app, const char* title) { glfwSetWindowTitle(app->window, title); }

void MinimalEnableDebug(MinimalApp* app, int b) { app->debug = b; }
void MinimalEnableVsync(MinimalApp* app, int b) { glfwSwapInterval(b); app->vsync = b; }

void MinimalToggleDebug(MinimalApp* app) { MinimalEnableDebug(app, !app->debug); }
void MinimalToggleVsync(MinimalApp* app) { MinimalEnableVsync(app, !app->vsync); }

/* --------------------------| glfw events |----------------------------- */
void MinimalGLFWWindowSizeCallback(GLFWwindow* window, int width, int height)
{
    MinimalApp* app = (MinimalApp*)glfwGetWindowUserPointer(window);
    if (app) MinimalDispatchEvent(app, MINIMAL_EVENT_WINDOW_SIZE, 0, width, height);
}

void MinimalGLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    MinimalApp* app = (MinimalApp*)glfwGetWindowUserPointer(window);
    if (app) MinimalDispatchEvent(app, MINIMAL_EVENT_KEY, key, action, mods);
}

void MinimalGLFWCharCallback(GLFWwindow* window, unsigned int keycode)
{
    MinimalApp* app = (MinimalApp*)glfwGetWindowUserPointer(window);
    if (app) MinimalDispatchEvent(app, MINIMAL_EVENT_CHAR, keycode, 0, 0);
}

void MinimalGLFWMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    MinimalApp* app = (MinimalApp*)glfwGetWindowUserPointer(window);
    if (!app) return;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    MinimalDispatchEvent(app, MINIMAL_EVENT_MOUSE_BUTTON, (button << 16) + action, (int32_t)xpos, (int32_t)ypos);
}

void MinimalGLFWCursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    MinimalApp* app = (MinimalApp*)glfwGetWindowUserPointer(window);
    if (app) MinimalDispatchEvent(app, MINIMAL_EVENT_MOUSE_MOVED, 0, (int32_t)xpos, (int32_t)ypos);
}

void MinimalGLFWScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
    MinimalApp* app = (MinimalApp*)glfwGetWindowUserPointer(window);
    if (app) MinimalDispatchEvent(app, MINIMAL_EVENT_MOUSE_SCROLLED, 0, (int32_t)xOffset, (int32_t)yOffset);
}
