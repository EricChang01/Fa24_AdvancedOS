#include <vector>
#include <string.h>
#include <iostream>

#include "pager.hpp"
#include "stack.hpp"

using namespace std;

void Pager::mmap_segments(ifstream &binary)
{
    this->phdrs = (Elf64_Phdr*)malloc(elfhdr.e_phnum * sizeof(Elf64_Phdr));
    binary.seekg(elfhdr.e_phoff, std::ios::beg);
    for (int i=0; i<elfhdr.e_phnum; i++) {
        binary.read(reinterpret_cast<char*>(&phdrs[i]), sizeof(Elf64_Phdr));
    }

    for (int i=0; i<elfhdr.e_phnum; i++) {
        Elf64_Phdr ph = this->phdrs[i];

        if (ph.p_type == PT_LOAD) { // Only process loadable segments 
            if (ph.p_flags & PF_X) { // text segment
                this->text.mmap_segment_phdr(binary, &ph);
                this->elfhdr.e_entry = (uint64_t)text.addr + (this->elfhdr.e_entry - ph.p_vaddr);
            } 

            if (ph.p_flags & (PF_R | PF_W)) { // bss or data
                if (ph.p_filesz < ph.p_memsz) { // bss segment
                    this->bss.mmap_segment_phdr(binary, &ph);
                } else { // data segment
                    this->data.mmap_segment_phdr(binary, &ph);
                }
            }

            if (ph.p_flags & PF_R) { // rodata segment
                this->rodata.mmap_segment_phdr(binary, &ph);
            }
        }
    }   
}