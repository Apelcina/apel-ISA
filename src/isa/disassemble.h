#ifndef APEL_ISA_DISASSEMBLE_H
#define APEL_ISA_DISASSEMBLE_H

#include "isa.h"
#include <stdbool.h>
#include <stddef.h>

/* Formats a decoded instruction as text into buf, e.g.
   "BGE r2, r3, 0x0000001C". addr is this instruction's own address -
   needed to turn a branch/jump's raw offset into an absolute target
   address for display, the reverse of what assemble_line() does when
   it turns a label into an offset. Uses the exact same target-address
   math as execute.c's take_branch_if/execute_j, so what's printed here
   always matches where execution would actually go.
   Returns false (buf holds a placeholder) for an encoding with no
   matching mnemonic - the same cases decode()/execute() treat as
   invalid or not-yet-implemented. */
bool disassemble(uint32_t addr, decoded_instr_t d, char *buf, size_t buf_size);

#endif
