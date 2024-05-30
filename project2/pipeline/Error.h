

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
        fprintf(this->file,"In cycle %d: Write $0 Error\n",cycle);
    }
    inline void addrover (int cycle){
        fprintf(this->file,"In cycle %d: Address Overflow\n",cycle);
        exit = true;
    }
    inline void overflow (int cycle){
        fprintf(this->file,"In cycle %d: Number Overflow\n",cycle);
    }
    inline void misalign (int cycle){
        fprintf(this->file,"In cycle %d: Misalignment Error\n",cycle);
        exit = true;
    }
    inline bool Exit(){return exit;}
};
