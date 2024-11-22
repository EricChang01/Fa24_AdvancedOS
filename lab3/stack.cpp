#include <iostream>
#include <cstring>
#include <unistd.h>
#include <cassert>
#include "pager.hpp"
#include "stack.hpp"

#define STACK_SIZE (8 * 1024 * 1024)

using namespace std;

void Stack::build_stack(char **argv, char **envp, Elf64_auxv_t *auxv, Pager& pager) {
    this->addr = malloc(STACK_SIZE);
    this->entry_point = (char*)this->addr + STACK_SIZE;
    char *stack = (char*)this->entry_point;

    int auxv_count = 0;
    Elf64_auxv_t *ptr = auxv;
    while (ptr -> a_type != AT_NULL){
        auxv_count++;
        ptr++;
    }

    // Auxiliary vectors
    stack -= sizeof(Elf64_auxv_t) * (auxv_count + 1); // Include space for AT_NULL
    Elf64_auxv_t *auxv_ptr = (Elf64_auxv_t *)stack;
    memcpy(auxv_ptr, auxv, (auxv_count + 1) * sizeof(Elf64_auxv_t));

    // Modify some fields to match elf of the target program
    Elf64_auxv_t* auxv_mod = auxv_ptr;
    while (auxv_mod -> a_type != AT_NULL) {
        switch (auxv_mod -> a_type) {
            case AT_PHDR:
                auxv_mod -> a_un.a_val = (uint64_t)pager.phdrs;
                break;
            case AT_BASE:
                auxv_mod -> a_un.a_val = 0;
                break;
            case AT_ENTRY:
                auxv_mod -> a_un.a_val = pager.elfhdr.e_entry;
                break;
            case AT_PHNUM:
                auxv_mod -> a_un.a_val = pager.elfhdr.e_phnum;
                break;
            case AT_EXECFN:
                auxv_mod -> a_un.a_val = (uint64_t)(argv[1]);
                break;
        }
        
        auxv_mod++;
    }

    int envp_count = 0;
    char** begin = environ;
    for (char** env = environ; *env != NULL; ++env) {
        envp_count++;
    }

    // Environment variables
    stack -= sizeof(char *) * (envp_count + 1); // Include NULL terminator
    char **envp_ptrs = (char **)stack;
    memcpy(envp_ptrs, envp, (envp_count + 1) * sizeof(char **));

    int argv_count = 0;
    while (argv && argv[argv_count]) {
        argv_count++;
    }

    // Argv
    stack -= sizeof(char *) * (argv_count + 1); // Include NULL terminator
    char **argv_ptrs = (char **)stack;
    memcpy(argv_ptrs, argv, (argv_count + 1) * sizeof(char **));

    // Argc
    stack -= sizeof(uint64_t);
    *((uint64_t *)stack) = argv_count;

    this->entry_point = (void*)stack;
}

void Stack::stack_check(void* top_of_stack, uint64_t argc, char** argv) {
	printf("----- stack check -----\n");

	assert(((uint64_t)top_of_stack) % 8 == 0);
	printf("top of stack is 8-byte aligned\n");

	uint64_t* stack = (uint64_t*)top_of_stack;
	uint64_t actual_argc = *(stack++);
	printf("argc: %lu\n", actual_argc);
	assert(actual_argc == argc);

	for (int i = 0; i < argc; i++) {
		char* argp = (char*)*(stack++);
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

Elf64_auxv_t* Stack::find_auxv_entry(void* top_of_stack, int argc, char **argv) {
    uint64_t* stack = (uint64_t*)top_of_stack;

	assert(((uint64_t)stack) % 8 == 0);

	uint64_t actual_argc = *(uint64_t*)(stack++);
    assert(actual_argc == argc);

	for (int i = 0; i < argc; i++) {
		char* argp = (char*)*(char**)(stack++);
		assert(strcmp(argp, argv[i]) == 0);
	}
	assert(*(char**)(stack++) == 0);

    int envp_count = 0;
	while (*(char**)(stack++) != 0)
		envp_count++;

	Elf64_auxv_t* auxv_start = (Elf64_auxv_t*)stack;
	Elf64_auxv_t* auxv_null = auxv_start;
	while (auxv_null->a_type != AT_NULL) {
		auxv_null++;
	}
    return auxv_start;
}
