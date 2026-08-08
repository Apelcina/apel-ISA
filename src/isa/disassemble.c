#include "disassemble.h"
#include <stdio.h>

bool disassemble(uint32_t addr, decoded_instr_t d, char *buf, size_t buf_size) {
    uint32_t funct = (d.format == FMT_R) ? d.funct : 0;
    const instr_info_t *info = lookup_by_encoding(d.format, d.opcode, funct);
    if (!info) {
        snprintf(buf, buf_size, "??? (unrecognized encoding)");
        return false;
    }

    switch (info->operands) {
    case OPERANDS_NONE:
        snprintf(buf, buf_size, "%s", info->mnemonic);
        break;
    case OPERANDS_RD_RS_RT:
        snprintf(buf, buf_size, "%s r%u, r%u, r%u", info->mnemonic, d.rd, d.rs, d.rt);
        break;
    case OPERANDS_RD_RT_SHAMT:
        snprintf(buf, buf_size, "%s r%u, r%u, %u", info->mnemonic, d.rd, d.rt, d.shamt);
        break;
    case OPERANDS_RS:
        snprintf(buf, buf_size, "%s r%u", info->mnemonic, d.rs);
        break;
    case OPERANDS_RD_RS:
        snprintf(buf, buf_size, "%s r%u, r%u", info->mnemonic, d.rd, d.rs);
        break;
    case OPERANDS_RT_RS_IMM:
        snprintf(buf, buf_size, "%s r%u, r%u, %d", info->mnemonic, d.rt, d.rs,
                 sign_extend18(d.imm));
        break;
    case OPERANDS_RT_IMM:
        snprintf(buf, buf_size, "%s r%u, %d", info->mnemonic, d.rt,
                 sign_extend18(d.imm));
        break;
    case OPERANDS_RT_IMM_RS:
        snprintf(buf, buf_size, "%s r%u, %d(r%u)", info->mnemonic, d.rt,
                 sign_extend18(d.imm), d.rs);
        break;
    case OPERANDS_RS_RT_LABEL: {
        /* matches take_branch_if's math exactly: shift in unsigned
           space to avoid left-shifting a negative signed value. */
        uint32_t offset = (uint32_t)sign_extend18(d.imm) << 2;
        uint32_t target = addr + 4 + offset;
        snprintf(buf, buf_size, "%s r%u, r%u, 0x%08X", info->mnemonic, d.rs,
                 d.rt, target);
        break;
    }
    case OPERANDS_LABEL: {
        /* matches execute_j's math exactly: top 4 bits borrowed from
           addr+4, not addr itself. */
        uint32_t next_addr = addr + 4;
        uint32_t target = (next_addr & 0xF0000000u) | (d.address << 2);
        snprintf(buf, buf_size, "%s 0x%08X", info->mnemonic, target);
        break;
    }
    }

    return true;
}
