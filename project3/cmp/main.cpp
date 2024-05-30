#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <cmath>
#include "readfile.h"
#include "Decode.h"
#include "Cache.h"
#include "TLB.h"

using namespace std;

/*** all unit in WORD !! ***/

unsigned int inPC,PC;
unsigned int iimage[256] = {0};
unsigned int SP;
unsigned int dimage[256] = {0};
unsigned int regis[32] = {0};

int cycle = 0;

///configure parameters
int I_mem_size = 64;
int D_mem_size = 32;
int I_page_size = 8;
int D_page_size  = 16;
int I_cache_size = 16;
int I_block_size = 4;
int I_set_way = 4;
int D_cache_size = 16;
int D_block_size = 4;
int D_set_way = 1;

int iP_hit=0, iP_miss=0;
int dP_hit=0, dP_miss=0;

void resetarray(int *ary,int size){
    for(int i=0; i<size; i++) ary[i] = -1;
}


///
unsigned int igetdata(unsigned int addr, Cache* cache, TLB* tlb,unsigned int* memory, int* mLRU, int* PT, bool* valid){

    int ofb = log2(I_page_size);
    int offset = addr & (int)((pow(2,ofb)-1)) ;
    int p = tlb->find(addr>>ofb, cycle);
    if( p>=0 ){ ///TLB hit
        tlb->hit++;
        unsigned int ppn = tlb->blocks[p].ppn;
        unsigned int phyaddr = (ppn<<ofb) + offset;
        ofb = log2(I_block_size);
        offset = phyaddr & (int)(pow(2,ofb)-1);
        pair<int,int> c = cache->find(phyaddr>>ofb, cycle, true);
        if(c.second>=0){   ///cache hit
            cache->hit++;
            unsigned int data = cache->blocks[c.first][c.second].data[offset/4];
            return data;
        }else{ ///cache miss
            cache->miss++;
            cache->replace( phyaddr>>ofb, &memory[phyaddr/4], cycle);    //update cache
            //mLRU[ppn] = cycle;
            c = cache->find(phyaddr>>ofb, cycle, true);
            unsigned int data = cache->blocks[c.first][c.second].data[offset/4];
            return data;
        }
    }else{  ///TLB miss
        tlb->miss++;
        unsigned int ppn;
        unsigned int phyaddr;

         if( valid[addr>>ofb] ){    /// PTE hit
            iP_hit++;
            ppn = PT[addr>>ofb];
            phyaddr = (ppn<<ofb) + offset;
            tlb->replace(addr>>ofb, ppn, cycle);

         }else{ /// PTE miss
            iP_miss++;
            int oldest = cycle; int k=0;
            for(int i=0; i<(I_mem_size/I_page_size); i++){
                if(mLRU[i]<oldest){
                    oldest = mLRU[i];
                    k = i;
                }
            }
            for(int i=0; i<1024/I_page_size; i++){  //invalid page been swapped
                if(valid[i] && PT[i]==k){
                    valid[i] = false;

                    int t = tlb->find(i, cycle);
                    if(t>=0) tlb->blocks[t].valid = false; //invalid TLB

                    for(int j=0; j<I_page_size/I_block_size; j++){  //invalid cache
                        pair<int,int> c = cache->find( k*I_page_size/I_block_size + j , cycle, false);
                        if(c.second>=0){
                            cache->blocks[c.first][c.second].valid = 0;
                            cache->blocks[c.first][c.second].MRU = 0;
                        }
                    }
                    break;
                }
            }

            for(int j=0; j<(I_page_size/4); j++) memory[k*I_page_size/4 + j] = iimage[(addr/I_page_size)*I_page_size/4 + j]; //update memory
            mLRU[k] = cycle;

            PT[addr/I_page_size] = k;  // updated PTE
            valid[addr/I_page_size] = true;

            ppn = PT[addr>>ofb];
            phyaddr = (ppn<<ofb) + offset;
            tlb->replace(addr>>ofb, ppn, cycle);
        }

        ofb = log2(I_block_size);
        offset = phyaddr & (int)(pow(2,ofb)-1);
        pair<int,int> c = cache->find(phyaddr>>ofb, cycle, true);
        if(c.second>=0){   ///cache hit
            cache->hit++;
            unsigned int data = cache->blocks[c.first][c.second].data[offset/4];
            return data;
        }else{ ///cache miss
            cache->miss++;
            cache->replace( phyaddr>>ofb, &memory[phyaddr/4], cycle);    //update cache
            //mLRU[ppn] = cycle;
            c = cache->find(phyaddr>>ofb, cycle, true);
            unsigned int data = cache->blocks[c.first][c.second].data[offset/4];
            return data;
        }

    }

}


unsigned int dgetdata(unsigned int addr, Cache* dcache, TLB* dtlb,unsigned int* memory, int* mLRU, int* dPT, bool* valid){

    int ofb = log2(D_page_size);
    int offset = addr & (int)((pow(2,ofb)-1)) ;
    int p = dtlb->find(addr>>ofb, cycle);
    if( p>=0 ){ ///TLB hit
        dtlb->hit++;
        unsigned int ppn = dtlb->blocks[p].ppn;
        unsigned int phyaddr = (ppn<<ofb) + offset;
        ofb = log2(D_block_size);
        offset = phyaddr & (int)(pow(2,ofb)-1);
        pair<int,int> c = dcache->find(phyaddr>>ofb, cycle, true);
        if(c.second>=0){   ///cache hit
            dcache->hit++;
            unsigned int data = dcache->blocks[c.first][c.second].data[offset/4];
            return data;
        }else{ ///cache miss
            dcache->miss++;
            dcache->replace( phyaddr>>ofb, &memory[phyaddr/4], cycle);    //update cache
            //mLRU[ppn] = cycle;
            c = dcache->find(phyaddr>>ofb, cycle, true);
            unsigned int data = dcache->blocks[c.first][c.second].data[offset/4];
            return data;
        }
    }else{  ///TLB miss
        dtlb->miss++;
        unsigned int ppn;
        unsigned int phyaddr;

         if( valid[addr>>ofb] ){    /// PTE hit
            dP_hit++;
            ppn = dPT[addr>>ofb];
            phyaddr = (ppn<<ofb) + offset;
            dtlb->replace(addr>>ofb, ppn, cycle);

         }else{ /// PTE miss
            dP_miss++;
            int oldest = cycle; int k=0;
            for(int i=0; i<(D_mem_size/D_page_size); i++){
                if(mLRU[i]<oldest){
                    oldest = mLRU[i];
                    k = i;
                }
            }
            for(int i=0; i<1024/D_page_size; i++){  //invalid page been swapped
                if(valid[i] && dPT[i]==k){
                    valid[i] = false;
                    int t = dtlb->find(i, cycle);
                    if(t>=0) dtlb->blocks[t].valid = false; //invalid TLB

                    for(int j=0; j<D_page_size/D_block_size; j++){  //invalid cache
                        pair<int,int> c = dcache->find(k*D_page_size/D_block_size + j , cycle, false);
                        if(c.second>=0){
                            dcache->blocks[c.first][c.second].valid = 0;
                            dcache->blocks[c.first][c.second].MRU = 0;
                        }
                    }
                    break;
                }
            }

            for(int j=0; j<(D_page_size/4); j++) memory[k*D_page_size/4 + j] = dimage[(addr/D_page_size)*D_page_size/4 + j]; //update memory
            mLRU[k] = cycle;

            dPT[addr/D_page_size] = k;  // updated PTE
            valid[addr/D_page_size] = true;

            ppn = dPT[addr>>ofb];
            phyaddr = (ppn<<ofb) + offset;
            dtlb->replace(addr>>ofb, ppn, cycle);
        }

        ofb = log2(D_block_size);
        offset = phyaddr & (int)(pow(2,ofb)-1);
        pair<int,int> c = dcache->find(phyaddr>>ofb, cycle, true);
        if(c.second>=0){   ///cache hit
            dcache->hit++;
            unsigned int data = dcache->blocks[c.first][c.second].data[offset/4];
            return data;
        }else{ ///cache miss
            dcache->miss++;
            dcache->replace( phyaddr>>ofb, &memory[phyaddr/4], cycle);    //update cache
            //mLRU[ppn] = cycle;
            c = dcache->find(phyaddr>>ofb, cycle, true);
            unsigned int data = dcache->blocks[c.first][c.second].data[offset/4];
            return data;
        }

    }

}


void dwritedata(unsigned int addr,unsigned int data, Cache* dcache, TLB* dtlb,unsigned int* memory, int* mLRU, int* dPT, bool* valid){

    int ofb = log2(D_page_size);
    int offset = addr & (int)((pow(2,ofb)-1)) ;
    int p = dtlb->find(addr>>ofb, cycle);
    if( p>=0 ){ ///TLB hit
        dtlb->hit++;
        unsigned int ppn = dtlb->blocks[p].ppn;
        unsigned int phyaddr = (ppn<<ofb) + offset;
        ofb = log2(D_block_size);
        offset = phyaddr & (int)(pow(2,ofb)-1);
        pair<int,int> c = dcache->find(phyaddr>>ofb, cycle, true);
        if(c.second>=0){   ///cache hit
            dcache->hit++;
            dcache->blocks[c.first][c.second].data[offset/4]=data;
            return;
        }else{ ///cache miss
            dcache->miss++;
            dcache->replace( phyaddr>>ofb, &memory[phyaddr/4], cycle);    //update cache
            memory[phyaddr/4] = data;
            //mLRU[ppn] = cycle;
            c = dcache->find(phyaddr>>ofb, cycle, true);
            dcache->blocks[c.first][c.second].data[offset/4]=data;
            return;
        }
    }else{  ///TLB miss
         dtlb->miss++;
         unsigned int ppn;
         unsigned int phyaddr;
         if( valid[addr>>ofb] ){    /// PTE hit
            dP_hit++;
            ppn = dPT[addr>>ofb];
            phyaddr = (ppn<<ofb) + offset;
            dtlb->replace(addr>>ofb, ppn, cycle);

         }else{ /// PTE miss
            dP_miss++;
            int oldest = cycle; int k=0;
            for(int i=0; i<(D_mem_size/D_page_size); i++){
                if(mLRU[i]<oldest){
                    oldest = mLRU[i];
                    k = i;
                }
            }
            for(int i=0; i<1024/D_page_size; i++){  //invalid page been swapped
                if(valid[i] && dPT[i]==k){
                    valid[i] = false;

                    int t = dtlb->find(i, cycle);
                    if(t>=0) dtlb->blocks[t].valid = false; //invalid TLB

                    for(int j=0; j<D_page_size/D_block_size; j++){
                        pair<int,int> c = dcache->find(k*D_page_size/D_block_size + j , cycle, false);
                        if(c.second>=0){
                            dcache->blocks[c.first][c.second].valid = 0;
                            dcache->blocks[c.first][c.second].MRU = 0;
                        }
                    }
                    break;
                }
            }

            for(int j=0; j<(D_page_size/4); j++) memory[k*D_page_size/4 + j] = dimage[(addr/D_page_size)*D_page_size/4 + j]; //update memory
            mLRU[k] = cycle;

            dPT[addr/D_page_size] = k;  // updated PTE
            valid[addr/D_page_size] = true;

            ppn = dPT[addr>>ofb];
            phyaddr = (ppn<<ofb) + offset;
            dtlb->replace(addr>>ofb, ppn, cycle);
         }

        ofb = log2(D_block_size);
        offset = phyaddr & (int)(pow(2,ofb)-1);
        pair<int,int> c = dcache->find(phyaddr>>ofb, cycle, true);
        if(c.second>=0){   ///cache hit
            dcache->hit++;
            dcache->blocks[c.first][c.second].data[offset/4]=data;
            return;
        }else{ ///cache miss
            dcache->miss++;
            memory[phyaddr/4] = data;
            dcache->replace( phyaddr>>ofb, &memory[phyaddr/4], cycle);    //update cache
            //mLRU[ppn] = cycle;
            c = dcache->find(phyaddr>>ofb, cycle, true);
            dcache->blocks[c.first][c.second].data[offset/4]=data;
            return;
        }


    }

}



inline void printreport(FILE *f, Cache* ic, Cache* id, TLB* it, TLB* dt){       ///output snapshot.rpt
    fprintf( f, "ICache :\n");
    fprintf( f, "# hits: %u\n", ic->hit);
    fprintf( f, "# misses: %u\n\n", ic->miss );
    fprintf( f, "DCache :\n");
    fprintf( f, "# hits: %u\n", id->hit );
    fprintf( f, "# misses: %u\n\n", id->miss );
    fprintf( f, "ITLB :\n");
    fprintf( f, "# hits: %u\n", it->hit );
    fprintf( f, "# misses: %u\n\n", it->miss );
    fprintf( f, "DTLB :\n");
    fprintf( f, "# hits: %u\n", dt->hit );
    fprintf( f, "# misses: %u\n\n", dt->miss );
    fprintf( f, "IPageTable :\n");
    fprintf( f, "# hits: %u\n",iP_hit );
    fprintf( f, "# misses: %u\n\n", iP_miss );
    fprintf( f, "DPageTable :\n");
    fprintf( f, "# hits: %u\n", dP_hit );
    fprintf( f, "# misses: %u\n\n", dP_miss );
}


///

inline void output(FILE *f, int cycle ){       ///output snapshot.rpt
    regis[0] = 0;

    fprintf(f,"cycle %d\n",cycle);
    for(int i=0; i<32; i++){
        fprintf(f,"$%02d: 0x%08X\n",i ,regis[i]);
    }
    fprintf(f,"PC: 0x%08X\n\n\n",PC);
}

inline unsigned int sign_extend(int c, int length){
    if(c>>(length-1)==1 && length<32){
        int tmp = 0xffffffff<<length;
        unsigned int t = c | tmp;
        return t;
    }else return (unsigned int)c;
}

inline unsigned int getmemaddr(unsigned int r, int c){   ///compute address and handle number overflow

    unsigned int uc = sign_extend(c,16);
    unsigned int addr = uc + r;

    return addr;
}



int main(int argc, char* argv[])
{
    Readfile input;
    input.readinput(inPC, iimage, SP, dimage );
    regis[29] = SP;

    FILE *snapshot, *report;
    snapshot = fopen("snapshot.rpt","w");
    report = fopen("report.rpt","w");

    Decode code;
    PC = inPC;

    ///

    if(argc>1){
        I_mem_size = atoi(argv[1]);
        D_mem_size = atoi(argv[2]);
        I_page_size = atoi(argv[3]);
        D_page_size = atoi(argv[4]);
        I_cache_size = atoi(argv[5]);
        I_block_size = atoi(argv[6]);
        I_set_way = atoi(argv[7]);
        D_cache_size = atoi(argv[8]);
        D_block_size = atoi(argv[9]);
        D_set_way = atoi(argv[10]);
    }


    unsigned int D_mem[D_mem_size/4];
    int dmemLRU[D_mem_size/D_page_size];    /// -1 : empty
    fill(dmemLRU,dmemLRU+(D_mem_size/D_page_size),-1);
    //for(int i=0; i<D_mem_size/D_page_size; i++) dmemLRU[i] = 0;
    int DPT[1024/D_page_size];
    resetarray(DPT, 1024/D_page_size);
    bool dptvalid[1024/D_page_size];
    fill(dptvalid, dptvalid+(1024/D_page_size) ,false);

    Cache D_cache(D_cache_size, D_block_size, D_set_way);
    TLB dtlb(1024/D_page_size/4, D_page_size);


    unsigned int I_mem[I_mem_size/4];
    int imemLRU[I_mem_size/I_page_size];    /// -1 : empty
    fill(imemLRU,imemLRU+(I_mem_size/I_page_size),-1);
    //for(int i=0; i<I_mem_size/I_page_size; i++) imemLRU[i] = -1;
    int IPT[1024/I_page_size];
    resetarray(IPT, 1024/I_page_size);
    bool iptvalid[1024/I_page_size];
    fill(iptvalid, iptvalid+(1024/I_page_size) ,false);

    Cache I_cache(I_cache_size, I_block_size, I_set_way);
    TLB itlb(1024/I_page_size/4, I_page_size);

    ///

    while(cycle<=500000){

        output(snapshot, cycle);
        //printreport(report, &I_cache, &D_cache, &itlb, &dtlb);
        //fprintf(report,"%d\n\n",cycle);

        code.getinstruc( iimage[ PC /4 ] );

        igetdata(PC,&I_cache,&itlb,I_mem,imemLRU,IPT,iptvalid);


        /*for(int i=0; i<I_cache.set_num; i++){ ///print i content
            for(int j=0; j<I_cache.n_way; j++) fprintf(report, "%d ",I_cache.blocks[i][j].tag*I_cache.set_num+i);
            fprintf(report,"\n");
       }
       for(int i=0; i<itlb.size; i++) fprintf(report, "%d ",itlb.blocks[i].tag);
        fprintf(report,"\n");*/


        //if(cycle==40 || cycle==41)cout<<code.instruc<<" "<<code.rd<<" "<<code.rs<<" "<<code.rt<<" "<<hex<<code.C<<" "<<dimage[code.rs]<<endl;

        if(code.instruc == add){

            regis[code.rd] = regis[code.rs] + regis[code.rt];

        }else if(code.instruc == addu){

            regis[code.rd] = regis[code.rs] + regis[code.rt];

        }else if(code.instruc == sub){

            regis[code.rd] = regis[code.rs] + (~regis[code.rt] + 1);

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

            regis[code.rt] = regis[code.rs] + sign_extend(code.C,16);

        }else if(code.instruc == addiu){

            regis[code.rt] = regis[code.rs] + sign_extend(code.C,16);

        }else if(code.instruc == lw){

           unsigned int addr = getmemaddr(regis[code.rs], code.C);
           regis[code.rt] = dimage[ addr/4 ];
           dgetdata(addr,&D_cache,&dtlb,D_mem,dmemLRU,DPT,dptvalid);

           /*fprintf(report,"\n");
           for(int i=0; i<D_cache.set_num; i++){
                for(int j=0; j<D_cache.n_way; j++) fprintf(report, "%d ",D_cache.blocks[i][j].tag*D_cache.set_num+i);
                fprintf(report,"\n");
           }
           for(int i=0; i<dtlb.size; i++) fprintf(report, "%d ",dtlb.blocks[i].tag);*/

        }else if(code.instruc == lh){


            unsigned int addr = getmemaddr(regis[code.rs], code.C);
            addr = addr/2;
            if(addr%2==0) regis[code.rt] = sign_extend(dimage[ addr/2 ] >> 16 , 16) ;
            else if(addr%2==1) regis[code.rt] = sign_extend(dimage[ addr/2 ]&0x0000ffff,16);

            dgetdata(addr*2,&D_cache,&dtlb,D_mem,dmemLRU,DPT,dptvalid);



            /*fprintf(report,"\n");
            for(int i=0; i<D_cache.set_num; i++){
                for(int j=0; j<D_cache.n_way; j++) fprintf(report, "%d ",D_cache.blocks[i][j].tag*D_cache.set_num+i);
                fprintf(report,"\n");
           }
           for(int i=0; i<dtlb.size; i++) fprintf(report, "%d ",dtlb.blocks[i].tag);*/



        }else if(code.instruc == lhu){

            unsigned int addr = getmemaddr(regis[code.rs], code.C);
            addr = addr/2;
            if(addr%2==0) regis[code.rt] = dimage[ addr/2 ] >> 16 ;
            else if(addr%2==1) regis[code.rt] = dimage[ addr/2 ] & 0x0000ffff;

            dgetdata(addr*2,&D_cache,&dtlb,D_mem,dmemLRU,DPT,dptvalid);


            /*fprintf(report,"\n");
            for(int i=0; i<D_cache.set_num; i++){
                for(int j=0; j<D_cache.n_way; j++) fprintf(report, "%d ",D_cache.blocks[i][j].tag*D_cache.set_num+i);
                fprintf(report,"\n");
           }
           for(int i=0; i<dtlb.size; i++) fprintf(report, "%d ",dtlb.blocks[i].tag);*/



        }else if(code.instruc == lb){

            unsigned int addr = getmemaddr(regis[code.rs], code.C);
            if(addr%4==0) regis[code.rt] = sign_extend(dimage[addr/4] >> 24 ,8);
            else if(addr%4==1) regis[code.rt] = sign_extend(dimage[addr/4] >> 16 &0x000000ff ,8);
            else if(addr%4==2) regis[code.rt] = sign_extend(dimage[addr/4] >> 8 &0x000000ff ,8);
            else if(addr%4==3) regis[code.rt] = sign_extend(dimage[addr/4] & 0x000000ff ,8);

            dgetdata(addr,&D_cache,&dtlb,D_mem,dmemLRU,DPT,dptvalid);


            /*fprintf(report,"\n");
            for(int i=0; i<D_cache.set_num; i++){
                for(int j=0; j<D_cache.n_way; j++) fprintf(report, "%d ",D_cache.blocks[i][j].tag*D_cache.set_num+i);
                fprintf(report,"\n");
           }
           for(int i=0; i<dtlb.size; i++) fprintf(report, "%d ",dtlb.blocks[i].tag);*/


        }else if(code.instruc == lbu){

            unsigned int addr = getmemaddr(regis[code.rs], code.C);
            if(addr%4==0) regis[code.rt] = dimage[addr/4] >> 24;
            else if(addr%4==1) regis[code.rt] = dimage[addr/4] >> 16 & 0x000000ff;
            else if(addr%4==2) regis[code.rt] = dimage[addr/4] >> 8 & 0x000000ff;
            else if(addr%4==3) regis[code.rt] = dimage[addr/4] & 0x000000ff;

            dgetdata(addr,&D_cache,&dtlb,D_mem,dmemLRU,DPT,dptvalid);


            /*fprintf(report,"\n");
            for(int i=0; i<D_cache.set_num; i++){
                for(int j=0; j<D_cache.n_way; j++) fprintf(report, "%d ",D_cache.blocks[i][j].tag*D_cache.set_num+i);
                fprintf(report,"\n");
           }
           for(int i=0; i<dtlb.size; i++) fprintf(report, "%d ",dtlb.blocks[i].tag);*/


        }else if(code.instruc == sw){

            unsigned int addr = getmemaddr(regis[code.rs],code.C);
            dimage[addr/4] = regis[code.rt];
            dwritedata(addr,regis[code.rt],&D_cache,&dtlb,D_mem,dmemLRU,DPT,dptvalid);

            /*fprintf(report,"\n");
            for(int i=0; i<D_cache.set_num; i++){
                for(int j=0; j<D_cache.n_way; j++) fprintf(report, "%d ",D_cache.blocks[i][j].tag*D_cache.set_num+i);
                fprintf(report,"\n");
           }
           for(int i=0; i<dtlb.size; i++) fprintf(report, "%d ",dtlb.blocks[i].tag);*/

        }else if(code.instruc == sh){

            unsigned int addr = getmemaddr(regis[code.rs], code.C);
            addr = addr/2;

            if(addr%2==0){
                dimage[ addr/2 ] = (dimage[ addr/2 ]<<16) >>16;
                dimage[ addr/2 ] = ( (regis[code.rt]&0x0000ffff)<<16 ) |  dimage[ addr/2 ];
            }else if(addr%2==1){
                dimage[ addr/2 ] = (dimage[ addr/2 ]>>16) <<16;
                dimage[ addr/2 ] = ( regis[code.rt]&0x0000ffff ) |  dimage[ addr/2 ] ;
            }

            dgetdata(addr*2,&D_cache,&dtlb,D_mem,dmemLRU,DPT,dptvalid);


            /*fprintf(report,"\n");
            for(int i=0; i<D_cache.set_num; i++){
                for(int j=0; j<D_cache.n_way; j++) fprintf(report, "%d ",D_cache.blocks[i][j].tag*D_cache.set_num+i);
                fprintf(report,"\n");
           }
           for(int i=0; i<dtlb.size; i++) fprintf(report, "%d ",dtlb.blocks[i].tag);*/


        }else if(code.instruc == sb){

            unsigned int addr = getmemaddr(regis[code.rs],code.C);
            if(addr%4==0){
                dimage[addr/4] = ( dimage[addr/4]&0x00ffffff ) | (regis[code.rt]&0xff)<<24;
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

            dgetdata(addr,&D_cache,&dtlb,D_mem,dmemLRU,DPT,dptvalid);


            /*fprintf(report,"\n");
            for(int i=0; i<D_cache.set_num; i++){
                for(int j=0; j<D_cache.n_way; j++) fprintf(report, "%d ",D_cache.blocks[i][j].tag*D_cache.set_num+i);
                fprintf(report,"\n");
           }
           for(int i=0; i<dtlb.size; i++) fprintf(report, "%d ",dtlb.blocks[i].tag);*/


        }else if(code.instruc == lui){

            regis[code.rt] = code.C << 16;

        }else if(code.instruc == andi){

            regis[code.rt] =  regis[code.rs] & code.C ;

        }else if(code.instruc == ori){

            regis[code.rt] = regis[code.rs] | code.C;

        }else if(code.instruc == nori){

            regis[code.rt] = ~( regis[code.rs] | code.C );

        }else if(code.instruc == slti){

            unsigned int C = sign_extend(code.C,16);
            if( regis[code.rs]>>31 > C>>31 ) regis[code.rt] = 1;
            else if(regis[code.rs]>>31 < C>>31 ) regis[code.rt] = 0;
            else if(regis[code.rs]>>31 == 1 && C>>31 == 1){
                if( regis[code.rs] < C ) regis[code.rt] = 1;
                else regis[code.rt] = 0;
            }else if( regis[code.rs]>>31 == 0 && C>>31 == 0 ){
                if(regis[code.rs] < C ) regis[code.rt] = 1;
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

        PC+=4;
        cycle++;
    }


    printreport(report, &I_cache, &D_cache, &itlb, &dtlb);

    fclose(report);
    fclose(snapshot);

    return 0;
}
