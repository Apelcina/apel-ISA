#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>

/* strncpy is standard, portable C - see assemble_line.c for why we
   don't use MSVC's non-standard strncpy_s instead. */
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

void symbol_table_init(symbol_table_t *table) {
    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
}

void symbol_table_free(symbol_table_t *table) {
    free(table->symbols);
    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
}

bool symbol_table_add(symbol_table_t *table, const char *name, uint32_t address) {
    if (strlen(name) >= sizeof(((symbol_t *)0)->name)) {
        return false;
    }
    uint32_t existing;
    if (symbol_table_lookup(table, name, &existing)) {
        return false; /* duplicate definition */
    }

    if (table->count == table->capacity) {
        size_t new_capacity = table->capacity == 0 ? 8 : table->capacity * 2;
        symbol_t *grown = realloc(table->symbols, new_capacity * sizeof(symbol_t));
        if (!grown) {
            return false;
        }
        table->symbols = grown;
        table->capacity = new_capacity;
    }

    strncpy(table->symbols[table->count].name, name, sizeof(table->symbols[0].name) - 1);
    table->symbols[table->count].name[sizeof(table->symbols[0].name) - 1] = '\0';
    table->symbols[table->count].address = address;
    table->count++;
    return true;
}

bool symbol_table_lookup(const symbol_table_t *table, const char *name,
                          uint32_t *out_address) {
    size_t i;
    for (i = 0; i < table->count; i++) {
        if (strcmp(table->symbols[i].name, name) == 0) {
            *out_address = table->symbols[i].address;
            return true;
        }
    }
    return false;
}
