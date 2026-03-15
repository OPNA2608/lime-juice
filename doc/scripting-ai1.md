# AI1 Engine Scripting Reference

This is a reference for the RKT script format used by AI1 engine games (also AI2, which is functionally identical). AI1 is the older, simpler engine used by earlier titles like Dragon Knight 1/2, Dragoon Armor, Pinky Ponky, Angel Hearts, and RAY-GUN.

## File Structure

AI1 scripts begin with a `(mes ...)` form containing metadata and the script body. There is no dictionary in AI1.

```racket
(mes
 (meta (engine 'AI1) (charset "english"))
 (define-proc 6 (<> ...))
 (define-proc 7 (<> ...))
 ;; script body follows
 (com 7)
 (slot 0)
 ...)
```

## Syntax Basics

Same as AI5. See the [AI5 Scripting Reference](scripting-ai5.md#syntax-basics) for the full primer on S-expression syntax.

## Variables

AI1 uses the same variable set as AI5: `@` (system array) and `A` through `Z` (max variable index: 25).

```racket
(set-var S 0)          ; S = 0
(set-var I (+ I 1))    ; I = I + 1
```

## Arrays and Registers

### Writing

```racket
(set-arr~ A 0 (- (~ A 0) P))    ; A[0] = A[0] - P
(set-arr~ A 8 I)                 ; A[8] = I
(set-arr~b A 10 (+ (? 15) 10))  ; A[10] = random(15) + 10  (byte width)
(set-arr~ @ 7 19)                ; @[7] = 19
```

### Reading

```racket
(~ A 3)       ; read A[3]
(~ A 0)       ; read A[0]
(~b A 10)     ; read A[10] as byte
```

### System Array (`@`)

AI1 uses different system array indices from AI5:

| Index | Purpose |
|-------|---------|
| `@ 5` | Display height |
| `@ 6` | Display layer |
| `@ 7` | Font/display width setting |
| `@ 9` | Input configuration |
| `@ 10`-`@ 13` | Menu layout parameters (multiple pairs for position/size) |

Note: AI1 uses `@ 12` for font width control, not `@ 21` like AI5.

## Expressions and Operators

AI1 has the same operator set as AI5 with two additions:

| Operator | Meaning |
|----------|---------|
| `^` | Bitwise XOR |
| `?` | Random number: `(? N)` returns a random value from 0 to N-1 |

All other operators (`+`, `-`, `*`, `/`, `%`, `==`, `!=`, `>`, `<`, `&&`, `//`, `~`, `~b`, `:`) work the same as AI5.

### Random Numbers

```racket
(set-arr~ A 3 (+ (? 15) 7))      ; A[3] = random(0..14) + 7
(set-arr~b A 10 (+ (? 15) 10))   ; A[10] = random(0..14) + 10
```

## Control Flow

### if / if-else

```racket
(if (== S 2) (<> (cmd:181 0)))
```

```racket
(if-else (< (~ A 0) P)
         (<> (text "Too expensive!"))
         (<> (text "Buy it?")))
```

### cond (multi-branch)

```racket
(cond ((== (~ A 6) 2) (<> (recover) (cmd:187 4 "\\dungeon\\pic\\fc-k.ada")))
      ((== (~ A 6) 3) (<> (recover) (cmd:187 4 "\\dungeon\\pic\\fc-n.ada")))
      ((== (~ A 6) 4) (<> (recover) (cmd:187 4 "\\dungeon\\pic\\fc-f.ada"))))
```

### while / break / continue

```racket
(while (== 1 1)
       (<>
        ;; generate random stats
        (set-arr~ A 3 (+ (? 15) 7))
        (set-var S 0)
        (while (== S 0)
               (<> (menu (<.> (text "Accept") (text "Reroll")))))
        (if (== S 1) (<> (break)))))
```

### slot

In AI1, `slot` has different forms:

```racket
(slot 0)                              ; execute/update current slot
(slot 1 "\\dungeon\\music\\peace.m")  ; load a resource by slot and path
```

## Text and Display

### Displaying Text

```racket
(text "O brave warrior! Strength be upon thee.")
```

### Text with Inline Numbers

```racket
(text "LEVEL:" (number (~ A 1)) "  HP:" (number (~ A 3)) "/" (number (~ A 2)) "\n")
(text "STR  :" (number (~b A 10)) "  DEX:" (number (~b A 11)) "\n")
```

The `number` command inserts the numeric value of an expression into the text output. `\n` produces a newline.

### Text Color

```racket
(text-color 7)     ; set text color to 7 (persists until changed)
(text-color 112)   ; set text color to 112 (used for menus in some games)
```

The `#:color` keyword shorthand also works:

```racket
(text #:color 2 "Take good care of it, lad.")
```

### Text Position

AI1 has explicit text positioning:

```racket
(text-position x y)
```

```racket
(text-position 63 191)    ; move cursor to (63, 191)
(text (number (~ A 3)))   ; display value at that position
(text-position 71 213)    ; move to next position
(text (number (~b A 10)))
```

### Window

`window` sets the text display area:

```racket
(window x1 y1 x2 y2)
```

```racket
(window 1 330 78 391)    ; text window at (1, 330) to (78, 391)
```

### Clear

```racket
(clear)    ; clear the text display area
(wait)     ; wait for player input
```

## Menus

AI1 uses `menu` instead of `menu-show`:

```racket
(set-var S 0)
(while (== S 0)
       (<>
        (text-color 112)
        (menu (<.> (text "Buy") (text "Don't buy")))))
```

The player's selection is stored in `S` (1 for first option, 2 for second, etc.). Wrapping the menu in a `while (== S 0)` loop ensures the player must make a selection.

## Procedures

### Defining and Calling

```racket
(define-proc 6
 (<>
  (text-color 7)
  (set-arr~ @ 7 19)
  (text-position 63 191)
  (text (number (~ A 3)))))

(com 7)     ; call procedure 7 (AI1 uses "com" instead of "proc")
```

AI1 also supports inline procedure calls using `(proc N)` notation in the bytecode (encoded as bytes `0xC0`-`0xFF`).

### call

```racket
(call Z)    ; call the procedure whose number is stored in variable Z
```

## Graphics

### Images

```racket
(image "b:\\f1.ada")                    ; load and display an image
(image "\\dungeon\\pic\\moto.ada")     ; paths use backslash
```

### Screen Control

```racket
(screen ...)    ; screen display mode control
```

### Background

`set-bg` loads a background image or animation with layer and mode parameters:

```racket
(set-bg layer mode path-or-data)
```

```racket
(set-bg 2 1 "b:\\f1_a.mda")           ; load background animation on layer 2
(set-bg 3 1 "ｱ.ｲ.ｳ.ｴ.")               ; set animation sequence on layer 3
(set-bg 4 1 7)                         ; set background parameter on layer 4
```

### Recover

```racket
(recover)    ; restore the display to its previous state (undo overlays)
```

## Sound

```racket
(sound track)
(sound track channel)
```

Works the same as AI5.

## File Operations

### Jumping

```racket
(mes-jump "\\dungeon\\mes\\shop.mes")    ; jump to another script
(mes-call "SUB.MES")                     ; call another script (returns)
(mes-skip offset length)                 ; skip ahead in script data
```

### Loading

```racket
(load filename address)
```

```racket
(load "mouse.dat" 49152)
```

### Execute

```racket
(execute filename)    ; execute an external program/script
```

## System Functions

### Flag

```racket
(flag operation value)
```

```racket
(flag 1 0)
(flag 2 (- N 1))
```

### Slot

```racket
(slot 0)                                 ; update/refresh current state
(slot 1 "\\dungeon\\music\\peace.m")    ; load music/resource
```

### Animate

```racket
(animate layer frame flags)
```

Works the same as AI5.

### Set Memory

```racket
(set-mem ...)    ; write directly to engine memory
```

### Wait and Delay

```racket
(wait)        ; wait for player input
(delay N)     ; pause for N milliseconds
```

## Unresolved Commands

Commands that don't have known names appear as `cmd:NNN`:

```racket
(cmd:181 0)                              ; unresolved command 181
(cmd:187 4 "\\dungeon\\pic\\fc-k.ada")  ; unresolved command 187 with arguments
```

These are engine-specific operations whose exact purpose may vary by game.
If you figure out what an unresolved command does, please let me know. I'd be glad to make the decompiler more complete.

## Tips for Editing AI1 Scripts

- AI1 is simpler than AI5. Most scripts are straightforward sequences of text, menus, and conditionals.
- AI1 does **not** support font width adjustment via the system array (unlike AI5). Text is always double-width.
- Paths in AI1 scripts use backslashes (`\\`).
- Use `(com N)` to call procedures (AI1's equivalent of AI5's `(proc N)`).
