#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <elf.h>
#include <vector>
using namespace std;

void load_and_execute(string filepath){
    ifstream binary(filepath, std::ios::binary);
    if(!binary){
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

    for(auto& phdr: phdrs)
        cout << phdr.p_type << "\n";


}

int main(int argc, char* argv[]){
    if(argc != 2){
        fprintf(stderr, "Incorrect arguments.\n");
        return 0;
    }
    
    string filepath = argv[1];

    load_and_execute(filepath);
}