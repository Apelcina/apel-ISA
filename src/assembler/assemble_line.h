#ifndef APEL_ASSEMBLER_ASSEMBLE_LINE_H
#define APEL_ASSEMBLER_ASSEMBLE_LINE_H

#include "symbol_table.h"
#include <stddef.h>
#include <stdint.h>

typedef enum {
    ASSEMBLE_LINE_EMPTY, /* blank or comment-only - nothing to encode */
    ASSEMBLE_LINE_OK,    /* *out_word is valid */
    ASSEMBLE_LINE_ERROR  /* error_msg is valid */
} assemble_line_result_t;

/* Parses and encodes one line of assembly text (already stripped of any
   leading "label:" prefix - see assemble_program.h for that).
   current_addr is this line's own address, needed to turn a branch's
   label into a PC-relative word offset. symtab may be NULL, in which
   case branch/jump operands only accept raw numbers, same as step 2.
   Comments start with '#'. */
assemble_line_result_t assemble_line(const char *line, uint32_t current_addr,
                                      const symbol_table_t *symtab,
                                      uint32_t *out_word, char *error_msg,
                                      size_t error_msg_size);

#endif
