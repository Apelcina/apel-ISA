#include "assemble_program.h"
#include "file_io.h"
#include <stdio.h>

#define MAX_WORDS 65536

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <input.asm> <output.bin>\n", argv[0]);
        return 1;
    }

    const char *input_path = argv[1];
    const char *output_path = argv[2];

    source_lines_t src;
    if (!read_source_lines(input_path, &src)) {
        fprintf(stderr, "error: could not open '%s'\n", input_path);
        return 1;
    }

    /* static, not a local array - a 256 KiB buffer as a plain stack
       local risks the same kind of stack overflow we hit with cpu_t's
       embedded memory array back in phase 2. */
    static uint32_t words[MAX_WORDS];
    assemble_program_result_t result = assemble_program(
        (const char *const *)src.lines, src.count, words, MAX_WORDS);
    free_source_lines(&src);

    if (!result.ok) {
        fprintf(stderr, "%s:%zu: error: %s\n", input_path, result.error_line,
                result.error_msg);
        return 1;
    }

    if (!write_binary_file(output_path, words, result.word_count)) {
        fprintf(stderr, "error: could not write '%s'\n", output_path);
        return 1;
    }

    printf("assembled %zu words -> %s\n", result.word_count, output_path);
    return 0;
}
