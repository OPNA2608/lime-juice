Format documentation copied from [supersakura](https://gitlab.com/bunnylin/supersakura/-/blob/dev/doc/gfx/gp4-ada.md).

GP4 images
----------

Elf's later graphic format, used in Words Worth, Metal Eye, Dragon Knight 4, etc.
This is similar to the Pi format that was published a few years earlier, but stores
the image vertically and uses a different RLE encoding. GP4 may compress slightly
better than Pi, contending for the best image compressor on the PC98.

These can be viewed with MLD and Grapholic.

Source code for reading these is in [/inc/gfx/gp4.pas](../../inc/gfx/gp4.pas).

- Image saved in 4px columns
- Move-to-front transform with aggressively short bitcodes for most-recently-used colors
- RLE runs can't cross columns
- Bitplanes are not split
- Script-selected palette transparency, usually color 8


##### Header

Words are big-endian.

```
uint16 - X pixel offset
uint16 - Y pixel offset
uint16 - image pixel width, minus 1
uint16 - image pixel height, minus 1
byte[32] - palette
```

Since the image is stored in 4px columns, the image width should always be a multiple
of 4, but there are a few exceptions: Shangrlia 2 has `nb3aan.gp4` and `nb4ban.gp4`
with a width of 201 pixels. The final column is empty, so it seems safe to round the
specified pixel width down to 200 in this case.

The palette is stored bitpacked, able to use 5 bits per channel, but on the PC98 the
fifth bit is ignored. Each color uses 15 bits, and one extra bit is discarded so each
color uses 2 bytes exactly. The binary color layout is `gggg0rrr r0bbbb0x`.

For example:
- Input bytes `A3 1D`
- Input binary `10100 011-00 01110 1`
- Green `1010-0` is `A`
- Red `0110-0` is `6`
- Blue `0111-0` is `7`
- Palette RGB color is `66AA77`

Color transparency is not specified in any of the discarded bits. The discardable
final bit is nearly always 1, except for like one file in Dragon Knight 4 Special Disk.

The compressed image data follows directly after the palette.


##### Uncompressing

The image data is a bitstream, where bits in each byte are read from 0x80 to 0x01.
The output is handled 4 pixels at a time, top-to-bottom, then left-to-right, creating
4px columns.

Uncompression main loop pseudocode:
```
Initialise delta_table, 17 rows of 16 cells each

While not all columns complete:
	last_color := 16

	While current column not complete:
		If next input bit is 0, then:
			Repeat 4 times:
				x := Read delta code from input
				c := delta_table[row: last_color, cell: x]
				Move cell x to the front of its row
				Output c as the next literal pixel
				last_color := c
		Else:
			copy_dist := Read an offset code from input
			copy_length := Read a length code from input
			Copy copy_length rows from copy_dist rows ago
```

###### Delta codes

The algorithm predicts the next pixel by tracking the most-recently-following colors
from each color. The extra row (17th) is used for the first pixel in each column.

The table is initialised to an ascending series for each color, modulo the number of
colors, thus:

```
delta_table := array of bytes [17 rows, 16 columns]

For y := 0 to 16:
	i := y
	For x := 0 to 15:
		delta_table[y, x] := i & 0xF
		i += 1
```

An example 7x6 table (with only 6 colors) would look like this:

| last_color |
|:----------:|---|---|---|---|---|---|
|     0      | 0 | 1 | 2 | 3 | 4 | 5 |
|------------|---|---|---|---|---|---|
|     1      | 1 | 2 | 3 | 4 | 5 | 0 |
|------------|---|---|---|---|---|---|
|     2      | 2 | 3 | 4 | 5 | 0 | 1 |
|------------|---|---|---|---|---|---|
|     3      | 3 | 4 | 5 | 0 | 1 | 2 |
|------------|---|---|---|---|---|---|
|     4      | 4 | 5 | 0 | 1 | 2 | 3 |
|------------|---|---|---|---|---|---|
|     5      | 5 | 0 | 1 | 2 | 3 | 4 |
|------------|---|---|---|---|---|---|
|   extra    | 0 | 1 | 2 | 3 | 4 | 5 |
|------------|---|---|---|---|---|---|

Delta codes in the input stream are encoded simply: Read 1-bits until a 0-bit is
encountered. The count of 1-bits is the delta code. This strongly prioritises
most-recently-used colors, but produces very long codes for the far end of the
table.

When you have a delta code, use the last output color as the row index, and use
the delta code as the cell index on that row. Output the cell value, and move
the cell to the front of the row.

For example, for the above table, starting with last_color == 2, suppose we read
the input bits `0, 1110, 1110, 0`, which yields delta codes `0, 3, 3, 0`.

- table row 2, cell 0: Output color 2. Cell 0 is already at the front of the row.
- table row 2, cell 3: Output color 5. Move cell 3 to the front of row 2.
- table row 5, cell 3: Output color 2. Move cell 3 to the front of row 5.
- table row 2, cell 0: Output color 5. Cell 0 is already at the front of the row.

After those codes, the table would look like this:

| last_color |
|:----------:|---|---|---|---|---|---|
|     0      | 0 | 1 | 2 | 3 | 4 | 5 |
|------------|---|---|---|---|---|---|
|     1      | 1 | 2 | 3 | 4 | 5 | 0 |
|------------|---|---|---|---|---|---|
|     2      |*5*| 2 | 3 | 4 | 0 | 1 |
|------------|---|---|---|---|---|---|
|     3      | 3 | 4 | 5 | 0 | 1 | 2 |
|------------|---|---|---|---|---|---|
|     4      | 4 | 5 | 0 | 1 | 2 | 3 |
|------------|---|---|---|---|---|---|
|     5      |*2*| 5 | 0 | 1 | 3 | 4 |
|------------|---|---|---|---|---|---|
|   extra    | 0 | 1 | 2 | 3 | 4 | 5 |
|------------|---|---|---|---|---|---|

The extra row is only ever used for the first pixel in each 4px column.

Copied sections don't use this table or change last_color, regardless of what they output.
When next outputting literals, last_color still refers to the last output color literal,
ignoring any copied sections in between.


###### Copy offset codes

When a copy command is encountered, it is followed by an offset code, then a length
code. The offset code is at least 6 bits, but can be more. the first 2 bits indicate
how to understand the rest of the code.

```
0xxxx: Copy from previous column, (8 - xxxx) rows ago

10xxx: where xxx is...
	0: Copy from 16 rows ago
	1: Copy from 8 rows ago
	2..7: Copy from (8 - xxx) rows ago

11:
	Keep reading further bits, and...
	x = Count of 1-bits until 0 encountered
	y = Read the next 4 bits (value 0..15)
	Copy from (2 + x) columns ago, (8 - y) rows ago
```

###### Copy length codes

```
00: Copy 2 rows
01: Copy 3 rows
10xx: Copy 4 + xx rows (values 4..7)
110xxx: Copy 8 + xxx rows (values 8..15)
111xxxxxx:
	If xxxxxx < 63 then
		Copy 16 + xxxxxx rows (values 16..78)
	Else
		x = Read the next 10 bits (value 0..1023)
		Copy 79 + x rows (values 79..1102)
```


### ADA animation data

Words Worth has some ADA files, which are a wrapper around a GP4.

Header: (words are x86-native little-endian, unlike GP4)
```
uint16 - offset of the animation data section
uint16 - file size - 1, points to the file's last byte, always an 0x1A
```

The GP4 image immediately follows the header, then the animation data after the image.

The animation data specifies rectangles in the GP4 bitmap to use as each frame, and
specifies the drawing offsets where they are to appear on screen. The graphic offset at
the top of the embedded GP4 header is included in the frame source offsets, but doesn't
affect destination offsets.

Animation data section header:
```
byte - number of animation sequences defined, typically 0x0A
uint16[] - sequence start offsets relative to section header, one for each defined sequence
```

The section header is directly followed by a series of 33-byte frame descriptors,
up to the where the first sequence begins. Each defines a particular rectangle in the
GP4 bitmap.

```
byte - bitflag
byte - source X left edge, multiply by 8
uint16 - source Y top edge
byte - source X right edge, multiply by 8
uint16 - source Y bottom edge
byte - destination X offset relative to full screen, multiply by 8
uint16 - destination Y offset relative to full screen
byte - unknown
uint16 - unknown
byte[20] - unknown
```

Bitflag values:
- 0x01: unknown, found in 9s14
- 0x02: unknown
- 0x04: unknown, found in 9b05, 8b01
- 0x10: unknown
- 0x40: use color 8 as transparent for this frame
- 0x80: frame data is actually a palette, array of 16 uint16's, color order `RB 0G`

A pair of 0x80 palette entries are found in `9s09.ada`, with a slight difference between
them. Presumably the game flips between the two palettes to cause a light flicker effect.

The frame coordinates require a bit more clarification. The X values all need to be
multiplied by 8 to get the real pixel values. Also, the second coordinate pair does not
directly specify the frame size, but rather its right and bottom edge, so to get the
frame pixel size, subtract the first coordinate pair from the second, and add +1 pixel
to both size dimensions.

For example, for top edge 2 and bottom edge 7, the pixel height would be 7 - 2 + 1 = 6.

Furthermore, the source coordinates include the total image offset from the GP4 header
at the start of the file. (The GP4 image is drawn in a 640x400 off-screen buffer with
normal image position handling, and animation frame source coordinates are relative to
that full buffer, so they must include the normal image position.)

For example, if the GP4 is drawn at Y offset 4, and an animation frame's source top edge
coordinate is 6, that means the real top edge relative to the image bitmap is 6 - 4 = 2.

Finally, the destination coordinates for animation frames are relative to the full 640x400
screen, but most graphics are displayed inside a 352x240 viewframe, whose top left corner
is at (144,8). So, if you want the destination coordinates relative to the parent graphic
inside the viewframe, subtract 0x90 and 0x08 from the destination coordinates.

Full example:
- GP4 image offset 0x50, 0x30
- Frame data `12, 17, 50 00, 1F, 67 00, 22, 50 00 ...`

- The bitflag 0x12 indicates color 8 is not transparent
- Source X offset is 0x17 * 8 - image X offset 0x50 = 0x68
- Source Y offset is 0x0050 - image Y offset 0x30 = 0x20
- Frame width is (0x1F - 0x17 + 1) * 8 = 0x48
- Frame height is 0x0067 - 0x0050 + 1 = 0x18
- Destination X offset is 0x22 * 8 = 0x110 relative to full screen, or 0x80 inside viewframe
- Destination Y offset is 0x0050 relative to full screen, or 0x48 inside viewframe

After the frame descriptors comes the data for animation sequences. It's not clear how to
understand these; the values could be delays between frames. These sequences contain some
large numbers above the framecount at irregular intervals, so they can't just be frame
indexes. But without obvious frame indicators, does that mean all frames are displayed
synchronously?
