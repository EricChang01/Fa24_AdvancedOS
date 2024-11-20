#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <elf.h>
#include <vector>
#include <sys/mman.h>
using namespace std;

void load_and_execute(string filepath){
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
    binary.read((char*)(phdrs.data()), elf_header.e_phnum * sizeof(Elf64_Phdr));

    // for (auto& phdr: phdrs)
    //     cout << phdr.p_type << "\n";

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
    cout << "entry point = " << entry_point << "\n";

    // Transfer control to the loaded program
    using EntryPoint = void(*)();
    EntryPoint execute = (EntryPoint)(entry_point);
    execute();

}

int main(int argc, char* argv[]){
    if(argc != 2){
        fprintf(stderr, "Incorrect arguments.\n");
        return 0;
    }
    
    string filepath = argv[1];

    load_and_execute(filepath);
}