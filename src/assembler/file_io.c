#include "file_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* fopen/fgets are standard, portable C - see assemble_line.c for why we
   don't use MSVC's non-standard _s alternatives instead. */
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

#define MAX_LINE_LEN 512

bool read_source_lines(const char *path, source_lines_t *out) {
    FILE *f = fopen(path, "r");
    if (!f) {
        return false;
    }

    out->lines = NULL;
    out->count = 0;
    size_t capacity = 0;
    char buf[MAX_LINE_LEN];

    while (fgets(buf, sizeof(buf), f)) {
        size_t len = strlen(buf);
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
            buf[--len] = '\0';
        }

        if (out->count == capacity) {
            size_t new_capacity = capacity == 0 ? 16 : capacity * 2;
            char **grown = realloc(out->lines, new_capacity * sizeof(char *));
            if (!grown) {
                fclose(f);
                free_source_lines(out);
                return false;
            }
            out->lines = grown;
            capacity = new_capacity;
        }

        char *copy = malloc(len + 1);
        if (!copy) {
            fclose(f);
            free_source_lines(out);
            return false;
        }
        memcpy(copy, buf, len + 1);
        out->lines[out->count++] = copy;
    }

    fclose(f);
    return true;
}

void free_source_lines(source_lines_t *lines) {
    size_t i;
    for (i = 0; i < lines->count; i++) {
        free(lines->lines[i]);
    }
    free(lines->lines);
    lines->lines = NULL;
    lines->count = 0;
}

bool write_binary_file(const char *path, const uint32_t *words, size_t count) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        return false;
    }

    size_t i;
    for (i = 0; i < count; i++) {
        uint8_t bytes[4];
        bytes[0] = (uint8_t)(words[i] & 0xFF);
        bytes[1] = (uint8_t)((words[i] >> 8) & 0xFF);
        bytes[2] = (uint8_t)((words[i] >> 16) & 0xFF);
        bytes[3] = (uint8_t)((words[i] >> 24) & 0xFF);
        if (fwrite(bytes, 1, 4, f) != 4) {
            fclose(f);
            return false;
        }
    }

    fclose(f);
    return true;
}

bool read_binary_file(const char *path, uint32_t *words, size_t max_words,
                       size_t *out_count) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return false;
    }

    size_t count = 0;
    uint8_t bytes[4];
    size_t got = 4; /* assume fine if the loop body never runs (max_words == 0) */
    while (count < max_words && (got = fread(bytes, 1, 4, f)) == 4) {
        words[count++] = (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
                          ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
    }

    fclose(f);
    *out_count = count;
    /* got is anything other than 0 (clean EOF) or 4 (a full word) only
       when the file's size isn't a multiple of 4 - a truncated file. */
    return got == 0 || got == 4;
}
