#include <fstream>
#include "readfile.h"

using namespace std;

unsigned int Readfile::s_b_swap (unsigned int origin)
{
    unsigned int tmp[4]={0};
    tmp[3] = origin & 255;
    tmp[2] = origin >> 8 & 255;
    tmp[1] = origin >> 16 & 255;
    tmp[0] = origin >> 24 & 255;

    return tmp[0] | tmp[1]<<8 | tmp[2]<<16 | tmp[3]<<24;
}


void Readfile::readinput(unsigned int& PC, unsigned int iimage[256], unsigned int & SP, unsigned int dimage[256])
{
    FILE *image;
    image = fopen("iimage.bin","rb");
    fread(&PC, sizeof(unsigned int), 1, image);
    PC = s_b_swap(PC);

    unsigned int i_num;
    fread(&i_num, sizeof(unsigned int), 1, image);
    i_num = s_b_swap(i_num);

    int tmp[256];
    fread(tmp, sizeof(unsigned int), i_num, image);
    for(unsigned int i=0; i<i_num; i++){
        iimage[i+(PC/4)] = s_b_swap(tmp[i]);
    }
    fclose(image);


    FILE *dmage;
    dmage = fopen("dimage.bin","rb");
    fread(&SP, sizeof(unsigned int), 1, dmage);
    SP = s_b_swap(SP);

    unsigned int d_num;
    fread(&d_num, sizeof(unsigned int), 1, dmage);
    d_num = s_b_swap(d_num);

    fread(dimage, sizeof(unsigned int), d_num, dmage);
    for(unsigned int i=0; i<d_num; i++){
        dimage[i] = s_b_swap(dimage[i]);
    }
    fclose(dmage);

}

