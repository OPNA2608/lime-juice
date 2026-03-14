Format documentation copied from [supersakura](https://gitlab.com/bunnylin/supersakura/-/blob/dev/doc/gfx/gpc-gpa.md).

GPC/GPA image format
--------------------

Used in most PC98 games by Fairytale. GPC is typically a single image, GPA is
a multi-frame animation. GPC can also have more than one frame, but in that
case it's still encoded as a single image.

- One or more 4bpp bitmaps in each file, each frame can have a unique size and offset
- Transparency is supported, usually color 0
- Pixel data is in scanline order, but may be vertically and horizontally interlaced
- Pixel data is split by bitplane within each row
- No RLE or LZ copying
- Rows are XOR'ed to achieve as many 00 bytes as possible
- 00 bytes are encoded as single flag bits, all other bytes as full literals

Source code for reading these is in [/inc/gfx/gpc.pas](../../inc/gfx/gpc.pas).

MLD can view these, and there's an existing converter by Gufino2:
https://github.com/Gufino2/gpc2bmp

There's also an encoder: [https://mooncore.eu/filus/makegpc.py]


##### Header

Words are little-endian, native x86 order.

GPC header:
```
0x00: char[16] - sig "PC98)GPCFILE   " 00
0x10: dword - interlacing; after each row, move ahead this many rows
0x14: dword - points to start of palette info
0x18: dword - points to start of image info
```

GPA header:
```
0x00: char[16] - sig "PC98)GPAFILE   " 00
0x10: dword - frame count
0x14: dword - points to start of palette info, if present
0x18: dword - points to start of image info
... more dwords pointing to further image infos in this file, until an invalid
    pointer or the palette info or first image info block is encountered
```

Palette info:
```
uint16 - number of palette colors = 0x0010
uint16 - bytes per color? = 0x0002
16 x uint16 - 32-byte palette, color order RB 0G
```

Palette info is optional. If an image doesn't have a palette, the pointer to it is 0.
This is most common with sprite animations, which share a palette with their parent sprite.
There doesn't seem to be anything explicitly specifying transparency, so it's likely
hardcoded to always be color 0, or it's one of the unknown 0 header values.

Palette info is often followed by "n*bys" 00, mysterious.

GPC image info:
```
uint16 - image pixel width
uint16 - image pixel height
uint16 - compressed bytestream size (sometimes including image info block size)
uint16 - unknown, always 0x0000?
uint16 - bits per pixel? 0x0004
uint16 - image X location
uint16 - image Y location
uint16 - unknown, always 0x0000?
```

GPA image info blocks are simpler, and there's one at the start of every frame's data:
```
uint16 - vertical interlacing
uint16 - image X location
uint16 - image Y location
uint16 - image pixel width
uint16 - image pixel height
```

The frames can have different sizes, and different offsets, and different interlacing!

The compressed bytestream directly follows each image info section.


##### Decompressing

Although the image can declare any pixel width in its info section, the compressed buffer
is always padded to a multiple of 8 pixels (4 bytes). The padding is on the right edge,
and normally is all color 0, which is usually transparent anyway.

So when decompressing, expect a padded byte width of `((pixel width + 7) & 0xFF8) >> 1`.
The padding columns can be removed at the end of post-processing to make the image match
the specified pixel size exactly.

Additionally, much like PNG, every row will start with a horizontal interlacing byte.
Therefore, you need a working buffer of `(byte width + 1) * pixel height`.

Don't worry about the interlacing yet, decompress the whole buffer first.

```
While buffer not full:
	flagA := next input byte
	For each bit in flagA, from 0x80 to 0x01:
		If bit not set: output 8 bytes of 00
		Else:
			flagB := next input byte
			For each bit in flagB, from 0x80 to 0x01:
				If bit not set: output 00 byte
				Else: output next input byte
```

If you ran out of input bytes when trying to read the next input byte, stop.
If there's still space in the output buffer, fill the rest with zeroes.

Note that, just as with PNG's filter bytes, the horizontal interlacing byte gets compressed
with the image data; since those bytes are a different type of data, it interrupts the
flow of pixel data, weakening compression slightly. With GPC, this is even worse, since the
image is padded to a multiple of 4 bytes, and compressed in 8-byte sections, but then the
interlacing causes rows to be a multiple of 4, plus 1 byte, misaligned. This reduces
opportunities for 8-byte 00 runs, weakening compression further.


##### Post-processing

The remaining steps are:
- Process horizontal interlaced XOR
- Process vertical XOR
- Undo vertical interlacing
- Merge the bitplanes

Part of the compression scheme is XOR'ing bytes with earlier bytes on the same row
and/or some number of rows directly above. This is quite good at optimising out
repetitive dither patterns, if the rows and pixels are appropriately interlaced.

Vertical XOR is always applied, but vertical interlacing may be skipped in some images,
Horizontal XOR is applied in an interlaced order in a single step, which may be skipped.

The vertical interlacing value is in the image's header or start of each image info block.
If the value is 0 or 1, there is no interlacing, but vertical XOR is still applied.
Vertical interlacing is typically in the 1..4 range, but can go at least as high as 128,
in which case it doesn't optimise out any dithering but rather some other horizontal
feature that repeats after 128 pixels, like a popup graphic's surrounding frame.

The horizontal interlacing value appears as the first byte of every row. If the value is 0,
there is no horizontal interlacing and XOR on this row.

Horizontal interlacing and XOR is applied first to every image row. This table shows
what an example result would be at different interlacing values:

|   Row of pixels   | Interlacing |       Order       |     After XOR     |
|:-----------------:|:-----------:|:-----------------:|:-----------------:|
|  3 6 9 B B 3 6 0  |      0      |         -         |     unchanged     |
|  3 6 9 B B 3 6 0  |      1      |  1 2 3 4 5 6 7 8  |  3 5 C 7 C 4 2 2  |
|  3 6 9 B B 3 6 0  |      2      |  1 5 2 6 3 7 4 8  |  3 1 A A 1 9 7 9  |
|  3 6 9 B B 3 6 0  |      3      |  1 4 7 2 5 8 3 6  |  3 8 A 8 3 9 E 3  |

After horizontal XOR, vertical XOR is applied. Every row after the first is XOR'ed by the
row directly above it. Vertical interlacing will be processed separately after this.

To apply XOR for both horizontal and vertical in one pass through the buffer:

```
For each row in image from the top:
	horizontal interlacing i := first byte in row
	(think of that as row_byte[-1], the row's pixel data begins at the next byte)
	If i != 0:
		x := 0
		last_byte := 0
		start_ofs := 0
		Repeat:
			last_byte := row_byte[x] XOR last_byte
			row_byte[x] := last_byte
			x := x + i
			If x >= end of row:
				start_ofs := start_ofs + 1
				If start_ofs == i: work complete, break out of repeat
				x := start_ofs

	If not on first row of image:
		For each byte on row:
			row_byte := row_byte XOR (above byte on previous row)
```

At this point, the rows are still vertically interlaced. To undo this interlacing,
rows need to be shuffled into the correct order, like this:

```
start_row := 0
target_row := 0
For source_row := 0 to image_height - 1:
	target_image[target_row] := source_image[source_row]
	target_row := target_row + vertical_interlacing
	If target_row > last row:
		start_row := start_row + 1
		If start_row == vertical_interlacing: work complete, stop
		target_row := start_row
```

Finally, the image data remains vertically split into four side-by-side bitplanes.
So the first quarter of each row has the bits for plane 0, the second quarter for plane 1,
and so on. Each byte is 8 1bpp pixels. Within each byte, bits are read from 0x80 to 0x01.

Read one bit from each plane, and you have a 4bpp pixel.

```
+--------+--------+--------+--------+
|        |        |        |        |
|        |        |        |        |
| plane0 | plane1 | plane2 | plane3 |
|        |        |        |        |
|        |        |        |        |
+--------+--------+--------+--------+

pixel[0] :=
	((plane0[0] & 0x80) >> 7) +
	((plane1[0] & 0x80) >> 6) +
	((plane2[0] & 0x80) >> 5) +
	((plane3[0] & 0x80) >> 4)
pixel[1] :=
	((plane0[0] & 0x40) >> 6) +
	((plane1[0] & 0x40) >> 5) +
	((plane2[0] & 0x40) >> 4) +
	((plane3[0] & 0x40) >> 3)
pixel[2] :=
	((plane0[0] & 0x20) >> 5) +
	((plane1[0] & 0x20) >> 4) +
	((plane2[0] & 0x20) >> 3) +
	((plane3[0] & 0x20) >> 2)
... and so on
```

##### Problems

- Kounai Shasei 2: there are 10 GPA images, various blinking animations, which don't
  have a base sprite in this game, but have them in vol. 3.
- Kurutta Kajitsu: the animations ED01x have huge image offsets
- Lipstick Adv 3: the image LP012 appears to be corrupted.
- Mai: tlogo must not have its width padded to a multiple of 8.
- Saori: the image S11 appears to be corrupted even in the original engine.
