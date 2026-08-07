#include "assemble_program.h"
#include "cpu.h"
#include "loop.h"
#include <stdio.h>
#include <string.h>

static int failed = 0;

static void check(const char *label, int ok) {
    if (!ok) {
        printf("FAIL: %s\n", label);
        failed = 1;
    }
}

static void test_forward_and_backward_labels(void) {
    /* The sum-1..5 loop, written with real labels this time - no more
       hand-computing offsets like -4 or word-index 3. "end" is a
       forward reference (used before it's defined); "loop" is a
       backward reference (used after). Both must resolve correctly. */
    const char *lines[] = {
        "        ADDI r1, r0, 0   # sum = 0",
        "        ADDI r2, r0, 1   # i = 1",
        "        ADDI r3, r0, 6   # limit = 6",
        "loop:   BGE  r2, r3, end",
        "        ADD  r1, r1, r2",
        "        ADDI r2, r2, 1",
        "        J    loop",
        "end:    HALT",
    };
    size_t line_count = sizeof(lines) / sizeof(lines[0]);

    uint32_t words[32];
    assemble_program_result_t result =
        assemble_program(lines, line_count, words, 32);

    if (!result.ok) {
        printf("FAIL: assembly failed at line %zu: %s\n", result.error_line,
               result.error_msg);
        failed = 1;
        return;
    }
    check("assembled exactly 8 words (label-only lines produce none)",
          result.word_count == 8);

    cpu_t cpu;
    cpu_init(&cpu);
    check("assembled program fits in memory",
          cpu_load_program(&cpu, words, result.word_count));
    cpu_run(&cpu, 100);

    check("halted", cpu.halted == true);
    check("sum == 15, assembled from real labels this time",
          cpu_reg_read(&cpu, 1) == 15);
    cpu_destroy(&cpu);
}

static void test_undefined_label_reports_line_number(void) {
    const char *lines[] = {
        "ADDI r1, r0, 5",
        "J nowhere",
    };
    uint32_t words[8];
    assemble_program_result_t result = assemble_program(lines, 2, words, 8);

    check("undefined label fails", !result.ok);
    check("error points at line 2", result.error_line == 2);
}

static void test_duplicate_label_is_an_error(void) {
    const char *lines[] = {
        "again: ADDI r1, r0, 1",
        "again: ADDI r2, r0, 2",
    };
    uint32_t words[8];
    assemble_program_result_t result = assemble_program(lines, 2, words, 8);

    check("duplicate label fails", !result.ok);
}

static void test_label_only_lines_dont_produce_words(void) {
    const char *lines[] = {
        "start:",
        "ADDI r1, r0, 1",
        "  # just a comment",
        "",
        "HALT",
    };
    uint32_t words[8];
    assemble_program_result_t result = assemble_program(lines, 5, words, 8);

    check("blank/label-only/comment-only lines produce nothing",
          result.ok && result.word_count == 2);
}

int main(void) {
    test_forward_and_backward_labels();
    test_undefined_label_reports_line_number();
    test_duplicate_label_is_an_error();
    test_label_only_lines_dont_produce_words();

    if (!failed) {
        printf("PASS: two-pass assembly resolves forward and backward "
               "labels correctly, runs through the real emulator\n");
    }
    return failed;
}
