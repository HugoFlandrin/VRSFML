#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/VRSFML/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Config.hpp"

#ifdef SFML_SYSTEM_ANDROID

    #include "SFML/Base/SizeT.hpp"

    #include <cstdio>


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
struct SDL_IOStream;


namespace sf::priv
{
////////////////////////////////////////////////////////////
/// \brief A file opened through SDL: an APK asset or a regular file
///
/// The assets packaged in the APK are not files on disk, so the usual `open`/`fopen` cannot read them.
/// `SDL_IOFromFile` reads them with the Android asset manager (relative paths such as "ships.png") and
/// falls back to regular files otherwise.
///
////////////////////////////////////////////////////////////
struct AndroidFile
{
    SDL_IOStream* io{nullptr};
    base::SizeT   size{0u};
};


////////////////////////////////////////////////////////////
/// \brief Open `path` for reading (APK asset first, then regular file); `false` if it cannot be opened
///
////////////////////////////////////////////////////////////
[[nodiscard]] bool androidOpenFile(const char* path, AndroidFile& file);

////////////////////////////////////////////////////////////
/// \brief Read up to `size` bytes; returns the number of bytes actually read
///
////////////////////////////////////////////////////////////
[[nodiscard]] base::SizeT androidReadFile(AndroidFile& file, void* data, base::SizeT size);

////////////////////////////////////////////////////////////
/// \brief Close a file opened with `androidOpenFile`
///
////////////////////////////////////////////////////////////
void androidCloseFile(AndroidFile& file);

////////////////////////////////////////////////////////////
/// \brief Open `path` as a read-only C `FILE*` (APK asset first, then regular file); `nullptr` on failure
///
/// The returned stream owns the underlying file: `std::fclose` releases it.
///
////////////////////////////////////////////////////////////
[[nodiscard]] std::FILE* androidOpenStdio(const char* path);

} // namespace sf::priv

#endif
