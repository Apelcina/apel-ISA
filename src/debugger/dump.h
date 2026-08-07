#ifndef APEL_DEBUGGER_DUMP_H
#define APEL_DEBUGGER_DUMP_H

#include "cpu.h"
#include <stdio.h>

/* Prints pc, halted status, and all 16 registers, 4 per line. */
void dump_registers(const cpu_t *cpu, FILE *out);

/* Prints a hex + ASCII dump of [start, start+length), 16 bytes per line.
   Silently clamps length if start+length would exceed cpu->mem_size -
   this is a human-facing debug tool, not an instruction execution path,
   so there's no "halt the cpu" response that would make sense here. */
void dump_memory(const cpu_t *cpu, FILE *out, uint32_t start, uint32_t length);

#endif
