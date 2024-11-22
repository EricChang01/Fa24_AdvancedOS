#ifndef LAB3_PAGER
#define LAB3_PAGER
#include <elf.h>

#include "segment.hpp"
#include "stack.hpp"

using namespace std;
class Pager {
public:
    Pagers type;
    Stack stack;
    Segment bss;
    Segment data;
    Segment rodata;
    Segment text;
    Elf64_Ehdr elfhdr;
    Elf64_Phdr *phdrs;

    void mmap_segments(ifstream &file);

    Pager(Pagers type){
        stack.type = type;
        bss.type = type;
        data.type = type;
        rodata.type = type;
        text.type = type;
    }

    ~Pager() {
        free(phdrs);
    }
};

#endif