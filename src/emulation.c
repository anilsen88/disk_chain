#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NVRAM_SIZE 1024
char nvram[NVRAM_SIZE];

void emulate_nvram() {
    memset(nvram, 0, NVRAM_SIZE);
    printf("NVRAM emulation initialized.\n");
    UINTN attributes = 0;
    EFI_GUID variable_guid = EFI_GLOBAL_VARIABLE_GUID;
    EFI_STATUS status = gRT->SetVariable(
        "SetupMode",
        &variable_guid,
        attributes,
        sizeof(nvram),
        nvram
    );

    if (EFI_ERROR(status)) {
        printf("Failed to set SetupMode variable: %s\n", get_error_message(status));
        exit(EXIT_FAILURE);
    }

    printf("SetupMode variable set.\n");

    void* set_variable_hook = dlsym(RTLD_NEXT, "SetVariable");
    if (!set_variable_hook) {
        printf("Failed to find SetVariable function.\n");
        exit(EXIT_FAILURE);
    }

    printf("Hooking into SetVariable function...\n");
    void (*original_set_variable)(EFI_GUID*, CHAR16*, UINT32, UINTN, UINTN) = set_variable_hook;
    void hook_set_variable(EFI_GUID* variable_guid, CHAR16* variable_name, UINT32 attributes, UINTN data_size, void* data) {
        printf("Hooked SetVariable call for variable %s.\n", variable_name);
        CHAR16* target_variables[] = {
            L"SetupMode",
            L"SecureBoot",
            L"BootNext",
            L"BootOrder",
            L"BootCurrent"
        };
        int i;
        for (i = 0; i < sizeof(target_variables) / sizeof(target_variables[0]); i++) {
            if (StrCmp(variable_name, target_variables[i]) == 0) {
                printf("Intercepting modification of Secure Boot variable %s.\n", variable_name);
                if (data_size > NVRAM_SIZE) {
                    printf("Error: Data size %u exceeds NVRAM size %u.\n", data_size, NVRAM_SIZE);
                    exit(1);
                }
                memcpy(nvram, data, data_size);
                attributes |= EFI_VARIABLE_BOOTSERVICE_ACCESS;
                attributes |= EFI_VARIABLE_RUNTIME_ACCESS;
                attributes |= EFI_VARIABLE_NON_VOLATILE;
                attributes &= ~EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;
                printf("Modified variable attributes to %u.\n", attributes);
                break;
            }
        }
        original_set_variable(variable_guid, variable_name, attributes, data_size, data);
    }

    printf("Hooked SetVariable function.\n");
}

int main() {
    emulate_nvram();
    return 0;
}
