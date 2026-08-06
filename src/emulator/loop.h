#ifndef APEL_EMULATOR_LOOP_H
#define APEL_EMULATOR_LOOP_H

#include "cpu.h"
#include <stddef.h>

/* Reads the 4 bytes at cpu->pc as a little-endian 32-bit word. Does not
   bounds-check pc against memory size - safe today because cpu_run's
   max_steps bound keeps pc well within the allocated buffer. Revisit
   once jumps/branches make pc a computed, potentially-arbitrary value. */
uint32_t cpu_fetch(const cpu_t *cpu);

/* Writes `words` into memory starting at address 0, little-endian.
   Returns false (and writes nothing) if the program doesn't fit. */
bool cpu_load_program(cpu_t *cpu, const uint32_t *words, size_t count);

/* One fetch-decode-execute cycle. No-op if already halted. */
void cpu_step(cpu_t *cpu);

/* Steps until halted or max_steps is reached, whichever comes first.
   Returns the number of steps actually run. */
uint32_t cpu_run(cpu_t *cpu, uint32_t max_steps);

#endif
