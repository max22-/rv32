#include <stdio.h>
#include <stdbool.h>
#include <SDL.h>
#include "vm.h"
#include "shared.h"
#include "macros.h"

uint16_t *pixels = NULL;
bool render = false;

int main(int argc, char *argv[]) {
    int res = 0;
    if(argc != 2) {
        fprintf(stderr, "usage: %s file.bin\n", argv[0]);
        return 1;
    }
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        ERROR("failed to initialize SDL : %s", SDL_GetError());
        return 1;
    }
    SDL_Window *window = SDL_CreateWindow(argv[0], SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if(!window) {
        ERROR("failed to create window : %s", SDL_GetError());
        res = 1;
        goto cleanup1;
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if(!renderer) {
        ERROR("failed to create renderer : %s", SDL_GetError());
        res = 1;
        goto cleanup2;
    }

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STATIC, SCREEN_WIDTH, SCREEN_HEIGHT);
    if(!texture) {
        ERROR("failed to create texture : %s", SDL_GetError());
        res = 1;
        goto cleanup3;
    }
    pixels = (uint16_t*)malloc(sizeof(uint16_t) * SCREEN_WIDTH * SCREEN_HEIGHT);
    if(!pixels) {
        ERROR("failed to allocate pixels memory");
        res = 1;
        goto cleanup4;
    }
    SDL_UpdateTexture(texture, NULL, pixels, sizeof(uint16_t));
    if(!vm_start(argv[1])) {
        ERROR("%s", vm_get_error());
        res = 1;
        goto cleanup5;
        return 1;
    }
    
    bool quit = false;
    while(!quit) {
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT)
                quit = true;
            else if(e.type == SDL_TEXTINPUT) {
                char *c;
                for(c = e.text.text; *c; c++)
                    printf("%c", *c);
                fflush(stdout);
            }
        }
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 0xff, 0x00, 0x00, 0xff);
        if(render) {
            SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * sizeof(uint16_t));
            render = false;
        }
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

cleanup5:
    free(pixels);
cleanup4:
    SDL_DestroyTexture(texture);
cleanup3:
    SDL_DestroyRenderer(renderer);
cleanup2:
    SDL_DestroyWindow(window);
cleanup1:
    SDL_Quit();
    return res;
}