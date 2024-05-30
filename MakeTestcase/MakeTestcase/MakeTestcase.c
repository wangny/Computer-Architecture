#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define OP_FUNC  0x00

#define ADDI  0x08
#define LW    0x23
#define LH    0x21
#define LHU   0x25
#define LB    0x20
#define LBU   0x24
#define SW    0x2B
#define SH    0x29
#define SB    0x28
#define LUI   0x0F
#define ANDI  0x0C
#define ORI   0x0D
#define NORI  0x0E
#define SLTI  0x0A
#define BEQ   0x04
#define BNE   0x05

#define J     0x02
#define JAL   0x03

#define HALT  0x3F

#define ADD  0x20
#define SUB  0x22
#define AND  0x24
#define OR   0x25
#define XOR  0x26
#define NOR  0x27
#define NAND 0x28
#define SLT  0x2A
#define SLL  0x00
#define SRL  0x02
#define SRA  0x03
#define JR   0x08

typedef unsigned int u32;

u32 MIPS;

void mapOpcode(char []);
int getReg(char str[]);
u32 R_instr(int,int,int,int,int,int);
u32 I_instr(int,int,int,int);
u32 J_instr(int,char[]);
void divideInstr(char*);
void storeLabel(char[]);
int branchPC(char []);
void cutLine(char []);
void reverseU32(u32,FILE*);
int StrToHex(char []);
void D_image();

char arg[3][10];
int n_line=0,PC_init=-1,PC_offset=0;
char label[1100][100],instr[1100][100];
FILE *output;

int main ()
{
    FILE *I_file=fopen("I_image.txt","r");
    int i;
    char buff[512];

    output = fopen("iimage.bin","wb");

    if (I_file==NULL)
    {
        printf("Can not find I_image.txt!");
        return 1;
    }

    fgets(buff,512,I_file);
    sscanf(buff,"%s\n",buff);
    PC_init=StrToHex(buff);
    reverseU32((unsigned int)PC_init,output);
    printf("PC_init = 0x%08x\n",PC_init);
    while (fgets(buff,512,I_file)!=NULL)
    {
        cutLine(buff);
    }
    printf("total %d instructions.\n",n_line);
    reverseU32((unsigned int)n_line,output);
    for (i=0;i<n_line;i++)
    {
        PC_offset=i;
        divideInstr(instr[i]);
    }
    D_image();
    return 0;
}

void D_image()
{
    char buff[512];
    u32 data[1100];
    int n_data=0,i;
    FILE *D_file=fopen("D_image.txt","r");
    FILE *out = fopen("dimage.bin","wb");

    if (D_file==NULL)
    {
        printf("Can't find D_image.txt");
        return;
    }
    else
        printf("Loading D_image.txt...\n");
    fgets(buff,512,D_file);
    sscanf(buff,"%s\n",buff);
    printf("$sp = %s\n",buff);
    reverseU32(StrToHex(buff),out);
    while (fgets(buff,512,D_file)!=NULL)
    {
        sscanf(buff,"%s\n",buff);
        data[n_data++]=StrToHex(buff);
    }
    reverseU32(n_data,out);
    for (i=0;i<n_data;i++)
    {
        printf("0x%08x\n",data[i]);
        reverseU32(data[i],out);
    }
}

int StrToHex(char str[])
{
    u32 sum=0;
    int i,len=strlen(str)-2;
    for (i=0;i<len;i++)
    {
        if (len-i>0)
        {
            if (str[2+i]-65>6)
                sum=sum+(str[2+i]-87)*pow(16,len-i-1);
            else if (str[2+i]-48>9)
                sum=sum+(str[2+i]-55)*pow(16,len-i-1);
            else
                sum=sum+(str[2+i]-48)*pow(16,len-i-1);
        }
    }
    return sum;
}

void reverseU32(u32 str,FILE* file)
{
    unsigned char buff[1];
    u32 mask;
    int i;

    for (i=0;i<4;i++)
    {
        mask=0x000000ff<<(i*8);
        buff[0]=(str&mask)>>(i*8);
        printf("%08x\n",buff[0]);
        fwrite(buff, sizeof(unsigned char), 1,file);
    }
}

void cutLine(char line[])
{
    strcpy(label[n_line],strtok(line," "));
    strcpy(instr[n_line],strtok(NULL,"\n"));
    //printf("%s = %s\n",label[n_line],instr[n_line]);
    n_line++;
}

int findLabel(char str[])
{
    int i;

    for (i=0;i<n_line;i++)
    {
        if (!strcmp(label[i],"."))
            continue;
        if (!strcmp(label[i],str))
        {
            printf("[%s] = 0x%08x\n",str,PC_init+4*i);
            return PC_init+4*i;
        }
    }
    printf("Can't find label = %s\n",str);
    exit(1);
    return -1;
}

void divideInstr(char* instruction)
{

    int n_arg=0;
    char * pch,op[10],tmp[100],cut[100]=" ,()\n";

    strcpy(tmp,instruction);

    pch = strtok(instruction,cut);
    strcpy(op,pch);

    while (pch!=NULL)
    {
        pch = strtok (NULL,cut);

        if (pch!=NULL)
        {
            printf("arg = [%s]\n",pch);
            strcpy(arg[n_arg++],pch);
        }
    }
    mapOpcode(op);
    printf("%s = 0x%08X\n",tmp,MIPS);
    reverseU32((unsigned int)MIPS,output);
}

void mapOpcode(char op[])
{
    MIPS=0;
    if (!strcmp(op,"add"))
        MIPS=R_instr(0,getReg(arg[1]),getReg(arg[2]),getReg(arg[0]),0,ADD);
    else if (!strcmp(op,"sub"))
        MIPS=R_instr(0,getReg(arg[1]),getReg(arg[2]),getReg(arg[0]),0,SUB);
    else if (!strcmp(op,"and"))
        MIPS=R_instr(0,getReg(arg[1]),getReg(arg[2]),getReg(arg[0]),0,AND);
    else if (!strcmp(op,"or"))
        MIPS=R_instr(0,getReg(arg[1]),getReg(arg[2]),getReg(arg[0]),0,OR);
    else if (!strcmp(op,"xor"))
        MIPS=R_instr(0,getReg(arg[1]),getReg(arg[2]),getReg(arg[0]),0,XOR);
    else if (!strcmp(op,"nor"))
        MIPS=R_instr(0,getReg(arg[1]),getReg(arg[2]),getReg(arg[0]),0,NOR);
    else if (!strcmp(op,"nand"))
        MIPS=R_instr(0,getReg(arg[1]),getReg(arg[2]),getReg(arg[0]),0,NAND);
    else if (!strcmp(op,"slt"))
        MIPS=R_instr(0,getReg(arg[1]),getReg(arg[2]),getReg(arg[0]),0,SLT);
    else if (!strcmp(op,"sll"))
        MIPS=R_instr(0,0,getReg(arg[1]),getReg(arg[0]),atoi(arg[2]),SLL);
    else if (!strcmp(op,"srl"))
        MIPS=R_instr(0,0,getReg(arg[1]),getReg(arg[0]),atoi(arg[2]),SRL);
    else if (!strcmp(op,"sra"))
        MIPS=R_instr(0,0,getReg(arg[1]),getReg(arg[0]),atoi(arg[2]),SRA);
    else if (!strcmp(op,"jr"))
        MIPS=R_instr(0,getReg(arg[0]),0,0,0,JR);
    else if (!strcmp(op,"addi"))
        MIPS=I_instr(ADDI,getReg(arg[1]),getReg(arg[0]),atoi(arg[2]));
    else if (!strcmp(op,"lw"))
        MIPS=I_instr(LW,getReg(arg[2]),getReg(arg[0]),atoi(arg[1]));
    else if (!strcmp(op,"lh"))
        MIPS=I_instr(LH,getReg(arg[2]),getReg(arg[0]),atoi(arg[1]));
    else if (!strcmp(op,"lhu"))
        MIPS=I_instr(LHU,getReg(arg[2]),getReg(arg[0]),atoi(arg[1]));
    else if (!strcmp(op,"lb"))
        MIPS=I_instr(LB,getReg(arg[2]),getReg(arg[0]),atoi(arg[1]));
    else if (!strcmp(op,"lbu"))
        MIPS=I_instr(LBU,getReg(arg[2]),getReg(arg[0]),atoi(arg[1]));
    else if (!strcmp(op,"sw"))
        MIPS=I_instr(SW,getReg(arg[2]),getReg(arg[0]),atoi(arg[1]));
    else if (!strcmp(op,"sh"))
        MIPS=I_instr(SH,getReg(arg[2]),getReg(arg[0]),atoi(arg[1]));
    else if (!strcmp(op,"sb"))
        MIPS=I_instr(SB,getReg(arg[2]),getReg(arg[0]),atoi(arg[1]));
    else if (!strcmp(op,"lui"))
        MIPS=I_instr(LUI,0,getReg(arg[0]),StrToHex(arg[1]));
    else if (!strcmp(op,"andi"))
        MIPS=I_instr(ANDI,getReg(arg[1]),getReg(arg[0]),StrToHex(arg[2]));
    else if (!strcmp(op,"ori"))
        MIPS=I_instr(ORI,getReg(arg[1]),getReg(arg[0]),StrToHex(arg[2]));
    else if (!strcmp(op,"nori"))
        MIPS=I_instr(NORI,getReg(arg[1]),getReg(arg[0]),StrToHex(arg[2]));
    else if (!strcmp(op,"slti"))
        MIPS=I_instr(SLTI,getReg(arg[1]),getReg(arg[0]),atoi(arg[2]));
    else if (!strcmp(op,"beq"))
        MIPS=I_instr(BEQ,getReg(arg[0]),getReg(arg[1]),branchPC(arg[2]));
    else if (!strcmp(op,"bne"))
        MIPS=I_instr(BNE,getReg(arg[0]),getReg(arg[1]),branchPC(arg[2]));
    else if (!strcmp(op,"j"))
        MIPS=J_instr(J,arg[0]);
    else if (!strcmp(op,"jal"))
        MIPS=J_instr(JAL,arg[0]);
    else if (!strcmp(op,"halt"))
        MIPS=0xffffffff;
}

int branchPC(char label[])
{
    int targetPC=findLabel(label);
    return (targetPC-(PC_init+(PC_offset+1)*4))/4;
}

int getReg(char str[])
{
    return  atoi(str+1);
}

u32 J_instr(int opcode,char label[])
{
    u32 instr=0;
    instr=opcode;
    instr=(instr<<26)|(findLabel(label)<<4>>6);
    return instr;
}

u32 R_instr(int opcode,int rs,int rt,int rd,int shamt,int func)
{
    u32 instr=0;
    instr=instr|opcode;
    instr=((((instr<<5|rs)<<5|rt)<<5|rd)<<5|shamt)<<6|func;

    return instr;
}

u32 I_instr(int opcode,int rs,int rt,int immediate)
{
    u32 instr=0;
    instr=instr|opcode;
    instr=((instr<<5|rs)<<5|rt)<<16|(immediate&0x0000ffff);
    return instr;
}
