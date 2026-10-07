// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/VRSFML/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Window/AppLifecycle.hpp"

#include "SFML/Window/AppLifecycleWatch.hpp"

#include "SFML/System/Atomic.hpp"

#include <SDL3/SDL_events.h>


namespace
{
////////////////////////////////////////////////////////////
sf::Atomic<sf::AppLifecycleCallback> lifecycleCallback{nullptr};
sf::Atomic<void*>                    lifecycleUserData{nullptr};


////////////////////////////////////////////////////////////
// Runs synchronously on the thread that reports the event (on Android, the Java UI thread), as SDL
// documents these events must be handled before the application gets suspended
bool SDLCALL lifecycleWatch(void* /* userData */, SDL_Event* const event)
{
    const auto callback = lifecycleCallback.loadSeqCst();

    if (callback == nullptr)
        return true;

    sf::AppLifecycleEvent mapped{};

    switch (event->type)
    {
        case SDL_EVENT_WILL_ENTER_BACKGROUND: mapped = sf::AppLifecycleEvent::WillEnterBackground; break;
        case SDL_EVENT_DID_ENTER_BACKGROUND: mapped = sf::AppLifecycleEvent::DidEnterBackground; break;
        case SDL_EVENT_WILL_ENTER_FOREGROUND: mapped = sf::AppLifecycleEvent::WillEnterForeground; break;
        case SDL_EVENT_DID_ENTER_FOREGROUND: mapped = sf::AppLifecycleEvent::DidEnterForeground; break;
        case SDL_EVENT_LOW_MEMORY: mapped = sf::AppLifecycleEvent::LowMemory; break;
        case SDL_EVENT_TERMINATING: mapped = sf::AppLifecycleEvent::Terminating; break;
        default: return true;
    }

    callback(mapped, lifecycleUserData.loadSeqCst());
    return true;
}
} // namespace


namespace sf
{
////////////////////////////////////////////////////////////
void setAppLifecycleCallback(const AppLifecycleCallback callback, void* const userData)
{
    lifecycleUserData.storeSeqCst(userData);
    lifecycleCallback.storeSeqCst(callback);
}


namespace priv
{
////////////////////////////////////////////////////////////
void installAppLifecycleWatch()
{
    (void)SDL_AddEventWatch(&lifecycleWatch, nullptr);
}


////////////////////////////////////////////////////////////
void removeAppLifecycleWatch()
{
    SDL_RemoveEventWatch(&lifecycleWatch, nullptr);
}

} // namespace priv
} // namespace sf
