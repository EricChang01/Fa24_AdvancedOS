#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <elf.h>
#include <vector>
#include <sys/mman.h>
#include "util_func.hpp"
using namespace std;

char** copy_args (void* stack, int argc, char* argv[]) {
    int* argcPtr = (int*)stack;

    *argcPtr = argc - 1;
    argcPtr++;

    char** argvPtr = (char**)(argcPtr);  // Move the pointer past the 'argc' value
    for (int i = 1; i < argc; ++i) {
        memcpy(argvPtr, argv[i], sizeof(char*));
        argvPtr++;
    }
    return argvPtr;
}

Elf64_auxv_t* copy_auxv(Elf64_auxv_t *auxvPtr) {
    int fd = open("/proc/self/auxv", O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    Elf64_auxv_t auxv;
    int num_auxv = 0;

    // Read the auxiliary vector entries one at a time
    while (read(fd, &auxv, sizeof(auxv)) == sizeof(auxv)) {
        // Copy the auxv to the destination pointer (auxvPtr + num_auxv)
        memcpy(auxvPtr, &auxv, sizeof(Elf64_auxv_t));
        num_auxv++;
        auxvPtr++;

        // Stop reading when encountering the AT_NULL entry
        if (auxv.a_type == AT_NULL) {
            break;
        }

        // Print out the type and value for demonstration purposes
        // printf("Type: %lu, Value: %lx\n", auxv.a_type, auxv.a_un.a_val);
    }
    return auxvPtr;
}

extern char** environ;
char** copy_envp(char** envpPtr) {
    for (char** env = environ; *env != NULL; ++env) {
        memcpy(envpPtr, env, sizeof(char*));
        envpPtr++;
    }
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
    cout << "Init: " << stack << "\n";
#endif

    char** argvPtr = copy_args(stack, argc, argv);

#ifdef StackBuild
    cout << "Args: " << argvPtr << "\n";
#endif

    Elf64_auxv_t* auxvPtr = copy_auxv((Elf64_auxv_t*)(argvPtr));

#ifdef StackBuild
    cout << "Auxv: " << auxvPtr << "\n";
#endif

    char** envpPtr = copy_envp((char**) auxvPtr);

#ifdef StackBuild
    cout << "Envp: " << envpPtr << "\n";
#endif
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