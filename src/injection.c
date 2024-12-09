#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define CUSTOM_PK "MIIEZQIBAAKCAQEAr2k9XN9j4Y6A9V+h0hj1J1b0GvBjXc6G4l0kXp5L3h6M5o7L6e5K4c3B2a1D0f9G8E7C6B5A4D3F2E1C0D1B3A2C1D0E0F1B1A0C9D8B7A6C5D4E3F2E1D0C0A1B3C2D1E0F2D1C0B1A0C9D8B7A6C5D4E3F2E1D0C0A1B3C2D1E0F2D1C0B1A0C9D8B7A6C5D4E3F2E1D0C0A1B3C2D1E0F2D1C0B1A0C9D8B7A6C5D4E3F2E1D0"


void inject_keys(const char* key_data) {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("Error opening /dev/mem");
        exit(EXIT_FAILURE);
    }

    void* target_memory_location = mmap(NULL, sysconf(_SC_PAGE_SIZE), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x100000);
    if (target_memory_location == MAP_FAILED) {
        perror("Error mapping memory");
        exit(EXIT_FAILURE);
    }

    size_t key_data_length = strlen(key_data);
    memcpy(target_memory_location, key_data, key_data_length);

    munmap(target_memory_location, sysconf(_SC_PAGE_SIZE));
}

int main() {
    inject_keys(CUSTOM_PK);
    return 0;
}
