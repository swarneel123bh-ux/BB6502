#include "fake6502.h"
#include <stdint.h>
#include <stdio.h>

uint32_t instructions = 0;
uint8_t callexternal = 0;
uint32_t clockticks6502 = 0, clockgoal6502 = 0;
uint8_t penaltyop, penaltyaddr;

// 6502 CPU registers
uint16_t pc;
uint8_t sp, a, x, y, status;

// helper variables
uint32_t instructions; // keep track of total instructions executed
uint32_t clockticks6502, clockgoal6502;
uint16_t oldpc, ea, reladdr, value, result;
uint8_t opcode, oldstatus;
uint8_t callexternal;
void (*loopexternal)(void);

// addressing mode functions, calculates effective addresses
static void imp(void);
static void acc(void);
static void imm(void);
static void zp(void);
static void zpx(void);
static void zpy(void);
static void rel(void);
static void abso(void);
static void absx(void);
static void absy(void);
static void ind(void);
static void indx(void);
static void indy(void);
static uint16_t getvalue(void);
// static uint16_t getvalue16();
static void putvalue(uint16_t saveval);

// instruction handler functions
static void adc(void);
static void and_(void);
static void asl(void);
static void bcc(void);
static void bcs(void);
static void beq(void);
static void bit(void);
static void bmi(void);
static void bne(void);
static void bpl(void);
static void brk_(void);
static void bvc(void);
static void bvs(void);
static void clc(void);
static void cld(void);
static void cli(void);
static void clv(void);
static void cmp(void);
static void cpx(void);
static void cpy(void);
static void dec(void);
static void dex(void);
static void dey(void);
static void eor(void);
static void inc(void);
static void inx(void);
static void iny(void);
static void jmp(void);
static void jsr(void);
static void lda(void);
static void ldx(void);
static void ldy(void);
static void lsr(void);
static void nop(void);
static void ora(void);
static void pha(void);
static void php(void);
static void pla(void);
static void plp(void);
static void rol(void);
static void ror(void);
static void rti(void);
static void rts(void);
static void sbc(void);
static void sec(void);
static void sed(void);
static void sei(void);
static void sta(void);
static void stx(void);
static void sty(void);
static void tax(void);
static void tay(void);
static void tsx(void);
static void txa(void);
static void txs(void);
static void tya(void);

// undocumented instructions
#ifdef UNDOCUMENTED
static void lax(void);
static void sax(void);
static void dcp(void);
static void isb(void);
static void slo(void);
static void rla(void);
static void sre(void);
static void rra(void);
#else
#define lax nop
#define sax nop
#define dcp nop
#define isb nop
#define slo nop
#define rla nop
#define sre nop
#define rra nop
#endif

static void (*addrtable[256])(void) = {
    imp,  indx, imp,  indx, zp,   zp,   zp,   zp,   imp,  imm,  acc,  imm,
    abso, abso, abso, abso, rel,  indy, imp,  indy, zpx,  zpx,  zpx,  zpx,
    imp,  absy, imp,  absy, absx, absx, absx, absx, abso, indx, imp,  indx,
    zp,   zp,   zp,   zp,   imp,  imm,  acc,  imm,  abso, abso, abso, abso,
    rel,  indy, imp,  indy, zpx,  zpx,  zpx,  zpx,  imp,  absy, imp,  absy,
    absx, absx, absx, absx, imp,  indx, imp,  indx, zp,   zp,   zp,   zp,
    imp,  imm,  acc,  imm,  abso, abso, abso, abso, rel,  indy, imp,  indy,
    zpx,  zpx,  zpx,  zpx,  imp,  absy, imp,  absy, absx, absx, absx, absx,
    imp,  indx, imp,  indx, zp,   zp,   zp,   zp,   imp,  imm,  acc,  imm,
    ind,  abso, abso, abso, rel,  indy, imp,  indy, zpx,  zpx,  zpx,  zpx,
    imp,  absy, imp,  absy, absx, absx, absx, absx, imm,  indx, imm,  indx,
    zp,   zp,   zp,   zp,   imp,  imm,  imp,  imm,  abso, abso, abso, abso,
    rel,  indy, imp,  indy, zpx,  zpx,  zpy,  zpy,  imp,  absy, imp,  absy,
    absx, absx, absy, absy, imm,  indx, imm,  indx, zp,   zp,   zp,   zp,
    imp,  imm,  imp,  imm,  abso, abso, abso, abso, rel,  indy, imp,  indy,
    zpx,  zpx,  zpy,  zpy,  imp,  absy, imp,  absy, absx, absx, absy, absy,
    imm,  indx, imm,  indx, zp,   zp,   zp,   zp,   imp,  imm,  imp,  imm,
    abso, abso, abso, abso, rel,  indy, imp,  indy, zpx,  zpx,  zpx,  zpx,
    imp,  absy, imp,  absy, absx, absx, absx, absx, imm,  indx, imm,  indx,
    zp,   zp,   zp,   zp,   imp,  imm,  imp,  imm,  abso, abso, abso, abso,
    rel,  indy, imp,  indy, zpx,  zpx,  zpx,  zpx,  imp,  absy, imp,  absy,
    absx, absx, absx, absx};

static void (*optable[256])(void) = {
    brk_, ora,  nop,  slo, nop, ora,  asl,  slo,  php, ora,  asl,  nop,  nop,
    ora,  asl,  slo,  bpl, ora, nop,  slo,  nop,  ora, asl,  slo,  clc,  ora,
    nop,  slo,  nop,  ora, asl, slo,  jsr,  and_, nop, rla,  bit,  and_, rol,
    rla,  plp,  and_, rol, nop, bit,  and_, rol,  rla, bmi,  and_, nop,  rla,
    nop,  and_, rol,  rla, sec, and_, nop,  rla,  nop, and_, rol,  rla,  rti,
    eor,  nop,  sre,  nop, eor, lsr,  sre,  pha,  eor, lsr,  nop,  jmp,  eor,
    lsr,  sre,  bvc,  eor, nop, sre,  nop,  eor,  lsr, sre,  cli,  eor,  nop,
    sre,  nop,  eor,  lsr, sre, rts,  adc,  nop,  rra, nop,  adc,  ror,  rra,
    pla,  adc,  ror,  nop, jmp, adc,  ror,  rra,  bvs, adc,  nop,  rra,  nop,
    adc,  ror,  rra,  sei, adc, nop,  rra,  nop,  adc, ror,  rra,  nop,  sta,
    nop,  sax,  sty,  sta, stx, sax,  dey,  nop,  txa, nop,  sty,  sta,  stx,
    sax,  bcc,  sta,  nop, nop, sty,  sta,  stx,  sax, tya,  sta,  txs,  nop,
    nop,  sta,  nop,  nop, ldy, lda,  ldx,  lax,  ldy, lda,  ldx,  lax,  tay,
    lda,  tax,  nop,  ldy, lda, ldx,  lax,  bcs,  lda, nop,  lax,  ldy,  lda,
    ldx,  lax,  clv,  lda, tsx, lax,  ldy,  lda,  ldx, lax,  cpy,  cmp,  nop,
    dcp,  cpy,  cmp,  dec, dcp, iny,  cmp,  dex,  nop, cpy,  cmp,  dec,  dcp,
    bne,  cmp,  nop,  dcp, nop, cmp,  dec,  dcp,  cld, cmp,  nop,  dcp,  nop,
    cmp,  dec,  dcp,  cpx, sbc, nop,  isb,  cpx,  sbc, inc,  isb,  inx,  sbc,
    nop,  sbc,  cpx,  sbc, inc, isb,  beq,  sbc,  nop, isb,  nop,  sbc,  inc,
    isb,  sed,  sbc,  nop, isb, nop,  sbc,  inc,  isb};

static const uint32_t ticktable[256] = {
    7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6, 2, 5, 2, 8, 4, 4, 6, 6,
    2, 4, 2, 7, 4, 4, 7, 7, 6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, 6, 6, 2, 8, 3, 3, 5, 5,
    3, 2, 2, 2, 3, 4, 6, 6, 2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
    6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6, 2, 5, 2, 8, 4, 4, 6, 6,
    2, 4, 2, 7, 4, 4, 7, 7, 2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
    2, 6, 2, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5, 2, 6, 2, 6, 3, 3, 3, 3,
    2, 2, 2, 2, 4, 4, 4, 4, 2, 5, 2, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
    2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, 2, 5, 2, 8, 4, 4, 6, 6,
    2, 4, 2, 7, 4, 4, 7, 7, 2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7};

// a few general functions used by various other functions
void push16(uint16_t pushval) {
  write6502(BASE_STACK + sp, (pushval >> 8) & 0xFF);
  write6502(BASE_STACK + ((sp - 1) & 0xFF), pushval & 0xFF);
  sp -= 2;
}

void push8(uint8_t pushval) { write6502(BASE_STACK + sp--, pushval); }

uint16_t pull16(void) {
  uint16_t temp16;
  temp16 = read6502(BASE_STACK + ((sp + 1) & 0xFF)) |
           ((uint16_t)read6502(BASE_STACK + ((sp + 2) & 0xFF)) << 8);
  sp += 2;
  return (temp16);
}

uint8_t pull8(void) { return (read6502(BASE_STACK + ++sp)); }

void reset6502(void) {
  pc = (uint16_t)read6502(0xFFFC) | ((uint16_t)read6502(0xFFFD) << 8);
  a = 0;
  x = 0;
  y = 0;
  sp = 0xFD;
  status |= FLAG_CONSTANT;
}

// addressing mode functions, calculates effective addresses
static void imp(void) { // implied
}

static void acc(void) { // accumulator
}

static void imm(void) { // immediate
  ea = pc++;
}

static void zp(void) { // zero-page
  ea = (uint16_t)read6502((uint16_t)pc++);
}

static void zpx(void) { // zero-page,X
  ea = ((uint16_t)read6502((uint16_t)pc++) + (uint16_t)x) &
       0xFF; // zero-page wraparound
}

static void zpy(void) { // zero-page,Y
  ea = ((uint16_t)read6502((uint16_t)pc++) + (uint16_t)y) &
       0xFF; // zero-page wraparound
}

static void rel(void) { // relative for branch ops (8-bit immediate value, sign-extended)
  reladdr = (uint16_t)read6502(pc++);
  if (reladdr & 0x80)
    reladdr |= 0xFF00;
}

static void abso(void) { // absolute
  ea = (uint16_t)read6502(pc) | ((uint16_t)read6502(pc + 1) << 8);
  pc += 2;
}

static void absx(void) { // absolute,X
  uint16_t startpage;
  ea = ((uint16_t)read6502(pc) | ((uint16_t)read6502(pc + 1) << 8));
  startpage = ea & 0xFF00;
  ea += (uint16_t)x;

  if (startpage !=
      (ea & 0xFF00)) { // one cycle penlty for page-crossing on some opcodes
    penaltyaddr = 1;
  }

  pc += 2;
}

static void absy(void) { // absolute,Y
  uint16_t startpage;
  ea = ((uint16_t)read6502(pc) | ((uint16_t)read6502(pc + 1) << 8));
  startpage = ea & 0xFF00;
  ea += (uint16_t)y;

  if (startpage !=
      (ea & 0xFF00)) { // one cycle penlty for page-crossing on some opcodes
    penaltyaddr = 1;
  }

  pc += 2;
}

static void ind(void) { // indirect
  uint16_t eahelp, eahelp2;
  eahelp = (uint16_t)read6502(pc) | (uint16_t)((uint16_t)read6502(pc + 1) << 8);
  eahelp2 =
      (eahelp & 0xFF00) |
      ((eahelp + 1) & 0x00FF); // replicate 6502 page-boundary wraparound bug
  ea = (uint16_t)read6502(eahelp) | ((uint16_t)read6502(eahelp2) << 8);
  pc += 2;
}

static void indx(void) { // (indirect,X)
  uint16_t eahelp;
  eahelp = (uint16_t)(((uint16_t)read6502(pc++) + (uint16_t)x) &
                      0xFF); // zero-page wraparound for table pointer
  ea = (uint16_t)read6502(eahelp & 0x00FF) |
       ((uint16_t)read6502((eahelp + 1) & 0x00FF) << 8);
}

static void indy(void) { // (indirect),Y
  uint16_t eahelp, eahelp2, startpage;
  eahelp = (uint16_t)read6502(pc++);
  eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF); // zero-page wraparound
  ea = (uint16_t)read6502(eahelp) | ((uint16_t)read6502(eahelp2) << 8);
  startpage = ea & 0xFF00;
  ea += (uint16_t)y;

  if (startpage !=
      (ea & 0xFF00)) { // one cycle penlty for page-crossing on some opcodes
    penaltyaddr = 1;
  }
}

static uint16_t getvalue(void) {
  if (addrtable[opcode] == acc)
    return ((uint16_t)a);
  else
    return ((uint16_t)read6502(ea));
}

/*
static uint16_t getvalue16(void) {
    return((uint16_t)read6502(ea) | ((uint16_t)read6502(ea+1) << 8));
} */

static void putvalue(uint16_t saveval) {
  if (addrtable[opcode] == acc)
    a = (uint8_t)(saveval & 0x00FF);
  else
    write6502(ea, (saveval & 0x00FF));
}

// instruction hand_ler functions
static void adc(void) {
  penaltyop = 1;
  value = getvalue();
  result = (uint16_t)a + value + (uint16_t)(status & FLAG_CARRY);

  carrycalc(result);
  zerocalc(result);
  overflowcalc(result, a, value);
  signcalc(result);

#ifndef NES_CPU
  if (status & FLAG_DECIMAL) {
    clearcarry();

    if ((a & 0x0F) > 0x09) {
      a += 0x06;
    }
    if ((a & 0xF0) > 0x90) {
      a += 0x60;
      setcarry();
    }

    clockticks6502++;
  }
#endif

  saveaccum(result);
}

static void and_(void) {
  penaltyop = 1;
  value = getvalue();
  result = (uint16_t)a & value;

  zerocalc(result);
  signcalc(result);

  saveaccum(result);
}

static void asl(void) {
  value = getvalue();
  result = value << 1;

  carrycalc(result);
  zerocalc(result);
  signcalc(result);

  putvalue(result);
}

static void bcc(void) {
  if ((status & FLAG_CARRY) == 0) {
    oldpc = pc;
    pc += reladdr;
    if ((oldpc & 0xFF00) != (pc & 0xFF00))
      clockticks6502 += 2; // check if jump crossed a page boundary
    else
      clockticks6502++;
  }
}

static void bcs(void) {
  if ((status & FLAG_CARRY) == FLAG_CARRY) {
    oldpc = pc;
    pc += reladdr;
    if ((oldpc & 0xFF00) != (pc & 0xFF00))
      clockticks6502 += 2; // check if jump crossed a page boundary
    else
      clockticks6502++;
  }
}

static void beq(void) {
  if ((status & FLAG_ZERO) == FLAG_ZERO) {
    oldpc = pc;
    pc += reladdr;
    if ((oldpc & 0xFF00) != (pc & 0xFF00))
      clockticks6502 += 2; // check if jump crossed a page boundary
    else
      clockticks6502++;
  }
}

static void bit(void) {
  value = getvalue();
  result = (uint16_t)a & value;

  zerocalc(result);
  status = (status & 0x3F) | (uint8_t)(value & 0xC0);
}

static void bmi(void) {
  if ((status & FLAG_SIGN) == FLAG_SIGN) {
    oldpc = pc;
    pc += reladdr;
    if ((oldpc & 0xFF00) != (pc & 0xFF00))
      clockticks6502 += 2; // check if jump crossed a page boundary
    else
      clockticks6502++;
  }
}

static void bne(void) {
  if ((status & FLAG_ZERO) == 0) {
    oldpc = pc;
    pc += reladdr;
    if ((oldpc & 0xFF00) != (pc & 0xFF00))
      clockticks6502 += 2; // check if jump crossed a page boundary
    else
      clockticks6502++;
  }
}

static void bpl(void) {
  if ((status & FLAG_SIGN) == 0) {
    oldpc = pc;
    pc += reladdr;
    if ((oldpc & 0xFF00) != (pc & 0xFF00))
      clockticks6502 += 2; // check if jump crossed a page boundary
    else
      clockticks6502++;
  }
}

static void brk_(void) {
  pc++;
  push16(pc);                 // push next instruction address onto stack
  push8(status | FLAG_BREAK); // push CPU status to stack
  setinterrupt();             // set interrupt flag
  pc = (uint16_t)read6502(0xFFFE) | ((uint16_t)read6502(0xFFFF) << 8);
}

static void bvc(void) {
  if ((status & FLAG_OVERFLOW) == 0) {
    oldpc = pc;
    pc += reladdr;
    if ((oldpc & 0xFF00) != (pc & 0xFF00))
      clockticks6502 += 2; // check if jump crossed a page boundary
    else
      clockticks6502++;
  }
}

static void bvs(void) {
  if ((status & FLAG_OVERFLOW) == FLAG_OVERFLOW) {
    oldpc = pc;
    pc += reladdr;
    if ((oldpc & 0xFF00) != (pc & 0xFF00))
      clockticks6502 += 2; // check if jump crossed a page boundary
    else
      clockticks6502++;
  }
}

static void clc(void) { clearcarry(); }

static void cld(void) { cleardecimal(); }

static void cli(void) { clearinterrupt(); }

static void clv(void) { clearoverflow(); }

static void cmp(void) {
  penaltyop = 1;
  value = getvalue();
  result = (uint16_t)a - value;

  if (a >= (uint8_t)(value & 0x00FF))
    setcarry();
  else
    clearcarry();

  if (a == (uint8_t)(value & 0x00FF))
    setzero();
  else
    clearzero();

  signcalc(result);
}

static void cpx(void) {
  value = getvalue();
  result = (uint16_t)x - value;

  if (x >= (uint8_t)(value & 0x00FF))
    setcarry();
  else
    clearcarry();

  if (x == (uint8_t)(value & 0x00FF))
    setzero();
  else
    clearzero();

  signcalc(result);
}

static void cpy(void) {
  value = getvalue();
  result = (uint16_t)y - value;

  if (y >= (uint8_t)(value & 0x00FF))
    setcarry();
  else
    clearcarry();

  if (y == (uint8_t)(value & 0x00FF))
    setzero();
  else
    clearzero();

  signcalc(result);
}

static void dec(void) {
  value = getvalue();
  result = value - 1;

  zerocalc(result);
  signcalc(result);

  putvalue(result);
}

static void dex(void) {
  x--;

  zerocalc(x);
  signcalc(x);
}

static void dey(void) {
  y--;

  zerocalc(y);
  signcalc(y);
}

static void eor(void) {
  penaltyop = 1;
  value = getvalue();
  result = (uint16_t)a ^ value;

  zerocalc(result);
  signcalc(result);

  saveaccum(result);
}

static void inc(void) {
  value = getvalue();
  result = value + 1;

  zerocalc(result);
  signcalc(result);

  putvalue(result);
}

static void inx(void) {
  x++;

  zerocalc(x);
  signcalc(x);
}

static void iny(void) {
  y++;

  zerocalc(y);
  signcalc(y);
}

static void jmp(void) { pc = ea; }

static void jsr(void) {
  push16(pc - 1);
  pc = ea;
}

static void lda(void) {
  penaltyop = 1;
  value = getvalue();
  a = (uint8_t)(value & 0x00FF);

  zerocalc(a);
  signcalc(a);
}

static void ldx(void) {
  penaltyop = 1;
  value = getvalue();
  x = (uint8_t)(value & 0x00FF);

  zerocalc(x);
  signcalc(x);
}

static void ldy(void) {
  penaltyop = 1;
  value = getvalue();
  y = (uint8_t)(value & 0x00FF);

  zerocalc(y);
  signcalc(y);
}

static void lsr(void) {
  value = getvalue();
  result = value >> 1;

  if (value & 1)
    setcarry();
  else
    clearcarry();

  zerocalc(result);
  signcalc(result);

  putvalue(result);
}

static void nop(void) {
  switch (opcode) {
  case 0x1C:
  case 0x3C:
  case 0x5C:
  case 0x7C:
  case 0xDC:
  case 0xFC:
    penaltyop = 1;
    break;
  }
}

static void ora(void) {
  penaltyop = 1;
  value = getvalue();
  result = (uint16_t)a | value;

  zerocalc(result);
  signcalc(result);

  saveaccum(result);
}

static void pha(void) { push8(a); }

static void php(void) { push8(status | FLAG_BREAK); }

static void pla(void) {
  a = pull8();

  zerocalc(a);
  signcalc(a);
}

static void plp(void) { status = pull8() | FLAG_CONSTANT; }

static void rol(void) {
  value = getvalue();
  result = (value << 1) | (status & FLAG_CARRY);

  carrycalc(result);
  zerocalc(result);
  signcalc(result);

  putvalue(result);
}

static void ror(void) {
  value = getvalue();
  result = (value >> 1) | ((status & FLAG_CARRY) << 7);

  if (value & 1)
    setcarry();
  else
    clearcarry();

  zerocalc(result);
  signcalc(result);

  putvalue(result);
}

static void rti(void) {
  status = pull8();
  value = pull16();
  pc = value;
}

static void rts(void) {
  value = pull16();
  pc = value + 1;
}

static void sbc(void) {
  penaltyop = 1;
  value = getvalue() ^ 0x00FF;
  result = (uint16_t)a + value + (uint16_t)(status & FLAG_CARRY);

  carrycalc(result);
  zerocalc(result);
  overflowcalc(result, a, value);
  signcalc(result);

#ifndef NES_CPU
  if (status & FLAG_DECIMAL) {
    clearcarry();

    a -= 0x66;
    if ((a & 0x0F) > 0x09) {
      a += 0x06;
    }
    if ((a & 0xF0) > 0x90) {
      a += 0x60;
      setcarry();
    }

    clockticks6502++;
  }
#endif

  saveaccum(result);
}

static void sec(void) { setcarry(); }

static void sed(void) { setdecimal(); }

static void sei(void) { setinterrupt(); }

static void sta(void) { putvalue(a); }

static void stx(void) { putvalue(x); }

static void sty(void) { putvalue(y); }

static void tax(void) {
  x = a;

  zerocalc(x);
  signcalc(x);
}

static void tay(void) {
  y = a;

  zerocalc(y);
  signcalc(y);
}

static void tsx(void) {
  x = sp;

  zerocalc(x);
  signcalc(x);
}

static void txa(void) {
  a = x;

  zerocalc(a);
  signcalc(a);
}

static void txs(void) { sp = x; }

static void tya(void) {
  a = y;

  zerocalc(a);
  signcalc(a);
}

// undocumented instructions
#ifdef UNDOCUMENTED
static void lax(void) {
  lda();
  ldx();
}

static void sax(void) {
  sta();
  stx();
  putvalue(a & x);
  if (penaltyop && penaltyaddr)
    clockticks6502--;
}

static void dcp(void) {
  dec();
  cmp();
  if (penaltyop && penaltyaddr)
    clockticks6502--;
}

static void isb(void) {
  inc();
  sbc();
  if (penaltyop && penaltyaddr)
    clockticks6502--;
}

static void slo(void) {
  asl();
  ora();
  if (penaltyop && penaltyaddr)
    clockticks6502--;
}

static void rla(void) {
  rol();
  and_();
  if (penaltyop && penaltyaddr)
    clockticks6502--;
}

static void sre(void) {
  lsr();
  eor();
  if (penaltyop && penaltyaddr)
    clockticks6502--;
}

static void rra(void) {
  ror();
  adc();
  if (penaltyop && penaltyaddr)
    clockticks6502--;
}
#else
#define lax nop
#define sax nop
#define dcp nop
#define isb nop
#define slo nop
#define rla nop
#define sre nop
#define rra nop
#endif

void nmi6502(void) {
  push16(pc);
  push8(status);
  status |= FLAG_INTERRUPT;
  pc = (uint16_t)read6502(0xFFFA) | ((uint16_t)read6502(0xFFFB) << 8);
}

void irq6502(void) {
  push16(pc);
  push8(status);
  status |= FLAG_INTERRUPT;
  pc = (uint16_t)read6502(0xFFFE) | ((uint16_t)read6502(0xFFFF) << 8);
}

void exec6502(uint32_t tickcount) {
  clockgoal6502 += tickcount;

  while (clockticks6502 < clockgoal6502) {
    opcode = read6502(pc++);
    status |= FLAG_CONSTANT;

    penaltyop = 0;
    penaltyaddr = 0;

    (*addrtable[opcode])();
    (*optable[opcode])();
    clockticks6502 += ticktable[opcode];
    if (penaltyop && penaltyaddr)
      clockticks6502++;

    instructions++;

    if (callexternal)
      (*loopexternal)();
  }
}

void step6502(void) {
  opcode = read6502(pc++);
  status |= FLAG_CONSTANT;

  penaltyop = 0;
  penaltyaddr = 0;

  (*addrtable[opcode])();
  (*optable[opcode])();
  clockticks6502 += ticktable[opcode];
  if (penaltyop && penaltyaddr)
    clockticks6502++;
  clockgoal6502 = clockticks6502;

  instructions++;

  if (callexternal)
    (*loopexternal)();
}

void hookexternal(void *funcptr) {
  if (funcptr != (void *)NULL) {
    union {
      void *data;
      void (*func)(void);
    } convertor;
    convertor.data = funcptr;
    loopexternal = convertor.func;
    callexternal = 1;
  } else
    callexternal = 0;
}
