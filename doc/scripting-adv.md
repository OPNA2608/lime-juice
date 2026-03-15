# ADV Engine Scripting Reference

This is a reference for the RKT script format used by ADV engine games. ADV is the most advanced of the three engine types, featuring segment-based branching, extended variable ranges, and plenty of commands for manipulating graphics and sound.

## File Structure

Every ADV RKT file has a `(mes ...)` form containing metadata and a segment list:

```racket
(mes
 (meta (engine 'ADV) (charset "pc98") (extraop #t))
 (seg*
  ;; script body: segments, conditions, and statements
  ...))
```

The `(extraop #t)` flag indicates the script uses the extended variable/opcode encoding. Most later ADV games (1994+) require this.

## Syntax Basics

Same as AI5. See the [AI5 Scripting Reference](scripting-ai5.md#syntax-basics) for a primer on S-expression syntax.

One ADV-specific convention: file paths use `¥` (the yen sign) as a path separator, which represents a backslash in the PC-98's character set:

```racket
(image-file "b:¥gpc¥s01.gpc")
(sound ' "a:¥uso¥t_003.uso")
```

## Variables

### Naming

ADV variables use a base-26 naming scheme similar to Excel column names. They start with `A` through `Z`, then continue with two-letter names:

```
A, B, C, ... Z, AA, AB, AC, ... AZ, BA, BB, ... BL
```

The maximum variable index depends on the encoding mode:

| Mode | Max Index | Max Name | Range |
|------|-----------|----------|-------|
| Non-extraop | 63 | BL | 64 variables |
| Extraop | 31 | AF | 32 variables |

Extraop mode has fewer addressable variables but supports a wider value range (0-255 vs 0-15) and variable-to-variable operations.

### Variable Operations

```racket
(set-var M 3)       ; M = 3
(set-var Q 0)       ; Q = 0
(set-var AA 1)      ; AA = 1 (extended variable)
(inc-var A)         ; A = A + 1
(dec-var A)         ; A = A - 1
```

## Registers

Registers are boolean flags indexed by number. They are commonly used for tracking game state (events seen, items collected, etc.).

### Setting Registers

```racket
(set-reg 80 #t)     ; register 80 = true
(set-reg 80 #f)     ; register 80 = false
(set-reg 199 #t)    ; register 199 = true
```

### Testing Registers in Conditions

Register tests appear in `(?)` condition blocks using boolean comparisons:

```racket
(? (= 80 #t))       ; is register 80 true?
(? (= 10 #t))       ; is register 10 true?
(? (= 1 #f))        ; is register 1 false?
```

These look like numeric comparisons syntactically, but the first value is a register index and the second is always `#t` or `#f`. This is how the ADV engine encodes register conditions in its bitfield format.

### Testing Variables in Conditions

Variable tests compare a variable name against a value:

```racket
(? (= M 3))          ; M == 3
(? (= K 0) (= AA 1)) ; K == 0 AND AA == 1
```

Multiple tests in a single `(?)` are ANDed together.

## Segments

ADV scripts are organized into **segments**, which are condition-action pairs. The engine evaluates segments in order and executes the first one whose condition matches.

### seg*

`seg*` is the main segment list. It always executes (no condition):

```racket
(seg*
  (text "This always runs.")
  (wait))
```

### seg

`seg` is a conditional segment. It only executes if its condition is true:

```racket
(seg (? (= K 0) (= AA 1))
  (text "K is 0 and AA is 1.")
  (wait))
```

### seg-call

`seg-call` exits the current segment and scans subsequent segments in the file. It executes the first segment whose condition matches, then returns to where `seg-call` was invoked:

```racket
(seg*
  (text "before seg call") (wait) (text-reset)
  (set-var V 3)
  (seg-call)
  (text "after seg call") (wait) (text-reset))
(seg (? (= V 2))
  (text "V is 2") (wait) (text-reset))
(seg*
  (text "this runs because V is 3, not 2") (wait) (text-reset))
```

Output:
```
before seg call
this runs because V is 3, not 2
after seg call
```

## Conditions and Branching

### Condition Blocks

ADV uses a structured system for conditional logic built from these primitives:

- **`(?)`**: A condition test. Contains one or more comparisons that are ANDed together.
- **`(//)`**: A condition-action pair. The condition `(?)` is followed by statements to execute if true.
- **`(/)`**: An unconditional branch (the "else" case). Just statements, no condition.
- **`(</>)`**: A list of branches. The engine evaluates each `//` in order and executes the first match; a trailing `/` acts as the default.

### if

```racket
(if (</>
     (// (? (= A 1)) (text "A is 1"))
     (// (? (= B 2)) (text "B is 2"))
     (// (nop@) (text "neither matched"))))
```

Equivalent to:
```
if (A == 1) {
    print("A is 1")
} else if (B == 2) {
    print("B is 2")
} else {
    print("neither matched")
}
```

`(nop@)` serves as a "no condition" placeholder, making the branch unconditional (like an else clause).

### when

`when` is like `if`, but every matching branch executes (not just the first):

```racket
(when (</>
    (// (? (= A 1))
        (text "A = 1") (wait) (text-reset))
    (// (? (= A 0))
        (text "A = 0") (wait) (text-reset))
    (// (? (= B 1))
        (text "B = 1") (wait) (text-reset))))
```

Equivalent to:
```
if (A == 1) { print("A = 1") }
if (A == 0) { print("A = 0") }
if (B == 1) { print("B = 1") }
```

### branch-reg

Branch on a register's boolean value:

```racket
(branch-reg 80
    (</>
        (/ (text "reg 80 is false"))
        (/ (text "reg 80 is true"))))
```

Equivalent to:
```
if (reg[80] == false) {
    print("reg 80 is false")
} else {
    print("reg 80 is true")
}
```

### branch-var

Branch on a variable's value (like a switch statement). Each `/` branch corresponds to a consecutive value starting from 0:

```racket
(set-var A 0)
(branch-var A
 (</>
  (/ (text "A is 0"))
  (/ (text "A is 1"))
  (/ (text "A is 2"))
  (/ (text "A is 3"))))
```

Equivalent to:
```
A = 0
switch (A) {
    case 0: print("A is 0"); break
    case 1: print("A is 1"); break
    case 2: print("A is 2"); break
    case 3: print("A is 3"); break
}
```

### branch-random

Branch randomly (selects one of the branches at random):

```racket
(branch-random
 (</>
  (/ (text "outcome 1"))
  (/ (text "outcome 2"))
  (/ (text "outcome 3"))))
```

### branch-index

Branch based on a computed index value. I couldn't find any examples of this command in the ~hundred games I downloaded for testing; this cmmand seems pretty rare. It probably works similarly to `branch-var` but with an index computed from an expression.

## Control Flow

### loop

Infinite loop. ADV loops do NOT have a `break` command; they are typically exited by jumping to another script with `mes-jump`:

```racket
(loop
 (branch-reg 81
  (</>
   (/
    ;; main loop body
    (set-var G 3)
    (text-frame 5 250 24 309)
    (text-reset)
    ...)
   (/ (nop@)))))    ; do nothing when reg 81 is true
```

Exiting the loop:

```racket
(loop
 ...
 (branch-reg 96
  (</>
   (/ (set-var AE 1) (mes-jump "b:¥mes¥kma042bc.mes"))   ; exits loop by jumping
   (/ (set-reg 96 #f) (mes-jump "b:¥mes¥kma041bc.mes")))))
```

### repeat

Fixed-count repetition:

```racket
(repeat count
  body...)
```

### define-proc / proc

Define and call reusable procedures:

```racket
(define-proc 0
 (</>
  (/
   (exec-mem 17904 2)
   (text-frame 6 312 73 387)
   (cmd:203 1 0))))

(proc 0)    ; call procedure 0
```

In ADV, procedures are often used to define different text frame layouts, so calling `(proc N)` sets up a particular text display configuration:

```racket
(define-proc 1 (</> (/ (text-frame 12 332 67 387))))
(define-proc 2 (</> (/ (text-frame 6 312 73 331))))
(define-proc 3 (</> (/ (text-frame 10 332 67 387))))
```

## Text and Display

### Displaying Text

```racket
(text "ここは普通教室である。" 'br)
(text "言わば年頃の少女ばかりを押し込んだセットである。")
```

`'br` at the end of a text command inserts a line break.

### text-raw

Display text using raw Shift-JIS character codes as decimal uint16 values. For example, the characters キヨミ are SJIS `834C 8388 837E`, which in decimal are `33612 33672 33662`:

```racket
(text-raw 33612 33672 33662)    ; displays キヨミ when charset = pc98
```

**Note on Charsets:** When using a custom charset in your MES file (e.g. "english"), the compilation process actually replaces the UTF8 indices with SJIS indices, meaning many of your letters or characters are actually mapping to Japanese characters. I assume this means for games using the chinese, korean, or european charset, that you'll need a custom FONT.BMP in order to properly view games compiled like this.

### str

String data used for labels and display elements:

```racket
(str " ")
(str "A 1")
```

### text-frame

Set the text display area as pixel coordinates:

```racket
(text-frame x1 y1 x2 y2)
```

```racket
(text-frame 6 312 73 387)     ; main text area
(text-frame 12 332 67 387)    ; narrower text area
(text-frame 5 250 24 309)     ; small overlay frame
```

### text-pos

Set the text cursor position:

```racket
(text-pos x y)
```

<!-- TODO: is the cursor origin relative to the whole screen or to the current text-frame? -->

### text-color

Set the text foreground color, or foreground and background:

```racket
(text-color color)
(text-color foreground bg1 bg2)
```

```racket
(text-color 0 11 11)    ; foreground 0, background 11/11
```

Color persists until changed.

### text-reset

Clear the text frame and fill it with a color from the loaded palette:

```racket
(text-reset)        ; reset with default color
(text-reset 0)      ; fill text frame with palette colour 0
(text-reset 15)     ; fill text frame with palette colour 15
```

### text-delay

Control the speed of text display (character-by-character reveal):

```racket
(text-delay speed)
```

### text-break

Force a line break in text output:

```racket
(text-break)
```

### wait

Wait for player input:

```racket
(wait)
```

### delay

Pause for a duration:

```racket
(delay frames)
```

## Graphics

### Loading Images

```racket
(image-file "b:¥gpc¥s01.gpc")    ; load an image file (GPC format)
(image-file "gpc¥frame.gpc")     ; relative path
```

### Displaying Images

```racket
(image-mem layer)
(image-mem layer mode)
```

```racket
(image-mem 0)       ; display loaded image on layer 0
(image-mem 1)       ; display on layer 1
(image-mem 0 3)     ; display on layer 0 with mode 3
```

### exec-mem

`exec-mem` is ADV's most versatile command. It executes operations from memory using either numeric opcodes or string-based commands. The same opcodes are used consistently across multiple games:

**Display setup (opcode 256):**

```racket
(exec-mem 256 1 80 1 80 78 40 1 120 78 8 20)
(exec-mem 256 4 30 22 10 1 0 0)
(exec-mem 256 2 40 1 120 78 80 1 80 78 0 20)
```

**Graphics/layer operations (opcode 9216):**

```racket
(exec-mem 9216 "A 1")                              ; enable display layer A
(exec-mem 9216 "A 0")                              ; disable display layer A
(exec-mem 9216 "G 0 4 240 22 136")                 ; graphics operation
(exec-mem 9216 "P 0 4 240")                        ; palette operation
(exec-mem 9216 "F 0")                              ; flush/finalize
(exec-mem 9216 "F 0,F 1,F 2,F 3,F 6")             ; multiple flush operations
(exec-mem 9216 "C 0 58 272 18 40 1 58 272,A 0")   ; composite command
```

**Expression language (opcode 35072):**

```racket
(exec-mem 35072 "V=1:V'=1:A'=0:B'=0:C'=0:D'=0:E'=0")
(exec-mem 35072 "DISP @0")
(exec-mem 35072 "#190=((J*100+K)>900)")
(exec-mem 35072 "L=O/10:#50=L>=1")
```

**External resource loading (opcode 4096):**

```racket
(exec-mem 4096 "EXIT")                          ; unload resources
(exec-mem 4096 "INIT")                          ; initialize
(exec-mem 4096 "MODE V0E0M64")                  ; set display mode
(exec-mem 4096 "LOAD b:¥gpc¥m08.gpc,KEEP")    ; load and keep in memory
```

**File copy (opcode 2944):**

```racket
(exec-mem 2944 "Copy A:¥RB_PARTS¥...")
```

### print-var

Display a variable's value on screen. Takes the variable name and optional numeric parameters that may control formatting (such as width or padding):

```racket
(print-var variable)
(print-var variable params...)
```

```racket
(print-var H)           ; display the value of H
(print-var B 1 0 2)     ; display B with formatting parameters
(print-var A)            ; display the value of A
```

This command is only available with `extraop` enabled.

## Sound

### Loading and Playing

Sound in ADV is a two-step process: load a file, then play it.

```racket
(sound ' "a:¥uso¥t_003.uso")    ; load sound file
(sound ' 1)                      ; play loaded sound
(sound ' 0)                      ; stop sound
(sound ' 2)                      ; stop with fade / alternate stop
(sound ' "uso¥can5_01.uso")     ; load music
```

The first argument is always `'` (quoted). The second is either a file path (to load) or a number (to control playback: 0 = stop, 1 = play, 2 = stop/fade).

## File Operations

### Script Navigation

```racket
(mes-jump "A:¥RB_MES¥MSG01.MES")    ; jump to another MES file (no return)
(exec-file "filepath")               ; execute another file
```

### Loading and Executing MEC Files

`mes-load?` loads and executes MEC (compiled sub-script) files. It works in pairs: one call to load a file into a memory bank, and another to execute it.

```racket
(mes-load? "b:¥mes¥system.mec" 0)    ; load system.mec into memory bank 0
(mes-load? 0)                         ; execute the script at memory bank 0
```

```racket
(mes-load? "mes¥md_a2pl1.mec" 0)     ; load into bank 0
(mes-load? 0)                         ; execute bank 0
(mes-load? "mes¥icon_clf.mec" 4096)   ; load into bank 4096
(mes-load? 4096)                       ; execute bank 4096
(mes-load? "mes¥md_a2pl2.mec" 0)      ; override bank 0 (no execute yet)
```

If the load call is omitted, the previously loaded script remains in memory until the scene changes or it is overridden. Most games use memory banks `0` and `4096`, though some use other addresses like `2048` or `6144`.

This is how developers of the past broke large scripts into manageable pieces, since MES files exceeding 12-14KB can cause memory errors. A .mec file is literally just a renamed MES file, so you could do the same thing if your compiled script gets too large.

### Memory Loading

```racket
(load-mem "cmd¥quakeh.tcm" 28672)    ; load binary data into memory
```

## Menus

ADV has two menu commands:

### menu1

Simpler menu with a branch list:

```racket
(menu1 x1 y1 x2 y2
 (</>
  (/ (text #:col 4 '34368 "はい") ...)
  (/ (text #:col 4 '34368 "いいえ") ...)))
```

### menu2

More complex menu with conditional items. Menu items use `(*)` blocks, and conditions with `(?)` can hide items based on game state:

```racket
(menu2 2 154 22 154 42 154 62 154 2 174)
(cmd:195 1 255
 (<*>
  (* (text "辺りを見る") (<+> (+)))
  (* (text "レイディと話す") (<+> (+)))
  (* (text "被害を聞く") (<+> (+)))
  (* (? (= 5 #t) (= 6 #t) (= 7 #t)) (text "考える") (<+> (+)))
  ...))
```

The `(?)` condition on the fourth item means "考える" only appears when registers 5, 6, and 7 are all true.

For simpler menu examples, see the [AI5](scripting-ai5.md#menus) and [AI1](scripting-ai1.md#menus) references.

## System Functions

### Mouse

```racket
(mouse mode left top right bottom)
```

### Flag Save/Load

```racket
(flag-save)    ; save game state flags to disk
(flag-load)    ; load game state flags from disk
```

### Decrypt

Some games encrypt portions of their script data. The `decrypt` command decrypts remaining bytes in the segment with a given key, then the decrypted content is parsed as normal script:

```racket
(decrypt key (mes ...))
```

- `key`: an integer encryption key. If 0, no decryption occurs.
- `(mes ...)`: the decrypted content, represented as a nested script block.

The encryption is a simple nibble-swap XOR: each byte's high and low 4 bits are swapped, then XORed with the key.

### nop@

No-operation placeholder. Commonly used in condition blocks as an "always true" marker or to fill empty branches:

```racket
(// (? (= 266 #t)) (nop@) (text "event 266 happened"))
;;                  ^^^^^^ does nothing, execution continues to the text
```

### execute-var

Executes a command dynamically based on a variable's value or a string argument:

```racket
(execute-var "string-argument")      ; execute with string parameter
(execute-var A 5)                    ; execute based on variable A with value 5
```

This takes either a string argument (ARG) or a variable name followed by a number. The exact behavior depends on the game.

## Special Forms

### bytes

Raw byte data, used when the decompiler encounters data it can't represent as a higher-level construct:

```racket
(bytes 235 186 0)
```

### Unresolved Commands

Commands whose purpose is unknown appear with their raw opcode number:

```racket
(cmd:203 1 0)
(cmd:193 0 "gpa¥s01a.gpa")
(cmd:194 0 "ｱ...........ｲ..ｳ..ｲ..ｱ..........")
(cmd:209 0 2)
(cmd:195 0 65535)
(cmd:196 0 0)
```

If you figure out what an unresolved command does, please let me know. I'd be glad to make the decompiler more complete.

## Tips for Editing ADV Scripts

- **The segment structure matters.** Don't rearrange `seg` and `seg*` blocks without understanding the game's flow.
- **`seg-call` scans forward.** It only looks at segments that come after the current one in the file.
- **`(nop@)` is meaningful in conditions.** It's the "always true" / "else" case; don't remove it.
- **Loops exit via `mes-jump`.** There is no `break` command in ADV, loops continue forever until the script jumps elsewhere.
- **Sound loading is two-step.** Load the file first with a path argument, then play it with a numeric argument.
- **Enable syntax highlighting for Racket, Scheme, or Lisp** in your editor for a better experience.