ASM        := ./vasm
SRCDIR     := ./asmsrc
BUILDDIR   := ./build

ASMSRC     := $(wildcard $(SRCDIR)/*.s)
BINS       := $(patsubst $(SRCDIR)/%.s,$(BUILDDIR)/%.out,$(ASMSRC))
SYMS       := $(patsubst $(SRCDIR)/%.s,$(BUILDDIR)/%.sym,$(ASMSRC))
VOBJS			 := $(patsubst $(SRCDIR)/%.s,$(BUILDDIR)/%.vobj,$(ASMSRC))

VASM_ASM_FLAGS  := -Fbin -dotdir -Iutils
VASM_SYM_FLAGS	:= -dotdir -Iutils
VASM_VOBJ_FLAGS := -Fvobj -dotdir -Iutils

.PHONY: all clean builddir dbgsym binaries symbols vobjs test

all: builddir binaries symbols vobjs

builddir:
	mkdir -p $(BUILDDIR)

binaries: $(BINS)

symbols: $(SYMS)

vobjs: $(VOBJS)

$(BUILDDIR)/%.out: $(SRCDIR)/%.s
	$(ASM) $(VASM_ASM_FLAGS) -o $@ $<

$(BUILDDIR)/%.sym: $(SRCDIR)/%.s
	$(ASM) $(VASM_SYM_FLAGS) -Llo -o $@ $<

$(BUILDDIR)/%.vobj: $(SRCDIR)/%.s
	$(ASM) $(VASM_VOBJ_FLAGS) -o $@ $<

dbgsym: builddir symbols vobjs

clean:
	rm -rf $(BUILDDIR)
