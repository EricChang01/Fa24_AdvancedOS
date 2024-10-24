- To lock the perf_event process onto a single processor
    - `taskset -c 0 sudo ./perf_event 0 0 0 0 0`
    - arguments are <opt_random_access> <file_based_mmap> <mmap_flag> <opt_map_populate> <opt_memset_msync>
    - <mmap_flag> <opt_map_populate> can only be set when mapped to a file


- Remember to add a file (file.dat) for file-based mmap