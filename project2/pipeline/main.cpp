#include <iostream>
#include <stdio.h>
#include <string>
#include "readfile.h"
#include "Decode.h"
#include "Error.h"

using namespace std;

unsigned int inPC,PC, nextPC;
unsigned int iimage[256] = {0};
unsigned int SP;
unsigned int dimage[256] = {0};
unsigned int regis[32] = {0};

Error error;
int cycle = 0;
Decode *If, *id, *ex, *mem, *wb ;
string fwdid, fwdex;


enum Except {
    stall, fls, fwd_ex_rt, fwd_ex_rs, fwd_ex_both, fwd_rt, fwd_rs, fwd_both
};


inline string getype(unsigned int type){
    switch(type){
        case NOP:   return "NOP";
        case add:   return "ADD";
        case addu:  return "ADDU";
        case sub:   return "SUB";
        case And:   return "AND";
        case Or:    return "OR";
        case Xor:   return "XOR";
        case nor:   return "NOR";
        case nand:  return "NAND";
        case slt:   return "SLT";
        case sll:   return "SLL";
        case srl:   return "SRL";
        case sra:   return "SRA";
        case jr:    return "JR";
        case addi:  return "ADDI";
        case addiu: return "ADDIU";
        case lw:    return "LW";
        case lh:    return "LH";
        case lhu:   return "LHU";
        case lb:    return "LB";
        case lbu:   return "LBU";
        case sw:    return "SW";
        case sh:    return "SH";
        case sb:    return "SB";
        case lui:   return "LUI";
        case andi:  return "ANDI";
        case ori:   return "ORI";
        case nori:  return "NORI";
        case slti:  return "SLTI";
        case beq:   return "BEQ";
        case bne:   return "BNE";
        case bgtz:  return "BGTZ";
        case j:     return "J";
        case jal:   return "JAL";
        case halt:  return "HALT";
        default : return "NOP";
    }
}


inline void output(FILE *f, int cycle ){       ///output snapshot.rpt
    regis[0] = 0;

    fprintf(f,"cycle %d\n",cycle);
    for(int i=0; i<32; i++){
        fprintf(f,"$%02d: 0x%08X\n",i ,regis[i]);
    }
    fprintf(f,"PC: 0x%08X\n",PC);

    fprintf(f,"IF: 0x%08X",If->instruc);
    if(If->except==stall) fprintf(f," to_be_stalled");
    else if(If->except==fls) fprintf(f," to_be_flushed");
    fprintf(f,"\n");

    fprintf(f,"ID: %s",getype(id->instruc).c_str() );
    if(id->except==stall)fprintf(f," to_be_stalled");
    else if(!fwdid.empty()) fprintf(f,fwdid.c_str());
    fprintf(f,"\n");

    fprintf(f,"EX: %s",getype(ex->instruc).c_str() );
    if(!fwdex.empty()) fprintf(f,fwdex.c_str());
    fprintf(f,"\n");

    fprintf(f,"DM: %s\n",getype(mem->instruc).c_str() );
    fprintf(f,"WB: %s\n\n\n",getype(wb->instruc).c_str() );

}


inline unsigned int sign_extend(unsigned int c, int length){
    if(c>>(length-1)==1 && length<32){
        unsigned int tmp = 0xffffffff<<length;
        unsigned int t = c | tmp;
        return t;
    }else return c;
}


inline void IF(){
    if(PC<inPC){
        If = new Decode();
        If->instruc = 0x00000000 ;
    }else{
        If = new Decode();
        If->instruc = iimage[ (PC - inPC)/4 ] ;
    }
}


inline void ID(){

    if(id->rd <32){         ///only set dest and dt
        id->dest = id->rd;
        id->dt = 1;
    }else if(id->rt < 32){
        if(!(id->instruc<=sb && id->instruc>=sw ) && !(id->instruc<=bgtz && id->instruc>=beq ) ){
            id->dest = id->rt;
            id->dt = 0;
        }
    }else{
        id->dest = -1;
        id->dt = -1;
    }

    /*if(id->rd <32) id->rd = regis[id->rd];
    else id->rd = 0;*/

    ///forwarding or register access

    if(id->except==fwd_both){
        if(mem->dt==1){
            id->rt = mem->rd; id->rs = mem->rd;
        }else{
            id->rt = mem->rt; id->rs = mem->rt;
        }
        id->except = -1;
    }else if(id->except==fwd_rt){
        if(mem->dt==1) id->rt = mem->rd;
        else id->rt = mem->rt;
        if(id->rs <32) id->rs = regis[id->rs];
        else id->rs = 0;
        id->except = -1;
    }else if(id->except==fwd_rs){
        if(mem->dt==1) id->rs = mem->rd;
        else id->rs = mem->rt;
        if(id->rt <32) id->rt = regis[id->rt];
        else id->rt = 0;
        id->except = -1;
    }else {
        if(id->rt <32) id->rt = regis[id->rt];
        else id->rt = 0;
        if(id->rs <32) id->rs = regis[id->rs];
        else id->rs = 0;
    }

    if(id->instruc>=addi && id->instruc<=sb) id->C = sign_extend(id->C,16);
    else if(id->instruc==slti) id->C = sign_extend(id->C,16);

    ///branch and jump

    if(id->instruc == jr){

        nextPC = id->rs;

    }else if(id->instruc == j){

        nextPC = (PC+4)&0xf0000000;
        nextPC = nextPC | (id->C&0x03ffffff)<<2;

    }else if(id->instruc == jal){

        id->rt = PC-4;  ///$SP = PC + 4;
        id->dest = 31;
        id->dt = 0;
        nextPC = (PC+4)&0xf0000000;
        nextPC = nextPC | (id->C&0x03ffffff)<<2;

    }else if(id->instruc == beq){

        if(id->rs==id->rt){
            if(id->C >> 15 == 1){
                id->C =  ( (~id->C + 1)<<16 ) >> 16;
                nextPC = PC -4 - (id->C<<2);
            }else{
                id->C =  ( id->C<<16 ) >> 16;
                nextPC = PC -4 + (id->C<<2);
            }
        }

    }else if(id->instruc == bne){

        if(id->rs!=id->rt){
            if(id->C >> 15 == 1){
                id->C =  ( (~id->C + 1)<<16 ) >> 16;
                nextPC = PC -4 - (id->C<<2);
            }else{
                id->C =  ( id->C<<16 ) >> 16;
                nextPC = PC -4 + (id->C<<2);
            }
        }

    }else if(id->instruc == bgtz){

        if( (id->rs>0 ) && ( id->rs>>31!=1 ) ){
            if(id->C >> 15 == 1){
                id->C =  ( (~id->C + 1)<<16 ) >> 16;
                nextPC = PC -4 - (id->C<<2);
            }else{
                id->C =  ( id->C<<16 ) >> 16;
                nextPC = PC -4 + (id->C<<2);
            }
        }
    }

}


inline void EX(){

    if(ex->instruc == add){

        unsigned int sign = 2;
        if(ex->rs>>31 == ex->rt>>31) sign = ex->rs>>31;
        ex->rd = ex->rs + ex->rt;
        if(sign<=1)if(sign != ex->rd>>31 ) error.overflow(cycle);

    }else if(ex->instruc == addu){

        ex->rd = ex->rs + ex->rt;

    }else if(ex->instruc == sub){

        unsigned int sign = 2;
        if(ex->rs >>31 == (~ex->rt + 1)>>31) sign = ex->rs>>31;
        ex->rd = ex->rs + (~ex->rt + 1);
        if(sign<=1) if(sign != ex->rd>>31 ) error.overflow(cycle);

    }else if(ex->instruc == And){

        ex->rd = ex->rs & ex->rt;

    }else if(ex->instruc == Or){

        ex->rd = ex->rs | ex->rt;

    }else if(ex->instruc == Xor){

        ex->rd = ex->rs ^ ex->rt;

    }else if(ex->instruc == nor){

        ex->rd = ~ ( ex->rs | ex->rt );

    }else if(ex->instruc == nand){

        ex->rd = ~ ( ex->rs & ex->rt );

    }else if(ex->instruc == slt){

        if( ex->rs>>31 > ex->rt>>31 ) ex->rd = 1;
        else if(ex->rs>>31 < ex->rt>>31 ) ex->rd = 0;
        else if(ex->rs>>31 == 1 && ex->rt>>31 == 1){
            if(ex->rs < ex->rt) ex->rd = 1;
            else ex->rd = 0;
        }else if( ex->rs>>31 == 0 && ex->rt>>31==0 ){
            if(ex->rs < ex->rt ) ex->rd = 1;
            else ex->rd = 0;
        }

    }else if(ex->instruc == sll){

        ex->rd = ex->rt<<ex->C;

    }else if(ex->instruc == srl){

        ex->rd = ex->rt>>ex->C;

    }else if(ex->instruc == sra){

        ex->rd = sign_extend(ex->rt>>ex->C, 32-ex->C);

    }else if(ex->instruc == addi){

        unsigned int sign = 2;
        if(ex->rs>>31 == ex->C >>31) sign = ex->rs>>31;
        ex->rt = ex->rs + ex->C;

        if(sign<=1) if(sign != ex->rt>>31 ) error.overflow(cycle);

    }else if(ex->instruc == addiu){

        ex->rt = ex->rs + ex->C;

    }else if(ex->instruc >= lw && ex->instruc <= sb){  ///mem access type

        unsigned int sign=2;
        if(ex->C >>31 == ex->rs>>31) sign = ex->rs >>31;
        ex->C = ex->C + ex->rs;
        if(sign<=1) if(sign != ex->C >>31 ) error.overflow(cycle);


    }else if(ex->instruc == lui){

        ex->rt = ex->C << 16;

    }else if(ex->instruc == andi){

        ex->rt =  ex->rs & ex->C ;

    }else if(ex->instruc == ori){

        ex->rt = ex->rs | ex->C;

    }else if(ex->instruc == nori){

        ex->rt = ~( ex->rs | ex->C );

    }else if(ex->instruc == slti){


        if( ex->rs>>31 > ex->C>>31 ) ex->rt = 1;
        else if(ex->rs>>31 < ex->C>>31 ) ex->rt = 0;
        else if(ex->rs>>31 == 1 && ex->C>>31 == 1){
            if( ex->rs < ex->C ) ex->rt = 1;
            else ex->rt = 0;
        }else if( ex->rs>>31 == 0 && ex->C>>31 == 0 ){
            if(ex->rs < ex->C ) ex->rt = 1;
            else ex->rt = 0;
        }

    }

}


inline void MEM(){

     if(mem->instruc == lw){

        if(mem->C>1020 || mem->C<0) error.addrover(cycle);
        if(mem->C%4!=0) error.misalign(cycle);
        if(!error.Exit()) mem->rt = dimage[ mem->C/4 ];

    }else if(mem->instruc == lh){

        if(mem->C>1022 || mem->C<0) error.addrover(cycle);
        if(mem->C%2!=0) error.misalign(cycle);
        if(!error.Exit()) {
            mem->C = mem->C/2;
            if(mem->C%2==0) mem->rt = sign_extend(dimage[ mem->C/2 ] >> 16 , 16) ;
            else if(mem->C%2==1) mem->rt = sign_extend(dimage[ mem->C/2 ]&0x0000ffff,16);
        }

    }else if(mem->instruc == lhu){

        if(mem->C>1022 || mem->C<0) error.addrover(cycle);
        if(mem->C%2!=0) error.misalign(cycle);
        if(!error.Exit()) {
            mem->C = mem->C/2;
            if(mem->C%2==0) mem->rt = dimage[ mem->C/2 ] >> 16 ;
            else if(mem->C%2==1) mem->rt = dimage[ mem->C/2 ] & 0x0000ffff;
        }

    }else if(mem->instruc == lb){

        if(mem->C>1023 || mem->C<0) error.addrover(cycle);
        else if(mem->C%4==0) mem->rt = sign_extend(dimage[mem->C/4] >> 24 ,8);
        else if(mem->C%4==1) mem->rt = sign_extend(dimage[mem->C/4] >> 16 &0x000000ff ,8);
        else if(mem->C%4==2) mem->rt = sign_extend(dimage[mem->C/4] >> 8 &0x000000ff ,8);
        else if(mem->C%4==3) mem->rt = sign_extend(dimage[mem->C/4] & 0x000000ff ,8);

    }else if(mem->instruc == lbu){

        if(mem->C>1023 || mem->C<0) error.addrover(cycle);
        else if(mem->C%4==0) mem->rt = dimage[mem->C/4] >> 24;
        else if(mem->C%4==1) mem->rt = dimage[mem->C/4] >> 16 & 0x000000ff;
        else if(mem->C%4==2) mem->rt = dimage[mem->C/4] >> 8 & 0x000000ff;
        else if(mem->C%4==3) mem->rt = dimage[mem->C/4] & 0x000000ff;

    }else if(mem->instruc == sw){

        if(mem->C>1020 || mem->C<0) error.addrover(cycle);
        if(mem->C%4!=0) error.misalign(cycle);
        if(!error.Exit()) dimage[mem->C/4] = mem->rt;

    }else if(mem->instruc == sh){

        if(mem->C>1022 || mem->C<0) error.addrover(cycle);
        if(mem->C%2!=0) error.misalign(cycle);
        if(!error.Exit()){
            mem->C = mem->C/2;

            if(mem->C%2==0){
                dimage[ mem->C/2 ] = (dimage[ mem->C/2 ]<<16) >>16;
                dimage[ mem->C/2 ] = ( (mem->rt&0x0000ffff)<<16 ) |  dimage[ mem->C/2 ];
            }else if(mem->C%2==1){
                dimage[ mem->C/2 ] = (dimage[ mem->C/2 ]>>16) <<16;
                dimage[ mem->C/2 ] = ( mem->rt&0x0000ffff ) |  dimage[ mem->C/2 ] ;
            }
        }

    }else if(mem->instruc == sb){

        if(mem->C>1023 || mem->C<0) error.addrover(cycle);
        else if(mem->C%4==0){
            dimage[mem->C/4] = (dimage[mem->C/4]&0x00ffffff ) | (mem->rt&0xff)<<24;
        }else if(mem->C%4==1){
            dimage[mem->C/4] = dimage[mem->C/4]&0xff00ffff;
            dimage[mem->C/4] = dimage[mem->C/4] | (mem->rt&0xff)<<16 ;
        }else if(mem->C%4==2){
            dimage[mem->C/4] = dimage[mem->C/4]&0xffff00ff;
            dimage[mem->C/4] = dimage[mem->C/4] | (mem->rt&0xff)<<8 ;
        }else if(mem->C%4==3){
            dimage[mem->C/4] = dimage[mem->C/4]&0xffffff00;
            dimage[mem->C/4] = dimage[mem->C/4] | (mem->rt&0xff) ;
        }

    }
}


inline void WB(){
    if(wb->dest<0) return;

    if(wb->dest==0) error.write0(cycle);
    else if(wb->dt==1) regis[wb->dest] = wb->rd;
    else if(wb->dt==0) regis[wb->dest] = wb->rt;
}


inline void hazard_detect(){


    ///data hazard
    if( id->rd<32 || id->instruc==beq || id->instruc==bne || id->instruc==sw || id->instruc==sb || id->instruc==sh){       ///for R-type
                /**forward : ID is branch and can get from mem
                   fwd in ex: not branch and can get from cur_ex
                   else if any mem and ex write dest --> stall**/
        bool a = ( (ex->dest==id->rt && ex->dest!=0) || (mem->dest==id->rt && mem->dest!=0) );
        bool b = ( (ex->dest==id->rs && ex->dest!=0) || (mem->dest==id->rs && mem->dest!=0) );
        if( a && b && !(id->instruc>=sll && id->instruc<=sra) && id->rs!=id->rt ) id->except = stall;
        else if(id->instruc==beq || id->instruc==bne || id->instruc==bgtz){
            if( ( ex->dest==id->rt || ex->dest==id->rs ) && ex->dest!=0  ) id->except = stall;
            else if( (mem->instruc<=lbu && mem->instruc>=lw) && (mem->dest==id->rt || mem->dest==id->rs) && mem->dest!=0 ) id->except = stall;
            else if(mem->dest==id->rt && mem->dest==id->rs && mem->dest!=0) id->except = fwd_both;
            else if(mem->dest==id->rt && mem->dest!=0) id->except = fwd_rt;
            else if(mem->dest==id->rs && mem->dest!=0) id->except = fwd_rs;
        }else if( ( ex->dest==id->rt || ex->dest==id->rs ) &&  ex->dest!=0){
            if( ex->instruc<=lbu && ex->instruc>=lw ) id->except=stall;
            else if(ex->dest==id->rt && ex->dest==id->rs ) id->except = fwd_ex_both;
            else if(ex->dest==id->rt ) id->except = fwd_ex_rt;
            else if(ex->dest==id->rs ) id->except = fwd_ex_rs;
        }else if( ( mem->dest==id->rt || mem->dest==id->rs ) &&  mem->dest!=0){
            id->except=stall;
        }

    }else if( id->rd==32){          ///for I-type

        if(id->instruc==bgtz || id->instruc==jr){
            if( ex->dest==id->rs && ex->dest!=0  ) id->except = stall;
            else if(mem->dest==id->rs && mem->dest!=0){
                if (mem->instruc<=lbu && mem->instruc>=lw) id->except = stall;
                else id->except = fwd_rs;
            }
        }else if( ex->dest==id->rs &&  ex->dest!=0){
            if( ex->instruc<=lbu && ex->instruc>=lw ) id->except=stall;
            else id->except = fwd_ex_rs;
        }else if(  mem->dest==id->rs &&  mem->dest!=0){
            id->except=stall;
        }
    }


    ///flush detect
    if(id->except==stall) If->except=stall;
    else if(id->instruc==j || id->instruc==jal || id->instruc==jr) If->except=fls;
    else if(id->instruc==beq || id->instruc==bne || id->instruc==bgtz){
        unsigned int a = regis[id->rs];
        unsigned int b = regis[id->rt];
        //if(wb->instruc>=lw && wb->instruc<=lb){
            if(wb->dest==id->rs){
                if(wb->dt==0) a = wb->rt;
                else  a = wb->rd;
            }
            if(wb->dest==id->rt){
                if(wb->dt==0) b = wb->rt;
                else b = wb->rd;
            }
        //}

        if(id->except==-1){
            if( (id->instruc == beq && a==b) || (id->instruc == bne && a!=b) || (id->instruc == bgtz && (a>0 ) && ( a>>31!=1 ) ) )If->except=fls;
        }else if(id->except==fwd_both){
            if( id->instruc == beq || (id->instruc == bgtz && (((mem->dt==1)&&(mem->rd>0)&&(mem->rd>>31!=1))||((mem->dt==0)&&(mem->rt>0)&&(mem->rt>>31!=1))) ) ) If->except=fls;
        }else if(id->except==fwd_rs){
            int temp;
            if(mem->dt==1) temp=mem->rd;
            else temp=mem->rt;

            if( (id->instruc == beq && temp==b) || (id->instruc == bne && temp!=b) || (id->instruc == bgtz && (temp>0) && (temp>>31!=1) ) ) If->except=fls;
        }else if(id->except==fwd_rt){
            int temp;
            if(mem->dt==1) temp=mem->rd;
            else temp=mem->rt;

            if( (id->instruc == beq && a==temp) || (id->instruc == bne && a!=temp) || (id->instruc == bgtz && (a>0 ) && ( a>>31!=1 ) ) )If->except=fls;
        }
    }
}


int main(){
    Readfile input;
    input.readinput(inPC, iimage, SP, dimage );
    regis[29] = SP;

    FILE *snapshot;
    snapshot = fopen("snapshot.rpt","w");

    PC = inPC;nextPC = 0;

    If = new Decode();
    id = new Decode();
    ex = new Decode();
    mem = new Decode();
    wb = new Decode();


    while(cycle<500000){

         fwdex.clear(); fwdid.clear();

         if(ex->except==fwd_ex_both){
            if(mem->dt==1){
                ex->rt = mem->rd; ex->rs = mem->rd;
            }else{
                ex->rt = mem->rt; ex->rs = mem->rt;
            }
            ex->except = -1;
        }else if(ex->except==fwd_ex_rt){
            if(mem->dt==1) ex->rt = mem->rd;
            else ex->rt = mem->rt;
            ex->except = -1;
        }else if(ex->except==fwd_ex_rs){
            if(mem->dt==1) ex->rs = mem->rd;
            else ex->rs = mem->rt;
            ex->except = -1;
        }


        WB();

        MEM();

        EX();

        if(id->except==stall){
            delete wb;
            wb = mem;
            mem = ex;
            ex = new Decode();
            id->except = -1;
            If->except = -1;
        }else if(If->except==fls){
            ID();
            delete wb;
            wb = mem;
            mem = ex;
            ex = id;
            id = new Decode();
            id->getinstruc(id->instruc);
            delete If;
            PC = nextPC;
            IF();
        }else{
            ID();
            delete wb;
            wb = mem;
            mem = ex;
            ex = id;
            id = If;
            id->getinstruc(id->instruc);
            IF();
        }


        hazard_detect();


        if(id->except==fwd_both){
            ///fwdid = " fwd_EX-DM_rs_$"+to_string(id->rs)+" fwd_EX-DM_rt_$"+to_string(id->rt);
            fwdid = " fwd_EX-DM_rs_$ fwd_EX-DM_rt_$";
        }else if(id->except==fwd_rs){
            ///fwdid = " fwd_EX-DM_rs_$"+to_string(id->rs);
            fwdid = " fwd_EX-DM_rs_$";
        }else if(id->except==fwd_rt){
             ///fwdid = " fwd_EX-DM_rt_$"+to_string(id->rt);
            fwdid = " fwd_EX-DM_rt_$";
        }
        if(ex->except==fwd_ex_both){
            ///fwdex = " fwd_EX-DM_rs_$"+to_string(mem->dest)+" fwd_EX-DM_rt_$"+to_string(mem->dest);
            fwdex = " fwd_EX-DM_rs_$ fwd_EX-DM_rt_$";
        }else if(ex->except==fwd_ex_rs){
            ///fwdex = " fwd_EX-DM_rs_$"+to_string(mem->dest);
            fwdex = " fwd_EX-DM_rs_$";
        }else if(ex->except==fwd_ex_rt){
             ///fwdex = " fwd_EX-DM_rt_$"+to_string(mem->dest);
            fwdex = " fwd_EX-DM_rt_$";
        }

        //cout<<mem->dest<<" "<<ex->dest<<" "<<id->instruc<<" "<<id->rd<<" "<<id->rs<<" "<<id->rt<<" "<<id->except<<endl;

        if(error.Exit()) break;
        output(snapshot, cycle);

        if(wb->instruc==halt && mem->instruc==halt && ex->instruc==halt && id->instruc==halt && (If->instruc|0x000FFFFF)==0xFFFFFFFF) break;

        nextPC = 0;
        cycle++;
        if(nextPC<=0 && If->except!=stall) PC +=4;
    }

    fclose(snapshot);

    return 0;
}
