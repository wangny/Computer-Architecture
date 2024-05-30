#include <vector>

class TLB_Unit{

public:
    bool valid;
    int LRU;
    unsigned int tag;
    unsigned int ppn;

    TLB_Unit():valid(false),LRU(0),tag(0),ppn(0){};
};

class TLB{
public:
    int size;
    int page_size;
    int miss;
    int hit;
    std::vector<TLB_Unit> blocks;

    TLB(int, int);
    int find(unsigned int, int cycle);

    void replace(unsigned int, unsigned int, int cycle);

};
