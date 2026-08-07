#include "assemble_line.h"
#include "isa.h"
#include <stdio.h>
#include <string.h>

static int failed = 0;

static void check(const char *label, int ok) {
    if (!ok) {
        printf("FAIL: %s\n", label);
        failed = 1;
    }
}

static uint32_t expect_ok(const char *line) {
    uint32_t word;
    char err[128];
    assemble_line_result_t r = assemble_line(line, &word, err, sizeof(err));
    if (r != ASSEMBLE_LINE_OK) {
        printf("FAIL: expected '%s' to assemble, got result %d (%s)\n", line,
               r, r == ASSEMBLE_LINE_ERROR ? err : "empty");
        failed = 1;
        return 0;
    }
    return word;
}

static void expect_error(const char *line) {
    uint32_t word;
    char err[128];
    assemble_line_result_t r = assemble_line(line, &word, err, sizeof(err));
    if (r != ASSEMBLE_LINE_ERROR) {
        printf("FAIL: expected '%s' to error, got result %d\n", line, r);
        failed = 1;
    }
}

static void test_round_trips_every_operand_shape(void) {
    decoded_instr_t d;

    d = decode(expect_ok("ADD r3, r1, r2"));
    check("ADD rd,rs,rt", d.format == FMT_R && d.funct == 0x00 && d.rd == 3 &&
                               d.rs == 1 && d.rt == 2);

    d = decode(expect_ok("SLL r4, r1, 3"));
    check("SLL rd,rt,shamt (not rd,rs,rt)",
          d.format == FMT_R && d.funct == 0x06 && d.rd == 4 && d.rt == 1 &&
              d.shamt == 3);

    d = decode(expect_ok("JR r5"));
    check("JR rs", d.format == FMT_R && d.funct == 0x0B && d.rs == 5);

    d = decode(expect_ok("JALR r6, r7"));
    check("JALR rd,rs",
          d.format == FMT_R && d.funct == 0x0C && d.rd == 6 && d.rs == 7);

    d = decode(expect_ok("HALT"));
    check("HALT no operands", d.format == FMT_R && d.funct == 0x0D);

    d = decode(expect_ok("ADDI r2, r1, 100"));
    check("ADDI rt,rs,imm", d.format == FMT_I && d.opcode == 0x08 &&
                                 d.rt == 2 && d.rs == 1 && d.imm == 100);

    d = decode(expect_ok("ADDI r2, r1, -5"));
    check("ADDI negative imm round-trips (raw 18-bit field)",
          d.imm == (uint32_t)(-5 & 0x3FFFF));

    d = decode(expect_ok("LUI r3, 42"));
    check("LUI rt,imm", d.format == FMT_I && d.opcode == 0x0F && d.rt == 3 &&
                             d.imm == 42);

    d = decode(expect_ok("LW r3, 8(r1)"));
    check("LW rt,imm(rs) memory syntax", d.format == FMT_I && d.opcode == 0x23 &&
                                              d.rt == 3 && d.rs == 1 &&
                                              d.imm == 8);

    d = decode(expect_ok("SW r3, -4(r1)"));
    check("SW with negative offset",
          d.format == FMT_I && d.opcode == 0x2B && d.rt == 3 && d.rs == 1 &&
              d.imm == (uint32_t)(-4 & 0x3FFFF));

    d = decode(expect_ok("BEQ r1, r2, -4"));
    check("BEQ rs,rt,numeric-label-placeholder",
          d.format == FMT_I && d.opcode == 0x04 && d.rs == 1 && d.rt == 2 &&
              d.imm == (uint32_t)(-4 & 0x3FFFF));

    d = decode(expect_ok("J 10"));
    check("J numeric-label-placeholder",
          d.format == FMT_J && d.opcode == 0x02 && d.address == 10);
}

static void test_hex_and_case_insensitive_mnemonic(void) {
    decoded_instr_t d = decode(expect_ok("addi r1, r0, 0x2A"));
    check("lowercase mnemonic + hex immediate",
          d.opcode == 0x08 && d.rt == 1 && d.imm == 0x2A);
}

static void test_comments_and_blank_lines(void) {
    uint32_t word;
    char err[128];

    check("blank line is EMPTY",
          assemble_line("", &word, err, sizeof(err)) == ASSEMBLE_LINE_EMPTY);
    check("whitespace-only line is EMPTY",
          assemble_line("   \t  ", &word, err, sizeof(err)) ==
              ASSEMBLE_LINE_EMPTY);
    check("comment-only line is EMPTY",
          assemble_line("# just a comment", &word, err, sizeof(err)) ==
              ASSEMBLE_LINE_EMPTY);

    decoded_instr_t d = decode(expect_ok("ADD r1, r2, r3 # trailing comment"));
    check("trailing comment stripped correctly",
          d.rd == 1 && d.rs == 2 && d.rt == 3);
}

static void test_error_cases(void) {
    expect_error("FROBNICATE r1, r2, r3"); /* unknown mnemonic */
    expect_error("ADD r16, r1, r2");       /* register out of range */
    expect_error("ADD rX, r1, r2");        /* not a register at all */
    expect_error("ADD r1, r2");            /* too few operands */
    expect_error("HALT r1");               /* unexpected operand */
    expect_error("ADDI r1, r0, 999999");   /* immediate out of 18-bit range */
    expect_error("SLL r1, r2, 32");        /* shamt out of 0..31 range */
    expect_error("LW r1, 4 r2");           /* malformed memory operand */
}

int main(void) {
    test_round_trips_every_operand_shape();
    test_hex_and_case_insensitive_mnemonic();
    test_comments_and_blank_lines();
    test_error_cases();

    if (!failed) {
        printf("PASS: assemble_line handles every operand shape, hex/case, "
               "comments, and rejects malformed input\n");
    }
    return failed;
}
