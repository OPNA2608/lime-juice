# MES Binary Format Specification

The MES format is a binary instruction encoding used by visual novel engines from Elf, Alice Soft, Silky's, Fairytale, and others. It stores game scripts as compact bytecode that controls text display, graphics, sound, branching, and other game logic.

There are three functionally different variants of the MES format, each with a different bytecode layout:

- **AI5** (also AI4): Dictionary-compressed bytecode with a header
- **AI1** (also AI2): Raw bytecode with no header or dictionary
- **ADV** (also AI3): Segment-based bytecode with bitfield-encoded variables

This document describes the binary structure of each variant at the byte level.

## Engine Detection

When the engine type is unknown, it can be determined heuristically from the file contents.

**ADV:** Check the last two non-null bytes of the file. ADV files end with either `FF FF` (end-of-segment) or `FF FE` (end-of-message). If either marker is found, the file is ADV.

**AI5 vs AI1:** Read the first two bytes as a little-endian uint16. This value is the AI5 dictionary offset. Validate it: it must be >= 2, must be <= file size, and (offset - 2) must be even (dictionary entries are 2-byte pairs). If the offset looks valid, scan the file for byte patterns characteristic of each engine (AI5 uses tokens like `0x04`, `0x06`, `0x0F`; AI1 uses `0x7B`, `0x7D`, `0x9D`, `0x22`). Whichever set appears more often indicates the engine type. The exact heuristic I use is in `src/auto_detect.cpp`.

---

## AI5 Format

### File Layout

```
Offset 0x0000: [uint16 LE] dictionary_end_offset
Offset 0x0002: [dictionary entries ... ]
Offset dictionary_end_offset: [bytecode stream ... ]
```

The first two bytes are a little-endian 16-bit integer pointing to where the bytecode begins. Everything between offset 2 and this value is the dictionary.

### Dictionary

The dictionary is a sequence of 2-byte SJIS character pairs. Each pair is one entry:

```
Entry 0: bytes[2], bytes[3]
Entry 1: bytes[4], bytes[5]
...
Entry N: bytes[2 + N*2], bytes[2 + N*2 + 1]
```

Dictionary references in the bytecode use byte values at or above the **dictionary base** (default `0x80`, some games use `0xD0`). A byte `B >= dict_base` references entry `B - dict_base`.

### Token Types

| Token | Byte(s) | Description |
|-------|---------|-------------|
| END | `0x00` | End of block |
| BEG | `0x01` | Begin block |
| CNT | `0x02` | Separator / continue |
| VAL | `0x03` | Expression terminator |
| SYS | `0x04` + opcode | System function call |
| STR | `0x06` ... `0x06` | String literal (delimited) |
| NUM1 | `0x07` + 1 byte | 8-bit number |
| NUM2 | `0x08` + 2 bytes | 16-bit number |
| NUM3 | `0x09` + 3 bytes | 24-bit number |
| SETRC | `0x0A` | Set register (indexed) |
| SETRE | `0x0B` | Set register (expression-indexed) |
| SETV | `0x0C` | Set variable |
| SETAW | `0x0D` | Set array element (word) |
| SETAB | `0x0E` | Set array element (byte) |
| CND | `0x0F` | Conditional / if |
| CMD | `0x10`-`0x1F` | Command operation |
| TERM2 | `0x20`-`0x2C` | Binary operator |
| TERM0 | `0x2D`, `0x2F` | Unary operator / field read |
| TERM1 | `0x2E` | Unary operator |
| NUM0 | `0x30`-`0x3F` | Inline number (value = byte - 0x30) |
| VAR | `0x40`-`0x5A` | Variable reference (A=0x41 through Z=0x5A, @=0x40) |
| CHR | `0x60`+ | SJIS character (2 bytes) or dictionary reference |

### Number Encoding

Numbers use a variable-length base-64 scheme:

**NUM0** (`0x30`-`0x3F`): Single byte. Value = byte - `0x30`. Range: 0-15.

**NUM1** (`0x07` + 1 byte): The payload byte is encoded as `(value << 2) | 0x03`. To decode: `payload >> 2`. Range: 0-63.

**NUM2** (`0x08` + 2 bytes): Each payload byte encodes 6 bits. Decode recursively: `((first_byte >> 2) << 8 | second_byte) >> 2`. Range: 0-4,095.

**NUM3** (`0x09` + 3 bytes): Same encoding, one more byte. Range: 0-262,143.

To encode a value `N`:
```
if N < 16:     emit 0x30 + N
if N < 64:     emit 0x07, (N << 2) | 3
if N < 4096:   emit 0x08, ((N >> 6) << 2) | 3, ((N & 63) << 2) | 3
if N < 262144: emit 0x09, ((N >> 12) << 2) | 3, (((N >> 6) & 63) << 2) | 3, ((N & 63) << 2) | 3
```

### Character Encoding

With dictionary enabled (default), byte values are split into three ranges:

- `0x60` to `dict_base - 1`: SJIS character pair. Read the next byte as well. The first byte has an offset of `+0x20` applied: `j1 = byte + 0x20`, `j2 = next_byte`. The resulting (j1, j2) pair is a standard Shift-JIS code point.
- `dict_base` to `0xFF`: Dictionary reference. Index = `byte - dict_base`. Look up the SJIS pair from the dictionary.

Without dictionary (`use_dict = false`), bytes `0x80`-`0xFF` begin a 2-byte SJIS pair with no offset applied.

### System Calls

Format: `0x04` followed by an opcode byte, then parameters.

With the `extraop` flag enabled, opcodes `0x29` & `0x2A` are followed by 3 extra payload bytes (decoded as a number using the same base-64 scheme as NUM3).

### String Literals

Format: `0x06` [contents] `0x06`

Valid content bytes: `0x09` (tab), `0x20`-`0x7E` (printable ASCII), `0xA1`-`0xDF` (half-width katakana).

### Block Structure

Blocks are delimited by BEG (`0x01`) and END (`0x00`):

```
0x01 [statements...] 0x00
```

Blocks can be nested. Statements within a block are parsed sequentially.

### Expressions

An expression is a sequence of terms terminated by VAL (`0x03`). Terms can be numbers, variables, or operators. Multiple expressions separated by CNT (`0x02`) form an expression list.

### Command and System Call Names

When the decompiler resolves names, these opcode values map to symbolic names:

**Commands (0x10-0x1F):**

| Opcode | Name |
|--------|------|
| `0x10` | text-color |
| `0x11` | wait |
| `0x12` | define-proc |
| `0x13` | proc |
| `0x14` | call |
| `0x15` | number |
| `0x16` | delay |
| `0x17` | clear |
| `0x18` | color |
| `0x19` | util |
| `0x1A` | animate |

**System calls (0x04 prefix):**

| Opcode | Name |
|--------|------|
| `0x10` | while |
| `0x11` | continue |
| `0x12` | break |
| `0x13` | menu-show |
| `0x14` | menu-init |
| `0x15` | mouse |
| `0x16` | palette |
| `0x17` | box |
| `0x18` | box-inv |
| `0x19` | blit |
| `0x1A` | blit-swap |
| `0x1B` | blit-mask |
| `0x1C` | load |
| `0x1D` | image |
| `0x1E` | mes-jump |
| `0x1F` | mes-call |
| `0x21` | flag |
| `0x22` | slot |
| `0x23` | click |
| `0x24` | sound |
| `0x26` | field |

**Operators (expression terms):**

| Opcode | Name | Description |
|--------|------|-------------|
| `0x20` | `+` | Addition |
| `0x21` | `-` | Subtraction |
| `0x22` | `*` | Multiplication |
| `0x23` | `/` | Division |
| `0x24` | `%` | Modulo |
| `0x25` | `//` | Logical OR |
| `0x26` | `&&` | Logical AND |
| `0x27` | `==` | Equal |
| `0x28` | `!=` | Not equal |
| `0x29` | `>` | Greater than |
| `0x2A` | `<` | Less than |
| `0x2B` | `~` | Array read |
| `0x2C` | `~b` | Array read (byte) |
| `0x2D` | `:` | Field/register read |
| `0x2E` | `::` | Field read (extended) |
| `0x2F` | `?` | Conditional test |

---

## AI1 Format

### File Layout

AI1 files have no header and no dictionary. The entire file is raw bytecode, parsed from byte 0.

### Token Types

| Token | Byte(s) | Description |
|-------|---------|-------------|
| REG1 | `0x00` + 1 byte | Register (extended index) |
| REG0 | `0x01`-`0x07` | Register (index = byte - 1) |
| REG2 | `0x08` + 2 bytes | Register (16-bit index, big-endian) |
| NUM1 | `0x10` + 1 byte | Number (8-bit value) |
| NUM0 | `0x11`-`0x17` | Inline number (value = byte - 0x11) |
| NUM2 | `0x18` + 2 bytes | Number (16-bit value, big-endian) |
| STR | `0x22` ... `0x22` | String literal (delimited) |
| CNT | `0x2C` | Separator / cut |
| VAR | `0x40`-`0x5A` | Variable reference |
| BEG | `0x7B` | Begin block |
| END | `0x7D` | End block |
| CHR | `0x80`-`0x98` + 1 byte | SJIS character pair |
| CND | `0x9D` | Conditional / if |
| CMD | `0x99`-`0x9C`, `0x9E`-`0xBF` | Command operation |
| PROC | `0xC0`-`0xFF` | Procedure call (proc = byte - 0xC0) |

### Number Encoding

AI1 uses straightforward encoding with no bit-shifting:

**NUM0** (`0x11`-`0x17`): Value = byte - `0x11`. Range: 0-6.

**NUM1** (`0x10` + 1 byte): Value = the byte directly. Range: 0-255.

**NUM2** (`0x18` + 2 bytes): Value = `(byte1 << 8) | byte2` (big-endian). Range: 0-65,535.

### Register Encoding

**REG0** (`0x01`-`0x07`): Index = byte - 1. Range: 0-6.

**REG1** (`0x00` + 1 byte): Index = the byte. Range: 0-255.

**REG2** (`0x08` + 2 bytes): Index = `(byte1 << 8) | byte2` (big-endian). Range: 0-65,535.

### Character Encoding

Bytes `0x80`-`0x98` begin a 2-byte SJIS character pair. The second byte follows immediately. No offset is applied.

### String Literals

Format: `0x22` [contents] `0x22`

Valid content bytes: `0x20`-`0x7E` (printable ASCII, excluding `0x22` which is the delimiter), `0x80`-`0xDF` (high bytes including half-width katakana).

Half-width katakana bytes (`0xA1`-`0xDF`) map to JIS X 0201 characters. Other high bytes (`0x80`-`0xA0`) are treated as Latin-1.

### Block Structure

Blocks use `{` and `}` characters as delimiters (byte values `0x7B` and `0x7D`):

```
0x7B [statements...] 0x7D
```

### Procedure Calls

Bytes `0xC0`-`0xFF` encode inline procedure calls. The procedure number is `byte - 0xC0`, giving a range of 0-63.

### Expressions

Unlike AI5, AI1 expressions do not have a VAL terminator. They are parsed greedily, consuming terms until a non-term byte is encountered.

### Command Names

| Opcode | Name |
|--------|------|
| `0x99` | set-reg: |
| `0x9A` | set-var |
| `0x9B` | set-arr~ |
| `0x9C` | set-arr~b |
| `0x9E` | while |
| `0x9F` | continue |
| `0xA0` | break |
| `0xA1` | menu |
| `0xA2` | mes-jump |
| `0xA3` | mes-call |
| `0xA4` | define-proc |
| `0xA5` | com |
| `0xA6` | wait |
| `0xA7` | window |
| `0xA8` | text-position |
| `0xA9` | text-color |
| `0xAA` | clear |
| `0xAB` | number |
| `0xAC` | call |
| `0xAD` | image |
| `0xAE` | load |
| `0xAF` | execute |
| `0xB0` | recover |
| `0xB1` | set-mem |
| `0xB2` | screen |
| `0xB3` | mes-skip |
| `0xB4` | flag |
| `0xB6` | sound |
| `0xB7` | animate |
| `0xB8` | slot |
| `0xB9` | set-bg |

**Operators:**

| Opcode | Name |
|--------|------|
| `0x21` | `!=` |
| `0x23` | `~b` |
| `0x25` | `%` |
| `0x26` | `&&` |
| `0x2A` | `*` |
| `0x2B` | `+` |
| `0x2D` | `-` |
| `0x2F` | `/` |
| `0x3C` | `<` |
| `0x3D` | `==` |
| `0x3E` | `>` |
| `0x3F` | `?` |
| `0x5C` | `~` |
| `0x5E` | `^` |
| `0x7C` | `//` |

---

## ADV Format

### File Layout

ADV files are a raw bytecode stream with no header. The stream ends with an end-of-segment (`0xFF 0xFF`) or end-of-message (`0xFF 0xFE`) marker, possibly followed by null padding.

### Overview

ADV bytecode uses a more complex encoding than AI5 or AI1. Variables and registers are encoded as multi-byte bitfield structures rather than simple byte tokens. This section provides a practical overview of the token structure; see the source code (`src/engine/adv/parser.cpp`) for exact bit-level layouts.

### Token Ranges

| Token | Byte Range | Description |
|-------|-----------|-------------|
| REG | `0x00`-`0x0F` + 1 byte | Register operation (2-byte bitfield) |
| VAR | `0x10`-`0x1F` + 1-2 bytes | Variable operation (2 or 3 byte bitfield) |
| CHR$ | `0x81` + special byte | Special sequence (LBEG, LEND, WAIT, NOP) |
| STR | `0x22` ... `0x22` | String literal |
| NUM0 | `0x23`-`0x27` | Inline number (value = byte - 0x23, range 0-4) |
| NUM1 | `0x28` + 1 byte | 8-bit number |
| NUM2 | `0x29`-`0x2C` + 2 bytes | Multi-byte number |
| CHR1 | `0x2D`-`0x7F` | Single-byte SJIS mapping |
| CHR2 | `0x80`-`0x9F`, `0xE0`-`0xEA` + 1 byte | Standard 2-byte SJIS pair |
| CHR2+ | `0xEB`-`0xEF` + 1 byte | Extended 2-byte SJIS pair |
| BEG | `0xA2` | Begin block |
| END | `0xA3` | End block |
| CMD | `0xA5`-`0xDF` | Command operation |
| EOS | `0xFF 0xFF` | End of segment |
| EOM | `0xFF 0xFE` | End of message (file) |

### Variable Encoding

Variables use a 2-byte (standard) or 3-byte (extraop) bitfield format starting with a byte in the `0x10`-`0x1F` range.

**Standard (non-extraop), 2 bytes:**

Bitfield layout: `[0001][f:2][i:6][v:4]`

- `f` (2 bits): operation type (0 = set, 1 = subtract, 3 = add)
- `i` (6 bits): variable index, range 0-63. Indices 0-25 map to A-Z; 26+ map to AA, AB, etc. (max index 63 = BL)
- `v` (4 bits): literal value, range 0-15

**Extraop, 3 bytes:**

Bitfield layout: `[0001:4][f:3][m:1] [pad:2][i:5][j1:1] [pad:1][j2:7]`

- `f` (3 bits): operation type (0 = set, 1 = add, 2 = subtract, 3 = not-equal test)
- `m` (1 bit): 0 = value `j` is a literal, 1 = value `j` is a variable index
- `i` (5 bits): variable index, range 0-31. Indices 0-25 map to A-Z; 26+ map to AA, AB, etc. (max index 31 = AF)
- `j` (8 bits, split across j1 and j2): value or source variable index, range 0-255

Extraop has fewer addressable variables (32 vs 64) but a much wider value range (0-255 vs 0-15) and the ability to use another variable as the source operand.

### Register Encoding

Registers use a 2-byte bitfield starting with a byte in the `0x00`-`0x0F` range.

Bitfield layout: `[0000][_:1][f:1][i:7][o:3]`

- `f` (1 bit): flag (typically #t or #f)
- `i` (7 bits): register group index
- `o` (3 bits): sub-index within the group
- The actual register number is `8 * i + o - 1`

### Number Encoding

**NUM0** (`0x23`-`0x27`): Value = byte - `0x23`. Range: 0-4.

**NUM1** (`0x28` + 1 byte): Value = the second byte directly. Range: 0-127.

**NUM2** (`0x29`-`0x2C` + 2 bytes): Three bytes total (the leading byte plus two more). Value = `(leading_byte - 0x29) * 16384 + second_byte * 128 + (third_byte & 0x7F)`. The leading byte determines which 16384-value "page" the number falls in (up to 4 pages since `0x29`-`0x2C` spans 4 values). Range: 0-65,535.

### Character Encoding

ADV supports several character encoding subtypes:

- **CHR1** (`0x2D`-`0x7F`): Maps to SJIS pair `(0x82, 0x72 + byte)`.
- **CHR2** (`0x80`-`0x9F`, `0xE0`-`0xEA` + 1 byte): Standard 2-byte SJIS pair.
- **CHR2+** (`0xEB`-`0xEF` + 1 byte): Extended 2-byte SJIS pair.
- **CHR$** (`0x81` + special byte): Special control sequences:
  - `0x81 0x6F`: Loop begin (LBEG)
  - `0x81 0x70`: Loop end (LEND)
  - `0x81 0x90`: Wait (WAIT)
  - `0x81 0x97`: No operation (NOP)

### Segment Structure

ADV scripts are organized into segments. Each segment can have an optional condition (using register or variable tests) followed by a body of statements. Segments are separated by EOS markers (`0xFF 0xFF`). The file ends with an EOM marker (`0xFF 0xFE`).

### Decrypt Opcode

Some ADV games use a decrypt opcode (`0xD5`) that processes subsequent data. When the `extraop` flag is set, the parser enables this opcode by default. However, some games use the 3-byte variable encoding without the decrypt opcode; juice handles this by automatically retrying with decrypt disabled if the first parse attempt produces empty or suspicious results.

### Command Names

| Opcode | Name |
|--------|------|
| `0xA5` | text-break |
| `0xA6` | text-frame |
| `0xA7` | text-pos |
| `0xA8` | text-color |
| `0xA9` | text-delay |
| `0xAA` | text-reset |
| `0xAB` | wait |
| `0xAC` | delay |
| `0xAD` | menu1 |
| `0xAE` | menu2 |
| `0xAF` | seg-call |
| `0xB0` | exec-file |
| `0xB1` | mes-jump |
| `0xB2` | branch-random |
| `0xB3` | branch-index |
| `0xB4` | branch-var |
| `0xB5` | branch-reg |
| `0xB6` | execute-var |
| `0xB7` | mouse |
| `0xB9` | define-proc |
| `0xBA` | proc |
| `0xBB` | repeat |
| `0xBC` | if |
| `0xBD` | when |
| `0xBE` | flag-save |
| `0xBF` | flag-load |
| `0xC0` | mes-load? |
| `0xC8` | load-mem |
| `0xC9` | image-file |
| `0xCA` | print-var |
| `0xCD` | exec-mem |
| `0xCF` | image-mem |
| `0xD0` | sound |
| `0xD5` | decrypt |

---

## Comparison Table

| Aspect | AI5 | AI1 | ADV |
|--------|-----|-----|-----|
| Header | 2-byte dict offset | None | None |
| Dictionary | Yes (SJIS pairs) | No | No |
| Dict base | `0x80` or `0xD0` | N/A | N/A |
| String delimiter | `0x06` | `0x22` | `0x22` |
| Block begin/end | `0x01` / `0x00` | `0x7B` / `0x7D` | `0xA2` / `0xA3` |
| Expression terminator | `0x03` (VAL) | Greedy (no terminator) | Depends on context |
| Number range | 0-262,143 | 0-65,535 | 0-65,535 |
| Variable encoding | Single byte (`0x40`-`0x5A`) | Single byte (`0x40`-`0x5A`) | Multi-byte bitfield |
| End-of-file marker | Optional trailing `0x00` | Optional trailing `0x7D` | `0xFF 0xFE` |
| Character encoding | SJIS + dict offset | Direct SJIS | SJIS with multiple subtypes |

---

## Character Encoding (SJIS)

All three engines store Japanese text as Shift-JIS byte pairs. The standard PC-98 character set uses JIS X 0208 with some platform-specific extensions in rows 9-15.

A Shift-JIS byte pair (j1, j2) can be converted to a JIS row/column (kuten) position, which maps to a specific character in the font. The juice tool handles this conversion internally and supports several charset mappings for different platforms and translation targets.

For details on the available charsets and their mappings, see the [README](../README.md#character-encoding).
