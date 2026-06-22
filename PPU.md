# Game Boy PPU — Teoria

## 1. Lo schermo

- Risoluzione: **160 × 144 pixel** (20 × 18 tile)
- Refresh: ~59.73 Hz
- Colori: **4 sfumature di grigio** 2 bit (dal Game Boy originale)
  - 00 = bianco, 01 = grigio chiaro, 10 = grigio scuro, 11 = nero

---

## 2. VRAM - Video RAM

**8 KB** — da 0x8000 a 0x9FFF. Divisa in due zone:

| Zona      | Indirizzo     | Contenuto                                                        |
| --------- | ------------- | ---------------------------------------------------------------- |
| Tile data | 0x8000–0x97FF | I pixel dei tile (384 tile da 8×8, o 256+256 con bank switching) |
| Tile maps | 0x9800–0x9FFF | Due griglie 32×32 che dicono "metti tile X a posizione Y"        |

---

## 3. Tile

Un **tile** è un quadratino **8×8 pixel**. Ogni pixel ha 2 bit → 4 colori.

### Come sono memorizzati

16 byte per tile, organizzati in 2 righe × 8 colonne:

```
Byte 0–1:  riga 0
Byte 2–3:  riga 1
...
Byte 14–15: riga 7
```

Ogni coppia di byte per riga funziona così:

```
Byte 0: bit plane basso (LSB del colore per ogni pixel)
Byte 1: bit plane alto (MSB del colore per ogni pixel)

Pixel 7:  (byte1[7] << 1) | byte0[7]
Pixel 6:  (byte1[6] << 1) | byte0[6]
...
Pixel 0:  (byte1[0] << 1) | byte0[0]
```

**Esempio:**

```
Byte 0 (low):  0100 1101
Byte 1 (high): 0011 0100
Colore:        0211 2301  ← 0=b, 1=g chiaro, 2=g scuro, 3=nero
Pixel:          ▓░██ █▌▒█
```

### Tile data: due modalità

L'indirizzo di partenza dei tile è controllato dal bit 4 di **LCDC**.

| LCDC bit 4 | Mappatura                                                                      |
| ---------- | ------------------------------------------------------------------------------ |
| 0          | 0x8800 (signed): tile 0–127 a 0x8800, tile 128–255 a 0x9000 (indice -128..127) |
| 1          | 0x8000 (unsigned): tile 0–255 a 0x8000                                         |

---

## 4. Tile map — la griglia

Un tile map è una matrice **32×32** (256×256 pixel = più grande dello schermo visibile di 160×144). Ogni cella è 1 byte:

```
Byte nella tile map = indice del tile da disegnare in quella cella
```

Ci sono due tile map, indirizzo controllato da LCDC:

| LCDC bit | Mappa          | Indirizzo                       |
| -------- | -------------- | ------------------------------- |
| bit 3    | Background map | 0x9800 (bit=0) o 0x9C00 (bit=1) |
| bit 6    | Window map     | 0x9800 (bit=0) o 0x9C00 (bit=1) |

Lo schermo mostra una **finestra** di 160×144 pixel sulla mappa 256×256. SCX e SCY controllano **cosa** è visibile.

---

## 5. Registri PPU (I/O memory: 0xFF40–0xFF4B)

| Registro | Indirizzo | Descrizione                            |
| -------- | --------- | -------------------------------------- |
| **LCDC** | 0xFF40    | Configurazione display                 |
| **STAT** | 0xFF41    | Stato PPU + interrupt                  |
| **SCY**  | 0xFF42    | Scroll Y del background                |
| **SCX**  | 0xFF43    | Scroll X del background                |
| **LY**   | 0xFF44    | Linea corrente (0–153)                 |
| **LYC**  | 0xFF45    | Linea di confronto (trigger interrupt) |
| **DMA**  | 0xFF46    | OAM DMA transfer                       |
| **BGP**  | 0xFF47    | Palette background/window              |
| **OBP0** | 0xFF48    | Palette sprite (bit 0-3)               |
| **OBP1** | 0xFF49    | Palette sprite (bit 4-5)               |
| **WY**   | 0xFF4A    | Window Y position                      |
| **WX**   | 0xFF4B    | Window X position (+7 offset)          |

---

## 6. LCDC Register (0xFF40) — bit per bit

| Bit | Nome             | 0                     | 1              |
| --- | ---------------- | --------------------- | -------------- |
| 7   | LCD enable       | Schermo spento        | Schermo acceso |
| 6   | Window tile map  | 0x9800                | 0x9C00         |
| 5   | Window enable    | Off                   | On             |
| 4   | Tile data select | 0x8800 (signed)       | 0x8000         |
| 3   | BG tile map      | 0x9800                | 0x9C00         |
| 2   | Sprite size      | 8×8                   | 8×16           |
| 1   | Sprite enable    | Off                   | On             |
| 0   | BG/Window enable | Off (priorità sprite) | On             |

---

## 7. Palette

Ogni pixel ha 2 bit → indice 0–3 in una palette a 4 colori.

### BGP (Background Palette) — 0xFF47

```
Bit: 7-6   5-4   3-2   1-0
     [33]  [22]  [11]  [00]  ← indice pixel → colore
```

Ogni coppia di bit mappa un indice pixel a un colore:

- 00 → bianco
- 01 → grigio chiaro
- 10 → grigio scuro
- 11 → nero

**Esempio**: `BGP = 0xE4 = 11 10 01 00`

```
Indice 0 = 00 = bianco
Indice 1 = 01 = grigio chiaro
Indice 2 = 10 = grigio scuro
Indice 3 = 11 = nero
```

OBP0 e OBP1 funzionano allo stesso modo, ma per gli sprite.

---

## 8. Lo scorrimento (Scrolling)

### SCX e SCY

Con SCX=0 e SCY=0, lo schermo mostra l'angolo in alto a sinistra della tile map (celle 0,0).

```
SCX = 5  → sposta tutto a SINISTRA di 5 pixel
SCY = 3  → sposta tutto in ALTO di 3 pixel
```

L'area visibile comincia a (SCX, SCY) nella mappa 256×256.

---

## 9. Sprites (OGGETTI)

Il Game Boy supporta fino a **40 sprite**, ma solo **10 per riga** visibili.

### OAM (Object Attribute Memory)

Area 0xFE00–0xFE9F, 160 byte. Ogni sprite occupa **4 byte**:

| Byte | Campo | Descrizione                                                      |
| ---- | ----- | ---------------------------------------------------------------- |
| 0    | Y     | Posizione Y - 16 (in alto a sinistra = Y=16 per sprite in cima)  |
| 1    | X     | Posizione X - 8 (in alto a sinistra = X=8 per sprite a sinistra) |
| 2    | Tile  | Indice del tile (0–255)                                          |
| 3    | Flags | Attributi: palette, flip X/Y, priorità                           |

### Flags (byte 3)

```
Bit 7: Priorità (0=sopra BG, 1=dietro BG nei pixel non trasparenti)
Bit 6: Flip Y
Bit 5: Flip X
Bit 4: Palette (0=OBP0, 1=OBP1)
```

### Sprites 8×16

Se LCDC bit 2 = 1, gli sprite sono 8×16 pixel (due tile impilati). Il bit 0 del tile index viene ignorato.

---

## 10. Window

Una "finestra" fissa sopra il background, non scrollata.

- WY (0xFF4A): dove comincia la window in verticale
- WX (0xFF4B): dove comincia in orizzontale **(+7 offset)** — WX=7 significa bordo sinistro

La window copre il background dal punto (WX-7, WY) fino al bordo destro e in basso. Usa la stessa tile map e gli stessi tile del background.

---

## 11. Ciclo di rendering di una scanline

Ogni riga dello schermo (scanline) passa attraverso 3 fasi:

### Modalità 2 — OAM Scan (80 clock)

La PPU cerca quali sprite compaiono su questa riga. Carica fino a 10 sprite nell'OAM interno.

### Modalità 3 — Pixel Transfer (172-289 clock)

La PPU disegna i pixel di questa riga:

1. Legge i tile dal tile map
2. Legge i dati pixel dei tile
3. Mescola background, window e sprite
4. Scrive i pixel finali nel buffer interno

### Modalità 0 — H-Blank (87-204 clock)

Pausa orizzontale. La PPU non disegna niente, attende la prossima riga.

---

## 12. Scanline totali e V-Blank

Le scanline vanno da **0 a 153**:

| Scanline (LY) | Cosa succede                              |
| ------------- | ----------------------------------------- |
| 0–143         | Righe visibili (144 linee)                |
| 144–153       | **V-Blank** — 10 righe di pausa verticale |

Durante il V-Blank (modalità 1), la PPU non disegna, VRAM e OAM sono accessibili.

---

## 13. STAT Register (0xFF41)

| Bit | Descrizione                                                     |
| --- | --------------------------------------------------------------- |
| 0–1 | Modalità PPU (0=HBlank, 1=VBlank, 2=OAM scan, 3=Pixel transfer) |
| 2   | LYC = LY flag (1 quando LY == LYC)                              |
| 3   | Interrupt su H-Blank (0=off, 1=on)                              |
| 4   | Interrupt su V-Blank (STAT)                                     |
| 5   | Interrupt su OAM scan                                           |
| 6   | Interrupt su LYC == LY                                          |

---

## 14. Interrupt

Il Game Boy ha 5 tipi di interrupt. Quelli rilevanti per la PPU:

| Interrupt    | Bit | Trigger                    |
| ------------ | --- | -------------------------- |
| **V-Blank**  | 0   | Inizio V-Blank (LY=144)    |
| **LCD STAT** | 1   | In base ai bit 3-6 di STAT |

La CPU ha due registri a 0xFF0F (IF) e 0xFFFF (IE). Quando un interrupt è abilitato (IE) e richiesto (IF), la CPU chiama il vettore corrispondente.

---

## 15. DMA (OAM DMA)

Scrivendo un valore a 0xFF46, vengono copiati 160 byte da SRAM/ROM a OAM (0xFE00) in 160 microsecondi (640 clock T-cycle):

```
0x0000 << DMA = 0x00 → copia da 0x0000
0xFE00 << DMA = 0xFE → copia da 0xFE00 (ROM area)
```

---

## 16. Pixel FIFO — come funziona dentro la PPU

Ogni scanline, la PPU ha due FIFO (code):

### FIFO Background

1. **Fetch tile index** dal tile map (2 clock)
2. **Fetch tile data low** dalla VRAM (2 clock)
3. **Fetch tile data high** dalla VRAM (2 clock)
4. Push di 8 pixel nella FIFO (2 clock di pausa dopo ogni tile)

Totale: **~10-11 clock per tile** (8 pixel)

### FIFO Sprite

Durante la modalità 2, gli sprite attivi vengono pre-caricati. Durante la modalità 3, i pixel sprite vengono mescolati coi pixel background nella FIFO finale.

---

## 17. Riepilogo: rendering di un frame

```
Per ogni scanline LY = 0..153:
    Se LY < 144 (visibile):
        Modo 2 (OAM scan):   ~80 clock
        Modo 3 (draw):      ~172 clock
        Modo 0 (H-Blank):   ~204 clock  (variabile)

    Se LY == 144:
        VBlank inizia → interrupt VBlank

    Se LY >= 144 (VBlank, modo 1):
        ~4560 clock (456 × 10 linee)
        OAM e VRAM liberamente accessibili

    Se LY == 153:
        Ultima linea VBlank, poi LY=0
```

Totale: **456 clock × 154 linee = 70224 clock per frame** (~59.73 fps).

---

## 18. Ordine di implementazione consigliato

1. Registri LCDC, STAT, LY — leggi/scrittura base
2. Modalità PPU e transizioni (OAM scan → draw → HBlank → VBlank)
3. Timer scanline (avanza LY ogni 456 clock)
4. Rendering background (leggi tile map + tile data + palette → pixel)
5. Scroll (SCX/SCY)
6. Sprites (OAM + priorità)
7. Interrupt (VBlank almeno)
8. Window
9. DMA

---

Se qualcosa non è chiaro, chiedi e aggiorno il documento.

---

## 19. Piano di implementazione

### Strutture dati

#### PPU

Stato globale della PPU. Contiene:

| Campo | Tipo | Descrizione |
|-------|------|-------------|
| `mode` | uint8_t | Modalità corrente (0=HBlank, 1=VBlank, 2=OAM, 3=Draw) |
| `clock` | uint16_t | Cicli accumulati sulla scanline corrente (0–455) |
| `ly` | uint8_t | Scanline corrente (0–153) |
| `lcdc` | uint8_t | Ombra del registro LCDC (cache locale) |
| `stat` | uint8_t | Ombra di STAT |
| `scy` | uint8_t | Scroll Y |
| `scx` | uint8_t | Scroll X |
| `lyc` | uint8_t | LY Compare |
| `bgp` | uint8_t | Palette background/window |
| `obp0` | uint8_t | Palette sprite 0 |
| `obp1` | uint8_t | Palette sprite 1 |
| `wy` | uint8_t | Window Y |
| `wx` | uint8_t | Window X |
| `dma_src` | uint16_t | Sorgente DMA in corso |
| `dma_len` | uint8_t | Byte rimanenti da copiare |
| `framebuffer` | uint8_t[144][160] | Immagine finale (indice colore 0–3 per pixel) |

Decidi tu se framebuffer è dentro la PPU o esterno (es. passato a una libreria grafica). L'alternativa è un array `uint32_t[160*144]` con colori RGB già risolti.

#### Sprite (OAM entry)

Struct da 4 byte per descrivere uno sprite:

| Campo | Tipo | Descrizione |
|-------|------|-------------|
| `y` | uint8_t | Y posizione reale (dal byte OAM) |
| `x` | uint8_t | X posizione reale |
| `tile` | uint8_t | Indice tile |
| `flags` | uint8_t | Byte attributi (priorità, flip, palette) |

#### Scanline sprites (buffer interno)

Array di massimo 10 sprite attivi sulla riga corrente. Riempito durante l'OAM scan. Ogni entry è una copia dell'OAM entry + qualche flag derivato (es. "ho già disegnato questo sprite").

---

### Funzioni principali

#### `ppu_init()`
- Inizializza la struct PPU
- Imposta `mode = 1` (VBlank)
- `ly = 0`, `clock = 0`
- Registri a valori di default (LCDC, palette, ecc.)

#### `ppu_step(cycles int) int`
- **Il cuore del timing.** Chiamata dalla CPU dopo ogni istruzione col numero di T-cycles consumati.
- Avanza `clock` di `cycles`.
- In base a `clock` e `ly`, determina se cambiare modalità o avanzare LY.
- Ritorna il tipo di interrupt generato (0=nessuno, o bitmask tipo `IF_VBLANK | IF_LCDSTAT`), così la CPU sa se settare IF.

**Logica interna di `ppu_step`:**

1. Se LCD spento (LCDC bit 7 = 0): resetta LY, clock e mode. Non fare altro.
2. Se `clock >= 456`: `clock -= 456`, `ly++`, e gestisci l'avanzamento di LY (vedi sotto).
3. Se `ly >= 144`: siamo in VBlank. Setta `mode = 1`.
4. Se `ly < 144`: siamo in riga visibile. Scegli la modalità in base a `clock`:
   - `clock < 80` → mode 2 (OAM scan)
   - `clock < 252` circa → mode 3 (draw)
   - `clock < 456` → mode 0 (HBlank)
5. Al cambio di modalità, aggiorna i bit 0–1 di STAT.
6. Al cambio di LY, verifica LYC == LY e aggiorna il bit 2 di STAT.

**Eventi speciali:**
- Quando **LY passa da 143 a 144**: inizio VBlank. Richiedi interrupt VBlank.
- Quando **mode passa a 0** (HBlank): se STAT bit 3 = 1, richiedi STAT interrupt.
- Quando **mode passa a 1** (VBlank): se STAT bit 4 = 1, richiedi STAT interrupt.
- Quando **mode passa a 2** (OAM): se STAT bit 5 = 1, richiedi STAT interrupt.
- Quando **LY == LYC**: se STAT bit 6 = 1, richiedi STAT interrupt.

#### `ppu_draw_scanline()`
- Chiamata all'inizio della modalità 3 (quando `clock` passa da 79 a 80).
- Produce i 160 pixel della riga `ly` e li scrive nel framebuffer.
- Si divide in sotto-funzioni.

**Sotto-funzioni:**

##### `draw_bg_pixels()`
- Per ogni x = 0..159:
  1. Calcola le coordinate nella tile map: `map_x = (scx + x) & 255`, `map_y = (scy + ly) & 255`
  2. Calcola la cella nella tile map: `col = map_x / 8`, `row = map_y / 8`
  3. Leggi il byte a `tile_map_base + row * 32 + col` → è l'indice del tile
  4. Risolvi l'indirizzo del tile in VRAM (modalità signed vs unsigned)
  5. Calcola l'offset dentro il tile: `tile_x = map_x % 8`, `tile_row = map_y % 8`
  6. Leggi i 2 byte della riga `tile_row` dal tile
  7. Estrai il bit plane per il pixel `tile_x`
  8. Combina: `color_idx = (high_byte_bit << 1) | low_byte_bit`
  9. Passa il `color_idx` nella palette BGP → ottieni il colore finale

##### `draw_win_pixels()`
- Attiva solo se LCDC bit 5 = 1 e se `ly >= wy` e `wx >= 7` (la window è visibile).
- Per ogni x che cade dentro la window:
  1. `win_x = x - (wx - 7)`
  2. `win_y = ly - wy`
  3. Stessa logica del background, ma con un **contatore interno** della window (non scrollato).
  4. Sostituisce il pixel background nella stessa posizione.

##### `draw_sprite_pixels()`
- Per ogni sprite nel buffer della scanline:
  1. Per ogni colonna x occupata dallo sprite su questa riga:
  2. Leggi i 2 byte del tile sprite come per il background
  3. Applica flip X/Y se richiesto
  4. Se il pixel sprite non è trasparente (color_idx ≠ 0):
     - Se priorità sprite sopra BG: sovrascrivi sempre
     - Se priorità dietro BG: sovrascrivi solo dove BG è colore 0
     - Se due sprite competono sullo stesso pixel: vince quello con X più basso
  5. Applica palette OBP0 o OBP1

---

### I/O Read/Write handlers

#### `ppu_read(addr uint16_t) uint8_t`
- Mappa gli indirizzi 0xFF40–0xFF4B ai campi della struct PPU.
- Per **LY**: restituisce il valore reale di `ly` (non una cache).
- Per **STAT**: restituisce `stat` con i bit 0–2 calcolati dinamicamente (mode + LYC flag).

#### `ppu_write(addr uint16_t, val uint8_t)`
- Mappa le scritture ai campi della struct PPU.
- **LCDC (0xFF40)**: se il bit 7 passa da 1 a 0 (LCD spento), resetta LY e mode.
- **STAT (0xFF41)**: solo i bit 3–6 sono scrivibili. I bit 0–2 sono read-only.
- **LYC (0xFF45)**: aggiorna e ricontrolla subito LYC==LY.
- **DMA (0xFF46)**: avvia il trasferimento DMA. Imposta `dma_src = val << 8` e `dma_len = 160`.

#### `ppu_dma_step(cycles int)`
- Chiamata dalla CPU durante DMA attivo.
- Copia `cycles` byte da `dma_src` a `0xFE00 + (160 - dma_len)`.
- Decrementa `dma_len`. Quando arriva a 0, DMA finito (gli ultimi byte non copiati restano).

---

### Flusso per frame

```
ppu_init()                          ← una volta all'avvio

loop principale:
    cycles = cpu_step()             ← CPU esegue un'istruzione, restituisce cicli
    
    if dma attivo:
        ppu_dma_step(cycles * 4)    ← DMA consuma 4 T-cycles per byte
    else:
        int_flags = ppu_step(cycles) ← PPU avanza
        if int_flags:
            cpu_set_if(int_flags)   ← notifica interrupt alla CPU
```

Dentro `ppu_step`, quando inizia il modo 3 di una scanline visibile, chiami `ppu_draw_scanline()`.

Quando il framebuffer è pronto (dopo LY=143), puoi presentarlo a schermo.

---

### Schema riassuntivo delle funzioni

```
ppu_init()
ppu_step(cycles)           → interrupt flags
ppu_read(addr)             → valore
ppu_write(addr, val)
ppu_dma_step(cycles)

// interne (chiamate da ppu_step):
ppu_draw_scanline()        // disegna la riga ly corrente
  └─ draw_bg_pixels()     // 160 pixel background
  └─ draw_win_pixels()    // sovrascrive dove serve
  └─ draw_sprite_pixels() // mescola sprite

// utility:
read_tile_pixel(tile_addr, row, col)  → indice colore 0–3
apply_palette(color_idx, palette)     → colore finale 0–3
```

---

### Note sul timing reale

La modalità 3 (draw) non ha una durata fissa: varia in base a quanti sprite sono attivi sulla riga. Un'approssimazione comune è **172 clock**, ma il Game Boy reale usa tra 172 e 289. Per il 99% dei giochi (incluso Tetris), 172 clock fissi funzionano.

C'è anche un bug hardware: il **LYC=LY** flag si attiva con 1 T-cycle di ritardo in alcuni casi. Non serve per Tetris.

---

### Cosa rimandare

- **Window**: implementala dopo che il background funziona
- **DMA**: necessario solo quando il gioco muove gli sprite
- **Sprites 8×16**: estensione degli 8×8
- **FIFO dettagliato**: se vuoi precisione hardware, ma per Tetris non serve

---

Se qualcosa non è chiaro, chiedi e aggiorno il documento.
