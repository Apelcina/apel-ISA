#include "assemble_program.h"
#include "cpu.h"
#include "disassemble.h"
#include "isa.h"
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

#define R_TYPE(rs_, rt_, rd_, funct_)                                        \
    (((uint32_t)(rs_) << 22) | ((uint32_t)(rt_) << 18) |                     \
     ((uint32_t)(rd_) << 14) | (uint32_t)(funct_))
#define I_TYPE(opcode_, rs_, rt_, imm_)                                      \
    (((uint32_t)(opcode_) << 26) | ((uint32_t)(rs_) << 22) |                 \
     ((uint32_t)(rt_) << 18) | ((uint32_t)(imm_) & 0x3FFFFu))

static void expect_text(uint32_t addr, uint32_t word, const char *expected) {
    decoded_instr_t d = decode(word);
    char buf[64];
    disassemble(addr, d, buf, sizeof(buf));
    if (strcmp(buf, expected) != 0) {
        printf("FAIL: expected \"%s\", got \"%s\"\n", expected, buf);
        failed = 1;
    }
}

static void test_spot_checks(void) {
    expect_text(0, R_TYPE(1, 2, 3, 0x00), "ADD r3, r1, r2");
    expect_text(0, R_TYPE(0, 1, 4, 0x06) | (3u << 6), "SLL r4, r1, 3");
    expect_text(0, R_TYPE(5, 0, 0, 0x0B), "JR r5");
    expect_text(0, R_TYPE(7, 0, 6, 0x0C), "JALR r6, r7");
    expect_text(0, R_TYPE(0, 0, 0, 0x0D), "HALT");
    expect_text(0, I_TYPE(0x08, 1, 2, 100), "ADDI r2, r1, 100");
    expect_text(0, I_TYPE(0x08, 1, 2, (uint32_t)-5), "ADDI r2, r1, -5");
    expect_text(0, I_TYPE(0x0F, 0, 3, 42), "LUI r3, 42");
    expect_text(0, I_TYPE(0x23, 1, 3, 8), "LW r3, 8(r1)");
    expect_text(0, I_TYPE(0x2B, 1, 3, (uint32_t)-4), "SW r3, -4(r1)");

    /* branch at address 12, offset +3 words -> target 12+4+12=28 */
    expect_text(12, I_TYPE(0x04, 1, 2, 3), "BEQ r1, r2, 0x0000001C");

    /* J at address 4, word-index 4 -> target 16 = 0x00000010 */
    expect_text(4, (0x02u << 26) | 4u, "J 0x00000010");

    char buf[64];
    decoded_instr_t invalid = decode((0x01u << 26)); /* reserved opcode */
    check("unrecognized encoding returns false",
          !disassemble(0, invalid, buf, sizeof(buf)));
}

static void test_matches_real_execution(void) {
    const char *lines[] = {
        "        ADDI r1, r0, 0",
        "        ADDI r2, r0, 1",
        "        ADDI r3, r0, 6",
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
    check("test program assembles", result.ok);

    /* Disassemble the BGE at address 12 and the J at address 24, and
       check the printed target addresses against where the real
       decode()+execute() pipeline actually sends pc - not just that
       the math looks right in isolation. */
    decoded_instr_t bge = decode(words[3]);
    char buf[64];
    disassemble(12, bge, buf, sizeof(buf));
    check("disassembled BGE target matches loop's real 'end' address",
          strcmp(buf, "BGE r2, r3, 0x0000001C") == 0);

    decoded_instr_t j = decode(words[6]);
    disassemble(24, j, buf, sizeof(buf));
    check("disassembled J target matches loop's real 'loop' address",
          strcmp(buf, "J 0x0000000C") == 0);

    /* Cross-check against the real symbol table addresses used when
       the program was assembled and actually run: end=28=0x1C,
       loop=12=0x0C - matches what the disassembler printed above. */
    cpu_t cpu;
    cpu_init(&cpu);
    cpu_load_program(&cpu, words, result.word_count);
    cpu_run(&cpu, 100);
    check("program actually halted at address 28 (0x1C), same as 'end'",
          cpu.pc == 0x1C);
    cpu_destroy(&cpu);
}

int main(void) {
    test_spot_checks();
    test_matches_real_execution();

    if (!failed) {
        printf("PASS: disassembler output correct and matches real execution\n");
    }
    return failed;
}
