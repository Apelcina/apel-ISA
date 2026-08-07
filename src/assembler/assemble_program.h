#ifndef APEL_ASSEMBLER_ASSEMBLE_PROGRAM_H
#define APEL_ASSEMBLER_ASSEMBLE_PROGRAM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    bool ok;
    size_t word_count;   /* valid if ok */
    size_t error_line;   /* 1-indexed, valid if !ok */
    char error_msg[128]; /* valid if !ok */
} assemble_program_result_t;

/* Two-pass assembly of a full source (one string per line, no trailing
   newlines expected). Pass 1 builds a symbol table from "label:"
   prefixes (address = byte offset of the next instruction-producing
   line). Pass 2 encodes every line, resolving label references against
   that table - so forward and backward references both just work,
   unlike assemble_line() called directly.

   out_words must have room for at least line_count words (an upper
   bound; blank/label-only lines produce none, so word_count is
   typically smaller). */
assemble_program_result_t assemble_program(const char *const *lines,
                                            size_t line_count,
                                            uint32_t *out_words,
                                            size_t max_words);

#endif
