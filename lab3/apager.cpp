#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <elf.h>
#include <vector>
#include <sys/mman.h>
using namespace std;

#include <stdlib.h>
#include <stdio.h>
#include <elf.h>
#include <assert.h>
#include <string.h>

extern char** environ;

void stack_check(void* top_of_stack, uint64_t argc, char** argv) {
	printf("----- stack check -----\n");

	assert(((uint64_t)top_of_stack) % 8 == 0);
	printf("top of stack is 8-byte aligned\n");

	uint64_t* stack = (uint64_t*)top_of_stack;
	uint64_t actual_argc = *(stack++);
	printf("argc: %lu\n", actual_argc);
	assert(actual_argc == argc);

	for (int i = 0; i < argc; i++) {
		char* argp = (char*)*(stack++);
        // printf("argp = %s, argv[i] = %s\n", argp, argv[i]);
		assert(strcmp(argp, argv[i]) == 0);
		printf("arg %d: %s\n", i, argp);
	}
	// Argument list ends with null pointer
	assert(*(stack++) == 0);

	int envp_count = 0;
	while (*(stack++) != 0)
		envp_count++;

	printf("env count: %d\n", envp_count);

	Elf64_auxv_t* auxv_start = (Elf64_auxv_t*)stack;
	Elf64_auxv_t* auxv_null = auxv_start;
	while (auxv_null->a_type != AT_NULL) {
		auxv_null++;
	}
	printf("aux count: %lu\n", auxv_null - auxv_start);
	printf("----- end stack check -----\n");
}

uint64_t* copy_args (char** argvPtr, int argc, char* argv[]) {
    argvPtr -= (argc - 1);
    argv++;
    memcpy(argvPtr, argv, (argc) * sizeof(char*));

    uint64_t* argcPtr = (uint64_t*)argvPtr;

    argcPtr--;
    *argcPtr = (argc - 1);

    return argcPtr;
}

Elf64_auxv_t* copy_auxv(Elf64_auxv_t* auxvBeg, Elf64_auxv_t* auxvPtr, char* fileName, Elf64_Ehdr elf_header, Elf64_Phdr* phdrs){
    int count = 0;
    Elf64_auxv_t* curr = auxvBeg;
    while(curr -> a_type != AT_NULL){
        count++;
        curr++;
    }
    auxvPtr -= count;
    memcpy(auxvPtr, auxvBeg, (count+1) * sizeof(Elf64_auxv_t));

    Elf64_auxv_t* auxv = auxvPtr;
    while (auxv -> a_type != AT_NULL) {
        switch(auxv -> a_type){
            case AT_PHDR:
                auxv -> a_un.a_val = (uint64_t) phdrs;
                break;
            case AT_BASE:
                auxv -> a_un.a_val = 0;
                break;
            case AT_ENTRY:
                auxv -> a_un.a_val = elf_header.e_entry;
                break;
            case AT_PHNUM:
                auxv -> a_un.a_val = elf_header.e_phnum;
                break;
            case AT_EXECFN:
                auxv -> a_un.a_val = (uint64_t) fileName;
                break;
        }
        auxv++;
    }
    return auxvPtr;

}

char** copy_envp(char** envpPtr) {
    char** begin = environ;
    int count = 0;
    for (char** env = environ; *env != NULL; ++env) {
        count++;
    }
    envpPtr -= count;
    memcpy(envpPtr, environ, count * sizeof(char*));
    return envpPtr;
} 

uint64_t* build_stack(int argc, char* argv[], Elf64_Ehdr elf_header, Elf64_Phdr* phdrs){
    // allocate stack
    size_t size = 8 * 1024 * 1024;
    void* stack = malloc(size);
    if (stack == nullptr) {
        fprintf(stderr, "stack allocation fails\n");
        return NULL;
    }
    
#ifdef StackBuild
    cout << "Beg: " << stack << "\n";
#endif

// move to highest address
stack = (void*) (reinterpret_cast<unsigned char*>(stack) + size);

#ifdef StackBuild
    cout << "Adj: " << stack << "\n";
#endif

    char** env = environ;
    size_t total_size = 0;
    for (env = environ; *env != NULL; ++env) {
        // iterate through
    }
    env++; // start of auxv

    Elf64_auxv_t* auxvPtr = copy_auxv((Elf64_auxv_t*)env, (Elf64_auxv_t*)stack, argv[1], elf_header, phdrs);

#ifdef StackBuild
    cout << "Auxv: " << auxvPtr << "\n";
#endif

    char** envpPtr = copy_envp((char**) auxvPtr);

#ifdef StackBuild
    cout << "Envp: " << envpPtr << "\n";
#endif

    uint64_t* argcPtr = copy_args((char**) envpPtr, argc, argv);

#ifdef StackBuild
    cout << "Args: " << argcPtr << "\n";
    cout << "Argc = " << *argcPtr << "\n";
#endif
    stack_check(argcPtr, argc - 1, ++argv);
    return argcPtr;
}

void transfer_control(void* stack, Elf64_Ehdr elf_header){
    cout << "entry point " << elf_header.e_entry << "\n";
    cout << "stack " << stack << "\n";
    __asm__ volatile (
        "mov %0, %%rsp\n\t"           // Set stack pointer

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
        : "r"((void*)stack), "r"(reinterpret_cast<void*>(elf_header.e_entry)) // Inputs: stack pointer and entry point
        : "memory"
    );

}

/** ELF header
    unsigned char e_ident[16];  // ELF identification
    uint16_t e_type;             // Type of the file (e.g., executable)
    uint16_t e_machine;          // Architecture
    uint32_t e_version;          // ELF version
    uint64_t e_entry;            // Entry point address
    uint64_t e_phoff;            // Program header table offset
    uint64_t e_shoff;            // Section header table offset
    uint32_t e_flags;            // Processor-specific flags
    uint16_t e_ehsize;           // ELF header size
    uint16_t e_phentsize;        // Size of program header entry
    uint16_t e_phnum;            // Number of program header entries
    uint16_t e_shentsize;        // Size of section header entry
    uint16_t e_shnum;            // Number of section header entries
    uint16_t e_shstrndx;         // Section name string table index
*/

Elf64_Ehdr read_elf(ifstream& binary) {
    Elf64_Ehdr elf_header;
    binary.read(reinterpret_cast<char *>(&elf_header), sizeof(elf_header));
    return elf_header;
}

Elf64_Phdr* read_program_headers(ifstream& binary, Elf64_Ehdr& elf_header) {
    Elf64_Phdr* phdrs = (Elf64_Phdr*)malloc(elf_header.e_phnum * sizeof(Elf64_Phdr));
    binary.seekg(elf_header.e_phoff, std::ios::beg);
    for(int i=0; i<elf_header.e_phnum; i++)
        binary.read(reinterpret_cast<char*>(&phdrs[i]), sizeof(Elf64_Phdr));

    return phdrs;
}

int main(int argc, char* argv[]){   
    string filepath = argv[1];
    ifstream binary(filepath, std::ios::binary);
    if (!binary){
        fprintf(stderr, "Failed to open binary\n");
        return 0;
    }
    Elf64_Ehdr elf_header = read_elf(binary);
    Elf64_Phdr* phdrs = read_program_headers(binary, elf_header);

    // Allocate memory for the program
    void* text_segment, *bss_segment, *ro_data_segment, *rw_data_segment;
    for (int i=0; i<elf_header.e_phnum; i++) {
        Elf64_Phdr ph = phdrs[i];

        if(ph.p_type != PT_LOAD)
            continue;

        if (ph.p_flags & PF_X) { // text section
            cout << "TEXT v_addr: " << ph.p_vaddr << "\n";
            text_segment = mmap((void*)(ph.p_vaddr), 
                        ph.p_memsz,
                        PROT_READ | PROT_WRITE | PROT_EXEC,
                        MAP_PRIVATE | MAP_ANONYMOUS,
                        -1, 0);

            binary.seekg(ph.p_offset, std::ios::beg);
            binary.read((char*)(text_segment), ph.p_filesz);
            cout << "TEXT mmap: " << hex << text_segment << "\n";
        } else if (ph.p_flags & (PF_X)) {
            if (ph.p_filesz < ph.p_memsz) { // bss section
                cout << "BSS v_addr: " << hex << ph.p_vaddr << "\n";
                cout << "BSS new: " << hex << (ph.p_vaddr & 0xFFFFF000) << "\n";
                uint64_t diff =  (uint64_t)ph.p_vaddr - (uint64_t)(ph.p_vaddr & 0xFFFFF000);
                bss_segment = mmap((void*)(ph.p_vaddr), 
                            ph.p_memsz + diff,
                            PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS,
                            -1, 0);
                binary.seekg(ph.p_offset, std::ios::beg);
                binary.read((char*)(ph.p_vaddr), ph.p_filesz);
                memset((char*)(ph.p_vaddr) + ph.p_filesz, 0, ph.p_memsz - ph.p_filesz);
                cout << "BSS mmap: " << hex << bss_segment << "\n";
            } else { // data section
                cout << "DATA v_addr: " << hex << ph.p_vaddr << "\n";
                rw_data_segment = mmap((void*)(ph.p_vaddr), 
                            ph.p_memsz,
                            PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS,
                            -1, 0);
                binary.seekg(ph.p_offset, std::ios::beg);
                binary.read((char*)(rw_data_segment), ph.p_filesz);
                cout << "DATA mmap: " << hex << rw_data_segment << "\n";
            }
        } else if (ph.p_flags & (PF_R)) {
            cout << "DATA v_addr: " << hex << ph.p_vaddr << "\n";
                ro_data_segment = mmap((void*)(ph.p_vaddr), 
                            ph.p_memsz,
                            PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS,
                            -1, 0);
                binary.seekg(ph.p_offset, std::ios::beg);
                binary.read((char*)(ro_data_segment), ph.p_filesz);
                cout << "DATA mmap: " << hex << ro_data_segment << "\n";
        }   
    }
    uint64_t* stack = build_stack(argc, argv, elf_header, phdrs);
    transfer_control((void*)(stack), elf_header);
}