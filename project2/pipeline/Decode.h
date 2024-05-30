#include <iostream>

enum Instruct {
    NOP,
    add, addu, sub,
    And, Or, Xor, nor, nand,
    slt,
    sll, srl, sra,
    jr,
    addi, addiu,
    lw, lh, lhu, lb, lbu,
    sw, sh, sb,
    lui,
    andi, ori, nori,
    slti, beq, bne, bgtz,
    j, jal,
    halt
};


class Decode     /// for analyze instructions
{
public:
    unsigned int instruc;
    unsigned int rs, rt, rd;
    unsigned int C;
    int dest;   ///the register no. for write back stage, -1 if none
    int dt; ///1 -> rd, 0->rt, -1->none
    int except;

    Decode();

    void getinstruc(unsigned int );

};
