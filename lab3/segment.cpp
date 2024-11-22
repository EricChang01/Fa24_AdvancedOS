#include <iostream>
#include <cstring>
#include <unistd.h>
#include "segment.hpp"

#define PAGE_SIZE 4096

using namespace std;

void Segment::mmap_segment_phdr(ifstream &file, Elf64_Phdr *elf_phdr) {
    this->phdr = elf_phdr;
    // bss can have unaligned address
    uint64_t start_addr = phdr->p_vaddr & 0xFFFFF000; // page size = 4096 bytes
    uint64_t diff = phdr->p_vaddr -start_addr;

    this->addr = mmap(reinterpret_cast<void *>(start_addr), 
                    phdr->p_memsz + diff,
                    PROT_READ | PROT_WRITE | PROT_EXEC,
                    MAP_PRIVATE | MAP_ANONYMOUS,
                    -1, 0);

    this->mem_size = phdr->p_memsz + diff;
    file.seekg(phdr->p_offset, std::ios::beg);
    file.read(reinterpret_cast<char*>(phdr->p_vaddr), phdr->p_filesz);

    if (phdr->p_filesz < phdr->p_memsz) { // bss
        memset(reinterpret_cast<char *>(phdr->p_vaddr + phdr->p_filesz), 0, phdr->p_memsz - phdr->p_filesz);
    }
}