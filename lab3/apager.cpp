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

Elf64_auxv_t* copy_auxv(Elf64_auxv_t* auxvBeg, Elf64_auxv_t* auxvPtr){
    int count = 0;
    Elf64_auxv_t* curr = auxvBeg;
    while(curr -> a_type != AT_NULL){
        count++;
        curr++;
    }
    count++;
    // cout << "auxv count: " << count << "\n";
    // cout << "Bef move: " << auxvPtr << "\n";
    auxvPtr -= count;
    // cout << "Aft move: " << auxvPtr << "\n";
    memcpy(auxvPtr, auxvBeg, count * sizeof(Elf64_auxv_t));
    // cout << "Aft copy: " << auxvPtr << "\n";
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

void build_stack(int argc, char* argv[]){
    // allocate stack
    size_t size = 8 * 1024 * 1024;
    void* stack = malloc(size);
    if (stack == nullptr) {
        fprintf(stderr, "stack allocation fails\n");
        return ;
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

    Elf64_auxv_t* auxvPtr = copy_auxv((Elf64_auxv_t*)env, (Elf64_auxv_t*)stack);

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
}

void load_and_execute(string filepath, int argc, char* argv[]){
    ifstream binary(filepath, std::ios::binary);
    if (!binary){
        fprintf(stderr, "Failed to open binary\n");
        return;
    }

    Elf64_Ehdr elf_header;
    binary.read((char*)(&elf_header), sizeof(elf_header));
    if (strncmp((const char*)(elf_header.e_ident), ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "Invalid ELF file\n");
        return;
    }

    binary.seekg(elf_header.e_phoff, std::ios::beg);
    vector<Elf64_Phdr> phdrs(elf_header.e_phnum); // program headers
    for(int i=0; i<elf_header.e_phnum; i++)
        binary.read(reinterpret_cast<char*>(&phdrs[i]), sizeof(Elf64_Phdr));

    // Allocate memory for the program
    void* entry_point = nullptr;
    for (const auto& ph : phdrs) {
        if (ph.p_type == PT_LOAD) { // loadable segment
            void* segment = mmap((void*)(ph.p_vaddr), 
                                ph.p_memsz,
                                PROT_READ | PROT_WRITE | PROT_EXEC,
                                MAP_PRIVATE | MAP_ANONYMOUS,
                                -1, 0);

            if (segment == MAP_FAILED) {
                perror("mmap failed");
                return;
            }

            // Load segment data
            binary.seekg(ph.p_offset, std::ios::beg);
            binary.read((char*)(segment), ph.p_filesz);

            // Zero out the remaining part if needed
            if (ph.p_filesz < ph.p_memsz) {
                memset((char*)(segment) + ph.p_filesz, 0, ph.p_memsz - ph.p_filesz);
            }

            if (!entry_point) {
                entry_point = (void*)(elf_header.e_entry);
            }
        }
    }
    build_stack(argc, argv);

}

int main(int argc, char* argv[]){   
    string filepath = argv[1];
    load_and_execute(filepath, argc, argv);
}