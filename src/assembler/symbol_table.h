#ifndef APEL_ASSEMBLER_SYMBOL_TABLE_H
#define APEL_ASSEMBLER_SYMBOL_TABLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    char name[32];
    uint32_t address;
} symbol_t;

typedef struct {
    symbol_t *symbols;
    size_t count;
    size_t capacity;
} symbol_table_t;

void symbol_table_init(symbol_table_t *table);
void symbol_table_free(symbol_table_t *table);

/* Returns false (and adds nothing) if name is already defined or too
   long, or if the table couldn't grow. */
bool symbol_table_add(symbol_table_t *table, const char *name, uint32_t address);

bool symbol_table_lookup(const symbol_table_t *table, const char *name,
                          uint32_t *out_address);

#endif
