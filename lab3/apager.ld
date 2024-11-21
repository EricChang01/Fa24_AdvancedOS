/* Define memory regions */
MEMORY
{
    /* A region for the loader program, assumed unused by system libraries */
    LOADER_TEXT (rx) : ORIGIN = 0x50000000, LENGTH = 0x100000  /* 1 MB */
    LOADER_DATA (rw) : ORIGIN = 0x60000000, LENGTH = 0x100000  /* 1 MB */
    LOADER_BSS (rw) : ORIGIN = 0x70000000, LENGTH = 0x100000  /* 1 MB */
}

/* Define the sections */
SECTIONS
{
    /* Place code (.text section) in LOADER_REGION */
    .text :
    {
        *(.text)
    } > LOADER_TEXT

    /* Place initialized data (.data section) in LOADER_REGION */
    .data :
    {
        *(.data)
    } > LOADER_DATA

    /* Place uninitialized data (.bss section) in LOADER_REGION */
    .bss :
    {
        *(.bss)
    } > LOADER_BSS
}
