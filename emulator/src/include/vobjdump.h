#pragma once
/*
 * vobjdump
 * Views the contents of a VOBJ file.
 * Written by Frank Wille <frank@phoenix.owl.de>.
 */

#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* maximum VOBJ version to support */
#define VOBJ_MAX_VERSION 3

/* symbol types */
#define LABSYM 1
#define IMPORT 2
#define EXPRESSION 3

/* symbol flags */
#define TYPE(sym) ((sym)->flags & 7)
#define TYPE_UNKNOWN 0
#define TYPE_OBJECT 1
#define TYPE_FUNCTION 2
#define TYPE_SECTION 3
#define TYPE_FILE 4

#define EXPORT (1 << 3)
#define INEVAL (1 << 4)
#define COMMON (1 << 5)
#define WEAK (1 << 6)
#define SYMINDIR (1 << 18) /* symbol indirection since V3+ */

typedef int64_t taddr;
typedef uint8_t ubyte;

// Define symbol name max length to 32 for comparisions
#define MAX_SYMBOL_LENGTH 32
struct vobj_symbol {
  size_t offs;
  const char *name;
  char symbolname[MAX_SYMBOL_LENGTH]; // used for comparisions, not populated by
                                      // vobjdump
  int type, flags, sec, size;
  taddr val;
};

struct vobj_section {
  size_t offs;
  const char *name;
  taddr dsize, fsize;
};

#define ABSOLUTE                                                               \
  (1 << 4) /* section-flags: has absolute start address (V3+)                  \
            */

#define STD_REL_TYPE(t) ((t) & 0x1f)
#define REL_MOD_S 0x20
#define REL_MOD_U 0x40
#define FIRST_CPU_RELOC 0x80
#define makemask(x)                                                            \
  (((x) >= sizeof(unsigned long long) * CHAR_BIT)                              \
       ? (~(unsigned long long)0)                                              \
       : ((((unsigned long long)1) << (x)) - 1u))

//static int vobjdump_main(int argc, char **argv);

extern int ver;      /* VOBJ version */
extern ubyte *vobj;  /* base address of VOBJ buffer */
extern size_t vlen;  /* length of VOBJ file in buffer */
extern ubyte *p;     /* current object pointer */
extern int bpb, bpt; /* bits per target-byte, target-bytes per taddr */
extern int bitspertaddr;
extern size_t opb;    /* octets (host-bytes) per target-byte */
extern taddr bptmask; /* mask LSB to fit bytes per taddr */
extern const char *cpu_name;

#define BPTMASK(x) (unsigned long long)((x) & bptmask)

extern void obj_corrupt(void);

extern taddr read_number(int is_signed);

extern void skip_string(void);
extern void print_sep(void);

extern int print_nreloc(const char *relname, struct vobj_section *vsect,
                 struct vobj_symbol *vsym, int nsyms, taddr offs, int bpos,
                 int bsiz, taddr mask, taddr addend, int sym);

extern int print_cpureloc(int type, ubyte *rstart);

extern const char *cpu_reloc_name(int type);

extern const char *standard_reloc_name(int type);

extern void read_symbol(struct vobj_symbol *vsym);

extern void read_section(struct vobj_section *vsect, struct vobj_symbol *vsym,
                  int nsyms);
extern const char *bind_name(int flags);

extern const char *def_name(struct vobj_symbol *vs, struct vobj_section *sec,
                     int nsecs);

extern struct vobj_symbol *vobjdump(int *nsyms);

extern size_t filesize(FILE *fp, const char *name);
