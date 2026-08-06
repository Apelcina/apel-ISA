# apel-ISA — Instruction Set Architecture Spec

Status: **draft / phase 1 in progress**

This is the source of truth for the architecture. The emulator, assembler,
and disassembler all derive their encode/decode tables from this document —
nothing here should be decided implicitly in code.

## Locked decisions

- Word size: **32-bit**
- Instruction width: **fixed, 32 bits** (one word per instruction)
- Byte order: TBD (little-endian recommended unless there's a reason not to)

## Open questions (phase 1)

- [ ] Register count and naming (e.g. 16 vs 32 general-purpose registers;
      is `r0` hardwired to zero?)
- [ ] Instruction formats (R/I/J-style split of opcode/register/immediate
      fields) and how many formats we need
- [ ] Opcode field width (how many instructions can we address) vs immediate
      field width (how large a constant fits in one instruction) — this is
      the central encoding tradeoff
- [ ] Addressing modes (register-direct, base+offset, PC-relative for
      branches, immediate)
- [ ] Condition handling: flags register vs compare-and-branch vs
      compare-into-register
- [ ] Core instruction categories to support: ALU ops, loads/stores,
      branches/jumps, and what we need later for syscalls/traps (phase 5)
- [ ] Sign extension rules for immediates
- [ ] Reserved opcode space for future use (traps, extensions)

## Instruction formats

_TBD — filled in once the open questions above are resolved._

## Opcode table

_TBD._

## Register file

_TBD._

## Memory model

_TBD — flat, byte-addressable, 32-bit address space assumed unless a reason
emerges to do otherwise._
