# Static recompilation of Diggers: the game (gen/: !RunImage as its own
# squeeze decompressor unpacks it, ../tools/unpack.py), the intro and outro
# (genintro/, genoutro/: Disk1.!Intro and Disk1.!Outro) and the two modules
# !Run loads (genmode/: 50KHzMode, the DiggersMode module; genptr/:
# FastPtr), from SuperSpider's analyse and gen, and game.c, linked with
# libroger (the RISC OS runtime).
ROGER   ?= $(HOME)/stuff/libroger
CC      ?= gcc
CFLAGS  ?= -O2 -g -w -fno-strict-aliasing $(if $(TRACE),-DRECOMP_TRACE)
GENS    := gen genintro genoutro genmode genptr
GEN     := $(foreach g,$(GENS),$(wildcard $(g)/code*.c) $(g)/entries.c)
OBJ     := $(GEN:.c=.o) game.o
LIBS    := $(shell pkg-config --libs sdl3) -ljpeg -lm -lpthread
INC     := -I$(ROGER)/include

diggers: $(OBJ) $(ROGER)/libroger.a
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(ROGER)/libroger.a $(LIBS)

$(ROGER)/libroger.a: FORCE
	$(MAKE) -C $(ROGER) libroger.a

%.o: %.c $(ROGER)/include/roger.h $(ROGER)/include/roger_cpu.h
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

# Code/data analysis: SuperSpider, into ../analysis/<name>.json, then
# superspider gen. Each image's ../analysis/<name>.roots adds entry points
# to the analysis: "make roots" merges the gap entries a --trace-gaps run
# logged (untranslated.log, or LOG=...) into them, so the next "make regen"
# translates them. The three programs share &8000 (one runs after the
# other): the log tags each entry with the image that was running. The
# modules are translated where game.c loads them in the RMA.
SS      ?= $(HOME)/stuff/superspider
SSBIN   := $(SS)/_build/default/bin/main.exe
AN      := ../analysis
UN      := ../unpacked
LOG     ?= untranslated.log
MODE_AT := 1800000
PTR_AT  := 1801000

# --roots for ../analysis/<name>.roots, if there is one
roots = $(if $(wildcard $(AN)/$(1).roots),--roots $(AN)/$(1).roots)

# $(call analyse,name,base[,more options])
define analyse
	$(SSBIN) analyse $(UN)/$(1) $(2) $(3) --full $(call roots,$(1)) -o $(AN)/$(1).json
endef

# $(call gen,name,base,dir[,more options])
define gen
	$(SSBIN) gen $(UN)/$(1) $(2) $(AN)/$(1).json --c $(3) --leaders=dispatch $(4)
endef

regen:
	cd $(SS) && dune build
	python3 -I ../tools/unpack.py
	mkdir -p $(AN)
	$(call analyse,runimage,8000)
	$(call analyse,intro,8000)
	$(call analyse,outro,8000)
	$(call analyse,diggersmode,$(MODE_AT),--module)
	$(call analyse,fastptr,$(PTR_AT),--module)
	rm -rf $(GENS)
	$(call gen,runimage,8000,gen)
	$(call gen,intro,8000,genintro,--prefix=I --tabprefix=recompI --files=1 --covtag=I)
	$(call gen,outro,8000,genoutro,--prefix=O --tabprefix=recompO --files=1 --covtag=O)
	$(call gen,diggersmode,$(MODE_AT),genmode,--prefix=M --tabprefix=recompM --files=1 --covtag=M)
	$(call gen,fastptr,$(PTR_AT),genptr,--prefix=P --tabprefix=recompP --files=1 --covtag=P)

roots:
	python3 -I $(SS)/tools/gap_roots.py $(UN)/runimage 8000 $(AN)/runimage.roots $(LOG)
	python3 -I $(SS)/tools/gap_roots.py $(UN)/intro 8000 $(AN)/intro.roots $(LOG)
	python3 -I $(SS)/tools/gap_roots.py $(UN)/outro 8000 $(AN)/outro.roots $(LOG)

clean:
	rm -f $(foreach g,$(GENS),$(g)/*.o) game.o diggers

.PHONY: FORCE regen roots clean
