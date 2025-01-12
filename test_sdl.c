//
// Created by Admin on 2024/10/28.
//
#include <stdio.h>

#include "SDL2/SDL.h"

const int bpp=12;

int screen_w=640,screen_h=360;
const int pixel_w=640,pixel_h=360;

#undef main
int main(void)
{
    if(SDL_Init(SDL_INIT_VIDEO)) {
        printf( "Could not initialize SDL - %s\n", SDL_GetError());
    } else{
        printf("Success init SDL");
    }
    if(SDL_Init(SDL_INIT_VIDEO)) {
        printf( "Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }

    SDL_Window *screen;
    //SDL 2.0 Support for multiple windows
    screen = SDL_CreateWindow("Simplest Video Play SDL2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              screen_w, screen_h,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    if(!screen) {
        printf("SDL: could not create window - exiting:%s\n",SDL_GetError());
        return -1;
    }
    SDL_Renderer* sdlRenderer = SDL_CreateRenderer(screen, -1, 0);

    Uint32 pixformat=0;
    //IYUV: Y + U + V  (3 planes)
    //YV12: Y + V + U  (3 planes)
    pixformat= SDL_PIXELFORMAT_IYUV;

    SDL_Texture* sdlTexture = SDL_CreateTexture(sdlRenderer,pixformat, SDL_TEXTUREACCESS_STREAMING,pixel_w,pixel_h);

    FILE *fp=NULL;
    fp=fopen("sintel_640_360.yuv","rb+");

    if(fp==NULL){
        printf("cannot open this file\n");
        return -1;
    }

    SDL_Rect sdlRect;
    unsigned char buffer[pixel_w*pixel_h*bpp/8];

    while(1){
        if (fread(buffer, 1, pixel_w*pixel_h*bpp/8, fp) != pixel_w*pixel_h*bpp/8){
            // Loop
            fseek(fp, 0, SEEK_SET);
            fread(buffer, 1, pixel_w*pixel_h*bpp/8, fp);
        }

        SDL_UpdateTexture( sdlTexture, NULL, buffer, pixel_w);

        sdlRect.x = 0;
        sdlRect.y = 0;
        sdlRect.w = screen_w;
        sdlRect.h = screen_h;

        SDL_RenderClear( sdlRenderer );
        SDL_RenderCopy( sdlRenderer, sdlTexture, NULL, &sdlRect);
        SDL_RenderPresent( sdlRenderer );
        //Delay 40ms
        SDL_Delay(40);

    }
    SDL_Quit();
    return 0;
}