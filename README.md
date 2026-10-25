Diggers (Millennium, 1994; Archimedes conversion by Andrew Hersee) - static recompilation
=====================================================================================

A recipe: this repository holds no game data. It rebuilds the game as a
native executable from your own copy, recognised by content (recipe.json),
so it can be shared without distributing the game.

    ./build.sh "Diggers (1994)(Millenium)(Disk 1 of 3).adf" "...(Disk 2 of 3).adf" "...(Disk 3 of 3).adf"
    cd recomp && ./diggers

build.sh runs recipe.json's steps with SuperSpider's tools/weave.py
(~/stuff/superspider, or $SUPERSPIDER; $SS below): it extracts the discs
and merges them into one !Diggers, unsqueezes !RunImage with its own
decompressor (tools/unpack.py), analyses and translates the programs and
modules, and links them with libroger (~/stuff/libroger, or make
ROGER=...). Needs python3, OCaml/dune, gcc and SDL3.

The inputs: TOSEC's three ADFS E discs. Disk 1: !Diggers (!Run, the
50KHzMode and 100KHzMode modules, FastPtr, !MaxMem, !RunImage), its Disk1
directory (!Intro, !Outro, fonts, the four languages, the intro's and
outro's pictures and music) and two utilities, !Install (a hard disc
installer) and !MakeDisk (which makes a saved games disc). Disk 2: Disk2
(the 34 levels, the zone maps and backgrounds, the title music). Disk 3:
Disk3 (zone sprites, the shop, bank and end screens, MapModule, the in-game
music) and Both (the trading centre, the menus).

What it makes:

    extracted/disk1-3        the discs, via weave.py
    game/!Diggers            libroger's appdir (../game): disk 2's !Diggers, disk 3's, then disk 1's
    game/!Diggers/SavedGames tools/unpack.py: !MakeDisk's saved games disc, where the game looks for it
    unpacked/runimage        tools/unpack.py: !RunImage as its decompressor leaves it
    unpacked/intro, outro,   ...and the other images, as they are on the disc (names
      diggersmode, fastptr      without the date-stamp suffixes)
    analysis/*.json          SuperSpider's analyses; *.roots: entry points it can't find
    recomp/gen*/             the C translations
    recomp/diggers           the game

Committed: recipe.json, build.sh, tools/unpack.py, analysis/*.roots,
recomp/Makefile, recomp/game.c, this file.

Playing
-------

    ./diggers               as !Run: the intro (Space ends it), the title page

The game is played with the mouse. Saved games and the options file go to
the saved games disc (<Diggers$Dir>.SavedGames): here into libroger's save
directory, read back from there.

How the game is put together
----------------------------

!Run RMRuns 50KHzMode (the DiggersMode module: *DigModeOn defines a
320x200 256-colour mode through Service_ModeExtension and changes to it)
and FastPtr (pointer and sound helpers), sets Diggers$Dir
(ADFS::0.$.!Diggers: the discs are swapped in drive 0 as the game asks for
them), the language file and the intro's and outro's directories, runs
!MaxMem (shrinks the RMA, sprites, font cache and RAM disc: as libroger
starts out, so it isn't run), then with a 680K WimpSlot runs Disk1.!Intro
and !RunImage. When the player quits, !RunImage *Runs Disk1.!Outro.

- !Intro (&8000-&1267C, plain AIF): the cockpit and credits, with AEH
  music on its own sound DMA handler (device 9). Its screen copy is an
  unrolled loop it cuts short by storing branches over four of its words
  (&BB18: a table of the addresses, and the words saved).
- !RunImage (&8000-&26CBC, squeezed AIF): the game. Its data files are
  compressed ("DIGG" files; the load address is the unpacked length), its
  music is AEH SMOD files.
- !Outro: the ending sequence.

What's translated (recomp/Makefile, make regen): !RunImage (gen/), !Intro
(genintro/) and !Outro (genoutro/), all for &8000, and the two modules
(genmode/, genptr/) at &1800000 and &1801000. game.c runs !Intro as the
program and, at its OS_Exit, loads !RunImage's unpacked image and runs it,
as !Run's next line would; !Outro is a program (roger_config.programs) run
when !RunImage asks for it.

The entry points the analysis can't find (analysis/*.roots):

- intro: the run-on into the screen copy past each branch &BB18 stores
  over (&EBCC, &EBFC; &EBF8 is the stored-over word itself);
- runimage: handlers in tables the game fills at run time (zeros in the
  image: cmp r2,#0 ; movne pc,r2 at &1047C).

A bug in the game, harmless on the hardware: &9A5C pops its caller's R2
into PC on one path (pushing R10 and returning with ldmia sp!,{r2,pc}^);
R2 then points into the byte table at &9464, whose words are all EQ
conditions, so with the flags clear the ARM runs through them as no-ops.
libroger's interpreter does the same if it happens.

What SuperSpider and libroger needed
------------------------------------

!RunImage was assembled with BASIC's assembler, and its two passes
disagreed about a table's length: every call from the code before the
table goes to &12704, the table's last word (&0000FFFF), one word before
the routine at &12708 that the code after the table calls. The game makes
such a call with Z set (&B9D8, after cmp lr,#0 at &C01C), so the word is
executed. Executed as andeq pc,r0,pc,ror pc, as Arculator and RPCEmu decode
it, the game jumps to &40000 and stops ("Trap ... caught at &FFFFFFF8").
It runs on the hardware, so the ARM2 doesn't execute it so: it takes bits 7
and 4 set (bits 27-22 clear) as a multiply, whatever bits 6-5 are, making
it muleq r0,pc,pc, which falls through (r0 is pushed by the routine and
restored).

SuperSpider:

- arm2.ml, gen.ml, tools/arm2.py: the ARM2's multiply decoding, as above.
  Such a word stays implausible ("multiply with bits 6-5 set": no
  assembler writes one), so guesses don't take data with it for code.
- S1: such a word at the entry of a function code calls is code, and runs
  on (slipped_call); the caller is recorded before the callee is analysed
  so the rule can see it. Without it the analysis found 1960 of
  !RunImage's code words; with it, 17330.

libroger:

- cpu.c: the interpreter's multiply decoding, as above.
- irq.c: the vsync event (EventV, 4) is called with R3 = IOC, as the
  kernel's VSync interrupt handler raises it. !Intro's event handler
  clears the vsync interrupt itself (strb r1,[r3,#&14]); with the program's
  R3 that store landed in MEMC's control register (&37E97F4 as a 26-bit
  address) and turned the sound DMA off, and the intro then waited for a
  sound interrupt for ever. Not checked against RISC OS's sources (RISC OS
  Open's GitLab was unavailable); the game's handler is the evidence.

- video.c: the pointer's image in the kernel's workspace, where the cursor
  DMA shows it from (two 32x32 buffers at &1F038F8 and &1F039F8). The
  address and layout are inferred from FastPtr's code alone, not checked
  against RISC OS: which versions keep it there, and whether the kernel
  copies the current shape there as libroger does, isn't known. The game defines
  its pointer once (OS_Word 21,0, shape 1) and then animates it through
  FastPtr's SetPointer32x16, which copies each frame straight into those
  buffers; libroger showed only the OS_Word 21 shape, so the pointer stayed
  a clock face (the game's first "wait" frame) all through. libroger now
  puts the current shape there and takes what a program writes there,
  once a frame, as the shape shown. The pointer is the game's only feedback
  on its menus: on the title page an arrow, an animated EXIT over QUIT
  GAME and OK over START GAME (the hotspots at &BB10 each name a pointer
  animation, &12664); elsewhere a turning clock while it's busy.

- host.c: the host mouse mapped onto the mode's own size in OS units (its
  pixels times its eig factors), not always 1280x1024. Diggers' mode is
  320x200 at eig 2,2, 1280x800 units, and the game takes the pointer's
  row as 199 - y/4; mapped onto 1024 units the hit point was above the
  pointer, by about 57 rows at the top of the screen, 15 at row 150 and
  none at the bottom. (Every standard Archimedes mode is 1280x1024, so
  nothing changes there.) Checked with an interactive run (SDL's offscreen
  driver): a tap at pixel row 112, on QUIT GAME, quits; it was row 87.

Testing: headless runs need --full-os. The intro and the game read
ScreenStart and DisplayStart (OS_ReadVduVariables 148, 149) after setting
the banks, and draw into the one not shown; headless runs otherwise give
the write bank's address for both. game.c's scenario only ends the intro
(the title page, the credits, then a demo's zone page); this script plays a
level of the jungle zone (Space during the credits, a click past the
zone's page; the diggers dig from about 330s):

    ./diggers --full-os --limit=900000000 --script=60000:key:99,160000:key:99,320000:click:640:400,320200:release

QUIT GAME on the title page: --script=60000:key:99,200000:click:380:260,200400:release
(the menu is polled from 196s), and the run ends with "Good bye, I hope
you enjoyed your visit to Zarg and will call again soon."

Regression: in every other game (31), make regen, the generated C compared
with the C before the changes, and headless runs (200M instructions: the
event log, VDU stream, screen, registers and --wav output) compared with
runs of the binaries from before; phase3/eval_s1b.py unchanged. The
generated C is the same in all but three: in Gods and Fire & Ice, words a
program stores values into (self-modified code, a case per value) whose
cases with bits 7 and 4 set are now multiplies, as the ARM2 runs them, not
ands; in Stunt Racer 2000, five words its code runs on into at &5BF5C (a
root from a --trace-gaps run, data it executes, EQ-conditional), now
translated instead of interpreted. The headless runs are byte-identical in
all 32 programs.

Not done yet: the game beyond its first screens. The scenario above, and
the runs while porting, reach the title page (QUIT GAME ends the run, as
on RISC OS), the credits, the attract mode's demo of a level (with no
untranslated code), and START GAME's first screen. The levels, the shop,
the bank, the trading centre, saving and loading, and the ending
(!Outro, run after a won game) haven't been played: play them with
--trace-gaps and close the gaps (make roots, make regen).
