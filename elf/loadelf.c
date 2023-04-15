#include <stdio.h>
#include <stdlib.h>
#include "elf.h"

int main(int argc, char *argv[])
{
    if(argc<2) {
        fprintf(stderr, "no file provided\n");
        return 1;
    }
    FILE *f = fopen(argv[1], "r");
    if(!f) perror(argv[1]);
    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buffer = (uint8_t*)malloc(size);
    fread(buffer, size, 1, f);
    fclose(f);
    
    Elf32_Ehdr *elf_hdr = (Elf32_Ehdr*)buffer;
    uint32_t entry = elf_hdr->e_entry;
    printf("entry: 0x%08x\n", entry);
    uint32_t phoff = elf_hdr->e_phoff;
    printf("phoff: %d\n", phoff);
    
    Elf32_Phdr *phdr = (Elf32_Phdr*)(buffer + phoff);
    
    for(int i = 0; i < elf_hdr->e_phnum; i++) {
        uint32_t vaddr = phdr[i].p_vaddr;
        uint32_t flags = phdr[i].p_flags;
        uint32_t file_offset = phdr[i].p_offset;
        uint32_t segment_size_file = phdr[i].p_filesz;
        uint32_t segment_size_mem = phdr[i].p_memsz;
        if(flags & PF_X) {
            printf("vaddr=0x%08x\n", vaddr);
            printf("size in file: %d\n", segment_size_file);
            printf("size in memory: %d\n", segment_size_mem);
            for(int j = 0; j < segment_size_file; j++) {
                printf("%02x ", buffer[file_offset+j]);
                if((j+1) % 0x10 == 0) printf("\n");
            }
            printf("\n");
        }
    }
    

    return 0;
}