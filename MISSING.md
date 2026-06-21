# Opcode mancanti

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
