#include "TLB.h"

using namespace std;

TLB::TLB(int s, int p_s){
    size = s;   //number of entries
    page_size = p_s/4;
    miss = 0;
    hit = 0;

    TLB_Unit t;
    blocks.assign(s,t);
}

int TLB::find(unsigned int vpn, int cycle){

    int result = -1;
    for(int i=0; i<size; i++){
        if(blocks[i].tag == vpn && blocks[i].valid){
            result = i;
            blocks[i].LRU = cycle;
            //hit++;
        }
    }

    //if(result==-1) miss++;
    return result;
}

void TLB::replace(unsigned int vpn, unsigned int ppn, int cycle){
    for(int i=0; i<size; i++){
        if(!blocks[i].valid){
            blocks[i].tag = vpn;
            blocks[i].ppn = ppn;
            blocks[i].LRU = cycle;
            blocks[i].valid = true;
            return;
        }
    }
    int oldest = cycle, j = -1;
    for(int i=0; i<size; i++){
        if(blocks[i].LRU < oldest){
            oldest = blocks[i].LRU;
            j = i;
        }
    }
    blocks[j].tag = vpn;
    blocks[j].ppn = ppn;
    blocks[j].LRU = cycle;
    blocks[j].valid = true;
}

