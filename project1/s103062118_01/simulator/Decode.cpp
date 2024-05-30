#include <math.h>
#include "Decode.h"

using namespace std;

void Decode::getinstruc(unsigned int ins){
    op = ins >> 26 & 63;
    rs = -1;
    rt = -1;
    rd = -1;
    C = -1;

    if(op == 0x00){                 ///R_type
        funct = ins & 63;
        if(funct == 0x08){
            rs = ins >> 21 & 31;
            instruc = jr;
        }else{
            rt = ins >> 16 & 31;
            rd = ins >> 11 & 31;
            if(funct < 0x04){
                C = ins >> 6 & 31;
                if(funct == 0x00) instruc = sll;
                else if(funct == 0x02) instruc = srl;
                else if(funct == 0x03) instruc = sra;
            }else{
                rs = ins >> 21 & 31;
                if(funct == 0x20) instruc = add;
                else if(funct == 0x21) instruc = addu;
                else if(funct == 0x22) instruc = sub;
                else if(funct == 0x24) instruc = And;
                else if(funct == 0x25) instruc = Or;
                else if(funct == 0x26) instruc = Xor;
                else if(funct == 0x27) instruc = nor;
                else if(funct == 0x28) instruc = nand;
                else if(funct == 0x2A) instruc = slt;
            }
        }
    }else if(op <= 0x03){                   ///J_type
        C = ins & ( (int)pow(2,26)-1 );
        if(op == 0x02) instruc = j;
        else if(op == 0x03) instruc = jal;
    }else if(op == 0x3F) instruc = halt;    ///halt
     else{                                  ///I_type
        C = ins & ( (int)pow(2,16)-1 );
        if(op==0x0F){
            rt = ins >> 16 & 31;
            instruc = lui;
        }else if(op==0x07){
            rs = ins >> 21 & 31;
            instruc = bgtz;
        }else{
            rs = ins >> 21 & 31;
            rt = ins >> 16 & 31;
            if(op == 0x08) instruc = addi;
            else if(op == 0x09) instruc = addiu;
            else if(op == 0x23) instruc = lw;
            else if(op == 0x21) instruc = lh;
            else if(op == 0x25) instruc = lhu;
            else if(op == 0x20) instruc = lb;
            else if(op == 0x24) instruc = lbu;
            else if(op == 0x2B) instruc = sw;
            else if(op == 0x29) instruc = sh;
            else if(op == 0x0C) instruc = andi;
            else if(op == 0x0D) instruc = ori;
            else if(op == 0x0E) instruc = nori;
            else if(op == 0x0A) instruc = slti;
            else if(op == 0x04) instruc = beq;
            else if(op == 0x05) instruc = bne;
        }
     }
}
