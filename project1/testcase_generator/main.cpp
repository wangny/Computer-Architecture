#include <iostream>
#include <stdio.h>
#include <random>
#include <chrono>

using namespace std;

unsigned int s_b_swap (const unsigned int origin)
{
    unsigned int tmp[4]={0};
    tmp[3] = origin & 255;
    tmp[2] = origin >> 8 & 255;
    tmp[1] = origin >> 16 & 255;
    tmp[0] = origin >> 24 & 255;

    return tmp[0] | tmp[1]<<8 | tmp[2]<<16 | tmp[3]<<24;
}



int main()
{
    FILE *image;
    image = fopen("iimage.bin","rb");

    unsigned int iimage[256];
    fread(&iimage[0], sizeof(unsigned int), 1, image);
    iimage[0] = s_b_swap(iimage[0]);

    unsigned int i_num;
    fread(&i_num, sizeof(unsigned int), 1, image);
    iimage[1] = s_b_swap(i_num);
    fread(&iimage[2], sizeof(unsigned int), i_num, image);
    for(unsigned int i=2; i<i_num+2; i++){
        iimage[i] = s_b_swap(iimage[i]);
    }
    fclose(image);
    image = fopen("iimage.bin","wb");
    fwrite(iimage,sizeof(unsigned int),i_num+2,image);
    fclose(image);



    FILE *dmage;
    dmage = fopen("dimage.bin","rb");
    unsigned int dimage[256];
    fread(&dimage[0], sizeof(unsigned int), 1, dmage);
    dimage[0] = s_b_swap(dimage[0]);

    unsigned int d_num;
    fread(&d_num, sizeof(unsigned int), 1, dmage);
    dimage[1] = s_b_swap(d_num);
    fread(&dimage[2], sizeof(unsigned int), d_num, dmage);
    for(unsigned int i=2; i<d_num+2; i++){
        dimage[i] = s_b_swap(dimage[i]);
    }
    fclose(dmage);
    dmage = fopen("dimage.bin","wb");
    fwrite(dimage,sizeof(unsigned int),d_num+2,dmage);
    fclose(dmage);


    /*FILE* image;
    FILE* dmage;

    dmage = fopen("dimage.bin","wb");
    unsigned int buf[256];
    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    minstd_rand0 generator (seed);

    buf[0] = 0x03000000;
    buf[1] = 0x20000000;

    for(int i=2; i<0x22; i++) buf[i] = i;

    fwrite(buf,sizeof(unsigned int),0x22,dmage);

    unsigned int buff[256];
    buff[0] = 0x04000000;
    buff[1] = 0x05000000;


    buff[2] = 0x8C020004;  ///lw
    buff[3] = 0x00210820; ///add !! correct!
    buff[4] = 0xA4020001;
    buff[5] = 0xAC020001;   ///sw
    //buff[3] = 0x4C030001;   /// 0100 1100 0000 0011
    //buff[4] = 0x4C030002; /// 0x01 00 11 00 000 0 0010 0000 0000 0000 0001
    //buff[5] = 0x4C030004;
    buff[6] = 0xFFFFFFFF;
    //buff[6] = 0x00643813; ///and
    //buff[7] = 0x00C35015; ///or



    for(int i=2; i<7; i++) buff[i] = s_b_swap(buff[i]);

    image = fopen("iimage.bin","wb");
    fwrite(buff,sizeof(unsigned int),7,image);*/

    return 0;
}
