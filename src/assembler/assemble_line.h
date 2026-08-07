#ifndef APEL_ASSEMBLER_ASSEMBLE_LINE_H
#define APEL_ASSEMBLER_ASSEMBLE_LINE_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    ASSEMBLE_LINE_EMPTY, /* blank or comment-only - nothing to encode */
    ASSEMBLE_LINE_OK,    /* *out_word is valid */
    ASSEMBLE_LINE_ERROR  /* error_msg is valid */
} assemble_line_result_t;

/* Parses and encodes one line of assembly text. No label support yet -
   branch/jump targets are a raw signed number for now (step 3 adds
   labels). Comments start with '#'. */
assemble_line_result_t assemble_line(const char *line, uint32_t *out_word,
                                      char *error_msg, size_t error_msg_size);

#endif
