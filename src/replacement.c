#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/mman.h>
#include <errno.h>

#define PK_LOCATION 0x7fbfffd08000
#define KEK_LOCATION 0x7fbfffd08001 
#define DB_LOCATION 0x7fbfffd08002
#define DBX_LOCATION 0x7fbfffd08003

#define MIN_KEY_LENGTH 32
#define MAX_KEY_LENGTH 512
#define KEY_ALIGNMENT 16

typedef struct {
    char *name;
    void *location; 
    size_t size;
    int required;
    unsigned char current_hash[32];
} key_entry_t;

key_entry_t keys[] = {
    {"PK", (void*)PK_LOCATION, 256, 1, {0}},
    {"KEK", (void*)KEK_LOCATION, 256, 1, {0}},
    {"db", (void*)DB_LOCATION, 512, 0, {0}},
    {"dbx", (void*)DBX_LOCATION, 512, 0, {0}}
};

static void calculate_hash(const unsigned char *data, size_t len, unsigned char *hash) {
    uint32_t fnv_prime = 0x01000193;
    uint32_t hash_val = 0x811c9dc5;
    
    for (size_t i = 0; i < len; i++) {
        hash_val ^= data[i];
        hash_val *= fnv_prime;
    }
    
    memcpy(hash, &hash_val, sizeof(hash_val));
}

static int verify_memory_permissions(void *addr, size_t len) {
    unsigned char *test = mmap(addr, len, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (test == MAP_FAILED) {
        return 0;
    }
    munmap(test, len);
    return 1;
}

int validate_key(const char* new_key_data, size_t new_key_length) {
    if (!new_key_data) {
        fprintf(stderr, "Error: Key data is NULL\n");
        return 0;
    }

    if (new_key_length < MIN_KEY_LENGTH || new_key_length > MAX_KEY_LENGTH) {
        fprintf(stderr, "Error: Key length must be between %d and %d bytes\n", 
                MIN_KEY_LENGTH, MAX_KEY_LENGTH);
        return 0;
    }

    if (new_key_length % KEY_ALIGNMENT != 0) {
        fprintf(stderr, "Error: Key length must be aligned to %d bytes\n", KEY_ALIGNMENT);
        return 0;
    }
    
    for (size_t i = 0; i < new_key_length; i++) {
        if (!isprint(new_key_data[i])) {
            fprintf(stderr, "Error: Invalid character at position %zu (must be printable ASCII)\n", i);
            return 0;
        }
    }

    return 1;
}

void replace_keys(const char* key_name, const char* new_key_data) {
    if (!key_name || !new_key_data) {
        fprintf(stderr, "Error: NULL parameters passed\n");
        exit(EXIT_FAILURE);
    }

    key_entry_t *target_key = NULL;
    for (size_t i = 0; i < sizeof(keys) / sizeof(key_entry_t); i++) {
        if (strcmp(keys[i].name, key_name) == 0) {
            target_key = &keys[i];
            break;
        }
    }

    if (!target_key) {
        fprintf(stderr, "Error: Key '%s' not found\n", key_name);
        exit(EXIT_FAILURE);
    }

    size_t new_key_length = strlen(new_key_data);
    if (!validate_key(new_key_data, new_key_length)) {
        exit(EXIT_FAILURE);
    }

    if (new_key_length > target_key->size) {
        fprintf(stderr, "Error: New key length %zu exceeds allocated space %zu for %s\n",
                new_key_length, target_key->size, key_name);
        exit(EXIT_FAILURE);
    }

    if (!verify_memory_permissions(target_key->location, target_key->size)) {
        fprintf(stderr, "Error: Cannot write to key location %p\n", target_key->location);
        exit(EXIT_FAILURE);
    }

    unsigned char old_hash[32];
    calculate_hash(target_key->location, target_key->size, old_hash);
    
    if (memcmp(old_hash, target_key->current_hash, sizeof(old_hash)) != 0) {
        fprintf(stderr, "Warning: Key %s appears to have been modified externally\n", key_name);
    }

    if (mprotect(target_key->location, target_key->size, PROT_READ | PROT_WRITE) != 0) {
        fprintf(stderr, "Error: Failed to set memory permissions: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    memset(target_key->location, 0, target_key->size);
    memcpy(target_key->location, new_key_data, new_key_length);
    calculate_hash(target_key->location, target_key->size, target_key->current_hash);

    if (mprotect(target_key->location, target_key->size, PROT_READ) != 0) {
        fprintf(stderr, "Error: Failed to restore memory permissions: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("Successfully replaced %s at %p (length: %zu bytes)\n",
           key_name, target_key->location, new_key_length);
}

int main(int argc, char **argv) {
    if (geteuid() != 0) {
        fprintf(stderr, "Error: This program must be run as root\n");
        return EXIT_FAILURE;
    }

    const char *key_files[] = {
        "/etc/secureboot/platform.key",
        "/etc/secureboot/enrollment.key",
        "/etc/secureboot/database.key",
        "/etc/secureboot/revocation.key"
    };

    printf("Starting secure boot key replacement...\n");

    for (size_t i = 0; i < sizeof(keys) / sizeof(key_entry_t); i++) {
        FILE *fp = fopen(key_files[i], "rb");
        if (!fp) {
            if (keys[i].required) {
                fprintf(stderr, "Error: Required key file %s not found\n", key_files[i]);
                return EXIT_FAILURE;
            }
            printf("Skipping optional key %s (file not found)\n", keys[i].name);
            continue;
        }

        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        char *key_data = malloc(file_size + 1);
        if (!key_data) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            fclose(fp);
            return EXIT_FAILURE;
        }

        if (fread(key_data, 1, file_size, fp) != (size_t)file_size) {
            fprintf(stderr, "Error: Failed to read key file %s\n", key_files[i]);
            free(key_data);
            fclose(fp);
            return EXIT_FAILURE;
        }
        key_data[file_size] = '\0';
        fclose(fp);

        replace_keys(keys[i].name, key_data);
        free(key_data);
    }

    printf("All secure boot keys have been successfully updated\n");
    return EXIT_SUCCESS;
}
