#include <stdio.h>
#include <stdlib.h>

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
        printf("%s", buffer);
    }

    // Close the file
    fclose(file);

    return EXIT_SUCCESS;
}