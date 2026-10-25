#!/usr/bin/env python3
"""The main program, unsqueezed by its own decompressor, and the saved
games directory.

    unpack.py           game/!Diggers/!RunImage -> unpacked/runimage
                        Disk1.!Intro, Disk1.!Outro, 50KHzMode, FastPtr
                        -> unpacked/intro, outro, diggersmode, fastptr
                        game/!Diggers/SavedGames, from !MakeDisk's

!Run runs Disk1.!Intro and then !RunImage, both absolutes at &8000.
!RunImage is an AIF image packed by Acorn's squeeze: its first word
branches to the decompressor at its end, which unpacks the image in place,
puts a NOP over that first word and returns to &8004. Here that runs under
SuperSpider's tools/arm2.py, until it returns. The unpacked AIF header then
gives the image's size (read-only + read-write; no zero-initialised data),
and the image is translated and run as it is then. !Intro and !Outro aren't
packed: they are translated as they are on the disc, as are the two
modules !Run loads (50KHzMode, the DiggersMode module, and FastPtr), copied
here only to have names without the ,load-exec suffixes (the date stamp,
which differs from one copy of a disc to another).

The saved games: the game keeps them on a disc of their own, which disk 1's
!MakeDisk makes: a !Diggers.SavedGames directory holding Present (one byte:
the game looks for it to know the disc is in) and the options file
(CHR$164 + "GameOpts"). The game reads them as <Diggers$Dir>.SavedGames,
so the template goes there, and what the game saves goes to libroger's
save directory beside it.
"""
import os
import shutil
import struct
import sys

SS = os.environ.get('SUPERSPIDER', os.path.expanduser('~/stuff/superspider'))
sys.path.insert(0, os.path.join(SS, 'tools'))
import arm2  # noqa: E402

TOP = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
APP = os.path.join(TOP, 'game', '!Diggers')
DISK1 = os.path.join(TOP, 'extracted', 'disk1')
OUT = os.path.join(TOP, 'unpacked')
BASE, DONE, STACK = 0x8000, 0x8004, 0x7F000
AIF_NOP = 0xFB000000


def find(d, name):
    """a RISC OS file in a host directory: any case, any ,load-exec suffix"""
    for f in os.listdir(d):
        if f.split(',')[0].lower() == name.lower():
            return os.path.join(d, f)
    raise FileNotFoundError(os.path.join(d, name))


def unsqueeze(name, packed):
    m = arm2.ARM(memsize=0x1000000)
    m.mem[BASE:BASE + len(packed)] = packed
    m.r[13], m.r[15] = STACK, BASE

    def swi(m, n, pc):
        sys.exit('unpack: %s calls SWI &%X at &%X while unsqueezing (not the known release?)' % (name, n, pc))
    m.swi_handler = swi
    while m.r[15] != DONE:
        m.step()
        if m.steps > 20_000_000:
            raise RuntimeError('unpack: %s: no end, pc=&%X' % (name, m.r[15]))
    head = struct.unpack_from('<9I', m.mem, BASE)
    if head[0] != AIF_NOP or head[8]:
        sys.exit('unpack: %s: not a squeezed AIF image as expected' % name)
    return bytes(m.mem[BASE:BASE + head[5] + head[6]])


def main():
    os.makedirs(OUT, exist_ok=True)
    data = unsqueeze('!RunImage', open(find(APP, '!RunImage'), 'rb').read())
    open(os.path.join(OUT, 'runimage'), 'wb').write(data)
    print('runimage: %d bytes at &%X-&%X' % (len(data), BASE, BASE + len(data)))
    for name, d, leaf in [('intro', 'Disk1', '!Intro'), ('outro', 'Disk1', '!Outro'),
                          ('diggersmode', '', '50KHzMode'), ('fastptr', '', 'FastPtr')]:
        shutil.copyfile(find(os.path.join(APP, d), leaf), os.path.join(OUT, name))
    saved = os.path.join(APP, 'SavedGames')
    shutil.rmtree(saved, ignore_errors=True)
    shutil.copytree(find(find(DISK1, '!MakeDisk'), 'SavedGames'), saved)
    print('SavedGames: %s' % ', '.join(sorted(os.listdir(saved))))


if __name__ == '__main__':
    main()
