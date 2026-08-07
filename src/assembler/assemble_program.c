#include "assemble_program.h"
#include "assemble_line.h"
#include "symbol_table.h"
#include "text_utils.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* strncpy is standard, portable C - see assemble_line.c for why we
   don't use MSVC's non-standard strncpy_s instead. */
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

#define LINE_BUF_SIZE 256

/* Strips a leading "identifier:" prefix from line (mutating it in
   place), writing the identifier into label_out. Returns true if a
   label was found. Requires a valid bare identifier before the colon,
   so ordinary operand syntax (e.g. LW's "imm(rs)") is never mistaken
   for one. */
static bool extract_label(char *line, char *label_out, size_t label_out_size) {
    char *colon = strchr(line, ':');
    if (!colon) {
        return false;
    }
    size_t len = (size_t)(colon - line);
    if (len == 0 || len >= label_out_size) {
        return false;
    }
    if (!(isalpha((unsigned char)line[0]) || line[0] == '_')) {
        return false;
    }
    size_t i;
    for (i = 1; i < len; i++) {
        if (!(isalnum((unsigned char)line[i]) || line[i] == '_')) {
            return false;
        }
    }
    memcpy(label_out, line, len);
    label_out[len] = '\0';
    memmove(line, colon + 1, strlen(colon + 1) + 1);
    return true;
}

static char *prepare_line(const char *raw, char *buf, size_t buf_size,
                           char *label_out, size_t label_out_size,
                           bool *had_label) {
    strncpy(buf, raw, buf_size - 1);
    buf[buf_size - 1] = '\0';
    strip_comment(buf);
    char *trimmed = trim(buf);
    *had_label = extract_label(trimmed, label_out, label_out_size);
    if (*had_label) {
        trimmed = trim(trimmed);
    }
    return trimmed;
}

assemble_program_result_t assemble_program(const char *const *lines,
                                            size_t line_count,
                                            uint32_t *out_words,
                                            size_t max_words) {
    assemble_program_result_t result = {0};
    symbol_table_t symtab;
    symbol_table_init(&symtab);

    /* Pass 1: record every label's address, without encoding anything
       yet - so a label used before its definition (a forward
       reference) still resolves correctly in pass 2. */
    uint32_t addr = 0;
    size_t i;
    for (i = 0; i < line_count; i++) {
        char buf[LINE_BUF_SIZE];
        char label[32];
        bool had_label;
        char *rest = prepare_line(lines[i], buf, sizeof(buf), label,
                                   sizeof(label), &had_label);

        if (had_label && !symbol_table_add(&symtab, label, addr)) {
            result.ok = false;
            result.error_line = i + 1;
            snprintf(result.error_msg, sizeof(result.error_msg),
                     "duplicate or invalid label '%s'", label);
            symbol_table_free(&symtab);
            return result;
        }
        if (*rest != '\0') {
            addr += 4;
        }
    }

    /* Pass 2: encode every line for real, now able to resolve any label
       reference regardless of definition order. */
    addr = 0;
    size_t word_count = 0;
    for (i = 0; i < line_count; i++) {
        char buf[LINE_BUF_SIZE];
        char label[32];
        bool had_label;
        char *rest = prepare_line(lines[i], buf, sizeof(buf), label,
                                   sizeof(label), &had_label);

        if (*rest == '\0') {
            continue; /* blank or label-only line */
        }
        if (word_count >= max_words) {
            result.ok = false;
            result.error_line = i + 1;
            snprintf(result.error_msg, sizeof(result.error_msg),
                     "program exceeds output buffer (%zu words)", max_words);
            symbol_table_free(&symtab);
            return result;
        }

        uint32_t word;
        char err[128];
        assemble_line_result_t r =
            assemble_line(rest, addr, &symtab, &word, err, sizeof(err));
        if (r == ASSEMBLE_LINE_ERROR) {
            result.ok = false;
            result.error_line = i + 1;
            strncpy(result.error_msg, err, sizeof(result.error_msg) - 1);
            symbol_table_free(&symtab);
            return result;
        }
        /* ASSEMBLE_LINE_EMPTY can't happen here - rest is non-empty and
           already had the label stripped - but handle it defensively
           rather than assume. */
        if (r == ASSEMBLE_LINE_OK) {
            out_words[word_count++] = word;
            addr += 4;
        }
    }

    symbol_table_free(&symtab);
    result.ok = true;
    result.word_count = word_count;
    return result;
}
