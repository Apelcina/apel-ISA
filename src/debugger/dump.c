#include "dump.h"
#include <ctype.h>

void dump_registers(const cpu_t *cpu, FILE *out) {
    /* "READY" rather than "running" - dump_registers gets called from a
       paused interactive debugger just as often as from something
       actually mid-execution, and !halted only ever means "hasn't
       stopped," never "is actively executing right now." */
    fprintf(out, "pc = 0x%08X  %s\n", cpu->pc, cpu->halted ? "HALTED" : "READY");

    for (unsigned i = 0; i < APEL_NUM_REGS; i++) {
        fprintf(out, "r%-2u=0x%08X%s", i, cpu_reg_read(cpu, i),
                (i % 4 == 3) ? "\n" : "  ");
    }
}

void dump_memory(const cpu_t *cpu, FILE *out, uint32_t start, uint32_t length) {
    if ((uint64_t)start >= cpu->mem_size) {
        fprintf(out, "(start address 0x%08X is out of bounds)\n", start);
        return;
    }

    uint64_t end = (uint64_t)start + length;
    if (end > cpu->mem_size) {
        end = cpu->mem_size;
    }

    for (uint32_t addr = start; addr < end; addr += 16) {
        uint32_t line_end = addr + 16;
        if (line_end > end) {
            line_end = (uint32_t)end;
        }

        fprintf(out, "0x%08X: ", addr);
        for (uint32_t i = addr; i < addr + 16; i++) {
            if (i < line_end) {
                fprintf(out, "%02X ", cpu->mem[i]);
            } else {
                fprintf(out, "   ");
            }
        }

        fprintf(out, " ");
        for (uint32_t i = addr; i < line_end; i++) {
            unsigned char c = cpu->mem[i];
            fprintf(out, "%c", isprint(c) ? c : '.');
        }
        fprintf(out, "\n");
    }
}
