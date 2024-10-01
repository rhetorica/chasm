#ifndef VIDEO_H
#define VIDEO_H

#include "cpu/cpu.h"
#include "SDL3/SDL.h"

void video_call(memt, regt*);

int video(void *p);

extern SDL_Window* ChasmWindow;
extern SDL_Renderer* ChasmRenderer;
extern SDL_Surface* ChasmSurface;
extern SDL_Texture* ChasmTexture;

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 1024
extern regt VIDEO_MEM_OFFSET;
#define VIDEO_MEM_DEFAULT_FRAME_START (regt)0x100000

// video memory size in bytes:
#define VIDEO_MEM_SIZE (32 * 1024 * 1024)

#endif // VIDEO_H
