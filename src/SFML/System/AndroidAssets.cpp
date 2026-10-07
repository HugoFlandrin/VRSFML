// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/VRSFML/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/System/AndroidAssets.hpp"

#ifdef SFML_SYSTEM_ANDROID

    #include <SDL3/SDL_iostream.h>

    #include <cstdio>


namespace sf::priv
{
////////////////////////////////////////////////////////////
bool androidOpenFile(const char* const path, AndroidFile& file)
{
    file = {};

    SDL_IOStream* const io = SDL_IOFromFile(path, "rb");

    if (io == nullptr)
        return false;

    const Sint64 size = SDL_GetIOSize(io);

    if (size < 0)
    {
        SDL_CloseIO(io);
        return false;
    }

    file.io   = io;
    file.size = static_cast<base::SizeT>(size);
    return true;
}


////////////////////////////////////////////////////////////
base::SizeT androidReadFile(AndroidFile& file, void* const data, const base::SizeT size)
{
    return static_cast<base::SizeT>(SDL_ReadIO(file.io, data, size));
}


////////////////////////////////////////////////////////////
void androidCloseFile(AndroidFile& file)
{
    if (file.io != nullptr)
        SDL_CloseIO(file.io);

    file = {};
}


////////////////////////////////////////////////////////////
namespace
{
int stdioRead(void* const cookie, char* const buffer, const int size)
{
    auto* const io = static_cast<SDL_IOStream*>(cookie);

    const size_t count = SDL_ReadIO(io, buffer, static_cast<size_t>(size));

    if (count == 0u && SDL_GetIOStatus(io) == SDL_IO_STATUS_ERROR)
        return -1;

    return static_cast<int>(count);
}


off64_t stdioSeek(void* const cookie, const off64_t offset, const int whence)
{
    SDL_IOWhence sdlWhence = SDL_IO_SEEK_SET;

    switch (whence)
    {
        case SEEK_SET: sdlWhence = SDL_IO_SEEK_SET; break;
        case SEEK_CUR: sdlWhence = SDL_IO_SEEK_CUR; break;
        case SEEK_END: sdlWhence = SDL_IO_SEEK_END; break;
        default: return -1;
    }

    return static_cast<off64_t>(SDL_SeekIO(static_cast<SDL_IOStream*>(cookie), static_cast<Sint64>(offset), sdlWhence));
}


int stdioClose(void* const cookie)
{
    return SDL_CloseIO(static_cast<SDL_IOStream*>(cookie)) ? 0 : -1;
}
} // namespace


////////////////////////////////////////////////////////////
std::FILE* androidOpenStdio(const char* const path)
{
    SDL_IOStream* const io = SDL_IOFromFile(path, "rb");

    if (io == nullptr)
        return nullptr;

    // bionic's `funopen64` builds a `FILE*` on top of user callbacks (`fread`/`fseek`/`ftell`/`fclose` then work)
    std::FILE* const file = funopen64(io, &stdioRead, nullptr, &stdioSeek, &stdioClose);

    if (file == nullptr)
        SDL_CloseIO(io);

    return file;
}

} // namespace sf::priv

#endif
