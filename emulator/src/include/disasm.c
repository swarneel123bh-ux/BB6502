#include "disasm.h"

/* ============================
   Disassembler
   ============================ */
int disassemble_6502(uint16_t pc, uint8_t (*read)(uint16_t), char *out) {

    uint8_t opcode = read(pc);
    const Opcode *op = &optable[opcode];

    uint8_t b1 = read(pc + 1);
    uint8_t b2 = read(pc + 2);
    uint16_t addr = (uint16_t)(b1 | (b2 << 8));

    switch (op->mode) {
        case AM_IMP:  sprintf(out, "%s", op->mnemonic); break;
        case AM_ACC:  sprintf(out, "%s A", op->mnemonic); break;
        case AM_IMM:  sprintf(out, "%s #$%02X", op->mnemonic, b1); break;
        case AM_ZP:   sprintf(out, "%s $%02X", op->mnemonic, b1); break;
        case AM_ZPX:  sprintf(out, "%s $%02X,X", op->mnemonic, b1); break;
        case AM_ZPY:  sprintf(out, "%s $%02X,Y", op->mnemonic, b1); break;
        case AM_ABS:  sprintf(out, "%s $%04X", op->mnemonic, addr); break;
        case AM_ABSX: sprintf(out, "%s $%04X,X", op->mnemonic, addr); break;
        case AM_ABSY: sprintf(out, "%s $%04X,Y", op->mnemonic, addr); break;
        case AM_IND:  sprintf(out, "%s ($%04X)", op->mnemonic, addr); break;
        case AM_INDX: sprintf(out, "%s ($%02X,X)", op->mnemonic, b1); break;
        case AM_INDY: sprintf(out, "%s ($%02X),Y", op->mnemonic, b1); break;
        case AM_REL: {
            int8_t off = (int8_t)b1;
            uint16_t target = pc + 2 + off;
            sprintf(out, "%s $%04X", op->mnemonic, target);
            break;
        }
    }

    return op->bytes;
}
