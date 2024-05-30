#include <vector>

class Cache_Unit{

public:
    bool valid;
    int MRU;
    unsigned int tag;
    std::vector<unsigned int> data;

    Cache_Unit(int b_size){
        valid = false;
        MRU = 0;
        tag = 0;
        data.assign(b_size,0);
    }


};

class Cache{
public:
    int total_size;
    int block_size;
    int n_way;
    int set_num;
    int miss, hit;
    //std::vector<int> MRU;

    std::vector< std::vector<Cache_Unit> > blocks;


    Cache(int t_s, int b_s, int n_way);

    std::pair<int,int> find(unsigned int , int, bool);

    void replace(unsigned int addr, unsigned int* data, int cycle);

    void resetMRU();
};
