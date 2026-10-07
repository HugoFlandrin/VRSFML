#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/VRSFML/blob/master/license.md


namespace sf::priv
{
////////////////////////////////////////////////////////////
/// \brief Register / unregister the SDL event watch that feeds `sf::setAppLifecycleCallback`
///
/// Called by `SDLLayer` right after SDL's video subsystem is initialized / right before it is shut down.
///
////////////////////////////////////////////////////////////
void installAppLifecycleWatch();
void removeAppLifecycleWatch();

} // namespace sf::priv
