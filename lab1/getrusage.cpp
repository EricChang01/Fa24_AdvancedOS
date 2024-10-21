#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>

int main() {
    // Open the /proc/self/maps file
    FILE *file = fopen("/proc/self/maps", "r");
    if (file == NULL) {
        perror("Error opening /proc/self/maps");
        return EXIT_FAILURE;
    }

    // Read and print the file line by line
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        // just read the file and do nothing
        // printf("%s", buffer);
    }

    // Close the file
    fclose(file);

     // Declare a struct to store resource usage
    struct rusage usage;

    // Get resource usage statistics for the calling process
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        // Print specific fields from the rusage struct
        printf("User CPU time (utime): %ld.%06ld seconds\n",
               (long)usage.ru_utime.tv_sec, (long)usage.ru_utime.tv_usec);
        printf("System CPU time (stime): %ld.%06ld seconds\n",
               (long)usage.ru_stime.tv_sec, (long)usage.ru_stime.tv_usec);
        printf("Max resident set size (maxrss): %ld KB\n", usage.ru_maxrss);
        printf("Page reclaims (minflt): %ld\n", usage.ru_minflt);
        printf("Page faults (majflt): %ld\n", usage.ru_majflt);
        printf("Block input operations (inblock): %ld\n", usage.ru_inblock);
        printf("Block output operations (oublock): %ld\n", usage.ru_oublock);
        printf("Voluntary context switches (nvcsw): %ld\n", usage.ru_nvcsw);
        printf("Involuntary context switches (nivcsw): %ld\n", usage.ru_nivcsw);
    } else {
        perror("getrusage");
    }

    return EXIT_SUCCESS;
}