#!/usr/bin/env python3
#
# lime-juice: C++ port of Tomyun's "Juice" de/recompiler for PC-98 games
# Copyright (C) 2026 Fuzion
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
"""barebones ADV MES token dumper. linear scan, no tree structure."""

import sys

def tokdump(path):
    data = open(path, 'rb').read()
    pos = 0
    size = len(data)

    def peek():
        return data[pos] if pos < size else None

    def peek2():
        return (data[pos], data[pos+1]) if pos + 1 < size else (None, None)

    def consume(n=1):
        nonlocal pos
        bs = data[pos:pos+n]
        pos += n
        return bs

    def hexbytes(bs):
        return ' '.join(f'{b:02X}' for b in bs)

    seg_num = 0

    while pos < size:
        b = peek()
        start = pos

        # EOM: FF FE
        b1, b2 = peek2()
        if b1 == 0xFF and b2 == 0xFE:
            print(f'  {start:5d}: EOM (FF FE)')
            consume(2)
            continue

        # EOS: FF FF
        if b1 == 0xFF and b2 == 0xFF:
            print(f'  {start:5d}: EOS (FF FF)')
            consume(2)
            seg_num += 1
            print(f'  --- segment {seg_num} ---')
            continue

        # REG*: 0x00-0x0F + byte
        if b <= 0x0F:
            bs = consume(2)
            print(f'  {start:5d}: REG*   {hexbytes(bs)}')
            continue

        # VAR*: 0x10-0x1F + bytes
        if 0x10 <= b <= 0x1F:
            bs = consume(2)
            # extraop: if high bit of second byte, consume third
            if bs[1] & 0x80:
                bs += consume(1)
            print(f'  {start:5d}: VAR*   {hexbytes(bs)}')
            continue

        # CHRS!: 0x21 + chars + (0x00 | 0xFF) -- must check before NUM0
        if b == 0x21:
            consume(1)
            content = bytearray()
            while pos < size and data[pos] not in (0x00, 0xFF):
                content.append(data[pos])
                pos += 1
            if pos < size:
                term = data[pos]
                pos += 1
                termstr = f'{term:02X}'
            else:
                termstr = 'EOF'
            preview = content[:20]
            print(f'  {start:5d}: CHRS!  ({len(content)} bytes, term={termstr}) {hexbytes(preview)}...')
            continue

        # ARG: 0x22 + chars + 0x22 -- must check before NUM0
        if b == 0x22:
            consume(1)
            content = bytearray()
            while pos < size and data[pos] != 0x22:
                content.append(data[pos])
                pos += 1
            if pos < size:
                pos += 1  # consume closing 0x22
            try:
                text = content.decode('ascii', errors='replace')
            except:
                text = hexbytes(content[:20])
            if len(text) > 50:
                text = text[:50] + '...'
            print(f'  {start:5d}: ARG    "{text}"')
            continue

        # NUM0: 0x20, 0x23-0x2F (value = b - 0x20)
        if b == 0x20 or (0x23 <= b <= 0x2F):
            consume(1)
            print(f'  {start:5d}: NUM0   {b:02X}  (={b - 0x20})')
            continue

        # NUM1: 0x30-0x31 + byte
        if b in (0x30, 0x31):
            bs = consume(2)
            val = (bs[0] - 0x30) * 256 + bs[1]
            print(f'  {start:5d}: NUM1   {hexbytes(bs)}  (={val})')
            continue

        # NUM2: 0x32 + 2 bytes
        if b == 0x32:
            bs = consume(3)
            val = (bs[1] << 8) | bs[2]
            print(f'  {start:5d}: NUM2   {hexbytes(bs)}  (={val})')
            continue

        # CHR-LBEG: 0x81 0x79
        b1, b2 = peek2()
        if b1 == 0x81 and b2 == 0x79:
            consume(2)
            print(f'  {start:5d}: LBEG   81 79')
            continue

        # CHR-LEND: 0x81 0x70
        if b1 == 0x81 and b2 == 0x70:
            consume(2)
            print(f'  {start:5d}: LEND   81 70')
            continue

        # CHR-WAIT: 0x81 0x7B
        if b1 == 0x81 and b2 == 0x7B:
            consume(2)
            print(f'  {start:5d}: WAIT   81 7B')
            continue

        # CHR-NOP: 0x81 0x7A
        if b1 == 0x81 and b2 == 0x7A:
            consume(2)
            print(f'  {start:5d}: NOP@   81 7A')
            continue

        # BEG+: 0xA0
        if b == 0xA0:
            consume(1)
            print(f'  {start:5d}: BEG+   A0')
            continue

        # END*: 0xA1
        if b == 0xA1:
            consume(1)
            print(f'  {start:5d}: END*   A1')
            continue

        # BEG: 0xA2
        if b == 0xA2:
            consume(1)
            print(f'  {start:5d}: BEG    A2')
            continue

        # END: 0xA3
        if b == 0xA3:
            consume(1)
            print(f'  {start:5d}: END    A3')
            continue

        # CNT: 0xA4
        if b == 0xA4:
            consume(1)
            print(f'  {start:5d}: CNT    A4')
            continue

        # CMD: 0xA5-0xDF
        if 0xA5 <= b <= 0xDF:
            consume(1)
            print(f'  {start:5d}: CMD    {b:02X}  (={b})')
            continue

        # CHR2: 0x80-0x9F + second byte (SJIS pair)
        if 0x80 <= b <= 0x9F:
            bs = consume(2)
            print(f'  {start:5d}: CHR2   {hexbytes(bs)}')
            continue

        # CHR1: 0x2D-0x7F (maps to SJIS via offset) or 0xE0-0xFE
        if (0x2D <= b <= 0x7F) or (0xE0 <= b <= 0xEA):
            consume(1)
            ascii_char = chr(b) if 0x20 <= b <= 0x7E else ''
            print(f'  {start:5d}: CHR1   {b:02X}  {ascii_char}')
            continue

        # unknown
        consume(1)
        print(f'  {start:5d}: ???    {b:02X}')

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print(f'usage: {sys.argv[0]} <file.mes>')
        sys.exit(1)
    tokdump(sys.argv[1])
