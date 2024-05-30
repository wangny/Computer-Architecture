#include <iostream>
#include <stdio.h>
#include "readfile.h"
#include "Decode.h"

using namespace std;

unsigned int inPC,PC;
unsigned int iimage[256] = {0};
unsigned int SP;
unsigned int dimage[256] = {0};
unsigned int regis[32] = {0};


class Error{
private:
    FILE *file;
    bool exit;
public:
    Error(){
        file = fopen("error_dump.rpt","w");
        exit = false;
    }
    ~Error(){fclose(file);}

    inline void write0 (int cycle){
        fprintf(file,"In cycle %d: Write $0 Error\n",cycle+1);
    }
    inline void addrover (int cycle){
        fprintf(file,"In cycle %d: Address Overflow\n",cycle+1);
        exit = true;
    }
    inline void overflow (int cycle){
        fprintf(file,"In cycle %d: Number Overflow\n",cycle+1);
    }
    inline void misalign (int cycle){
        fprintf(file,"In cycle %d: Misalignment Error\n",cycle+1);
        exit = true;
    }
    inline bool Exit(){return exit;}
};


Error error;
int cycle = 0;

inline void output(FILE *f, int cycle ){       ///output snapshot.rpt
    regis[0] = 0;

    fprintf(f,"cycle %d\n",cycle);
    for(int i=0; i<32; i++){
        fprintf(f,"$%02d: 0x%08X\n",i ,regis[i]);
    }
    fprintf(f,"PC: 0x%08X\n\n\n",PC);
}

inline unsigned int sign_extend(int c, int length){
    if(c>>(length-1)==1 ){
        int tmp = 0xffffffff<<length;
        unsigned int t = c | tmp;
        return t;
    }else return (unsigned int)c;
}

inline unsigned int getmemaddr(unsigned int r, int c){   ///compute address and handle number overflow

    unsigned int uc = sign_extend(c,16);
    unsigned int sign;
    if(uc>>31 == r>>31) sign = r>>31;
    unsigned int addr = uc + r;
    if(sign<=1) if(sign != addr>>31 ) error.overflow(cycle);

    return addr;
}


int main()
{
    Readfile input;
    input.readinput(inPC, iimage, SP, dimage );
    regis[29] = SP;

    FILE *snapshot;
    snapshot = fopen("snapshot.rpt","w");

    Decode code;
    PC = inPC;

    while(cycle<=500000){
        output(snapshot, cycle);
        code.getinstruc( iimage[ (PC - inPC)/4 ] );

        if(code.rd==0 && !( code.instruc==sll && code.rt==0 && code.C==0 )  ) error.write0(cycle);

        if(code.instruc == add){

            unsigned int sign = 2;
            if(regis[code.rs]>>31 == regis[code.rt]>>31) sign = regis[code.rs]>>31;
            regis[code.rd] = regis[code.rs] + regis[code.rt];
            if(sign<=1) if(sign != regis[code.rd]>>31 ) error.overflow(cycle);

        }else if(code.instruc == addu){

            regis[code.rd] = regis[code.rs] + regis[code.rt];

        }else if(code.instruc == sub){

            unsigned int sign = 2;
            if(regis[code.rs]>>31 == (~regis[code.rt] + 1)>>31) sign = regis[code.rs]>>31;
            regis[code.rd] = regis[code.rs] + (~regis[code.rt] + 1);
            if(sign<=1) if(sign != regis[code.rd]>>31 ) error.overflow(cycle);

        }else if(code.instruc == And){

            regis[code.rd] = regis[code.rs] & regis[code.rt];

        }else if(code.instruc == Or){

            regis[code.rd] = regis[code.rs] | regis[code.rt];

        }else if(code.instruc == Xor){

            regis[code.rd] = regis[code.rs] ^ regis[code.rt];

        }else if(code.instruc == nor){

            regis[code.rd] = ~ ( regis[code.rs] | regis[code.rt] );

        }else if(code.instruc == nand){

            regis[code.rd] = ~ ( regis[code.rs] & regis[code.rt] );

        }else if(code.instruc == slt){

            if( regis[code.rs]>>31 > regis[code.rt]>>31 ) regis[code.rd] = 1;
            else if(regis[code.rs]>>31 < regis[code.rt]>>31 ) regis[code.rd] = 0;
            else if(regis[code.rs]>>31 == 1 && regis[code.rt]>>31 == 1){
                if(regis[code.rs] < regis[code.rt]) regis[code.rd] = 1;
                else regis[code.rd] = 0;
            }else if( regis[code.rs]>>31 == 0 && regis[code.rt]>>31==0 ){
                if(regis[code.rs] < regis[code.rt] ) regis[code.rd] = 1;
                else regis[code.rd] = 0;
            }

        }else if(code.instruc == sll){

            regis[code.rd] = regis[code.rt]<<code.C;

        }else if(code.instruc == srl){

            regis[code.rd] = regis[code.rt]>>code.C;

        }else if(code.instruc == sra){

            regis[code.rd] = sign_extend(regis[code.rt]>>code.C, 32-code.C);

        }else if(code.instruc == jr){

            PC = regis[code.rs] - 4;

        }else if(code.instruc == addi){

            if(code.rt==0) error.write0(cycle);

            unsigned int sign = 2;
            if(regis[code.rs]>>31 == sign_extend(code.C,16)>>31) sign = regis[code.rs]>>31;
            regis[code.rt] = regis[code.rs] + sign_extend(code.C,16);

            if(sign<=1) if(sign != regis[code.rt]>>31 ) error.overflow(cycle);

        }else if(code.instruc == addiu){

            if(code.rt==0) error.write0(cycle);

            code.C = (code.C<<16)>>16;
            regis[code.rt] = regis[code.rs] + code.C;

        }else if(code.instruc == lw){

            if(code.rt==0) error.write0(cycle);

            unsigned int addr = getmemaddr(regis[code.rs], code.C);
            if(addr>1020 || addr<0) error.addrover(cycle);
            if(addr%4!=0) error.misalign(cycle);

            regis[code.rt] = dimage[ addr/4 ];

        }else if(code.instruc == lh){

            if(code.rt==0) error.write0(cycle);

            unsigned int addr = getmemaddr(regis[code.rs], code.C);
            if(addr>1022 || addr<0) error.addrover(cycle);
            if(addr%2!=0) error.misalign(cycle);
            else addr = addr/2;

            if(addr%2==0) regis[code.rt] = sign_extend(dimage[ addr/2 ] >> 16 , 16) ;
            else if(addr%2==1) regis[code.rt] = sign_extend(dimage[ addr/2 ]&0x0000ffff,16);

        }else if(code.instruc == lhu){

            if(code.rt==0) error.write0(cycle);

            unsigned int addr = getmemaddr(regis[code.rs], code.C);
            if(addr>1022 || addr<0) error.addrover(cycle);
            if(addr%2!=0) error.misalign(cycle);
            else addr = addr/2;

            if(addr%2==0) regis[code.rt] = dimage[ addr/2 ] >> 16 ;
            else if(addr%2==1) regis[code.rt] = dimage[ addr/2 ] & 0x0000ffff;

        }else if(code.instruc == lb){

            if(code.rt==0) error.write0(cycle);

            unsigned int addr = getmemaddr(regis[code.rs], code.C);
            if(addr>1023 || addr<0) error.addrover(cycle);
            if(addr%4==0) regis[code.rt] = sign_extend(dimage[addr/4] >> 24 ,8);
            else if(addr%4==1) regis[code.rt] = sign_extend(dimage[addr/4] >> 16 &0x000000ff ,8);
            else if(addr%4==2) regis[code.rt] = sign_extend(dimage[addr/4] >> 8 &0x000000ff ,8);
            else if(addr%4==3) regis[code.rt] = sign_extend(dimage[addr/4] & 0x000000ff ,8);

        }else if(code.instruc == lbu){

            if(code.rt==0) error.write0(cycle);

            unsigned int addr = getmemaddr(regis[code.rs], code.C);
            if(addr>1023 || addr<0) error.addrover(cycle);
            if(addr%4==0) regis[code.rt] = dimage[addr/4] >> 24;
            else if(addr%4==1) regis[code.rt] = dimage[addr/4] >> 16 & 0x000000ff;
            else if(addr%4==2) regis[code.rt] = dimage[addr/4] >> 8 & 0x000000ff;
            else if(addr%4==3) regis[code.rt] = dimage[addr/4] & 0x000000ff;

        }else if(code.instruc == sw){

            unsigned int addr = getmemaddr(regis[code.rs],code.C);
            if(addr>1020 || addr<0) error.addrover(cycle);
            if(addr%4!=0) error.misalign(cycle);
            dimage[addr/4] = regis[code.rt];

        }else if(code.instruc == sh){

            unsigned int addr = getmemaddr(regis[code.rs], code.C);
            if(addr>1022 || addr<0) error.addrover(cycle);
            if(addr%2!=0) error.misalign(cycle);
            else addr = addr/2;

            if(addr%2==0){
                dimage[ addr/2 ] = (dimage[ addr/2 ]<<16) >>16;
                dimage[ addr/2 ] = ( (regis[code.rt]&0x0000ffff)<<16 ) |  dimage[ addr/2 ];
            }else if(addr%2==1){
                dimage[ addr/2 ] = (dimage[ addr/2 ]>>16) <<16;
                dimage[ addr/2 ] = ( regis[code.rt]&0x0000ffff ) |  dimage[ addr/2 ] ;
            }

        }else if(code.instruc == sb){

            unsigned int addr = getmemaddr(regis[code.rs],code.C);
            if(addr>1023 || addr<0) error.addrover(cycle);
            if(addr%4==0){
                dimage[addr/4] = ( (dimage[addr/4]<<4)>>4 ) | (regis[code.rt]&0xff)<<24;
            }else if(addr%4==1){
                dimage[addr/4] = dimage[addr/4]&0xff00ffff;
                dimage[addr/4] = dimage[addr/4] | (regis[code.rt]&0xff)<<16 ;
            }else if(addr%4==2){
                dimage[addr/4] = dimage[addr/4]&0xffff00ff;
                dimage[addr/4] = dimage[addr/4] | (regis[code.rt]&0xff)<<8 ;
            }else if(addr%4==3){
                dimage[addr/4] = dimage[addr/4]&0xffffff00;
                dimage[addr/4] = dimage[addr/4] | (regis[code.rt]&0xff) ;
            }

        }else if(code.instruc == lui){

            if(code.rt==0) error.write0(cycle);
            regis[code.rt] = code.C << 16;

        }else if(code.instruc == andi){

            if(code.rt==0) error.write0(cycle);
            regis[code.rt] = regis[code.rs] & ( code.C << 16 ) >> 16;

        }else if(code.instruc == ori){

            if(code.rt==0) error.write0(cycle);
            regis[code.rt] = regis[code.rs] | ( code.C << 16 ) >> 16;

        }else if(code.instruc == nori){

            if(code.rt==0) error.write0(cycle);
            regis[code.rt] = ~( regis[code.rs] | ( code.C << 16 ) >> 16 );

        }else if(code.instruc == slti){

            if(code.rt==0) error.write0(cycle);

            code.C = sign_extend(code.C,16);
            if( regis[code.rs]>>31 > code.C>>31 ) regis[code.rt] = 1;
            else if(regis[code.rs]>>31 < code.C>>31 ) regis[code.rt] = 0;
            else if(regis[code.rs]>>31 == 1 && code.C>>31 == 1){
                if( regis[code.rs] < code.C ) regis[code.rt] = 1;
                else regis[code.rt] = 0;
            }else if( regis[code.rs]>>31 == 0 && code.C>>31 == 0 ){
                if(regis[code.rs] < code.C ) regis[code.rt] = 1;
                else regis[code.rt] = 0;
            }

        }else if(code.instruc == beq){

            if(regis[code.rs]==regis[code.rt]){
                if(code.C >> 15 == 1){
                    code.C =  ( (~code.C + 1)<<16 ) >> 16;
                    PC = PC - (code.C<<2);
                }else{
                    code.C =  ( code.C<<16 ) >> 16;
                    PC = PC + (code.C<<2);
                }
            }

        }else if(code.instruc == bne){

            if(regis[code.rs]!=regis[code.rt]){
                if(code.C >> 15 == 1){
                    code.C =  ( (~code.C + 1)<<16 ) >> 16;
                    PC = PC - (code.C<<2);
                }else{
                    code.C =  ( code.C<<16 ) >> 16;
                    PC = PC + (code.C<<2);
                }
            }

        }else if(code.instruc == bgtz){

            if( (regis[code.rs]>0 ) && ( regis[code.rs]>>31!=1 ) ){
                if(code.C >> 15 == 1){
                    code.C =  ( (~code.C + 1)<<16 ) >> 16;
                    PC = PC - (code.C<<2);
                }else{
                    code.C =  ( code.C<<16 ) >> 16;
                    PC = PC + (code.C<<2);
                }
            }

        }else if(code.instruc == j){

            PC = (PC+4)&0xf0000000;
            PC = PC | (code.C&0x03ffffff)<<2;
            PC = PC - 4;

        }else if(code.instruc == jal){

            regis[31] = PC+4;
            PC = (PC+4)&0xf0000000;
            PC = PC | (code.C&0x03ffffff)<<2;
            PC = PC - 4;

        }else if(code.instruc == halt){
            break;
        }

        if(error.Exit()) break;
        PC+=4;
        cycle++;
    }

    fclose(snapshot);

    return 0;
}
