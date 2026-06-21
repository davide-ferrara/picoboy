# Opcode mancanti

## Blocco semplificabile

### ADD HL,r16 (4 opcode)

Pattern: `(op & 0xCF) == 0x09` — pair = `(op >> 4) & 3`

| Opcode | Istruzione |
| ------ | ---------- |
| 0x09   | ADD HL,BC  |
| 0x19   | ADD HL,DE  |
| 0x29   | ADD HL,HL  |
| 0x39   | ADD HL,SP  |

## Blocchi piccoli

### Rotates / Shifts (4 opcode)

| Opcode | Istruzione | Flag               |
| ------ | ---------- | ------------------ |
| 0x07   | RLCA       | Z=0,N=0,H=0,C=bit7 |
| 0x0F   | RRCA       | Z=0,N=0,H=0,C=bit0 |
| 0x17   | RLA        | Z=0,N=0,H=0,C=bit7 |
| 0x1F   | RRA        | Z=0,N=0,H=0,C=bit0 |

### Flag ops (3 opcode)

| Opcode | Istruzione | Flag         |
| ------ | ---------- | ------------ |
| 0x2F   | CPL        | N=1,H=1      |
| 0x37   | SCF        | N=0,H=0,C=1  |
| 0x3F   | CCF        | N=0,H=0,C=!C |

### LDH (2 opcode)

| Opcode | Istruzione | Note                             |
| ------ | ---------- | -------------------------------- |
| 0xE0   | LDH (u8),A | write8(0xFF00 + fetch8(), cpu.a) |
| 0xF0   | LDH A,(u8) | cpu.a = read8(0xFF00 + fetch8()) |

---

## Singole

### Memory

| Opcode | Istruzione  | Note                                                                        |
| ------ | ----------- | --------------------------------------------------------------------------- |
| 0x08   | LD (u16),SP | 2 byte — fetch16() per indirizzo, poi write8 indirizzo e indirizzo+1 con SP |
| 0xE2   | LD (C),A    | write8(0xFF00 + cpu.c, cpu.a)                                               |
| 0xEA   | LD (u16),A  | write8(fetch16(), cpu.a)                                                    |
| 0xF2   | LD A,(C)    | cpu.a = read8(0xFF00 + cpu.c)                                               |
| 0xF9   | LD SP,HL    | cpu.sp = cpu.hl                                                             |
| 0xFA   | LD A,(u16)  | cpu.a = read8(fetch16())                                                    |

### Stack / 16-bit

| Opcode | Istruzione  | Note                                           |
| ------ | ----------- | ---------------------------------------------- |
| 0xE8   | ADD SP,i8   | SP += (int8_t)fetch8() — flag: Z=0,N=0,H,C     |
| 0xF8   | LD HL,SP+i8 | HL = SP + (int8_t)fetch8() — flag: Z=0,N=0,H,C |

### Interrupt

| Opcode | Istruzione | Note                                    |
| ------ | ---------- | --------------------------------------- |
| 0xD9   | RETI       | Come RET + EI implicito                 |
| 0xF3   | DI         | cpu.ime = 0                             |
| 0xFB   | EI         | cpu.ime = 1 (differito di 1 istruzione) |

### Varie

| Opcode | Istruzione | Note                                                                       |
| ------ | ---------- | -------------------------------------------------------------------------- |
| 0x10   | STOP       | Spegne LCD finché non arriva un input (necessita interrupt infrastruttura) |
| 0x27   | DAA        | Decimal adjust per BCD — modifica A e flag Z,H,C                           |

---

## CB Prefix (0xCB)

256 opcode. Pattern: `op >> 3` = tipo, `op & 7` = registro.

| Range     | Tipo    | Flag                           |
| --------- | ------- | ------------------------------ |
| 0x00–0x07 | RLC     | Z,N=0,H=0,b7→C                 |
| 0x08–0x0F | RRC     | Z,N=0,H=0,b0→C                 |
| 0x10–0x17 | RL      | Z,N=0,H=0,b7→C (con carry)     |
| 0x18–0x1F | RR      | Z,N=0,H=0,b0→C (con carry)     |
| 0x20–0x27 | SLA     | Z,N=0,H=0,b7→C                 |
| 0x28–0x2F | SRA     | Z,N=0,H=0,b0→C,bit7 preservato |
| 0x30–0x37 | SWAP    | Z,N=0,H=0,C=0                  |
| 0x38–0x3F | SRL     | Z,N=0,H=0,b0→C                 |
| 0x40–0x7F | BIT n,r | Z=bit, N=0, H=1                |
| 0x80–0xBF | RES n,r | nessuno                        |
| 0xC0–0xFF | SET n,r | nessuno                        |

Tutti 8t, tranne BIT n,(HL) = 12t e operazioni (HL) = 12t.

---

## Priorità per Tetris

1. RST — subito, semplice
2. ADD HL,r16 — calcolo indirizzi
3. LDH — joypad
4. LD (u16),A / LD A,(u16) — accesso RAM
5. LD SP,HL — setup
6. Interrupt (DI/EI/RETI) + VBlank
7. CB prefix (BIT) — pulsanti
8. Rotates / CPL/SCF/CCF
9. ADD SP,i8 / LD HL,SP+i8
10. DAA / STOP / LD (u16),SP
