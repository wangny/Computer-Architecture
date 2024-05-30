#include <iostream>
#include "Cache.h"

using namespace std;

Cache::Cache(int t_s, int b_s, int n_way){
    total_size = t_s/4;
    block_size = b_s/4;
    this->n_way = n_way;
    set_num = total_size /block_size /n_way;
    miss = 0;
    hit = 0;

    Cache_Unit t(block_size);
    vector<Cache_Unit> tmp(n_way, t);
    blocks.assign(set_num, tmp);
    //for(int i=0; i<set_num; i++) blocks.push_back(tmp);
}


pair<int,int> Cache::find(unsigned int phyaddr, int cycle, bool real){
    unsigned int block_num = phyaddr % set_num;
    pair<int,int> result (-1,-1);

    result.first = block_num;

    for(int i=0; i<n_way; i++){
        if(blocks[block_num][i].tag == phyaddr/set_num ){
            if(blocks[block_num][i].valid){

                if(real){   //false if not really accessing cache
                    blocks[block_num][i].MRU = 1;

                    bool full = true;
                    for(int j=0; j<n_way; j++) if(blocks[block_num][j].MRU==0) full = false;

                    if(full){
                        for(int j=0; j<n_way; j++) blocks[block_num][j].MRU=0;
                        blocks[block_num][i].MRU = 1;
                    }
                }

                result.second = i;
                //hit++;
            }

        }
    }

    //if(result.second == -1) miss++;
    return result;
}

void Cache::replace(unsigned int phyaddr, unsigned int* data, int cycle){
    unsigned int block_num = phyaddr % set_num;

    int t = -1;
    for(int i=0; i<n_way; i++){
        if(!blocks[block_num][i].valid){
            t = i;
            break;
        }
    }

    if(t<0){
        for(int i=0; i<n_way; i++){
            if(blocks[block_num][i].MRU == 0){
                t = i;
                break;
            }
        }
    }

    if(t<0){
        blocks[block_num][0].tag = phyaddr / set_num;
        blocks[block_num][0].data.clear();
        blocks[block_num][0].MRU = 1;
        blocks[block_num][0].valid = true;
        for(int i=0; i<block_size/4; i++) blocks[block_num][0].data.push_back(data[i]);

    }else{
        int j = -1;
        for(int i=0; i<n_way; i++){
            if(i!=t && blocks[block_num][i].MRU == 0){
                j = i;
                break;
            }
        }

        if(j < 0)
            for(int i=0; i<n_way; i++) blocks[block_num][i].MRU = 0;

        blocks[block_num][t].tag = phyaddr / set_num;
        blocks[block_num][t].data.clear();
        blocks[block_num][t].MRU = 1;
        blocks[block_num][t].valid = true;
        for(int i=0; i<block_size/4; i++) blocks[block_num][t].data.push_back(data[i]);
    }


}

