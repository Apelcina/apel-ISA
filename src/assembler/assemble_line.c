#include "assemble_line.h"
#include "isa.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* strtok/strncpy are standard, portable C - used deliberately over
   MSVC's non-standard strtok_s/strncpy_s (see split_commas). Silence
   MSVC's portability-reducing "consider the _s version" nudge. */
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

#define LINE_BUF_SIZE 256
#define IMM18_MIN (-131072) /* -(1 << 17) */
#define IMM18_MAX 131071    /* (1 << 17) - 1 */

static char *trim(char *s) {
    while (isspace((unsigned char)*s)) {
        s++;
    }
    if (*s == '\0') {
        return s;
    }
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
    return s;
}

static void strip_comment(char *line) {
    char *hash = strchr(line, '#');
    if (hash) {
        *hash = '\0';
    }
}

static void to_upper_inplace(char *s) {
    for (; *s; s++) {
        *s = (char)toupper((unsigned char)*s);
    }
}

static bool parse_register(const char *token, uint32_t *out) {
    if ((token[0] != 'r' && token[0] != 'R') || token[1] == '\0') {
        return false;
    }
    char *endptr;
    long value = strtol(token + 1, &endptr, 10);
    if (*endptr != '\0' || value < 0 || value > 15) {
        return false;
    }
    *out = (uint32_t)value;
    return true;
}

static bool parse_immediate(const char *token, int32_t *out) {
    if (*token == '\0') {
        return false;
    }
    char *endptr;
    int base = (token[0] == '0' && (token[1] == 'x' || token[1] == 'X')) ? 16 : 10;
    long value = strtol(token, &endptr, base);
    if (endptr == token || *endptr != '\0') {
        return false;
    }
    *out = (int32_t)value;
    return true;
}

static bool parse_mem_operand(char *token, int32_t *imm_out, uint32_t *rs_out) {
    char *open = strchr(token, '(');
    if (!open) {
        return false;
    }
    char *close = strchr(open, ')');
    if (!close || *(close + 1) != '\0') {
        return false; /* must end exactly at the closing paren */
    }
    *close = '\0';
    *open = '\0';
    return parse_immediate(trim(token), imm_out) &&
           parse_register(trim(open + 1), rs_out);
}

/* Splits on commas in place, trimming whitespace from each token.
   strtok is fine here - single-threaded, no concurrency, and it's the
   most portable option (strtok_s/strtok_r aren't standard C). */
static int split_commas(char *s, char *tokens[], int max_tokens) {
    int count = 0;
    char *tok = strtok(s, ",");
    while (tok != NULL && count < max_tokens) {
        tokens[count++] = trim(tok);
        tok = strtok(NULL, ",");
    }
    return count;
}

static uint32_t pack_r(uint32_t opcode, uint32_t rs, uint32_t rt, uint32_t rd,
                        uint32_t shamt, uint32_t funct) {
    return (opcode << 26) | (rs << 22) | (rt << 18) | (rd << 14) |
           (shamt << 6) | funct;
}

static uint32_t pack_i(uint32_t opcode, uint32_t rs, uint32_t rt, int32_t imm) {
    return (opcode << 26) | (rs << 22) | (rt << 18) | ((uint32_t)imm & 0x3FFFFu);
}

static uint32_t pack_j(uint32_t opcode, int32_t address) {
    return (opcode << 26) | ((uint32_t)address & 0x3FFFFFFu);
}

#define FAIL(...)                                                            \
    do {                                                                     \
        snprintf(error_msg, error_msg_size, __VA_ARGS__);                    \
        return ASSEMBLE_LINE_ERROR;                                          \
    } while (0)

#define EXPECT_REG(tok, out)                                                 \
    do {                                                                     \
        if (!parse_register(tok, out)) {                                     \
            FAIL("invalid register '%s'", tok);                              \
        }                                                                    \
    } while (0)

#define EXPECT_IMM18(tok, out)                                               \
    do {                                                                     \
        if (!parse_immediate(tok, out)) {                                    \
            FAIL("invalid immediate '%s'", tok);                             \
        }                                                                    \
        if (*(out) < IMM18_MIN || *(out) > IMM18_MAX) {                      \
            FAIL("immediate %ld out of 18-bit range (%d..%d)", (long)*(out), \
                 IMM18_MIN, IMM18_MAX);                                      \
        }                                                                    \
    } while (0)

assemble_line_result_t assemble_line(const char *line, uint32_t *out_word,
                                      char *error_msg, size_t error_msg_size) {
    char buf[LINE_BUF_SIZE];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    strip_comment(buf);
    char *rest = trim(buf);
    if (*rest == '\0') {
        return ASSEMBLE_LINE_EMPTY;
    }

    char *space = rest;
    while (*space && !isspace((unsigned char)*space)) {
        space++;
    }
    char mnemonic[32];
    size_t mnem_len = (size_t)(space - rest);
    if (mnem_len >= sizeof(mnemonic)) {
        FAIL("mnemonic too long");
    }
    memcpy(mnemonic, rest, mnem_len);
    mnemonic[mnem_len] = '\0';
    to_upper_inplace(mnemonic);

    char *operand_str = trim(space);

    const instr_info_t *info = lookup_mnemonic(mnemonic);
    if (!info) {
        FAIL("unknown mnemonic '%s'", mnemonic);
    }

    uint32_t rs = 0, rt = 0, rd = 0, shamt_reg = 0;
    int32_t imm = 0;
    char *tokens[3];
    int n;

    switch (info->operands) {
    case OPERANDS_NONE:
        if (*operand_str != '\0') {
            FAIL("%s takes no operands", mnemonic);
        }
        *out_word = pack_r(info->opcode, 0, 0, 0, 0, info->funct);
        break;

    case OPERANDS_RD_RS_RT:
        n = split_commas(operand_str, tokens, 3);
        if (n != 3) {
            FAIL("%s expects rd, rs, rt", mnemonic);
        }
        EXPECT_REG(tokens[0], &rd);
        EXPECT_REG(tokens[1], &rs);
        EXPECT_REG(tokens[2], &rt);
        *out_word = pack_r(info->opcode, rs, rt, rd, 0, info->funct);
        break;

    case OPERANDS_RD_RT_SHAMT:
        n = split_commas(operand_str, tokens, 3);
        if (n != 3) {
            FAIL("%s expects rd, rt, shamt", mnemonic);
        }
        EXPECT_REG(tokens[0], &rd);
        EXPECT_REG(tokens[1], &rt);
        if (!parse_immediate(tokens[2], &imm) || imm < 0 || imm > 31) {
            FAIL("shamt '%s' out of range (0..31)", tokens[2]);
        }
        shamt_reg = (uint32_t)imm;
        *out_word = pack_r(info->opcode, 0, rt, rd, shamt_reg, info->funct);
        break;

    case OPERANDS_RS:
        if (!parse_register(operand_str, &rs)) {
            FAIL("invalid register '%s'", operand_str);
        }
        *out_word = pack_r(info->opcode, rs, 0, 0, 0, info->funct);
        break;

    case OPERANDS_RD_RS:
        n = split_commas(operand_str, tokens, 2);
        if (n != 2) {
            FAIL("%s expects rd, rs", mnemonic);
        }
        EXPECT_REG(tokens[0], &rd);
        EXPECT_REG(tokens[1], &rs);
        *out_word = pack_r(info->opcode, rs, 0, rd, 0, info->funct);
        break;

    case OPERANDS_RT_RS_IMM:
        n = split_commas(operand_str, tokens, 3);
        if (n != 3) {
            FAIL("%s expects rt, rs, imm", mnemonic);
        }
        EXPECT_REG(tokens[0], &rt);
        EXPECT_REG(tokens[1], &rs);
        EXPECT_IMM18(tokens[2], &imm);
        *out_word = pack_i(info->opcode, rs, rt, imm);
        break;

    case OPERANDS_RT_IMM:
        n = split_commas(operand_str, tokens, 2);
        if (n != 2) {
            FAIL("%s expects rt, imm", mnemonic);
        }
        EXPECT_REG(tokens[0], &rt);
        EXPECT_IMM18(tokens[1], &imm);
        *out_word = pack_i(info->opcode, 0, rt, imm);
        break;

    case OPERANDS_RT_IMM_RS:
        n = split_commas(operand_str, tokens, 2);
        if (n != 2) {
            FAIL("%s expects rt, imm(rs)", mnemonic);
        }
        EXPECT_REG(tokens[0], &rt);
        if (!parse_mem_operand(tokens[1], &imm, &rs)) {
            FAIL("invalid memory operand '%s' (expected imm(rs))", tokens[1]);
        }
        if (imm < IMM18_MIN || imm > IMM18_MAX) {
            FAIL("offset %ld out of 18-bit range (%d..%d)", (long)imm,
                 IMM18_MIN, IMM18_MAX);
        }
        *out_word = pack_i(info->opcode, rs, rt, imm);
        break;

    case OPERANDS_RS_RT_LABEL:
        n = split_commas(operand_str, tokens, 3);
        if (n != 3) {
            FAIL("%s expects rs, rt, label", mnemonic);
        }
        EXPECT_REG(tokens[0], &rs);
        EXPECT_REG(tokens[1], &rt);
        /* numeric only for now - label support is step 3 */
        EXPECT_IMM18(tokens[2], &imm);
        *out_word = pack_i(info->opcode, rs, rt, imm);
        break;

    case OPERANDS_LABEL:
        /* numeric only for now - label support is step 3 */
        if (!parse_immediate(operand_str, &imm)) {
            FAIL("invalid address '%s'", operand_str);
        }
        if (imm < 0 || imm > 0x3FFFFFF) {
            FAIL("address %ld out of 26-bit range (0..%d)", (long)imm,
                 0x3FFFFFF);
        }
        *out_word = pack_j(info->opcode, imm);
        break;
    }

    return ASSEMBLE_LINE_OK;
}
