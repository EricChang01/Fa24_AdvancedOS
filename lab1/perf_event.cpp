#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <string>

#define CACHE_LINE_SIZE 64
#define FILE_PATH "file.dat"
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
    size_t cache_size = 600 * 1024;  // 64 KB buffer
    char *buffer = (char*)malloc(cache_size);
    
    // Write to the buffer to flush the cache (a simple write will force write-allocate on many CPUs)
    // for (size_t i = 0; i < cache_size; i++) {
    //     buffer[i] = 0;
    // }

    // Optional: read from the buffer to flush the cache via reads (depending on the CPU)
    for (size_t i = 0; i < cache_size; i++) {
        volatile char temp = buffer[i];
        (void)temp;  // Prevent compiler optimizations
    }

    free(buffer);  // Free allocated buffer after flushing
}

int get_perf_event_fd(int type, int access){
    struct perf_event_attr event;
    memset(&event, 0, sizeof(struct perf_event_attr));

    event.type = PERF_TYPE_HW_CACHE;
    event.size = sizeof(struct perf_event_attr);
    event.disabled = 1;
    // l1d_access.exclude_kernel = 1; // only monitor user space
    event.exclude_hv = 1;     // exclude hypervisor

    switch(type){
        case 0: // "l1d_access"
            event.config = (PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16));
            break;
        case 1: // "l1d_miss"
            event.config = (PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16));
            break;
        case 2: // "dtlb_miss"
            event.config = (PERF_COUNT_HW_CACHE_DTLB | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16));
            break;
    }

    switch(access){
        case 0:
            event.config |= (PERF_COUNT_HW_CACHE_OP_READ << 8);
            break;
        case 1:
            event.config |= (PERF_COUNT_HW_CACHE_OP_WRITE << 8);
            break;
        case 2:
            event.config |= (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8);
            break;
    }

    int fd = syscall(__NR_perf_event_open, &event, 0, -1, -1, 0);
    if (fd == -1) {
        perror("perf event");
        return -1;
    }
    return fd;
}

int main(int argc, char* argv[]){
    opt_random_access = atoi(argv[1]); // Set up opt_random_access with argument
    int file_based_mmap = atoi(argv[2]); // map anonymous or file-backed memory
    int mmap_flag = atoi(argv[3]); // MAP_PRIVATE, MAP_SHARED or MAP_POPULATE
    int opt_map_populate = atoi(argv[4]);
    int opt_memset_msync = atoi(argv[5]);

    // struct perf_event_attr l1d_access, l1d_miss, dtlb_miss;
    // memset(&l1d_access, 0, sizeof(struct perf_event_attr));
    // memset(&l1d_miss, 0, sizeof(struct perf_event_attr));
    // memset(&dtlb_miss, 0, sizeof(struct perf_event_attr));

    // l1d_access.type = PERF_TYPE_HW_CACHE;
    // l1d_access.size = sizeof(struct perf_event_attr);
    // l1d_access.config = (PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16));
    // l1d_access.disabled = 1;
    // // l1d_access.exclude_kernel = 1; // only monitor user space
    // l1d_access.exclude_hv = 1;     // exclude hypervisor

    // l1d_miss.type = PERF_TYPE_HW_CACHE;
    // l1d_miss.size = sizeof(struct perf_event_attr);
    // l1d_miss.config = (PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16));
    // l1d_miss.disabled = 1;
    // // l1d_miss.exclude_kernel = 1; // only monitor user space
    // l1d_miss.exclude_hv = 1;     // exclude hypervisor

    // dtlb_miss.type = PERF_TYPE_HW_CACHE;
    // dtlb_miss.size = sizeof(struct perf_event_attr);
    // dtlb_miss.config = (PERF_COUNT_HW_CACHE_DTLB | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16));
    // dtlb_miss.disabled = 1;
    // // dtlb_miss.exclude_kernel = 1; // only monitor user space
    // dtlb_miss.exclude_hv = 1;     // exclude hypervisor

    // int l1d_access_fd = syscall(__NR_perf_event_open, &l1d_access, 0, -1, -1, 0);
    // if (l1d_access_fd == -1) {
    //     fprintf(stderr, "Error opening lid access %llx, %s\n", l1d_access.config, strerror(errno));
    //     return -1;
    // }

    // int l1d_miss_fd = syscall(__NR_perf_event_open, &l1d_miss, 0, -1, -1, 0);
    // if (l1d_miss_fd == -1) {
    //     fprintf(stderr, "Error opening lid miss %llx, %s\n", l1d_miss.config, strerror(errno));
    //     return -1;
    // }

    // int dtlb_miss_fd = syscall(__NR_perf_event_open, &dtlb_miss, 0, -1, -1, 0);
    // if (dtlb_miss_fd == -1) {
    //     fprintf(stderr, "Error opening lid miss %llx, %s\n", dtlb_miss.config, strerror(errno));
    //     return -1;
    // }

    int l1d_access_read = get_perf_event_fd(0, 0);
    int l1d_miss_read = get_perf_event_fd(1, 0);
    int dtlb_miss_read = get_perf_event_fd(2, 0);

    int l1d_access_write = get_perf_event_fd(0, 1);
    int l1d_miss_write = get_perf_event_fd(1, 1);
    int dtlb_miss_write = get_perf_event_fd(2, 1);

    int l1d_access_pf = get_perf_event_fd(0, 1);
    int l1d_miss_pf = get_perf_event_fd(1, 1);
    int dtlb_miss_pf = get_perf_event_fd(2, 1);

    flush_L1_cache();

    ioctl(l1d_access_read, PERF_EVENT_IOC_RESET, 0);  // Reset the counter
    ioctl(l1d_access_read, PERF_EVENT_IOC_ENABLE, 0); // Start the counter
    ioctl(l1d_miss_read, PERF_EVENT_IOC_RESET, 0);  // Reset the counter
    ioctl(l1d_miss_read, PERF_EVENT_IOC_ENABLE, 0); // Start the counter
    ioctl(dtlb_miss_read, PERF_EVENT_IOC_RESET, 0);  // Reset the counter
    ioctl(dtlb_miss_read, PERF_EVENT_IOC_ENABLE, 0); // Start the counter

    ioctl(l1d_access_write, PERF_EVENT_IOC_RESET, 0);  // Reset the counter
    ioctl(l1d_access_write, PERF_EVENT_IOC_ENABLE, 0); // Start the counter
    ioctl(l1d_miss_write, PERF_EVENT_IOC_RESET, 0);  // Reset the counter
    ioctl(l1d_miss_write, PERF_EVENT_IOC_ENABLE, 0); // Start the counter
    ioctl(dtlb_miss_write, PERF_EVENT_IOC_RESET, 0);  // Reset the counter
    ioctl(dtlb_miss_write, PERF_EVENT_IOC_ENABLE, 0); // Start the counter

    ioctl(l1d_access_pf, PERF_EVENT_IOC_RESET, 0);  // Reset the counter
    ioctl(l1d_access_pf, PERF_EVENT_IOC_ENABLE, 0); // Start the counter
    ioctl(l1d_miss_pf, PERF_EVENT_IOC_RESET, 0);  // Reset the counter
    ioctl(l1d_miss_pf, PERF_EVENT_IOC_ENABLE, 0); // Start the counter
    ioctl(dtlb_miss_pf, PERF_EVENT_IOC_RESET, 0);  // Reset the counter
    ioctl(dtlb_miss_pf, PERF_EVENT_IOC_ENABLE, 0); // Start the counter

    struct rusage usage_start, usage_end;
    if (getrusage(RUSAGE_SELF, &usage_start) != 0) {
        perror("getrusage");
    }

    // add the code to monitor, allocate a 1GB buffer
    size_t bufferSize = 1024 * 1024 * 1024; // 1GB
    
    void* buffer;
    int mmap_fd;
    switch (file_based_mmap){
        case 0: // anonymous
            buffer = mmap(NULL, bufferSize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
            break;
        case 1:
            mmap_fd = open(FILE_PATH, O_RDWR);
            if (mmap_fd < 0) {
                perror("Failed to open file");
                exit(1);
            }
            if (ftruncate(mmap_fd, bufferSize) == -1) {
                perror("ftruncate failed");
                close(mmap_fd);
                exit(1);
            } 
            switch (mmap_flag){
                case 0: // MAP_PRIVATE
                    if(opt_map_populate)
                        buffer = mmap(NULL, bufferSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_POPULATE, mmap_fd, 0);
                    else
                        buffer = mmap(NULL, bufferSize, PROT_READ | PROT_WRITE, MAP_PRIVATE, mmap_fd, 0);
                    break;
                case 1: // MAP_SHARED
                    if(opt_map_populate)
                        buffer = mmap(NULL, bufferSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, mmap_fd, 0);
                    else
                        buffer = mmap(NULL, bufferSize, PROT_READ | PROT_WRITE, MAP_SHARED, mmap_fd, 0);
                    break;
                default:
                    fprintf(stderr, "Invalid mmap flag\n");
                    exit(1);
            }
            break;
        default:
            fprintf(stderr, "Invalid mmap type\n");
            exit(1);

    }
    if(opt_memset_msync == 1){
        memset(buffer, 0, bufferSize);

        // Use msync to synchronize changes
        if (msync(buffer, bufferSize, MS_SYNC) == -1) {
            perror("msync error");
        }
    }

    do_mem_access((char*)buffer, 1<<30);

    ioctl(l1d_access_read, PERF_EVENT_IOC_DISABLE, 0); // Stop the counter
    ioctl(l1d_miss_read, PERF_EVENT_IOC_DISABLE, 0); // Stop the counter
    ioctl(dtlb_miss_read, PERF_EVENT_IOC_DISABLE, 0); // Stop the counter

    long long count_l1d_access_read, count_l1d_miss_read, count_dtlb_miss_read;
    read(l1d_access_read, &count_l1d_access_read, sizeof(long long)); // Read the event count
    read(l1d_miss_read, &count_l1d_miss_read, sizeof(long long)); // Read the event count
    read(dtlb_miss_read, &count_dtlb_miss_read, sizeof(long long)); // Read the event count

    ioctl(l1d_access_write, PERF_EVENT_IOC_DISABLE, 0); // Stop the counter
    ioctl(l1d_miss_write, PERF_EVENT_IOC_DISABLE, 0); // Stop the counter
    ioctl(dtlb_miss_write, PERF_EVENT_IOC_DISABLE, 0); // Stop the counter

    long long count_l1d_access_write, count_l1d_miss_write, count_dtlb_miss_write;
    read(l1d_access_write, &count_l1d_access_write, sizeof(long long)); // Read the event count
    read(l1d_miss_write, &count_l1d_miss_write, sizeof(long long)); // Read the event count
    read(dtlb_miss_write, &count_dtlb_miss_write, sizeof(long long)); // Read the event count

    ioctl(l1d_access_pf, PERF_EVENT_IOC_DISABLE, 0); // Stop the counter
    ioctl(l1d_miss_pf, PERF_EVENT_IOC_DISABLE, 0); // Stop the counter
    ioctl(dtlb_miss_pf, PERF_EVENT_IOC_DISABLE, 0); // Stop the counter

    long long count_l1d_access_pf, count_l1d_miss_pf, count_dtlb_miss_pf;
    read(l1d_access_pf, &count_l1d_access_pf, sizeof(long long)); // Read the event count
    read(l1d_miss_pf, &count_l1d_miss_pf, sizeof(long long)); // Read the event count
    read(dtlb_miss_pf, &count_dtlb_miss_pf, sizeof(long long)); // Read the event count


    if (getrusage(RUSAGE_SELF, &usage_end) == 0) {
        printf("%lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %f, %f, %ld, %ld, %ld, %ld, %ld, %ld, %ld\n", 
                count_l1d_access_read, 
                count_l1d_access_write, 
                count_l1d_access_pf, 
                count_l1d_miss_read, 
                count_l1d_miss_write, 
                count_l1d_miss_pf, 
                count_dtlb_miss_read, 
                count_dtlb_miss_write, 
                count_dtlb_miss_pf, 
                ((double)usage_end.ru_utime.tv_sec + (double)usage_end.ru_utime.tv_usec / 1000000) - ((double)usage_start.ru_utime.tv_sec + (double)usage_start.ru_utime.tv_usec / 1000000), 
                ((double)usage_end.ru_stime.tv_sec + (double)usage_end.ru_stime.tv_usec / 1000000) - ((double)usage_start.ru_stime.tv_sec + (double)usage_start.ru_stime.tv_usec / 1000000),
                usage_start.ru_maxrss,
                usage_end.ru_minflt - usage_start.ru_minflt,
                usage_end.ru_majflt - usage_start.ru_majflt,
                usage_end.ru_inblock - usage_start.ru_inblock,
                usage_end.ru_oublock - usage_start.ru_oublock,
                usage_end.ru_nvcsw - usage_start.ru_nvcsw,
                usage_end.ru_nivcsw - usage_start.ru_nivcsw);
    } else {
        perror("getrusage");
    }
    
    close(l1d_access_read);
    close(l1d_miss_read);
    close(dtlb_miss_read);

    close(l1d_access_write);
    close(l1d_miss_write);
    close(dtlb_miss_write);

    close(l1d_access_pf);
    close(l1d_miss_pf);
    close(dtlb_miss_pf);
}