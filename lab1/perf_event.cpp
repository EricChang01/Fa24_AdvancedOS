#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <iostream>

#define CACHE_LINE_SIZE 64
bool opt_random_access;

// Simple, fast random number generator, here so we can observe it using profiler
long x = 1, y = 4, z = 7, w = 13;
long simplerand(void) {
	long t = x;
	t ^= t << 11;
	t ^= t >> 8;
	x = y;
	y = z;
	z = w;
	w ^= w >> 19;
	w ^= t;
	return w;
}

// p points to a region that is 1GB (ideally)
void do_mem_access(char* p, int size) {
	int i, j, count, outer, locality;
    int ws_base = 0;
    int max_base = ((size / CACHE_LINE_SIZE) - 512);
    for(outer = 0; outer < (1<<20); ++outer) {
        long r = simplerand() % max_base;
        // Pick a starting offset
        if( opt_random_access ) {
            ws_base = r;
        } else {
            ws_base += 512;
            if( ws_base >= max_base ) {
                ws_base = 0;
            }
        }
        for(locality = 0; locality < 16; locality++) {
            volatile char *a;
            char c;
            for(i = 0; i < 512; i++) {
                // Working set of 512 cache lines, 32KB
                a = p + (ws_base + i) * CACHE_LINE_SIZE;
                if((i%8) == 0) {
                    *a = 1;
                } else {
                    c = *a;
                }
            }
        }
    }
}

int main(int argc, char* argv[]){
    opt_random_access = argv[1]; // Set up opt_random_access with argument

    size_t bufferSize = 1024 * 1024 * 1024; // 1GB
    char* buffer = new char[bufferSize];
    if (buffer == nullptr) {
        std::cerr << "Memory allocation failed!" << std::endl;
        return 1;
    }

    struct perf_event_attr pe; // Initialize the perf event
    memset(&pe, 0, sizeof(struct perf_event_attr));


    // pe.type = PERF_TYPE_HARDWARE;   // Type of event (hardware, software, etc.)
    // pe.size = sizeof(struct perf_event_attr);
    // pe.config = PERF_COUNT_HW_CPU_CYCLES; // Specific event (CPU cycles, instructions, etc.)
    // pe.disabled = 1;               // Start disabled (we enable it later)
    // pe.exclude_kernel = 1;         // Don't count kernel events
    // pe.exclude_hv = 1;             // Don't count hypervisor events

    pe.type = PERF_TYPE_HW_CACHE;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = (PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16));
    pe.disabled = 1;
    pe.exclude_kernel = 1; // only monitor user space
    pe.exclude_hv = 1;     // exclude hypervisor

    int fd = syscall(__NR_perf_event_open, &pe, getpid(), -1, -1, 0);
    // int fd = perf_event_open(&pe, getpid(), -1, -1, 0);
    if (fd == -1) {
        fprintf(stderr, "Error opening leader %llx\n", pe.config);
        return -1;
    }

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);  // Reset the counter
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0); // Start the counter

    // add the code to monitor
    do_mem_access(buffer, 1<<30);

    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0); // Stop the counter

    long long count;
    read(fd, &count, sizeof(long long)); // Read the event count

    printf("CPU cycles: %lld\n", count);
    close(fd);
}