#pragma once

#include <stdint.h>
#include <stdio.h>


// Addressing Modes
typedef enum {
    AM_IMP,   // Implied
    AM_ACC,   // Accumulator
    AM_IMM,   // #$nn
    AM_ZP,    // $nn
    AM_ZPX,   // $nn,X
    AM_ZPY,   // $nn,Y
    AM_ABS,   // $nnnn
    AM_ABSX,  // $nnnn,X
    AM_ABSY,  // $nnnn,Y
    AM_IND,   // ($nnnn)
    AM_INDX,  // ($nn,X)
    AM_INDY,  // ($nn),Y
    AM_REL    // branch
} AddrMode;


// Opcode Descriptor
typedef struct {
    const char *mnemonic;
    AddrMode mode;
    uint8_t bytes;
} Opcode;


// Opcode Table
static const Opcode optable[256] = {
/* 00 */ {"BRK",AM_IMP,1},{"ORA",AM_INDX,2},{"???",AM_IMP,1},{"???",AM_IMP,1},
/* 04 */ {"NOP",AM_ZP,2},{"ORA",AM_ZP,2},{"ASL",AM_ZP,2},{"???",AM_IMP,1},
/* 08 */ {"PHP",AM_IMP,1},{"ORA",AM_IMM,2},{"ASL",AM_ACC,1},{"???",AM_IMP,1},
/* 0C */ {"NOP",AM_ABS,3},{"ORA",AM_ABS,3},{"ASL",AM_ABS,3},{"???",AM_IMP,1},

/* 10 */ {"BPL",AM_REL,2},{"ORA",AM_INDY,2},{"???",AM_IMP,1},{"???",AM_IMP,1},
/* 14 */ {"NOP",AM_ZPX,2},{"ORA",AM_ZPX,2},{"ASL",AM_ZPX,2},{"???",AM_IMP,1},
/* 18 */ {"CLC",AM_IMP,1},{"ORA",AM_ABSY,3},{"NOP",AM_IMP,1},{"???",AM_IMP,1},
/* 1C */ {"NOP",AM_ABSX,3},{"ORA",AM_ABSX,3},{"ASL",AM_ABSX,3},{"???",AM_IMP,1},

/* 20 */ {"JSR",AM_ABS,3},{"AND",AM_INDX,2},{"???",AM_IMP,1},{"???",AM_IMP,1},
/* 24 */ {"BIT",AM_ZP,2},{"AND",AM_ZP,2},{"ROL",AM_ZP,2},{"???",AM_IMP,1},
/* 28 */ {"PLP",AM_IMP,1},{"AND",AM_IMM,2},{"ROL",AM_ACC,1},{"???",AM_IMP,1},
/* 2C */ {"BIT",AM_ABS,3},{"AND",AM_ABS,3},{"ROL",AM_ABS,3},{"???",AM_IMP,1},

/* 30 */ {"BMI",AM_REL,2},{"AND",AM_INDY,2},{"???",AM_IMP,1},{"???",AM_IMP,1},
/* 34 */ {"NOP",AM_ZPX,2},{"AND",AM_ZPX,2},{"ROL",AM_ZPX,2},{"???",AM_IMP,1},
/* 38 */ {"SEC",AM_IMP,1},{"AND",AM_ABSY,3},{"NOP",AM_IMP,1},{"???",AM_IMP,1},
/* 3C */ {"NOP",AM_ABSX,3},{"AND",AM_ABSX,3},{"ROL",AM_ABSX,3},{"???",AM_IMP,1},

/* 40 */ {"RTI",AM_IMP,1},{"EOR",AM_INDX,2},{"???",AM_IMP,1},{"???",AM_IMP,1},
/* 44 */ {"NOP",AM_ZP,2},{"EOR",AM_ZP,2},{"LSR",AM_ZP,2},{"???",AM_IMP,1},
/* 48 */ {"PHA",AM_IMP,1},{"EOR",AM_IMM,2},{"LSR",AM_ACC,1},{"???",AM_IMP,1},
/* 4C */ {"JMP",AM_ABS,3},{"EOR",AM_ABS,3},{"LSR",AM_ABS,3},{"???",AM_IMP,1},

/* 50 */ {"BVC",AM_REL,2},{"EOR",AM_INDY,2},{"???",AM_IMP,1},{"???",AM_IMP,1},
/* 54 */ {"NOP",AM_ZPX,2},{"EOR",AM_ZPX,2},{"LSR",AM_ZPX,2},{"???",AM_IMP,1},
/* 58 */ {"CLI",AM_IMP,1},{"EOR",AM_ABSY,3},{"NOP",AM_IMP,1},{"???",AM_IMP,1},
/* 5C */ {"NOP",AM_ABSX,3},{"EOR",AM_ABSX,3},{"LSR",AM_ABSX,3},{"???",AM_IMP,1},

/* 60 */ {"RTS",AM_IMP,1},{"ADC",AM_INDX,2},{"???",AM_IMP,1},{"???",AM_IMP,1},
/* 64 */ {"NOP",AM_ZP,2},{"ADC",AM_ZP,2},{"ROR",AM_ZP,2},{"???",AM_IMP,1},
/* 68 */ {"PLA",AM_IMP,1},{"ADC",AM_IMM,2},{"ROR",AM_ACC,1},{"???",AM_IMP,1},
/* 6C */ {"JMP",AM_IND,3},{"ADC",AM_ABS,3},{"ROR",AM_ABS,3},{"???",AM_IMP,1},

/* 70 */ {"BVS",AM_REL,2},{"ADC",AM_INDY,2},{"???",AM_IMP,1},{"???",AM_IMP,1},
/* 74 */ {"NOP",AM_ZPX,2},{"ADC",AM_ZPX,2},{"ROR",AM_ZPX,2},{"???",AM_IMP,1},
/* 78 */ {"SEI",AM_IMP,1},{"ADC",AM_ABSY,3},{"NOP",AM_IMP,1},{"???",AM_IMP,1},
/* 7C */ {"NOP",AM_ABSX,3},{"ADC",AM_ABSX,3},{"ROR",AM_ABSX,3},{"???",AM_IMP,1},

/* 80 */ {"NOP",AM_IMM,2},{"STA",AM_INDX,2},{"NOP",AM_IMP,1},{"???",AM_IMP,1},
/* 84 */ {"STY",AM_ZP,2},{"STA",AM_ZP,2},{"STX",AM_ZP,2},{"???",AM_IMP,1},
/* 88 */ {"DEY",AM_IMP,1},{"NOP",AM_IMM,2},{"TXA",AM_IMP,1},{"???",AM_IMP,1},
/* 8C */ {"STY",AM_ABS,3},{"STA",AM_ABS,3},{"STX",AM_ABS,3},{"???",AM_IMP,1},

/* 90 */ {"BCC",AM_REL,2},{"STA",AM_INDY,2},{"???",AM_IMP,1},{"???",AM_IMP,1},
/* 94 */ {"STY",AM_ZPX,2},{"STA",AM_ZPX,2},{"STX",AM_ZPY,2},{"???",AM_IMP,1},
/* 98 */ {"TYA",AM_IMP,1},{"STA",AM_ABSY,3},{"TXS",AM_IMP,1},{"???",AM_IMP,1},
/* 9C */ {"NOP",AM_ABS,3},{"STA",AM_ABSX,3},{"NOP",AM_IMP,1},{"???",AM_IMP,1},

/* A0 */ {"LDY",AM_IMM,2},{"LDA",AM_INDX,2},{"LDX",AM_IMM,2},{"???",AM_IMP,1},
/* A4 */ {"LDY",AM_ZP,2},{"LDA",AM_ZP,2},{"LDX",AM_ZP,2},{"???",AM_IMP,1},
/* A8 */ {"TAY",AM_IMP,1},{"LDA",AM_IMM,2},{"TAX",AM_IMP,1},{"???",AM_IMP,1},
/* AC */ {"LDY",AM_ABS,3},{"LDA",AM_ABS,3},{"LDX",AM_ABS,3},{"???",AM_IMP,1},

/* B0 */ {"BCS",AM_REL,2},{"LDA",AM_INDY,2},{"???",AM_IMP,1},{"???",AM_IMP,1},
/* B4 */ {"LDY",AM_ZPX,2},{"LDA",AM_ZPX,2},{"LDX",AM_ZPY,2},{"???",AM_IMP,1},
/* B8 */ {"CLV",AM_IMP,1},{"LDA",AM_ABSY,3},{"TSX",AM_IMP,1},{"???",AM_IMP,1},
/* BC */ {"LDY",AM_ABSX,3},{"LDA",AM_ABSX,3},{"LDX",AM_ABSY,3},{"???",AM_IMP,1},

/* C0 */ {"CPY",AM_IMM,2},{"CMP",AM_INDX,2},{"NOP",AM_IMP,1},{"???",AM_IMP,1},
/* C4 */ {"CPY",AM_ZP,2},{"CMP",AM_ZP,2},{"DEC",AM_ZP,2},{"???",AM_IMP,1},
/* C8 */ {"INY",AM_IMP,1},{"CMP",AM_IMM,2},{"DEX",AM_IMP,1},{"???",AM_IMP,1},
/* CC */ {"CPY",AM_ABS,3},{"CMP",AM_ABS,3},{"DEC",AM_ABS,3},{"???",AM_IMP,1},

/* D0 */ {"BNE",AM_REL,2},{"CMP",AM_INDY,2},{"???",AM_IMP,1},{"???",AM_IMP,1},
/* D4 */ {"NOP",AM_ZPX,2},{"CMP",AM_ZPX,2},{"DEC",AM_ZPX,2},{"???",AM_IMP,1},
/* D8 */ {"CLD",AM_IMP,1},{"CMP",AM_ABSY,3},{"NOP",AM_IMP,1},{"???",AM_IMP,1},
/* DC */ {"NOP",AM_ABSX,3},{"CMP",AM_ABSX,3},{"DEC",AM_ABSX,3},{"???",AM_IMP,1},

/* E0 */ {"CPX",AM_IMM,2},{"SBC",AM_INDX,2},{"NOP",AM_IMP,1},{"???",AM_IMP,1},
/* E4 */ {"CPX",AM_ZP,2},{"SBC",AM_ZP,2},{"INC",AM_ZP,2},{"???",AM_IMP,1},
/* E8 */ {"INX",AM_IMP,1},{"SBC",AM_IMM,2},{"NOP",AM_IMP,1},{"???",AM_IMP,1},
/* EC */ {"CPX",AM_ABS,3},{"SBC",AM_ABS,3},{"INC",AM_ABS,3},{"???",AM_IMP,1},

/* F0 */ {"BEQ",AM_REL,2},{"SBC",AM_INDY,2},{"???",AM_IMP,1},{"???",AM_IMP,1},
/* F4 */ {"NOP",AM_ZPX,2},{"SBC",AM_ZPX,2},{"INC",AM_ZPX,2},{"???",AM_IMP,1},
/* F8 */ {"SED",AM_IMP,1},{"SBC",AM_ABSY,3},{"NOP",AM_IMP,1},{"???",AM_IMP,1},
/* FC */ {"NOP",AM_ABSX,3},{"SBC",AM_ABSX,3},{"INC",AM_ABSX,3},{"???",AM_IMP,1},
};

// Actual disassembling function
int disassemble_6502( uint16_t pc, uint8_t (*read)(uint16_t), char *out);
