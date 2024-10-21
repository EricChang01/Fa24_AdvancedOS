#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

// The event to be measured is specified by filling out a struct perf_event_attr structure.

int main(){
    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(struct perf_event_attr));

    pe.type = PERF_TYPE_HARDWARE;   // Type of event (hardware, software, etc.)
    pe.size = sizeof(struct perf_event_attr);
    pe.config = PERF_COUNT_HW_CPU_CYCLES; // Specific event (CPU cycles, instructions, etc.)
    pe.disabled = 1;               // Start disabled (we enable it later)
    pe.exclude_kernel = 1;         // Don't count kernel events
    pe.exclude_hv = 1;             // Don't count hypervisor events

    int fd = syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
    if (fd == -1) {
        fprintf(stderr, "Error opening leader %llx\n", pe.config);
        return -1;
    }

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);  // Reset the counter
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0); // Start the counter

    // add the code to monitor

    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0); // Stop the counter

    long long count;
    read(fd, &count, sizeof(long long)); // Read the event count

    printf("CPU cycles: %lld\n", count);
    close(fd);
}