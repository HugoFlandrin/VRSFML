#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/VRSFML/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Window/Export.hpp"


namespace sf
{
////////////////////////////////////////////////////////////
/// \brief Application lifecycle events of mobile platforms (Android, iOS)
///
////////////////////////////////////////////////////////////
enum class AppLifecycleEvent
{
    WillEnterBackground, //!< The application is about to go to the background: save state, pause audio
    DidEnterBackground,  //!< The application is now in the background (it may be suspended at any moment)
    WillEnterForeground, //!< The application is about to come back to the foreground
    DidEnterForeground,  //!< The application is back in the foreground and interactive
    LowMemory,           //!< The system is running low on memory: release what can be recreated
    Terminating,         //!< The system is about to terminate the application
};


////////////////////////////////////////////////////////////
/// \brief Callback type for `setAppLifecycleCallback`
///
////////////////////////////////////////////////////////////
using AppLifecycleCallback = void (*)(AppLifecycleEvent event, void* userData);


////////////////////////////////////////////////////////////
/// \brief Register a callback invoked synchronously when the application lifecycle changes
///
/// Unlike the window events (`sf::Event::FocusLost` / `FocusGained`), which are only delivered by
/// `pollEvent()` and therefore only once the application runs again, this callback is invoked at the
/// moment the platform reports the change. On Android and iOS the application's main loop is frozen
/// as soon as it goes to the background, so this is the only reliable place to pause audio, release
/// resources or save the game state *before* the suspension.
///
/// The callback may be invoked from a thread other than the one running the main loop (on Android,
/// the Java UI thread), and must return quickly. It is never invoked on platforms that have no such
/// lifecycle (desktop). Pass a null callback to unregister. Set it once at startup, before the
/// events can happen.
///
/// \param callback Function to call, or `nullptr` to unregister
/// \param userData Opaque pointer handed back to the callback
///
////////////////////////////////////////////////////////////
SFML_WINDOW_API void setAppLifecycleCallback(AppLifecycleCallback callback, void* userData = nullptr);

} // namespace sf
