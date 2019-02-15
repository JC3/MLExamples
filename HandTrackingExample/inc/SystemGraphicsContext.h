#ifndef SYSTEMGRAPHICSCONTEXT_H
#define SYSTEMGRAPHICSCONTEXT_H

#include <glad/glad.h> // Generated at https://glad.dav1d.de/

// Note: USE_GLFW isn't a standard built-in #define that mabu provides or 
// anything like that. It's set up in glfw.comp, which is included via the
// .mabu file USES statement. Also, note that VS doesn't seem to be aware
// of #defines set in .mabu/.comp/.package files so if you select a host
// target in VS, don't be surprised if the VS editor still makes it look like
// USE_GLFW is undefined.

#if USE_GLFW
#  include <GLFW/glfw3.h>
#else
#  include <EGL/egl.h>
#  include <EGL/eglext.h>
#endif

#include <ml_api.h> // MLHandle


//-----------------------------------------------------------------------------
/**
 * This is basically graphics_context_t from the SimpleGLApp SDK example, with
 * the addition of cleanup(), context(), gladLoader() and some other changes. 
 * It just wraps windowing system stuff.
 *
 * This example uses the OpenGL 3.0 Compatibility Profile and uses the fixed
 * function pipeline API to draw. You can use core profile and shaders if you
 * want, too. Same idea re: usage of camera transformations and such. The 3.0
 * compatibility profile is specified in the constructor, go look there.
 */
 //-----------------------------------------------------------------------------

class SystemGraphicsContext {
private:
#if USE_GLFW
    GLFWwindow *window;
#else
    EGLDisplay egl_display;
    EGLContext egl_context;
#endif
public:
    SystemGraphicsContext();
    ~SystemGraphicsContext() { cleanup(); }
    void cleanup();
    void makeCurrent();
    void swapBuffers();
    void unmakeCurrent();
    MLHandle context() const;
    GLADloadproc loader() const;
};

#endif
