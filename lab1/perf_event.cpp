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

void flush_L1_cache() {
    // L1 cache size is 512KB, allocate a buffer larger than this
    size_t cache_size = 512 * 1024;  // 64 KB buffer
    char *buffer = (char*)malloc(cache_size);
    
    // Write to the buffer to flush the cache (a simple write will force write-allocate on many CPUs)
    for (size_t i = 0; i < cache_size; i++) {
        buffer[i] = 0;
    }

    // Optional: read from the buffer to flush the cache via reads (depending on the CPU)
    for (size_t i = 0; i < cache_size; i++) {
        volatile char temp = buffer[i];
        (void)temp;  // Prevent compiler optimizations
    }

    free(buffer);  // Free allocated buffer after flushing
}

int main(int argc, char* argv[]){
    opt_random_access = argv[1]; // Set up opt_random_access with argument
    std::cout << argv[1] << "\n";

    size_t bufferSize = 1024 * 1024 * 1024; // 1GB
    char* buffer = new char[bufferSize];
    if (buffer == nullptr) {
        std::cerr << "Memory allocation failed!" << std::endl;
        return 1;
    }

    struct perf_event_attr l1d_access, l1d_miss, dtlb_miss;
    memset(&l1d_access, 0, sizeof(struct perf_event_attr));
    memset(&l1d_miss, 0, sizeof(struct perf_event_attr));
    memset(&dtlb_miss, 0, sizeof(struct perf_event_attr));

    l1d_access.type = PERF_TYPE_HW_CACHE;
    l1d_access.size = sizeof(struct perf_event_attr);
    l1d_access.config = (PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16));
    l1d_access.disabled = 1;
    l1d_access.exclude_kernel = 1; // only monitor user space
    l1d_access.exclude_hv = 1;     // exclude hypervisor

    l1d_miss.type = PERF_TYPE_HW_CACHE;
    l1d_miss.size = sizeof(struct perf_event_attr);
    l1d_miss.config = (PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16));
    l1d_miss.disabled = 1;
    l1d_miss.exclude_kernel = 1; // only monitor user space
    l1d_miss.exclude_hv = 1;     // exclude hypervisor

    dtlb_miss.type = PERF_TYPE_HW_CACHE;
    dtlb_miss.size = sizeof(struct perf_event_attr);
    dtlb_miss.config = (PERF_COUNT_HW_CACHE_DTLB | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16));
    dtlb_miss.disabled = 1;
    dtlb_miss.exclude_kernel = 1; // only monitor user space
    dtlb_miss.exclude_hv = 1;     // exclude hypervisor

    int l1d_access_fd = syscall(__NR_perf_event_open, &l1d_access, 0, -1, -1, 0);
    if (l1d_access_fd == -1) {
        fprintf(stderr, "Error opening lid access %llx, %s\n", l1d_access.config, strerror(errno));
        return -1;
    }

    int l1d_miss_fd = syscall(__NR_perf_event_open, &l1d_miss, 0, -1, -1, 0);
    if (l1d_miss_fd == -1) {
        fprintf(stderr, "Error opening lid miss %llx, %s\n", l1d_miss.config, strerror(errno));
        return -1;
    }

    int dtlb_miss_fd = syscall(__NR_perf_event_open, &dtlb_miss, 0, -1, -1, 0);
    if (dtlb_miss_fd == -1) {
        fprintf(stderr, "Error opening lid miss %llx, %s\n", dtlb_miss.config, strerror(errno));
        return -1;
    }

    flush_L1_cache();

    ioctl(l1d_access_fd, PERF_EVENT_IOC_RESET, 0);  // Reset the counter
    ioctl(l1d_access_fd, PERF_EVENT_IOC_ENABLE, 0); // Start the counter

    ioctl(l1d_miss_fd, PERF_EVENT_IOC_RESET, 0);  // Reset the counter
    ioctl(l1d_miss_fd, PERF_EVENT_IOC_ENABLE, 0); // Start the counter

    ioctl(dtlb_miss_fd, PERF_EVENT_IOC_RESET, 0);  // Reset the counter
    ioctl(dtlb_miss_fd, PERF_EVENT_IOC_ENABLE, 0); // Start the counter

    // add the code to monitor, allocate a 1GB buffer
    do_mem_access(buffer, 1<<30);

    ioctl(l1d_access_fd, PERF_EVENT_IOC_DISABLE, 0); // Stop the counter
    ioctl(l1d_miss_fd, PERF_EVENT_IOC_DISABLE, 0); // Stop the counter
    ioctl(dtlb_miss_fd, PERF_EVENT_IOC_DISABLE, 0); // Stop the counter

    long long count_l1d_access, count_l1d_miss, count_dtlb_miss;
    read(l1d_access_fd, &count_l1d_access, sizeof(long long)); // Read the event count
    read(l1d_miss_fd, &count_l1d_miss, sizeof(long long)); // Read the event count
    read(dtlb_miss_fd, &count_dtlb_miss, sizeof(long long)); // Read the event count

    printf("l1d access count: %lld\n", count_l1d_access);
    printf("l1d misses count: %lld\n", count_l1d_miss);
    printf("dtlb misses count: %lld\n", count_dtlb_miss);
    
    close(l1d_access_fd);
    close(l1d_miss_fd);
    close(dtlb_miss_fd);
}