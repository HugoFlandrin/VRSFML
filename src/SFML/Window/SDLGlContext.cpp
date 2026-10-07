// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/VRSFML/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Window/SDLGlContext.hpp"

#include "SFML/Window/ContextSettings.hpp"
#include "SFML/Window/SDLLayer.hpp"
#include "SFML/Window/SDLWindowImpl.hpp"
#include "SFML/Window/WindowContext.hpp"

#include "SFML/System/Err.hpp"

#include "SFML/Base/Assert.hpp"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_video.h>

#ifdef SFML_SYSTEM_ANDROID
    #include <EGL/egl.h>
    #include <EGL/eglext.h>

    #include <cstring>
#endif


#ifdef SFML_SYSTEM_ANDROID
namespace
{
////////////////////////////////////////////////////////////
/// EGL objects shared by every GL context on Android, taken from SDL once its single window exists.
////////////////////////////////////////////////////////////
struct AndroidEglState
{
    EGLDisplay display{EGL_NO_DISPLAY};
    EGLConfig  config{};
    bool       surfaceless{false}; // EGL_KHR_surfaceless_context: offscreen contexts need no surface at all
    bool       ready{false};
};

AndroidEglState& androidEglState()
{
    static AndroidEglState state;
    return state;
}


////////////////////////////////////////////////////////////
[[nodiscard]] bool androidInitEglState()
{
    auto& state = androidEglState();

    if (state.ready)
        return true;

    state.display = static_cast<EGLDisplay>(SDL_EGL_GetCurrentDisplay());
    state.config  = static_cast<EGLConfig>(SDL_EGL_GetCurrentConfig());

    if (state.display == EGL_NO_DISPLAY || state.config == nullptr)
    {
        sf::priv::errMsg("No EGL display/config available from SDL: {}", SDL_GetError());
        return false;
    }

    const char* const extensions = eglQueryString(state.display, EGL_EXTENSIONS);
    state.surfaceless = extensions != nullptr && std::strstr(extensions, "EGL_KHR_surfaceless_context") != nullptr;
    state.ready       = true;
    return true;
}


////////////////////////////////////////////////////////////
[[nodiscard]] EGLContext androidCreateEglContext(const sf::ContextSettings& settings, EGLContext shareContext)
{
    const auto& state = androidEglState();

    if (!eglBindAPI(EGL_OPENGL_ES_API))
    {
        sf::priv::errMsg("eglBindAPI(EGL_OPENGL_ES_API) failed (EGL error 0x{})", static_cast<unsigned int>(eglGetError()));
        return EGL_NO_CONTEXT;
    }

    const EGLint attributes[] = {EGL_CONTEXT_MAJOR_VERSION,
                                 static_cast<EGLint>(settings.majorVersion),
                                 EGL_CONTEXT_MINOR_VERSION,
                                 static_cast<EGLint>(settings.minorVersion),
                                 EGL_NONE};

    const EGLContext context = eglCreateContext(state.display, state.config, shareContext, attributes);

    if (context == EGL_NO_CONTEXT)
        sf::priv::errMsg("eglCreateContext failed (EGL error 0x{})", static_cast<unsigned int>(eglGetError()));

    return context;
}


////////////////////////////////////////////////////////////
/// A tiny pbuffer surface for offscreen contexts when surfaceless contexts are not supported.
////////////////////////////////////////////////////////////
[[nodiscard]] EGLSurface androidCreateOffscreenSurface()
{
    const auto& state = androidEglState();

    if (state.surfaceless)
        return EGL_NO_SURFACE;

    const EGLint attributes[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
    const EGLSurface surface  = eglCreatePbufferSurface(state.display, state.config, attributes);

    if (surface == EGL_NO_SURFACE)
        sf::priv::errMsg("eglCreatePbufferSurface failed (EGL error 0x{})", static_cast<unsigned int>(eglGetError()));

    return surface;
}
} // namespace
#endif


namespace sf::priv
{
////////////////////////////////////////////////////////////
void SDLGlContext::destroyWindowIfNeeded()
{
    if (!m_ownsWindow || m_window == nullptr)
        return;

    SDL_DestroyWindow(m_window);

    m_window     = nullptr;
    m_ownsWindow = false;
}


////////////////////////////////////////////////////////////
void SDLGlContext::initContext(SDLGlContext* const shared)
{
#ifdef SFML_SYSTEM_ANDROID
    // Every context is created with EGL directly, sharing with the shared context (see the comment on
    // `m_androidOffscreen`). SDL has already initialized EGL when it created the single window.
    if (!androidInitEglState())
    {
        destroyWindowIfNeeded();
        return;
    }

    const EGLContext eglContext = androidCreateEglContext(m_settings,
                                                          shared != nullptr ? reinterpret_cast<EGLContext>(shared->m_context)
                                                                            : EGL_NO_CONTEXT);

    if (eglContext == EGL_NO_CONTEXT)
    {
        destroyWindowIfNeeded();
        return;
    }

    m_context = reinterpret_cast<SDL_GLContextState*>(eglContext);

    if (m_androidOffscreen)
        m_androidSurface = androidCreateOffscreenSurface();

    return;
#else
    auto& sdlLayer = WindowContext::getSDLLayer();

    // Set context sharing attributes if a shared context is provided
    if (shared != nullptr)
    {
        if (!shared->makeCurrent(true))
        {
            destroyWindowIfNeeded();
            return;
        }

        // The next created context will be shared with the current one
        if (!sdlLayer.setGLAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1))
            errMsg("Failed to set shared GL context attribute");
    }

    // Create the OpenGL context
    m_context = SDL_GL_CreateContext(m_window);

    if (!m_context)
    {
        errMsg("Failed to create SDL GL context: {}", SDL_GetError());
        destroyWindowIfNeeded();
        return;
    }

    // Reset sharing attribute to default
    if (!sdlLayer.setGLAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0))
        errMsg("Failed to reset shared GL context attribute");
#endif
}


////////////////////////////////////////////////////////////
SDLGlContext::SDLGlContext(const unsigned int id, SDLGlContext* const shared, const ContextSettings& settings) :
    GlContext(id, settings),
    m_window(nullptr),
    m_context(nullptr),
    m_ownsWindow(false)
{
#ifdef SFML_SYSTEM_ANDROID
    m_androidOffscreen = true;

    // Only the shared context (the first one) takes SDL's single window; per-thread contexts have no window.
    if (shared != nullptr)
    {
        initContext(shared);
        return;
    }
#endif

    if (!WindowContext::getSDLLayer().applyGLContextSettings(m_settings))
        errMsg("Failed to apply SDL GL context settings for shared GL context hidden window");

    // Create a hidden window for the context
    m_window = SDL_CreateWindow("", 1, 1, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (m_window == nullptr)
    {
        errMsg("Failed to create hidden window for SDLGlContext: {}", SDL_GetError());
        return;
    }

    m_ownsWindow = true;
    initContext(shared);
}


////////////////////////////////////////////////////////////
SDLGlContext::SDLGlContext(const unsigned int     id,
                           SDLGlContext* const    shared,
                           const ContextSettings& settings,
                           const SDLWindowImpl&   owner,
                           const unsigned int /* bitsPerPixel */) :
    GlContext(id, settings),
    m_window(owner.getSDLHandle()),
    m_context(nullptr),
    m_ownsWindow(false)
{
    initContext(shared);
}


////////////////////////////////////////////////////////////
SDLGlContext::~SDLGlContext()
{
    WindowContext::cleanupUnsharedFrameBuffers(*this);

#ifdef SFML_SYSTEM_ANDROID
    if (m_androidOffscreen)
    {
        const auto& state = androidEglState();

        if (m_context != nullptr)
        {
            if (eglGetCurrentContext() == reinterpret_cast<EGLContext>(m_context))
                (void)makeCurrent(false);

            eglDestroyContext(state.display, reinterpret_cast<EGLContext>(m_context));
        }

        if (m_androidSurface != nullptr)
            eglDestroySurface(state.display, static_cast<EGLSurface>(m_androidSurface));

        m_context = nullptr;
        destroyWindowIfNeeded();
        return;
    }
#endif

    // Deactivate the context if it's current
    if (m_context && SDL_GL_GetCurrentContext() == m_context)
        (void)makeCurrent(false);

    // Delete the context
    if (m_context)
        SDL_GL_DestroyContext(m_context);

    // Destroy the window if owned
    destroyWindowIfNeeded();
}


////////////////////////////////////////////////////////////
GlFunctionPointer SDLGlContext::getFunction(const char* const name) const
{
    return SDL_GL_GetProcAddress(name);
}


////////////////////////////////////////////////////////////
SDL_Window* SDLGlContext::getSDLWindow() const noexcept
{
    return m_window;
}


////////////////////////////////////////////////////////////
bool SDLGlContext::makeCurrent(const bool activate)
{
    SFML_BASE_ASSERT((!activate || m_context != nullptr) &&
                     "Cannot activate SDL GL context: context was not successfully created");

#ifdef SFML_SYSTEM_ANDROID
    if (m_androidOffscreen)
    {
        // Keep SDL's own record of the current window/context in sync (it would otherwise skip a later
        // `SDL_GL_MakeCurrent` call, believing the window context is still current), then bind with EGL.
        (void)SDL_GL_MakeCurrent(nullptr, nullptr);

        const auto&      state   = androidEglState();
        const EGLSurface surface = activate ? static_cast<EGLSurface>(m_androidSurface) : EGL_NO_SURFACE;
        const EGLContext context = activate ? reinterpret_cast<EGLContext>(m_context) : EGL_NO_CONTEXT;

        if (!eglMakeCurrent(state.display, surface, surface, context))
        {
            errMsg("Failed to {} offscreen EGL context (EGL error 0x{})",
                   activate ? "activate" : "deactivate",
                   static_cast<unsigned int>(eglGetError()));
            return false;
        }

        return true;
    }
#endif

    auto*       targetWindow  = activate ? m_window : nullptr;
    auto*       targetContext = activate ? m_context : nullptr;
    const char* targetAction  = activate ? "activate" : "deactivate";

    if (!SDL_GL_MakeCurrent(targetWindow, targetContext))
    {
        errMsg("Failed to {} SDL GL context: {}", targetAction, SDL_GetError());
        return false;
    }

    return true;
}


////////////////////////////////////////////////////////////
void SDLGlContext::display()
{
    SDL_GL_SwapWindow(m_window);
}


////////////////////////////////////////////////////////////
void SDLGlContext::setVerticalSyncEnabled(const bool enabled)
{
#ifdef SFML_SYSTEM_EMSCRIPTEN
    // Emscripten path is handled by `Window::display()`
    m_vsyncRequested = enabled;
#else
    if (!SDL_GL_SetSwapInterval(enabled ? 1 : 0))
        errMsg("Failed to set vertical sync: {}", SDL_GetError());
#endif
}


////////////////////////////////////////////////////////////
bool SDLGlContext::isVerticalSyncEnabled() const
{
#ifdef SFML_SYSTEM_EMSCRIPTEN
    return m_vsyncRequested;
#else
    int interval{};

    if (!SDL_GL_GetSwapInterval(&interval))
    {
        errMsg("Failed to get vertical sync: {}", SDL_GetError());
        return false;
    }

    return interval != 0;
#endif
}


} // namespace sf::priv
