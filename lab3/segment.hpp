#ifndef LAB3_SEGMENT
#define LAB3_SEGMENT

#include <cstddef>
#include <fstream>

#include <sys/mman.h>
#include <elf.h>
#include <sys/auxv.h>

using namespace std;

enum class Pagers {
    APAGER,
    DPAGER,
    HPAGER,
};

class Segment {
public:
    void* addr;
    size_t mem_size;
    void* base_addr;
    
    Elf64_Phdr *phdr;
    Pagers type;

    void mmap_segment_phdr(ifstream &file, Elf64_Phdr *phdr);

    ~Segment() {
        if (this->addr != nullptr && this->addr != MAP_FAILED) {
            munmap(this->addr, this->mem_size);
        }
    }
};

#endif