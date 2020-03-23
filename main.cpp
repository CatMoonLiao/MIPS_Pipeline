#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

struct IFID{
    int PC;
    string ins;
    bool zero;
};
struct IDEX{
    int readData1;
    int readData2;
    string signext;
    int rs;
    int rd;
    int rt;
    char ctrlsignal[9];
    string ALUctr;
    bool zero;

};
struct EXMEM{
    int ALUout;
    int WriteData;
    int rtrd;
    char ctrlsignal[5];
    bool zero;
};
struct MEMWB{
    int readData;
    int ALUout;
    int rtrd;
    char ctrlsignal[2];
    bool zero;
};
struct circuit{
    IFID ifid;
    IDEX idex;
    EXMEM exmem;
    MEMWB memwb;
    int cycleC;
};
int registers[10]={0};
int memory[5]={0};

void initial();
int stringb2dec(string s);
void pipeline(vector<string> inslist,int time);
bool all_empty(circuit C);
void output(circuit C,int time);

int main()
{
    int time=1;
    while(time<=4){
        /**ÅªÀÉ**/
        vector<string> inslist;
        fstream file;
        switch(time){
        case 1:
            file.open("General",ios::in);
            break;
        case 2:
            file.open("Datahazard",ios::in);
            break;
        case 3:
            file.open("Lwhazard",ios::in);
            break;
        case 4:
            file.open("Branchhazard",ios::in);
            break;
        }

        char buffer[32]={0};
        while(file.read(buffer,sizeof(buffer))){
            string temp="";
            for(int i=0;i<32;i++)
                temp+=buffer[i];
            inslist.push_back(temp);
        }
        file.close();

        pipeline(inslist,time);
        time++;
    }
    return 0;
}
/***check if instructions in pipeline were done.***/
bool all_empty(circuit C){
    if(C.exmem.zero&&C.memwb.zero&&C.idex.zero&&C.ifid.zero)
        return true;
    return false;
}

void pipeline(vector<string> inslist,int time){
    /**initial**/
    circuit C={0};
    initial();
    //zero means pipeline of this step is empty.
    C.exmem.zero=true;
    C.idex.zero=true;
    C.memwb.zero=true;
    C.ifid.zero=false;
    while(!all_empty(C)){
        /**write back**/
        C.cycleC++;
        if(!C.memwb.zero){
            if(C.memwb.ctrlsignal[0]=='1'&&C.memwb.rtrd!=0){
                if(C.memwb.ctrlsignal[1]=='1')
                    registers[C.memwb.rtrd]=C.memwb.readData;
                else
                    registers[C.memwb.rtrd]=C.memwb.ALUout;
            }
            C.memwb.zero=true;
        }

        /**access memory**/
        if(!C.exmem.zero){
            C.memwb.zero=false;
            C.memwb.ALUout=C.exmem.ALUout;
            C.memwb.rtrd=C.exmem.rtrd;
            //control signals
            C.memwb.ctrlsignal[0]=C.exmem.ctrlsignal[3];
            C.memwb.ctrlsignal[1]=C.exmem.ctrlsignal[4];
            //load
            if(C.exmem.ctrlsignal[1]=='1')
                C.memwb.readData=memory[C.exmem.ALUout/4];
            else
                C.memwb.readData=0;
            //store
            if(C.exmem.ctrlsignal[2]=='1')
                memory[C.exmem.ALUout/4]=C.exmem.WriteData;
            C.exmem.zero=true;
        }
        else{
            C.memwb={0};
            C.memwb.zero=true;
        }

        /**Execute**/
        bool load=false;//for load hazard
        if(!C.idex.zero){
            C.exmem.zero=false;
            C.exmem.ctrlsignal[0]=C.idex.ctrlsignal[4];
            C.exmem.ctrlsignal[1]=C.idex.ctrlsignal[5];
            C.exmem.ctrlsignal[2]=C.idex.ctrlsignal[6];
            C.exmem.ctrlsignal[3]=C.idex.ctrlsignal[7];
            C.exmem.ctrlsignal[4]=C.idex.ctrlsignal[8];
            //register distination
            if(C.idex.ctrlsignal[0]=='1')
                C.exmem.rtrd=C.idex.rd;
            else
                C.exmem.rtrd=C.idex.rt;

            //ALU
            int source;
            if(C.idex.ctrlsignal[3]=='0')//R type
                source=C.idex.readData2;
            else//I type
                source=stringb2dec(C.idex.signext);


            if(C.idex.ALUctr=="000")//and
                C.exmem.ALUout=C.idex.readData1&source;
            else if(C.idex.ALUctr=="010"){//add
                C.exmem.ALUout=C.idex.readData1+source;
                if(C.idex.ctrlsignal[1]=='0'){
                        load=true;
                }
            }
            else if(C.idex.ALUctr=="110"){//sub
                C.exmem.ALUout=C.idex.readData1-source;
                if(C.idex.ctrlsignal[1]=='0'&& C.exmem.ALUout==0)//beq taken
                    //beacause C.ifid.PC is PC of the ins. which is IF this time
                    //so need to -4
                    C.ifid.PC+=stringb2dec(C.idex.signext)*4-4;
            }
            else if(C.idex.ALUctr=="001")//or
                C.exmem.ALUout=C.idex.readData1|source;
            else if(C.idex.ALUctr=="111"){//slt
                if(C.idex.readData1<C.idex.readData2)
                    C.exmem.ALUout=1;
                else
                    C.exmem.ALUout=0;
            }

            //write Data
            C.exmem.WriteData=C.idex.readData2;
            C.idex.zero=true;
        }
        else{
            C.exmem={0};
            C.exmem.zero=true;
        }

        /**ins. Decode**/
        bool bubble=false;//for load hazard
        bool flush=false;//for beq success
        if(!C.ifid.zero && C.ifid.PC!=0){
            C.idex.zero=false;
            bool branch=false;
            string op=C.ifid.ins.substr(0,6);
            //control signal
            if(op=="000000"){
                C.idex.ctrlsignal[0]='1';
                C.idex.ctrlsignal[1]='1';
                C.idex.ctrlsignal[2]='0';
                C.idex.ctrlsignal[3]='0';
                C.idex.ctrlsignal[4]='0';
                C.idex.ctrlsignal[5]='0';
                C.idex.ctrlsignal[6]='0';
                C.idex.ctrlsignal[7]='1';
                C.idex.ctrlsignal[8]='0';
                string func=C.ifid.ins.substr(26,6);
                if(func=="100000")
                    C.idex.ALUctr="010";
                else if(func=="100010")
                    C.idex.ALUctr="110";
                else if(func=="100100")
                    C.idex.ALUctr="000";
                else if(func=="100101")
                    C.idex.ALUctr="001";
                else if(func=="101010")
                    C.idex.ALUctr="111";
            }
            else if(op=="001000"){//addi
                C.idex.ALUctr="010";
                C.idex.ctrlsignal[0]='0';
                C.idex.ctrlsignal[1]='0';
                C.idex.ctrlsignal[2]='0';
                C.idex.ctrlsignal[3]='1';
                C.idex.ctrlsignal[4]='0';
                C.idex.ctrlsignal[5]='0';
                C.idex.ctrlsignal[6]='0';
                C.idex.ctrlsignal[7]='1';
                C.idex.ctrlsignal[8]='0';
            }
            else if(op=="001100"){//andi
                C.idex.ALUctr="000";
                C.idex.ctrlsignal[0]='0';
                C.idex.ctrlsignal[1]='1';
                C.idex.ctrlsignal[2]='1';
                C.idex.ctrlsignal[3]='1';
                C.idex.ctrlsignal[4]='0';
                C.idex.ctrlsignal[5]='0';
                C.idex.ctrlsignal[6]='0';
                C.idex.ctrlsignal[7]='1';
                C.idex.ctrlsignal[8]='0';
            }
            else if(op=="000100"){//beq
                C.idex.ALUctr="110";
                C.idex.ctrlsignal[0]='0';//don't care
                C.idex.ctrlsignal[1]='0';
                C.idex.ctrlsignal[2]='1';
                C.idex.ctrlsignal[3]='0';
                C.idex.ctrlsignal[4]='1';
                C.idex.ctrlsignal[5]='0';
                C.idex.ctrlsignal[6]='0';
                C.idex.ctrlsignal[7]='0';
                C.idex.ctrlsignal[8]='0';//don't care
                branch=true;
            }
            else if(op=="100011"){//lw
                C.idex.ALUctr="010";
                C.idex.ctrlsignal[0]='0';
                C.idex.ctrlsignal[1]='0';
                C.idex.ctrlsignal[2]='0';
                C.idex.ctrlsignal[3]='1';
                C.idex.ctrlsignal[4]='0';
                C.idex.ctrlsignal[5]='1';
                C.idex.ctrlsignal[6]='0';
                C.idex.ctrlsignal[7]='1';
                C.idex.ctrlsignal[8]='1';
            }
            else if(op=="101011"){//sw
                C.idex.ALUctr="010";
                C.idex.ctrlsignal[0]='0';//don't care
                C.idex.ctrlsignal[1]='0';
                C.idex.ctrlsignal[2]='0';
                C.idex.ctrlsignal[3]='1';
                C.idex.ctrlsignal[4]='0';
                C.idex.ctrlsignal[5]='0';
                C.idex.ctrlsignal[6]='1';
                C.idex.ctrlsignal[7]='0';
                C.idex.ctrlsignal[8]='0';//don't care
            }
            //register
            string Rs=C.ifid.ins.substr(6,5);
            string Rt=C.ifid.ins.substr(11,5);
            string Rd=C.ifid.ins.substr(16,5);
            C.idex.rs=stringb2dec(Rs);
            C.idex.rd=stringb2dec(Rd);
            C.idex.rt=stringb2dec(Rt);
            //sign extend
            C.idex.signext=C.ifid.ins.substr(16,16);
            //read Data
            C.idex.readData1=registers[C.idex.rs];
            C.idex.readData2=registers[C.idex.rt];
            C.ifid.zero=true;

            if(branch){//check branch hazard
                if(C.idex.readData1==C.idex.readData2)
                    flush=true;
            }
            if(load){//load hazard>>stalling
                if(C.idex.rt==C.exmem.rtrd || C.idex.rs==C.exmem.rtrd)
                    bubble=true;
                    for(int i=0;i<9;i++)//control signals all equal 0
                        C.idex.ctrlsignal[i]=0;
                    C.idex.zero=true;
            }

            //Data hazard>>forwarding from MEM
            //lw R-type
            if(C.memwb.ctrlsignal[0]=='1'&&C.memwb.rtrd!=0){
                if(C.memwb.rtrd==C.idex.rt){
                    if(C.memwb.ctrlsignal[1]=='0')//R type
                        C.idex.readData2=C.memwb.ALUout;
                    else if(C.memwb.ctrlsignal[1]=='1')//lw
                        C.idex.readData2=C.memwb.readData;
                }
                else if(C.memwb.rtrd==C.idex.rs){
                    if(C.memwb.ctrlsignal[1]=='0')//R type
                        C.idex.readData1=C.memwb.ALUout;
                    else if(C.memwb.ctrlsignal[1]=='1')//lw
                        C.idex.readData1=C.memwb.readData;
                }
            }

            //Data hazard>>forwarding from EX
            //R-type
            if(C.exmem.ctrlsignal[3]=='1'&&C.exmem.ctrlsignal[4]=='0'&&C.exmem.rtrd!=0){
                if(C.exmem.rtrd==C.idex.rt)
                    C.idex.readData2=C.exmem.ALUout;
                else if(C.exmem.rtrd==C.idex.rs)
                    C.idex.readData1=C.exmem.ALUout;
            }
            //if EX and MEM forward same, use EX's value
            //so put EX after MEM
        }
        else{
            C.idex={0};
            C.idex.zero=true;
        }

        /**ins. fetch**/
        if(bubble){//redo IF
            C.ifid.zero=false;
        }
        else if((C.ifid.PC/4)<inslist.size()&&!flush){
            C.ifid.PC+=4;
            C.ifid.zero=false;
            C.ifid.ins=inslist.at(C.ifid.PC/4-1);
        }
        else{//flush || ins. done
            C.ifid.PC+=4;
            C.ifid.ins="00000000000000000000000000000000";
            C.ifid.zero=true;

        }
        //output txt
        output(C,time);
    }

}

void output(circuit C,int time){
    fstream file;
    switch(time){
    case 1:
        file.open("genResult.txt",ios::app);
        break;
    case 2:
        file.open("dataResult.txt",ios::app);
        break;
    case 3:
        file.open("loadResult.txt",ios::app);
        break;
    case 4:
        file.open("branchResult.txt",ios::app);
        break;
    }
    file<<"CC "<<C.cycleC<<":\n";
    file<<"\nRegisters:\n";
    for(int i=0;i<10;i++)
        file<<"$"<<i<<": "<<registers[i]<<"\n";

    file<<"\nData memory:";
    file<<"\n0x00: "<<memory[0];
    file<<"\n0x04: "<<memory[1];
    file<<"\n0x08: "<<memory[2];
    file<<"\n0x0C: "<<memory[3];
    file<<"\n0x10: "<<memory[4]<<"\n";

    file<<"\nIF/ID :";
    file<<"\nPC\t\t"<<C.ifid.PC;
    file<<"\nInstruction\t"<<C.ifid.ins<<"\n";

    file<<"\nID/EX :";
    file<<"\nReadData1\t"<<C.idex.readData1;
    file<<"\nReadData2\t"<<C.idex.readData2;
    if(C.idex.signext=="")
        file<<"\nsign_ext\t0";
    else
        file<<"\nsign_ext\t"<<stringb2dec(C.idex.signext);
    file<<"\nRs\t\t"<<C.idex.rs;
    file<<"\nRt\t\t"<<C.idex.rt;
    file<<"\nRd\t\t"<<C.idex.rd;
    file<<"\nControl signals\t";
    for(int i=0;i<9;i++){
        if(C.idex.ctrlsignal[i]==0)
            file<<'0';
        else
            file<<C.idex.ctrlsignal[i];
    }
    file<<"\n";

    file<<"\nEX/MEM :";
    file<<"\nALUout\t\t"<<C.exmem.ALUout;
    file<<"\nWriteData\t"<<C.exmem.WriteData;
    file<<"\nRt/Rd\t\t"<<C.exmem.rtrd;
    file<<"\nControl signals\t";
    for(int i=0;i<5;i++){
        if(C.exmem.ctrlsignal[i]==0)
            file<<'0';
        else
            file<<C.exmem.ctrlsignal[i];
    }
    file<<"\n";

    file<<"\nMEM/WB :";
    file<<"\nReadData\t"<<C.memwb.readData;
    file<<"\nAlUout\t\t"<<C.memwb.ALUout;
    file<<"\nRt/Rd\t\t"<<C.memwb.rtrd;
    file<<"\nControl signals\t";
    for(int i=0;i<2;i++){
        if(C.memwb.ctrlsignal[i]==0)
            file<<'0';
        else
            file<<C.memwb.ctrlsignal[i];
    }
    file<<"\n";

    file<<"=================================================================\n";
    file.close();
}

/***change binarycode string to decimal integer***/
int stringb2dec(string s){
    int sum=0,base=1;
    for(int i=s.length()-1;i>=0;i--){
        if(s[i]=='1')
            sum+=base;
        base*=2;
    }
    return sum;
}

/***initial memory and registers***/
void initial(){
    registers[0]=0;
    registers[1]=9;
    registers[2]=5;
    registers[3]=7;
    registers[4]=1;
    registers[5]=2;
    registers[6]=3;
    registers[7]=4;
    registers[8]=5;
    registers[9]=6;
    memory[0]=5;
    memory[1]=9;
    memory[2]=4;
    memory[3]=8;
    memory[4]=7;
}
