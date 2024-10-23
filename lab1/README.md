- To lock the perf_event process onto a single processor
taskset -c 0 sudo ./perf_event 0 0 0 0 0
- Remember to add a file (file.dat) for mmap