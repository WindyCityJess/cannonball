/***************************************************************************
    CannonBall on the MiSTer hybrid core: the frame straight into the window
    surface (2026-11-25, in place of the SDL_Renderer path of rendersurface.cpp).

    The hybrid SDL has a software renderer only, and the core shows the picture
    at 320x240 on a 15kHz screen: with the renderer the 320x224 frame went
    through a 32-bit surface, a streaming texture, a clear, a scaled copy into
    the RGB565 window surface (640x448 at the config's window scale 2, which
    put the core in its 640x480 mode) and the renderer's simulated vsync, and
    the frame loop fell behind its 60 frames a second within minutes (the
    pacing log: 299 of 300 frames late; sound stuttering, video like
    screenshots). Here:
      - the window is always 320x240 windowed, whatever config.xml's mode and
        scale: the core's smallest 4:3 mode, the 224 rows of the frame centred
      - the window surface is the driver's RGB565 buffer, with SDL_MISTER_DIRECT=1
        the library's own frame buffer (no copy at all); draw_frame() writes the
        palette lookup into it, converting the table's 0RGB to 565 on the way
      - SDL_UpdateWindowSurface() hands the frame to the core; with
        SDL_MISTER_VSYNC=1 (the set's eduke32_env.sh) the driver waits for the
        core's vblank there, and supports_vsync() says so, so the game skips its
        own SDL_Delay pacing and its audio-gap speed adjustment: one frame per
        field (59.64 a second), the way the hybrid README asks for
    Scanlines and the full-screen modes are not drawn: the CRT does that.

    Copyright Manuel Alfayate, Chris White (the original). See license.txt.
***************************************************************************/

#include <iostream>

#include "rendersurface.hpp"
#include "frontend/config.hpp"

static const int MISTER_W = 320, MISTER_H = 240;

Render::Render(void)
{
    window = NULL;
    renderer = NULL;
    texture = NULL;
}

Render::~Render(void)
{
}

bool Render::init(int src_width, int src_height,
                    int scale,
                    int video_mode,
                    int scanlines)
{
    this->src_width  = src_width;
    this->src_height = src_height;
    this->scale      = 1;
    this->video_mode = video_settings_t::MODE_WINDOW;
    this->scanlines  = 0;
    (void)scale; (void)video_mode; (void)scanlines;

    if (!RenderBase::sdl_screen_size())
        return false;

    // hires (640x448) takes the core's 640x480 mode; everything else the 320x240 one (widescreen's 398
    // columns are cropped to 320: the screen is 4:3)
    scn_width  = src_height > MISTER_H ? MISTER_W * 2 : MISTER_W;
    scn_height = src_height > MISTER_H ? MISTER_H * 2 : MISTER_H;
    if (window)
    {
        int ww = 0, wh = 0;
        SDL_GetWindowSize(window, &ww, &wh);
        if (ww != scn_width || wh != scn_height)
            SDL_SetWindowSize(window, scn_width, scn_height);
    }
    src_rect.x = src_rect.y = 0;
    src_rect.w = src_width;
    src_rect.h = src_height;
    dst_rect.w = src_width  < scn_width  ? src_width  : scn_width;
    dst_rect.h = src_height < scn_height ? src_height : scn_height;
    dst_rect.x = (scn_width  - dst_rect.w) / 2;
    dst_rect.y = (scn_height - dst_rect.h) / 2;

    // the frame's own copy is not needed: the lookup goes straight into the window surface
    if (surface)
    {
        SDL_FreeSurface(surface);
        surface = NULL;
    }
    screen_pixels = NULL;

    if (!window)
    {
        window = SDL_CreateWindow("Cannonball", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  scn_width, scn_height, SDL_WINDOW_SHOWN);
        if (!window)
        {
            std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
            return false;
        }
    }
    SDL_ShowCursor(false);

    // the palette table in 0RGB (convert_palette() shifts 8-bit components); draw_frame() makes 565 of it
    Rshift = 16; Gshift = 8; Bshift = 0;
    Rmask = 0xff0000; Gmask = 0x00ff00; Bmask = 0x0000ff;
    return true;
}

void Render::disable()
{
    // the window stays: a settings change re-inits into the same one
}

bool Render::start_frame()
{
    return true;
}

bool Render::finalize_frame()
{
    SDL_UpdateWindowSurface(window);
    return true;
}

void Render::draw_frame(uint16_t* pixels)
{
    SDL_Surface* win = SDL_GetWindowSurface(window);
    if (!win || !win->pixels)
        return;
    const int w = dst_rect.w, h = dst_rect.h;
    const int skip = src_width - w;               // a wider frame (widescreen) shows its middle
    pixels += (src_height - h) / 2 * src_width + skip / 2;
    for (int y = 0; y < h; y++)
    {
        uint16_t* d = (uint16_t*)((uint8_t*)win->pixels + (dst_rect.y + y) * win->pitch) + dst_rect.x;
        for (int x = 0; x < w; x++)
        {
            const uint32_t c = rgb[*pixels++];
            *d++ = (uint16_t)(((c >> 8) & 0xf800) | ((c >> 5) & 0x07e0) | ((c >> 3) & 0x001f));
        }
        pixels += skip;
    }
}
