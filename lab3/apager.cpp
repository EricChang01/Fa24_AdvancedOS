#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <elf.h>    // Use system-provided ELF header definitions
#include <unistd.h>
#include <sys/auxv.h>
#include <assert.h>

#include "segment.hpp"
#include "pager.hpp"
#include "stack.hpp"

using namespace std;

void transfer_control(void* stack, void* entry_point) {
    cout << "entry point " << entry_point << "\n";
    cout << "stack " << stack << "\n";
    asm volatile (
        "mov %0, %%rsp\n\t"

        "push %1\n\t"

        "xor %%rax, %%rax\n\t"         // Clear RAX
        "xor %%rbx, %%rbx\n\t"         // Clear RBX
        "xor %%rcx, %%rcx\n\t"         // Clear RCX
        "xor %%rdx, %%rdx\n\t"         // Clear RDX
        "xor %%rsi, %%rsi\n\t"         // Clear RSI
        "xor %%rdi, %%rdi\n\t"         // Clear RDI
        "xor %%r8, %%r8\n\t"           // Clear R8
        "xor %%r9, %%r9\n\t"           // Clear R9
        "xor %%r10, %%r10\n\t"         // Clear R10
        "xor %%r11, %%r11\n\t"         // Clear R11
        "xor %%r12, %%r12\n\t"         // Clear R12
        "xor %%r13, %%r13\n\t"         // Clear R13
        "xor %%r14, %%r14\n\t"         // Clear R14
        "xor %%r15, %%r15\n\t"         // Clear R15

        "ret\n\t"                    // Jump to entry point
        :
        : "r"(stack), "r"(entry_point) // Inputs: stack pointer and entry point
        : "memory"
    );
}

extern char **environ;

int main(int argc, char **argv) {
    string filepath = argv[1];
    ifstream binary(filepath, std::ios::binary);

    Pager pager(Pagers::APAGER);

    // Read ELF header
    Elf64_Ehdr elf_header;
    binary.read(reinterpret_cast<char *>(&elf_header), sizeof(elf_header));
    pager.elfhdr = elf_header;

    // Validate ELF magic number
    if (memcmp(pager.elfhdr.e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "Incorrect ELF Magic Number");
    }

    pager.mmap_segments(binary);

    Elf64_auxv_t* auxv = pager.stack.find_auxv_entry((void*)(argv-1), argc, argv);

    pager.stack.build_stack(&argv[1], environ, auxv, pager);

    pager.stack.stack_check(pager.stack.entry_point, argc-1, &argv[1]);

    transfer_control(pager.stack.entry_point, (void*)pager.elfhdr.e_entry);

    // free(auxv);

    return 0;
}