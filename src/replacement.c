#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PK_LOCATION 0x7fbfffd08000


void replace_keys() {
    void* pk_location = (void*)PK_LOCATION;
    const char* new_pk_data = "new_platform_key_data";
    size_t new_pk_length = strlen(new_pk_data);

    if (new_pk_length > sizeof(pk_location)) {
        fprintf(stderr, "Error: New PK data length exceeds allocated space.\n");
        exit(EXIT_FAILURE);
    }

    memcpy(pk_location, new_pk_data, new_pk_length);
    printf("Platform Key replaced successfully at location %p.\n", pk_location);
}

int main() {
    replace_keys();
    return 0;
}
