#ifndef LAB3_STACK
#define LAB3_STACK

#include <sys/auxv.h>

#include "segment.hpp"

class Pager;

class Stack {
    public:
        void* addr;
        void* entry_point;
        Elf64_Phdr *phdr;
        Pagers type;

        void build_stack(char **argv, char **envp, Elf64_auxv_t *auxv, Pager& pager);
        void stack_check(void* top_of_stack, uint64_t argc, char** argv);
        Elf64_auxv_t* find_auxv_entry(void* top_of_stack, int argc, char **argv);
};
#endif