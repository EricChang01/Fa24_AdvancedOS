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

    cout << "addr after argc: " << stack << "\n";

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
    cout << "Before argv: " << argvPtr << "\n";
    argvPtr -= (argc - 1);
    argv++;
    memcpy(argvPtr, argv, (argc) * sizeof(char*));

    cout << "After argv: " << argvPtr << "\n";

    uint64_t* argcPtr = (uint64_t*)argvPtr;

    argcPtr--;
    *argcPtr = (argc - 1);

    return argcPtr;
}

Elf64_auxv_t* copy_auxv(Elf64_auxv_t* auxvBeg, Elf64_auxv_t* auxvPtr, char* execFn, Elf64_Ehdr elf_header, Elf64_Addr intp_base_address, Elf64_Phdr* phdrs){
    int count = 0;
    Elf64_auxv_t* curr = auxvBeg;
    while(curr -> a_type != AT_NULL){
        count++;
        curr++;
    }
    count++;
    auxvPtr -= count;
    memcpy(auxvPtr, auxvBeg, count * sizeof(Elf64_auxv_t));
    Elf64_auxv_t* replace = auxvPtr;
    while(replace -> a_type != AT_NULL){
        switch(replace -> a_type){
            case AT_PHDR:
                // check here, address of the program header table in memory
                replace -> a_un.a_val = (uint64_t) phdrs;
                break;
            case AT_BASE:
                replace -> a_un.a_val = (uint64_t) intp_base_address;
                break;
            case AT_ENTRY:
                replace -> a_un.a_val = elf_header.e_entry;
                break;
            case AT_RANDOM:
                cout << "AT_RANDOM " << replace -> a_un.a_val << "\n";
                break;
            case AT_EXECFN:
                strcpy((char*)replace -> a_un.a_val, execFn);
                replace -> a_un.a_val = (uint64_t)replace -> a_un.a_val;
                break;
        }
        replace++;
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

uint64_t* build_stack(int argc, char* argv[], Elf64_Ehdr elf_header, Elf64_Addr intp_base_address, Elf64_Phdr* phdrs){
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

    Elf64_auxv_t* auxvPtr = copy_auxv((Elf64_auxv_t*)env, (Elf64_auxv_t*)stack, argv[1], elf_header, intp_base_address, phdrs);

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

void tranfer_control(uint64_t* stack, Elf64_Ehdr elf_header){
    cout << "entry point " << elf_header.e_entry << "\n";
    __asm__ volatile(
        "movq %0, %%rsp\n"           // Set stack pointer
        "xor %%rbp, %%rbp\n"         // Clear base pointer (optional)
        "jmp *%1\n"                  // Jump to entry point
        :
        : "r"(stack), "r"(elf_header.e_entry) // Inputs: new stack and entry point
        : "memory"                   // Clobbered: memory
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

void load(string filepath, int argc, char* argv[]){
    ifstream binary(filepath, std::ios::binary);
    if (!binary){
        fprintf(stderr, "Failed to open binary\n");
        return ;
    }

    Elf64_Ehdr elf_header;
    binary.read((char*)(&elf_header), sizeof(elf_header));
    if (strncmp((const char*)(elf_header.e_ident), ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "Invalid ELF file\n");
        return ;
    }

    binary.seekg(elf_header.e_phoff, std::ios::beg);
    Elf64_Phdr* phdrs = (Elf64_Phdr*)malloc(elf_header.e_phnum * sizeof(Elf64_Phdr));
    for(int i=0; i<elf_header.e_phnum; i++)
        binary.read(reinterpret_cast<char*>(&phdrs[i]), sizeof(Elf64_Phdr));

    // Allocate memory for the program
    void* entry_point = nullptr;
    Elf64_Addr intp_base_address; // base address of the intepreter

    for (int i=0; i<elf_header.e_phnum; i++) {
        Elf64_Phdr ph = phdrs[i];

        if (ph.p_type == PT_LOAD) { // loadable segment
            std::cout << "Type: " << ph.p_type << ", Offset: 0x" 
                << std::hex << ph.p_offset << ", VirtAddr: 0x" 
                << ph.p_vaddr << ", FileSize: 0x" 
                << ph.p_filesz << ", MemSize: 0x" 
                << ph.p_memsz << "\n";
            int section_type = 0;
            if(ph.p_filesz < ph.p_memsz){
                cout << "--- BSS section ---\n";
                section_type = 2; // bss
            } else if (ph.p_filesz == ph.p_memsz){
                if((ph.p_flags & (PF_X | PF_R)) == (PF_X | PF_R)){
                    section_type = 0; // text
                    cout << "--- Text section --- \n";
                } else {
                    section_type = 1; // data
                    cout << "--- Data section --- \n";
                }
            }

            void* segment;
            switch(section_type){
                case 0: // text
                cout << "virtual start point of text: " << ph.p_vaddr << "\n";
                segment = mmap((void*)(ph.p_vaddr), 
                            ph.p_memsz,
                            PROT_READ | PROT_WRITE | PROT_EXEC,
                            MAP_PRIVATE | MAP_ANONYMOUS,
                            -1, 0);
                cout << "address actually allocated: " << hex << segment << "\n";
                break;
                case 1: // data
                cout << "virtual start point of data: " << hex << ph.p_vaddr << "\n";
                segment = mmap((void*)(ph.p_vaddr), 
                            ph.p_memsz,
                            PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS,
                            -1, 0);
                cout << "address actually allocated: " << hex << segment << "\n";
                break;
                case 2: // bss
                cout << "virtual start point of bss: " << hex << ph.p_vaddr << "\n";
                segment = mmap((void*)(ph.p_vaddr), 
                            ph.p_memsz,
                            PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS,
                            -1, 0);
                cout << "address actually allocated: " << hex << segment << "\n";
                break;

            }      

            if (segment == MAP_FAILED) {
                perror("mmap failed");
                return ;
            }

            // Load segment data
            binary.seekg(ph.p_offset, std::ios::beg);
            binary.read((char*)(segment), ph.p_filesz);

            // Zero out the remaining part if needed
            if (ph.p_filesz < ph.p_memsz) {
                memset((char*)(segment) + ph.p_filesz, 0, ph.p_memsz - ph.p_filesz);
            }

        } else if (ph.p_type == PT_INTERP){
            intp_base_address = ph.p_vaddr;
            cout << "Interpreter base address " << hex << intp_base_address << "\n";
        }
    }
    uint64_t* stack = build_stack(argc, argv, elf_header, intp_base_address, phdrs);
    tranfer_control(stack, elf_header);
}

int main(int argc, char* argv[]){   
    string filepath = argv[1];
    load(filepath, argc, argv);
}