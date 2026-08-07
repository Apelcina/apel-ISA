#include "assemble_program.h"
#include "cpu.h"
#include "file_io.h"
#include "loop.h"
#include <stdio.h>
#include <string.h>

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

static int failed = 0;

static void check(const char *label, int ok) {
    if (!ok) {
        printf("FAIL: %s\n", label);
        failed = 1;
    }
}

static const char *ASM_PATH = "file_io_smoke_test.asm";
static const char *BIN_PATH = "file_io_smoke_test.bin";

static void write_test_source(void) {
    FILE *f = fopen(ASM_PATH, "w");
    fprintf(f, "        ADDI r1, r0, 0   # sum = 0\r\n"); /* CRLF on purpose */
    fprintf(f, "        ADDI r2, r0, 1   # i = 1\n");      /* LF on purpose */
    fprintf(f, "        ADDI r3, r0, 6   # limit = 6\n");
    fprintf(f, "loop:   BGE  r2, r3, end\n");
    fprintf(f, "        ADD  r1, r1, r2\n");
    fprintf(f, "        ADDI r2, r2, 1\n");
    fprintf(f, "        J    loop\n");
    fprintf(f, "end:    HALT\n");
    fclose(f);
}

static void test_full_round_trip_through_real_files(void) {
    write_test_source();

    source_lines_t src;
    check("read_source_lines opens the file", read_source_lines(ASM_PATH, &src));
    check("mixed CRLF/LF both stripped correctly", src.count == 8);

    uint32_t words[32];
    assemble_program_result_t result =
        assemble_program((const char *const *)src.lines, src.count, words, 32);
    if (!result.ok) {
        printf("FAIL: assembly failed at line %zu: %s\n", result.error_line,
               result.error_msg);
        failed = 1;
    }
    check("assembled 8 words", result.word_count == 8);
    free_source_lines(&src);

    check("write_binary_file succeeds",
          write_binary_file(BIN_PATH, words, result.word_count));

    uint32_t read_back[32];
    size_t read_count;
    check("read_binary_file succeeds",
          read_binary_file(BIN_PATH, read_back, 32, &read_count));
    check("read back the same word count", read_count == result.word_count);
    check("read back the same bytes",
          memcmp(words, read_back, read_count * sizeof(uint32_t)) == 0);

    cpu_t cpu;
    cpu_init(&cpu);
    check("assembled-then-reloaded program fits in memory",
          cpu_load_program(&cpu, read_back, read_count));
    cpu_run(&cpu, 100);

    check("halted", cpu.halted == true);
    check("sum == 15, after a full .asm -> .bin -> emulator round trip",
          cpu_reg_read(&cpu, 1) == 15);
    cpu_destroy(&cpu);

    remove(ASM_PATH);
    remove(BIN_PATH);
}

static void test_missing_file_fails_cleanly(void) {
    source_lines_t src;
    check("reading a nonexistent .asm file fails, doesn't crash",
          !read_source_lines("does_not_exist.asm", &src));

    uint32_t words[8];
    size_t count;
    check("reading a nonexistent .bin file fails, doesn't crash",
          !read_binary_file("does_not_exist.bin", words, 8, &count));
}

static void test_truncated_binary_is_rejected(void) {
    FILE *f = fopen(BIN_PATH, "wb");
    uint8_t garbage[6] = {1, 2, 3, 4, 5, 6}; /* 6 bytes: one full word + 2 leftover */
    fwrite(garbage, 1, sizeof(garbage), f);
    fclose(f);

    uint32_t words[8];
    size_t count;
    bool ok = read_binary_file(BIN_PATH, words, 8, &count);
    check("truncated binary (not a multiple of 4 bytes) is rejected", !ok);
    check("still reports the whole words it did read", count == 1);

    remove(BIN_PATH);
}

int main(void) {
    test_full_round_trip_through_real_files();
    test_missing_file_fails_cleanly();
    test_truncated_binary_is_rejected();

    if (!failed) {
        printf("PASS: .asm -> words -> .bin -> words -> emulator round trip "
               "works through real files\n");
    }
    return failed;
}
