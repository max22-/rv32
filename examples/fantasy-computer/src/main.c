#include <stdio.h>
#include <stdbool.h>
#include <SDL.h>
#include <time.h>
#define RV32_IMPLEMENTATION
#define FANTASY_COMPUTER_IMPLEMENTATION
#include "fantasy_computer.h"
#include "shared.h"
#include "macros.h"

bool render = false;
uint16_t *pixels = NULL;

const unsigned int FPS = 60;
const size_t ram_size = 0x10000; /* 64k */

int main(int argc, char *argv[]) {
    int res = 0;
    if(argc != 2) {
        fprintf(stderr, "usage: %s file.bin\n", argv[0]);
        return 1;
    }

    const char *file_path = argv[1];
    uint8_t *memory = (uint8_t*)malloc(RV32_NEEDED_MEMORY(ram_size));
    if(!memory) {
        ERROR("failed to allocate memory");
        return 1;
    }

    RV32 *rv32 = rv32_new(memory, ram_size);
    if(!rv32) {
        ERROR("failed to create VM (not enough memory)");
        return 0;
    }

    FILE *f = fopen(file_path, "r");
    if(!f) {
        ERROR("failed to open %s", file_path);
        return 0;
    }

    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);

    if(file_size > ram_size) {
        free(rv32);
        fclose(f);
        ERROR("program too big");
        return 0;
    }

    fseek(f, 0, SEEK_SET);
    size_t n = fread(rv32->mem, file_size, 1, f);
    fclose(f);

    if(n != 1) {
        free(rv32);
        ERROR("failed to read program");
        return 0;
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
    
    bool quit = false;
    float frequency = SDL_GetPerformanceFrequency();
    while(!quit) {
        clock_t now = clock();
        uint64_t t_start = SDL_GetPerformanceCounter();
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT)
                quit = true;
            else if(e.type == SDL_TEXTINPUT) {
                char *c;
                for(c = e.text.text; *c; c++)
                    printf("%c", *c);
                fflush(stdout);
            } else if(e.type == SDL_KEYDOWN) {
                CSR_SET_BIT(rv32->mip, CSR_MIP_MEIP, 1); // trigger an interrupt
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
        do {
            rv32_cycle(rv32);
            switch(rv32->status) {
            case RV32_RUNNING:
                break;
            case RV32_HALTED:
                quit = true;
                printf("cpu halted.\n");
                printf("exit status: %d\n", rv32->r[REG_A0]);
                goto considered_harmful;
                break;
            case RV32_EBREAK:
                fprintf(stderr, "ebreak at pc=%08x\n", rv32->pc);
                break;
            default:
                fprintf(stderr, "Error %d at pc=%08x\n", rv32->status, rv32->pc);
                fprintf(stderr, "instr = %08x\n", *(uint32_t *)&rv32->mem[rv32->pc]);
                quit = true;
            }
        } while(!quit && (SDL_GetPerformanceCounter() - t_start) / frequency < 1 / (float)FPS);
        considered_harmful:
        //printf("%ld\n", clock() - now);
    }

    free(pixels);
cleanup4:
    SDL_DestroyTexture(texture);
cleanup3:
    SDL_DestroyRenderer(renderer);
cleanup2:
    SDL_DestroyWindow(window);
cleanup1:
    SDL_Quit();
    free(rv32);
    return res;
}