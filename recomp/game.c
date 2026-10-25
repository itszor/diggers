/* Diggers (Millennium, 1994; Archimedes version by A.E. Hersee): what
 * libroger needs to know about the game.
 *
 * !Run RMRuns 50KHzMode (the DiggersMode module: *DigModeOn defines a
 * 320x200 256-colour mode and changes to it) and FastPtr (its pointer and
 * sound helpers), sets Diggers$Dir and the language and intro/outro
 * directories, runs !MaxMem (a utility that shrinks the RMA, sprites, font
 * cache and RAM disc to nothing: as libroger starts out, so it isn't run
 * here), then, with a 680K WimpSlot, runs Disk1.!Intro and, when it exits,
 * !RunImage. Both are absolutes at &8000; !RunImage is an AIF image packed
 * with Acorn's squeeze, unpacked by ../tools/unpack.py. When the player
 * quits, !RunImage *Runs Disk1.!Outro, which ends the run.
 *
 * The game reads its files as <Diggers$Dir>.Disk1.<name> &c.; the three
 * discs are merged into one !Diggers (game/), and the saved games disc, as
 * !MakeDisk makes it, is !Diggers.SavedGames there. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <roger.h>

extern const struct roger_image recomp_image, recompI_image, recompO_image;  /* gen{,intro,outro}/ */
extern const struct roger_image recompM_image, recompP_image;               /* gen{mode,ptr}/ */

#define RUNIMAGE "../unpacked/runimage"

static const struct roger_module modules[] = {
    {"../unpacked/diggersmode", &recompM_image, 0, NULL, 0},
    {"../unpacked/fastptr", &recompP_image, 0, NULL, 0},
    {NULL, NULL, 0, NULL, 0},
};

static const struct roger_program programs[] = {
    {"!Outro", &recompO_image, 0, 0, 0},
    {NULL, NULL, 0, 0, 0},
};

/* !Run's variables (Diggers$Dir is ADFS::0.$.!Diggers there: the discs are
 * swapped in drive 0 as the game asks for them) */
static const char *const vars[] = {
    "Diggers$Dir", "!Diggers",
    "DigLang$File", "English",
    "DigIntro$Dir", "!Diggers.Disk1",
    "DigIntroRes$Dir", "!Diggers.Disk1",
    "DigOutro$Dir", "!Diggers.Disk1",
    "DigOutroRes$Dir", "!Diggers.Disk1",
    NULL};
static const char *const prefixes[] = {"ADFS::0.$.", "adfs::0.$.", NULL};

/* !Run's "<Diggers$Dir>.!RunImage" once !Intro exits: the unpacked image at
 * &8000, run from its start as a fresh program (R13 at the top of the
 * WimpSlot) */
static int game_swi(uint32_t n, int x)
{
    (void)x;
    if ((n & ~0x20000u) == 0x11 && roger_current_image() == &recompI_image) {
        if (roger_load_file(RUNIMAGE, ROGER_LOAD) <= 0) { perror(RUNIMAGE); roger_finish("no !RunImage"); }
        roger_set_image(&recomp_image);
        R[13] = ROGER_LOAD + 680 * 1024;
        R[12] = 0;
        roger_call_program(ROGER_LOAD, ROGER_HANDLER_RET);
        roger_finish("returned to OS");
    }
    return 0;
}

/* The headless scenario (--limit runs; instruction counts, 2000 a ms):
 * Space ends the intro; the title page and credits then cycle, and after
 * two cycles a demo's zone page comes up (grassland). To play a level, a
 * script instead: Space again during the credits, then a click past the
 * jungle zone's page (the README). (Given as this scenario, the same keys
 * and click lead to the grassland page, which doesn't take the click: why
 * the two differ isn't established.) The title page's menu - QUIT GAME,
 * START GAME, the hotspots at &BB10 - is only polled for 13s each cycle,
 * from 98s and 196s here. Run it with --full-os: the intro and the game
 * draw into the bank OS_ReadVduVariables 149 (DisplayStart) doesn't give,
 * and headless runs otherwise give the write bank's for both, so they'd
 * draw over what's shown */
static const struct roger_key keys[] = {
    {99, 120000000, 120400000},                     /* Space: end the intro */
};

static struct roger_config config = {
    .title = "Diggers (recompiled)",
    .image_path = "../unpacked/intro",
    .image = &recompI_image,
    .programs = programs,
    .modules = modules,
    .appdir = "../game",
    .dir_prefixes = prefixes,
    .variables = vars,
    .command_line = "<DigIntro$Dir>.!Intro",
    .initial_mode = 12,                 /* the desktop's: DigModeOn sets the game's */
    .wimpslot = 680 * 1024,             /* !Run's *WimpSlot 680k */
    .screen_size = 128 * 1024,          /* "Diggers requires 125k of screen memory" */
    .swi = game_swi,
    .keys = keys,
    .nkeys = sizeof keys / sizeof keys[0],
};

int main(int argc, char **argv) { roger_main(argc, argv, &config); }
