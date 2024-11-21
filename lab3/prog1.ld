/* Define memory regions */
MEMORY
{
    /* A region for the loader program, assumed unused by system libraries */
    LOADER_TEXT (rx) : ORIGIN = 0x80000000, LENGTH = 0x100000  /* 1 MB */
    LOADER_DATA (rw) : ORIGIN = 0x90000000, LENGTH = 0x100000  /* 1 MB */
    LOADER_BSS (rw) : ORIGIN = 0x100000000, LENGTH = 0x100000  /* 1 MB */
}

/* Define the sections */
SECTIONS
{
    .text :
    {
        *(.text)
    } > LOADER_TEXT

    .data :
    {
        *(.data)
    } > LOADER_DATA

    .bss :
    {
        *(.bss)
    } > LOADER_BSS
}
