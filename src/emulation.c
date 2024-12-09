#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#define NVRAM_SIZE 1024
#define DB_NAME "nvram.db"

typedef struct {
    sqlite3 *db;
    char nvram[NVRAM_SIZE];
} NVRAMContext;

void init_nvram(NVRAMContext *context) {
    if (sqlite3_open(DB_NAME, &context->db) != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(context->db));
        exit(1);
    }
    const char *sql = "CREATE TABLE IF NOT EXISTS nvram (name TEXT PRIMARY KEY, value TEXT, version INTEGER);";
    char *err_msg = 0;
    if (sqlite3_exec(context->db, sql, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
}

void set_nvram_variable(NVRAMContext *context, const char* name, const char* value) {
    const char *sql = "INSERT OR REPLACE INTO nvram (name, value, version) VALUES (?, ?, (SELECT IFNULL(MAX(version), 0) + 1 FROM nvram WHERE name = ?));";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(context->db, sql, -1, &stmt, 0) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(context->db));
        return;
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, value, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, name, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(context->db));
    }
    sqlite3_finalize(stmt);
    log_nvram_access(name, "set");
}

const char* get_nvram_variable(NVRAMContext *context, const char* name) {
    const char *sql = "SELECT value FROM nvram WHERE name = ?;";
    sqlite3_stmt *stmt;
    const char *value = NULL;
    if (sqlite3_prepare_v2(context->db, sql, -1, &stmt, 0) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(context->db));
        return NULL;
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        value = (const char*)sqlite3_column_text(stmt, 0);
    }
    sqlite3_finalize(stmt);
    log_nvram_access(name, "get");
    return value;
}

void log_nvram_access(const char* name, const char* action) {
    printf("%s variable '%s' accessed.\n", action, name);
}

void rollback_nvram(NVRAMContext *context) {
    const char *sql = "SELECT name, value, version FROM nvram ORDER BY version DESC;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(context->db, sql, -1, &stmt, 0) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement for rollback: %s\n", sqlite3_errmsg(context->db));
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *name = (const char *)sqlite3_column_text(stmt, 0);
        const char *value = (const char *)sqlite3_column_text(stmt, 1);
        int version = sqlite3_column_int(stmt, 2);

        printf("Rolling back variable '%s' to version %d with value: %s\n", name, version, value);
        set_nvram_variable(context, name, value);
    }

    sqlite3_finalize(stmt);
}

void emulate_nvram() {
    NVRAMContext context; // Create NVRAMContext instance
    init_nvram(&context);
    memset(context.nvram, 0, NVRAM_SIZE);
    printf("NVRAM emulation initialized.\n");
    UINTN attributes = 0;
    EFI_GUID variable_guid = EFI_GLOBAL_VARIABLE_GUID;
    EFI_STATUS status = gRT->SetVariable(
        "SetupMode",
        &variable_guid,
        attributes,
        sizeof(context.nvram),
        context.nvram
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
                memcpy(context.nvram, data, data_size);
                attributes |= EFI_VARIABLE_BOOTSERVICE_ACCESS;
                attributes |= EFI_VARIABLE_RUNTIME_ACCESS;
                attributes |= EFI_VARIABLE_NON_VOLATILE;
                attributes &= ~EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;
                printf("Modified variable attributes to %u.\n", attributes);
                set_nvram_variable(&context, "SetupMode", context.nvram);
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
