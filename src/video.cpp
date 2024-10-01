#include <SDL3/SDL.h>

#include "types.h"
#include "video.h"
#include "main.h"
#include "cpu/mem.h"
#include "dev.h"

regt VIDEO_MEM_OFFSET;

SDL_Window* ChasmWindow;
SDL_Renderer* ChasmRenderer;
// SDL_Surface* ChasmSurface;
SDL_Texture* ChasmTexture;
SDL_Surface* ChasmSurface2;
SDL_Texture* ChasmTexture2;

Uint32 EVENT_NEW_FRAME;
regt real_offset;

void video_call(memt device_number, regt* reg) {
    switch(reg[csrH]) {
        case 0: {
            octet* video_memory = mem_bank[device_number];
            
            regt vmem_offset = mem_to_reg64(*reinterpret_cast<regt*>(&video_memory[8]));
            /* ((regt)(video_memory[8]) << 56)
                | ((regt)(video_memory[9]) << 48)
                | ((regt)(video_memory[10]) << 40)
                | ((regt)(video_memory[11]) << 32)
                | ((regt)(video_memory[12]) << 24)
                | ((regt)(video_memory[13]) << 16)
                | ((regt)(video_memory[14]) << 8)
                | ((regt)(video_memory[15]) << 0); */

            if(verbosity)
                printf(" -- generating video frame from offset +%llx\n", vmem_offset);

            // dump_memory(VIDEO_MEM_OFFSET + (vmem_offset >> 1), 1000);

            // the blackest hell of bad ideas:
            real_offset = (((regt)video_memory) + vmem_offset);

            SDL_Event event;

            event.type = EVENT_NEW_FRAME;
            event.user.code = EVENT_NEW_FRAME;
            event.user.data1 = &real_offset;
            event.user.data2 = 0;
            SDL_PushEvent(&event);
            break;
        }
        
        case 1: { // copy/convert texture g to f
            
            break;
        }
            
        case 2: { // set pen and blending

            break;
        }
            
        case 3: { // get metrics

            break;
        }
            
        case 4: { // copy rect

            break;
        }

        case 5: { // draw line

            break;
        }

        case 6: { // outline rect
            
            break;
        }

        case 7: { // fill rect

            break;
        }
            
        case 8: { // draw lines

            break;
        }
            
        case 9: { // unused

            break;
        }

        case 10: { // outline polygon
            
            break;
        }

        case 11: { // fill polygon

            break;
        }

        case 12: { // draw ellipse

            break;
        }
            
        case 13: { // fill ellipse

            break;
        }
            
        case 14: { // draw bezier curve

            break;
        }

        case 15: { // fill bezier curve

            break;
        }
    }
}

int video(void *p) {
    SDL_Event event;

    EVENT_NEW_FRAME = SDL_RegisterEvents(1);


    if(!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 3;
    }

    if(!SDL_CreateWindowAndRenderer(
            "CHASM Capitoline ns86" VERSION,
            SCREEN_WIDTH,
            SCREEN_HEIGHT,
            SDL_WINDOW_KEYBOARD_GRABBED,
            &ChasmWindow, &ChasmRenderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 3;
    }
    /*
    ChasmSurface = SDL_CreateSurface(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_PIXELFORMAT_XRGB8888); // SDL_PIXELFORMAT_BGRX8888
    if(!ChasmSurface) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create surface from image: %s", SDL_GetError());
        return 3;
    }*/

    //ChasmTexture = SDL_CreateTextureFromSurface(ChasmRenderer, ChasmSurface);
    /*SDL_PropertiesID ChasmTextureProperties = SDL_CreateProperties();
    SDL_SetNumberProperty(ChasmTextureProperties, SDL_PROP_TEXTURE_WIDTH_NUMBER, SCREEN_WIDTH);
    SDL_SetNumberProperty(ChasmTextureProperties, SDL_PROP_TEXTURE_HEIGHT_NUMBER, SCREEN_HEIGHT);
    SDL_SetNumberProperty(ChasmTextureProperties,
    ChasmTexture = SDL_CreateTextureWithProperties(ChasmRenderer, ChasmTextureProperties); */
    ChasmTexture = SDL_CreateTexture(ChasmRenderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
    if(!ChasmTexture) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create texture: %s", SDL_GetError());
        return 3;
    }

    SDL_SetRenderDrawColor(ChasmRenderer, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(ChasmRenderer);
    SDL_RenderTexture(ChasmRenderer, ChasmTexture, NULL, NULL);
    SDL_RenderPresent(ChasmRenderer);
    /*
    ChasmSurface2 = SDL_CreateSurface(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_PIXELFORMAT_BGRX8888); // SDL_LoadBMP("test-image-2.bmp");
    ChasmTexture2 = SDL_CreateTextureFromSurface(ChasmRenderer, ChasmSurface2);
    */
    while(1) {
        SDL_PollEvent(&event);
        if(event.type == SDL_EVENT_QUIT) {
            break;
        } else if(event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            break;
        } else if(event.type == EVENT_NEW_FRAME) {
            if(event.user.code == EVENT_NEW_FRAME) {
                SDL_SetRenderDrawColor(ChasmRenderer, 0x00, 0x00, 0x00, 0x00);
                SDL_RenderClear(ChasmRenderer);
                void* pixels;
                int pitch;
                if(SDL_LockTexture(ChasmTexture, NULL, (void**)&pixels, &pitch)) {
                // *pixels = (void*)real_offset;
                    memcpy(pixels, (void*)real_offset, SCREEN_HEIGHT * pitch);
                    SDL_UnlockTexture(ChasmTexture);
                } else {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't lock texture: %s", SDL_GetError());
                }
                // SDL_UpdateTexture(ChasmTexture, NULL, (void*)real_offset, SCREEN_WIDTH * 4);
                SDL_RenderTexture(ChasmRenderer, ChasmTexture, NULL, NULL);
                SDL_RenderPresent(ChasmRenderer);
                
                /* SDL_DestroySurface(ChasmSurface);
                ChasmSurface = SDL_CreateSurfaceFrom(
                    SCREEN_WIDTH,
                    SCREEN_HEIGHT,
                    SDL_PIXELFORMAT_XRGB8888,
                    (void*)real_offset,
                    SCREEN_WIDTH * 4
                );
                if(ChasmSurface == NULL) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Surface creation failed: %s", SDL_GetError());
                }

                SDL_DestroyTexture(ChasmTexture);
                ChasmTexture = SDL_CreateTextureFromSurface(ChasmRenderer, ChasmSurface);
                if(!ChasmTexture) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create texture from surface: %s", SDL_GetError());
                }
                
                SDL_SetRenderDrawColor(ChasmRenderer, 0x00, 0x00, 0x00, 0x00);
                SDL_RenderClear(ChasmRenderer);
                SDL_RenderTexture(ChasmRenderer, ChasmTexture, NULL, NULL);
                SDL_RenderPresent(ChasmRenderer); */
            }
        }
    }

    // SDL_DestroyProperties(ChasmTextureProperties);
    SDL_DestroyTexture(ChasmTexture);
    SDL_DestroyRenderer(ChasmRenderer);
    // SDL_DestroySurface(ChasmSurface);
    SDL_DestroyWindow(ChasmWindow);
    
    SDL_Quit();

    exit(0);
}
