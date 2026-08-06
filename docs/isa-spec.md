# apel-ISA — Instruction Set Architecture Spec

Status: **phase 1 closed** — locked baseline for phase 2 (emulator). Amend
here first if a later phase needs an encoding change; don't decide it
implicitly in code.

This is the source of truth for the architecture. The emulator, assembler,
and disassembler all derive their encode/decode tables from this document —
nothing here should be decided implicitly in code.

## Locked decisions

- Word size: **32-bit**
- Instruction width: **fixed, 32 bits** (one word per instruction)
- Byte order: **little-endian**
- Registers: **16 general-purpose (`r0`-`r15`), `r0` hardwired to zero**
- Instruction formats: **MIPS-style R / I / J**
- Condition handling: **compare-and-branch** (no flags register)

## Register file

| Register | Convention | Notes |
|----------|------------|-------|
| `r0`     | zero       | hardwired to 0, writes are discarded |
| `r1`-`r15` | general purpose | no fixed roles yet — calling convention (sp/ra/args) gets assigned in phase 8 when the compiler needs one |

4-bit register fields (16 registers) throughout, vs. MIPS's 5-bit (32
registers) — the bits saved go toward a wider immediate field (see I-type).

## Instruction formats

All instructions are 32 bits, MSB-first field order.

### R-type — register-register ALU ops, shifts, jump-register, syscall/break

```
31        26 25   22 21   18 17   14 13         6 5      0
+-----------+-------+-------+-------+-------------+--------+
|  opcode   |  rs   |  rt   |  rd   |    shamt    | funct  |
|    (6)    |  (4)  |  (4)  |  (4)  |     (8)     |  (6)   |
+-----------+-------+-------+-------+-------------+--------+
```

- `opcode` is always `0x00` for R-type; `funct` selects the actual operation.
- `shamt` is 8 bits wide but only the low 5 bits are used (shift range 0-31
  for a 32-bit word); top 3 bits reserved for future use.
- result goes to `rd`; `rs`/`rt` are source operands (unused fields should
  be encoded as 0).

### I-type — ALU-immediate, loads/stores, branches

```
31        26 25   22 21   18 17                             0
+-----------+-------+-------+------------------------------+
|  opcode   |  rs   |  rt   |          immediate           |
|    (6)    |  (4)  |  (4)  |             (18)             |
+-----------+-------+-------+------------------------------+
```

- ALU-immediate: `rt = rs OP sign_or_zero_ext(immediate)`
- Load/store: `rs` = base address register, `rt` = dest/src data register,
  `immediate` = signed byte offset added to `rs`
- Branch: `rs`/`rt` = operands to compare, `immediate` = signed word offset
  added to `PC+4` if the comparison is true (PC-relative)

18-bit immediate (vs. MIPS's 16) — covers ±131072, or 0-262143 unsigned.
`LUI` exists for building full 32-bit constants when that's not enough.

### J-type — unconditional jumps

```
31        26 25                                             0
+-----------+--------------------------------------------- +
|  opcode   |                   address                    |
|    (6)    |                    (26)                      |
+-----------+-----------------------------------------------+
```

- Pseudo-direct addressing, MIPS-style: target = `(PC+4)[31:28] : address : 00`
  (address field holds a word address, shifted left 2 to byte-align; top 4
  bits inherited from the following instruction's PC since we don't have
  room to encode a full 32-bit address in 26 bits).

## Addressing modes

- **Register-direct** — R-type operands (`rs`, `rt`, `rd`)
- **Immediate** — ALU-immediate I-type ops
- **Base + offset** — loads/stores (`rs` + sign-extended `immediate`)
- **PC-relative** — branches (`immediate` is a signed word offset from `PC+4`)
- **Pseudo-direct** — `J`/`JAL` (26-bit address field + high PC bits)
- **Register-indirect** — `JR`/`JALR` (jump target comes from a register,
  needed for returns and computed jumps)

## Sign extension rules

- Loads, stores, branches, and arithmetic immediates (`ADDI`, `SLTI`,
  `SLTIU`) sign-extend the 18-bit immediate.
- Logical immediates (`ANDI`, `ORI`, `XORI`) zero-extend instead — this is
  the MIPS convention and avoids surprise sign bits leaking into bitwise
  masks.

## Opcode table

Opcode field is 6 bits (64 possible values). Currently allocated:

| Opcode (hex) | Mnemonic | Format | Meaning |
|---|---|---|---|
| `0x00` | *(R-type)* | R | see funct table below |
| `0x02` | `J`      | J | `PC = target` |
| `0x03` | `JAL`    | J | `r1 = PC+4; PC = target` (link register: `r1` until phase 8 picks a convention) |
| `0x04` | `BEQ`    | I | `if (rs == rt) PC += 4 + (imm << 2)` |
| `0x05` | `BNE`    | I | `if (rs != rt) PC += 4 + (imm << 2)` |
| `0x06` | `BLT`    | I | `if (rs < rt)  PC += 4 + (imm << 2)` (signed) |
| `0x07` | `BGE`    | I | `if (rs >= rt) PC += 4 + (imm << 2)` (signed) |
| `0x08` | `ADDI`   | I | `rt = rs + sext(imm)` |
| `0x09` | `SLTI`   | I | `rt = (rs < sext(imm)) ? 1 : 0` (signed) |
| `0x0A` | `SLTIU`  | I | `rt = (rs < sext(imm)) ? 1 : 0` (unsigned compare) |
| `0x0C` | `ANDI`   | I | `rt = rs & zext(imm)` |
| `0x0D` | `ORI`    | I | `rt = rs \| zext(imm)` |
| `0x0E` | `XORI`   | I | `rt = rs ^ zext(imm)` |
| `0x0F` | `LUI`    | I | `rt = imm << 14` (fills the top 18 bits of a 32-bit word) |
| `0x20` | `LB`     | I | `rt = sext(mem8[rs + sext(imm)])` |
| `0x21` | `LH`     | I | `rt = sext(mem16[rs + sext(imm)])` |
| `0x23` | `LW`     | I | `rt = mem32[rs + sext(imm)]` |
| `0x24` | `LBU`    | I | `rt = zext(mem8[rs + sext(imm)])` |
| `0x25` | `LHU`    | I | `rt = zext(mem16[rs + sext(imm)])` |
| `0x28` | `SB`     | I | `mem8[rs + sext(imm)] = rt[7:0]` |
| `0x29` | `SH`     | I | `mem16[rs + sext(imm)] = rt[15:0]` |
| `0x2B` | `SW`     | I | `mem32[rs + sext(imm)] = rt` |

Opcodes `0x01`, `0x0B`, `0x10`-`0x1F`, `0x22`, `0x26`-`0x27`, `0x2A`,
`0x2C`-`0x3F` are reserved / unallocated.

### R-type funct table (opcode `0x00`)

| Funct (hex) | Mnemonic | Meaning |
|---|---|---|
| `0x00` | `ADD`   | `rd = rs + rt` |
| `0x01` | `SUB`   | `rd = rs - rt` |
| `0x02` | `AND`   | `rd = rs & rt` |
| `0x03` | `OR`    | `rd = rs \| rt` |
| `0x04` | `XOR`   | `rd = rs ^ rt` |
| `0x05` | `NOR`   | `rd = ~(rs \| rt)` |
| `0x06` | `SLL`   | `rd = rt << shamt` |
| `0x07` | `SRL`   | `rd = rt >> shamt` (logical) |
| `0x08` | `SRA`   | `rd = rt >> shamt` (arithmetic) |
| `0x09` | `SLT`   | `rd = (rs < rt) ? 1 : 0` (signed) |
| `0x0A` | `SLTU`  | `rd = (rs < rt) ? 1 : 0` (unsigned) |
| `0x0B` | `JR`    | `PC = rs` |
| `0x0C` | `JALR`  | `rd = PC+4; PC = rs` |
| `0x3E` | `SYSCALL` | trap to kernel (phase 5 — syscall number convention TBD) |
| `0x3F` | `BREAK`   | trap to debugger (phase 4) |

Funct codes `0x0D`-`0x3D` reserved for future ALU ops / extensions.

## Pseudo-instructions

These have no dedicated opcode/funct — the assembler (phase 3) rewrites
them into a real instruction at assemble time. They exist purely for
readability; the hardware/emulator never sees them, only the expansion.
The zero register (`r0`) is what makes most of these free — no ALU
operation needed to be reserved just to move or clear data.

| Pseudo | Expands to | Why |
|---|---|---|
| `MOV rd, rs` | `ADD rd, rs, r0` | adding zero is a copy |
| `NOT rd, rs` | `NOR rd, rs, r0` | `~(rs \| 0) == ~rs` |
| `CLEAR rd`   | `ADD rd, r0, r0` | zero + zero is zero |
| `NOP`        | `ADD r0, r0, r0` | result discarded (writes to r0), no visible effect |

More will get added here as they come up (phase 3 is the natural place to
grow this list) — this isn't meant to be exhaustive yet.

## Memory model

Flat, byte-addressable, 32-bit address space. No MMU/paging (that's out of
scope until, if ever, we revisit phase 10). Loads/stores are unaligned-safe
in the *spec* sense (no fault requirement) but the emulator may choose to
enforce alignment later for realism — TBD when we get to phase 2.

Caches and virtual memory (page tables, TLB, physical vs. virtual
addressing) are deliberately **not** part of the ISA — a load/store's
meaning ("read/write N bytes at this address") is defined independently of
how a real implementation serves it. That's the point of the ISA/
microarchitecture split: caches and paging are performance/isolation
concerns layered *underneath* the contract these instructions define, not
part of the contract itself. Phase 10 (stretch) is where cache/pipeline
timing first enters, as a second, separate simulator. Virtual memory /
paging isn't currently scoped in any phase — worth revisiting the roadmap
explicitly if we want to learn it hands-on later.
