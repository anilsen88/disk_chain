#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


void manage_pxe_boot() {
    const char* pxe_config = "pxelinux.cfg/default";
    const char* modified_bootloader = "modified_bootloader.bin";

    printf("Managing PXE boot...\n");

    int fd = open(pxe_config, O_RDWR | O_CREAT | O_EXCL, 0644);
    if (fd == -1) {
        if (errno != EEXIST) {
            perror("Error opening PXE config");
            exit(EXIT_FAILURE);
        }
    } else {
        char buffer[4096];
        ssize_t bytes_read = read(open(modified_bootloader, O_RDONLY), buffer, sizeof(buffer));
        if (bytes_read == -1) {
            perror("Error reading modified bootloader");
            exit(EXIT_FAILURE);
        }

        if (write(fd, buffer, bytes_read) == -1) {
            perror("Error writing to PXE config");
            exit(EXIT_FAILURE);
        }

        close(fd);
    }

    if (chmod(pxe_config, 0644) == -1) {
        perror("Error changing permissions on PXE config");
        exit(EXIT_FAILURE);
    }
}

int main() {
    manage_pxe_boot();
    return 0;
}
