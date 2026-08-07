#ifndef APEL_ASSEMBLER_FILE_IO_H
#define APEL_ASSEMBLER_FILE_IO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    char **lines;
    size_t count;
} source_lines_t;

/* Reads a text file into an array of lines (newlines stripped, either
   \n or \r\n). Returns false if the file couldn't be opened or memory
   ran out. */
bool read_source_lines(const char *path, source_lines_t *out);
void free_source_lines(source_lines_t *lines);

/* Writes words as a flat, headerless binary - little-endian, 4 bytes
   each, in order. No sections, no symbols - a real object file format
   is Phase 4 scope. */
bool write_binary_file(const char *path, const uint32_t *words, size_t count);

/* Reads a flat binary written by write_binary_file back into words (up
   to max_words). Returns false if the file couldn't be opened, or if
   its size isn't a whole number of words (a truncated/corrupt file). */
bool read_binary_file(const char *path, uint32_t *words, size_t max_words,
                       size_t *out_count);

#endif
