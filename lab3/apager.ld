/* Define memory regions */
MEMORY
{
    /* A region for the loader program, assumed unused by system libraries */
    LOADER_REGION (rx) : ORIGIN = 0x50000000, LENGTH = 0xF00000  /* 1 MB */
}

/* Define the sections */
SECTIONS
{
    /* Place code (.text section) in LOADER_REGION */
    .text :
    {
        *(.text)
    } > LOADER_REGION

    /* Place initialized data (.data section) in LOADER_REGION */
    .data :
    {
        *(.data)
    } > LOADER_REGION

    /* Place uninitialized data (.bss section) in LOADER_REGION */
    .bss :
    {
        *(.bss)
    } > LOADER_REGION
}
