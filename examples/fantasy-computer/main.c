#include <stdio.h>
#include <stdbool.h>
#include <SDL.h>
#include "macros.h"

#define WIDTH 320
#define HEIGHT 240

int main(int argc, char *argv[]) {
    if(argc != 2) {
        fprintf(stderr, "usage: %s file.bin\n", argv[0]);
        return 1;
    }
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        ERROR("failed to initialize SDL : %s", SDL_GetError());
        return 1;
    }
    SDL_Window *window = SDL_CreateWindow(argv[0], SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    if(!window) {
        ERROR("failed to create window : %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if(!renderer) {
        ERROR("failed to create renderer : %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Rect rect = {.x = 10, .y = 10, .w = 100, .h = 50};

    bool quit = false;
    while(!quit) {
        SDL_Event e;
        SDL_WaitEvent(&e);
        if(e.type == SDL_QUIT)
            quit = true;
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 0xff, 0x00, 0x00, 0xff);
        SDL_RenderFillRect(renderer, &rect);
        SDL_RenderPresent(renderer);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}