
enum Instruct {
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
    int instruc;
    int op;
    int rs, rt,rd;
    int funct;
    int C;

    void getinstruc(unsigned int );
};
