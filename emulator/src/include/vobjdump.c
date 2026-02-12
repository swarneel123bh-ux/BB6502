/*
 * vobjdump
 * Views the contents of a VOBJ file.
 * Written by Frank Wille <frank@phoenix.owl.de>.
 */

/*
  Format:

  .byte 0x56,0x4f,0x42,0x4a
  .byte flags
    Bits 0-1:
     1: BIGENDIAN
     2: LITTLENDIAN
    Bits 2-7:
     VOBJ-Version (0-based)
  .number bitsperbyte
  .number bytespertaddr
  .string cpu
  .number nsections [1-based]
  .number nsymbols [1-based]

nsymbols
  .string name
  .number type
  .number flags
  .number secindex
  .number val
  .number size (in target-bytes)

nsections
  .string name
  .string attr
  .number flags
  .number address [vobj version 3+ and flags&ABSOLUTE only]
  .number align
  .number size (in target-bytes)
  .number nrelocs
  .number databytes (in target-bytes)
  .byte[databytes*(BITSPERBYTE+7)/8]

nrelocs [standard|special]
standard
   .number type
   .number byteoffset
   .number bitoffset
   .number size
   .number mask
   .number addend
   .number symbolindex | 0 (sectionbase)

special
    .number type
    .number size (0 means standard nreloc)
    .byte[size]

.number:[taddr]
    .byte 0--127 [0--127]
    .byte 128-191 [x-0x80 bytes little-endian], fill remaining with 0
    .byte 192-255 [x-0xC0 bytes little-endian], fill remaining with 0xff [vobj
version 2+]
*/

#include "vobjdump.h"

int ver;      /* VOBJ version */
ubyte *vobj;  /* base address of VOBJ buffer */
size_t vlen;  /* length of VOBJ file in buffer */
ubyte *p;     /* current object pointer */
int bpb, bpt; /* bits per target-byte, target-bytes per taddr */
int bitspertaddr;
size_t opb;    /* octets (host-bytes) per target-byte */
taddr bptmask; /* mask LSB to fit bytes per taddr */
const char *cpu_name;

const char *endian_name[] = {"big", "little"};
const char emptystr[] = "";
const char sstr[] = "s";
const char unknown[] = "UNKNOWN";

const char *std_reloc_name[] = {
    "NONE",    "ABS",     "PC",    "GOT",     "GOTPC",  "GOTOFF",
    "GLOBDAT", "PLT",     "PLTPC", "PLTOFF",  "SD",     "UABS",
    "LOCALPC", "LOADREL", "COPY",  "JMPSLOT", "SECOFF", "MEMID"};
const int num_std_relocs = sizeof(std_reloc_name) / sizeof(std_reloc_name[0]);

const char *ppc_reloc_name[] = {"EABISDA2",    "EABISDA21", "EABISDAI16",
                                "EABISDA2I16", "MOSDREL",   "AOSBREL"};
const int num_ppc_relocs = sizeof(ppc_reloc_name) / sizeof(ppc_reloc_name[0]);

const char *type_name[] = {"", "obj", "func", "sect", "file", NULL};

void obj_corrupt(void) {
  fprintf(stderr, "\nvobjdump: Object file is corrupt! Aborting.\n");
  exit(1);
}

taddr read_number(int is_signed) {
  taddr val;
  ubyte n;
  int i;

  if (p < vobj || p >= vobj + vlen)
    obj_corrupt();

  if ((n = *p++) <= 0x7f)
    return (taddr)n;

  if (p + (n & 0x3f) > vobj + vlen)
    obj_corrupt();

  if (n >= 0xc0) { /* version 2 negative numbers */
    n -= 0xc0;
    for (i = 0, val = ~makemask(n * 8); n--; i += 8)
      val |= (taddr)*p++ << i;
  } else {
    for (i = 0, n -= 0x80, val = 0; n--; i += 8)
      val |= (taddr)*p++ << i;
    if (is_signed && (val & (1LL << (bitspertaddr - 1))))
      val |= ~makemask(bitspertaddr); /* version 1 negative numbers support */
  }
  return val;
}

void skip_string(void) {
  if (p < vobj)
    obj_corrupt();
  while (*p) {
    p++;
    if (p >= vobj + vlen)
      obj_corrupt();
  }
  p++;
}

void print_sep(void) {
  /*
  printf("\n------------------------------------------------------------"
         "------------------\n"); */
}

int print_nreloc(const char *relname, struct vobj_section *vsect,
                 struct vobj_symbol *vsym, int nsyms, taddr offs, int bpos,
                 int bsiz, taddr mask, taddr addend, int sym) {
  const char *basesym;

  if (offs < 0 || offs + ((bpos + bsiz - 1) / bpb) >= vsect->dsize) {
    /*
    printf("offset %#llx is outside of section!\n",
           BPTMASK(offs + (bpos + bsiz - 1) / bpb)); */
    return 0;
  }
  if (sym < 0 || sym >= nsyms) {
    // printf("symbol index %d is illegal!\n", sym + 1);
    return 0;
  }
  if (bsiz < 0 || bsiz > bitspertaddr) {
    // printf("size of %d bits is illegal!\n", bsiz);
    return 0;
  }
  if (bpos < 0 || bpos + bsiz > bitspertaddr) {
    /*
    printf("bit field start=%d, size=%d doesn't fit into target address "
           "type (%d bits)!\n",
           bpos, bsiz, bitspertaddr); */
    return 0;
  }

  basesym = vsym[sym].name;
  if (!strncmp(basesym, " *current pc", 12)) {
    basesym = vsect->name;
    /*addend += offs;*/
  }

  /*
  printf("%08llx  %02d %02d %8llx %-8s %s%+" PRId64 "\n", BPTMASK(offs), bpos,
         bsiz, BPTMASK(mask), relname, basesym, addend); */
  return 1;
}

int print_cpureloc(int type, ubyte *rstart) {
  return 0; /* unknown cpu specific relocation */
}

const char *cpu_reloc_name(int type) {
  if (type >= FIRST_CPU_RELOC) {
    type -= FIRST_CPU_RELOC;

    if (!strcmp(cpu_name, "PowerPC") && type < num_ppc_relocs)
      return ppc_reloc_name[type];
  }
  return unknown;
}

const char *standard_reloc_name(int type) {
  int basetype = STD_REL_TYPE(type);

  if (basetype < num_std_relocs) {
    char *buf = (char*) malloc(32 * sizeof(char));

    if (strlen(std_reloc_name[basetype]) + 3 < 32) {
      strcpy(buf, std_reloc_name[basetype]);
      if (type & REL_MOD_S)
        strcat(buf, "_S");
      else if (type & REL_MOD_U)
        strcat(buf, "_U");
      return buf;
    }
  }
  return unknown;
}

void read_symbol(struct vobj_symbol *vsym) {
  vsym->offs = p - vobj;
  vsym->name = (const char *)p;
  skip_string();
  vsym->type = (int)read_number(0);
  vsym->flags = (int)read_number(0);
  vsym->sec = (int)read_number(0);
  vsym->val = read_number(1);
  vsym->size = (int)read_number(0);
}

void read_section(struct vobj_section *vsect, struct vobj_symbol *vsym,
                  int nsyms) {
  unsigned long flags;

  vsect->offs = p - vobj;
  vsect->name = (char *)p;
  skip_string();
  // const char* attr = (char *)p;
  skip_string();
  flags = (unsigned long)read_number(0);

  // print_sep();
  /* printf("%08llx: SECTION \"%s\" (attributes=\"%s\")", BPTMASK(vsect->offs),
         vsect->name, attr); */
  if (ver >= 3 && (flags & ABSOLUTE)) {
  }
  // printf(" @%llx\n", BPTMASK(read_number(0)));
  else {
  }
  // putchar('\n');

  // int align = (int)read_number(0);
  vsect->dsize = read_number(0);
  int nrelocs = (int)read_number(0);
  vsect->fsize = read_number(0);

  /*
  printf("Flags: %-8lx  Alignment: %-6d "
         "Total size: %-9" PRId64 " File size: %-9" PRId64 "\n",
         flags, align, vsect->dsize, vsect->fsize); */
  // if (nrelocs) {
  // }
  /*
  printf("%d Relocation%s present.\n", nrelocs,
         nrelocs == 1 ? emptystr : sstr); */

  p += vsect->fsize * opb; /* skip section contents */

  /* read and print relocations for this section */
  for (int i = 0; i < nrelocs; i++) {
    int type;
    size_t len;

    if (i == 0) {
      /* print header */
      // printf("\nfile offs sectoffs pos sz mask     type symbol+addend\n");
    }
    // printf("%08llx: ", BPTMASK(p - vobj));

    type = read_number(0);
    if (type >= FIRST_CPU_RELOC)
      len = read_number(0); /* length of spec. reloc entry in octets */
    else
      len = 0;

    if (type < FIRST_CPU_RELOC || len == 0) {
      /* we have either a standard nreloc or a cpu-specific nreloc */
      taddr offs, mask, addend;
      int bpos, bsiz, sym;

      if (type < FIRST_CPU_RELOC && STD_REL_TYPE(type) >= num_std_relocs) {
        fprintf(stderr, "vobjdump: Illegal standard reloc type: %d\n", type);
        exit(1);
      }
      offs = read_number(0);
      bpos = (int)read_number(0);
      bsiz = (int)read_number(0);
      mask = read_number(1);
      addend = read_number(1);
      sym = (int)read_number(0) - 1; /* symbol index */
      print_nreloc(type >= FIRST_CPU_RELOC ? cpu_reloc_name(type)
                                           : standard_reloc_name(type),
                   vsect, vsym, nsyms, offs, bpos, bsiz, mask, addend, sym);
    } else {
      /* special relocation in cpu-specific format */
      if (!print_cpureloc(type, p)) {
        char sname[32];

        sprintf(sname, "%.10s special reloc", cpu_name);
        /*
        printf("%-24s %d with a size of %u octets\n", sname, type,
               (unsigned)len); */
      }
      p += len;
      if (p < vobj || p > vobj + vlen)
        break;
    }
  }

  if (p < vobj || p > vobj + vlen) {
    fprintf(stderr, "\nvobjdump: Section \"%s\" is corrupt! Aborting.\n",
            vsect->name);
    exit(1);
  }
}

const char *bind_name(int flags) {
  if (flags & WEAK)
    return "WEAK";
  if (flags & EXPORT)
    return "GLOB";
  if (flags & COMMON)
    return "COMM";
  return "LOCL";
}

const char *def_name(struct vobj_symbol *vs, struct vobj_section *sec,
                     int nsecs) {
  switch (vs->type) {
  case EXPRESSION:
    if (vs->flags & SYMINDIR)
      return "*IND*";
    return "*ABS*";
  case IMPORT:
    return "*UND*";
  case LABSYM:
    if (vs->sec > 0 && vs->sec <= nsecs)
      return sec[vs->sec - 1].name;
  }
  return "???";
}

struct vobj_symbol *vobjdump(int *nsyms_) {
  p = vobj;

  if (!(vlen > 4 && p[0] == 0x56 && p[1] == 0x4f && p[2] == 0x42 &&
        p[3] == 0x4a)) {
    fprintf(stderr, "vobjdump: Not a VOBJ file!\n");
    return NULL;
  }

  int endian, nsecs, nsyms, i;
  struct vobj_symbol *vsymbols = NULL;
  struct vobj_section *vsect = NULL;

  p += 4;             /* skip ID */
  endian = (int)*p++; /* endianness and version */

  ver = (endian >> 2) + 1;
  if (ver > VOBJ_MAX_VERSION) {
    fprintf(stderr, "vobjdump: Version %d not supported!\n", ver);
    return NULL;
  }

  endian &= 3;
  if (endian < 1 || endian > 2) {
    fprintf(stderr, "vobjdump: Bad endianness: %d\n", endian);
    return NULL;
  }

  bpb = (int)read_number(0); /* bits per byte */
  if (bpb % 8) {
    fprintf(stderr, "vobjdump: %d bits per byte currently not supported!\n",
            bpb);
    return NULL;
  }
  opb = (bpb + 7) / 8;

  bpt = (int)read_number(0); /* bytes per taddr */
  bitspertaddr = bpb * bpt;  /* bits per taddr */
  if ((bitspertaddr + 7) / 8 > sizeof(taddr)) {
    fprintf(stderr, "vobjdump: %d bytes per taddr not supported!\n", bpt);
    return NULL;
  }
  bptmask = makemask(bitspertaddr);

  cpu_name = (char *)p;
  skip_string();               /* skip cpu-string */
  nsecs = (int)read_number(0); /* number of sections */
  nsyms = (int)read_number(0); /* number of symbols */

  /* read symbols */
  if (nsyms) {
    vsymbols = malloc(nsyms * sizeof(struct vobj_symbol));
    if (!vsymbols) {
      fprintf(stderr, "vobjdump: Cannot allocate %ld bytes for symbols!\n",
              (long)(nsyms * sizeof(struct vobj_symbol)));
      return NULL;
    }

    for (i = 0; i < nsyms; i++)
      read_symbol(&vsymbols[i]);
  }

  /* read and print sections */
  vsect = malloc(nsecs * sizeof(struct vobj_section));
  if (!vsect) {
    fprintf(stderr, "vobjdump: Cannot allocate %ld bytes for sections!\n",
            (long)(nsecs * sizeof(struct vobj_section)));
    return NULL;
  }

  for (i = 0; i < nsecs; i++)
    read_section(&vsect[i], vsymbols, nsyms);

  *nsyms_ = nsyms;
  return vsymbols;
}

size_t filesize(FILE *fp, const char *name) {
  long size;

  if (fgetc(fp) != EOF)
    if (fseek(fp, 0, SEEK_END) >= 0)
      if ((size = ftell(fp)) >= 0)
        if (fseek(fp, 0, SEEK_SET) >= 0)
          return (size_t)size;
  fprintf(stderr, "vobjdump: Cannot determine size of file \"%s\"!\n", name);
  return 0;
}
