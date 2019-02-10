#include "SystemGraphicsContext.h"

#if USE_GLFW

SystemGraphicsContext::SystemGraphicsContext() {

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    // We open a 1x1 window here -- this is not where the action happens, however;
    // this program renders to a texture.  This is done merely to make GL happy.
    window = glfwCreateWindow(1, 1, "SystemGraphicsContext_DummyWindow", NULL, NULL);

}

void SystemGraphicsContext::makeCurrent() {
    glfwMakeContextCurrent(window);
}

void SystemGraphicsContext::unmakeCurrent() {
}

void SystemGraphicsContext::swapBuffers() {
    glfwSwapBuffers(window);
}

void SystemGraphicsContext::cleanup() {
    if (window) {
        glfwDestroyWindow(window);
        window = NULL;
    }
}

MLHandle SystemGraphicsContext::context() const {
    return (MLHandle)glfwGetCurrentContext();
}

GLADloadproc SystemGraphicsContext::loader() const {
    return (GLADloadproc)glfwGetProcAddress;
}

#else

SystemGraphicsContext::SystemGraphicsContext() {
    egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    EGLint major = 4;
    EGLint minor = 0;
    eglInitialize(egl_display, &major, &minor);
    eglBindAPI(EGL_OPENGL_API);

    EGLint config_attribs[] = {
      EGL_RED_SIZE, 8,            // 5/6/5 is another option
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_ALPHA_SIZE, 0,
      EGL_DEPTH_SIZE, 24,
      EGL_STENCIL_SIZE, 0,        // 8 works for stencil size too if you need it
      EGL_NONE
    };
    EGLConfig egl_config = nullptr;
    EGLint config_size = 0;
    eglChooseConfig(egl_display, config_attribs, &egl_config, 1, &config_size);

    EGLint context_attribs[] = {
      EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
      EGL_CONTEXT_MINOR_VERSION_KHR, 0,
      EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR,
      EGL_NONE
    };
    egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attribs);
}

void SystemGraphicsContext::makeCurrent() {
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, egl_context);
}

void SystemGraphicsContext::unmakeCurrent() {
    eglMakeCurrent(NULL, EGL_NO_SURFACE, EGL_NO_SURFACE, NULL);
}

void SystemGraphicsContext::swapBuffers() {
    // buffer swapping is implicit on device (MLGraphicsEndFrame)
}

void SystemGraphicsContext::cleanup() {
    if (egl_display) {
        eglDestroyContext(egl_display, egl_context);
        eglTerminate(egl_display);
        egl_display = NULL;
    }
}

MLHandle SystemGraphicsContext::context() const {
    return (MLHandle)egl_context;
}

GLADloadproc SystemGraphicsContext::loader() const {
    return (GLADloadproc)eglGetProcAddress;
}

#endif
