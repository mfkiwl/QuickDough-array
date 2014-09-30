// =============================================================================
// Fucnction Description:
// Generally, it is the definition of global constants and global variables. 
//
// Version:
// Nov 23th 2011, initil version
// Sep 5th 2014, Clean up the coding style  
//
// Author:
// Cheng Liu
// st.liucheng@gmail.com
// E.E.E department, The University of Hong Kong
//
// =============================================================================

#ifndef _GLOBAL_DEF_H_
#define _GLOBAL_DEF_H_

#include <map>
#include <list>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sstream>
#include <cmath>

// Global definition
#define NaN -1
#define ERROR(FMT, ARG...) do {fprintf(stderr,"File=%s, Line=%d: "FMT" \n",__FILE__, __LINE__,##ARG); exit(1);} while(0)
#define PRINT(FMT, ARG...) do {fprintf(stdout,"File=%s, Line=%d  "FMT" \n",__FILE__, __LINE__,##ARG);} while(0)

enum Topology{
    Torus, Mesh, Full_Connect, Customized
};

enum Operand_State {
    In_IO_Buffer, Unavail, Avail
};

enum PE_Sel_Filter_Type{
    Dist_Filter, Mem_Util_Filter, DSP_Util_Filter, Write_Mem_Util_Filter, Input_Port_Util_Filter, Output_Port_Util_Filter, Inout_Port_Util_Filter
};

enum PE_Selection{
    Least_Recent_Used, Least_Ready_OP_Attached
};

enum Exe_Mode{
    Sim, Impl 
};

// High frequency, Less high frequency, Medium frequency, Low frequency
enum Pipeline_Intensity{
    HF, LHF, MF, LF, OLD
};

enum Routing_Alg{
    Dynamic_Dijkstra, Dynamic_XY, Static_XY, Static_Dijkstra 
};

enum IO_Placement{
    Sequential_Placement, Interleaving_Placement
};

enum Scheduling_Strategy{
    PE_Pref, OP_Pref, PE_OP_Together
};

enum Opcode{
    NC, MULADD, MULSUB, ADDADD, ADDSUB, SUBSUB, PHI, RSFAND, LSFADD, ABS, GT, LET, ANDAND
};

enum Operand_Type{
    INCONST, INVAR, UNUSED, OUTVAR, IM, IMOUT
};

std::ostream& operator<< (std::ostream &os, Opcode Inst_Opcode);
std::ostream& operator<< (std::ostream &os, Operand_Type OP_Type);

// ============================================================================
// Global variables
// ============================================================================
struct GL_Var{
    static int Print_Level;
    static int Verify_On;
    static int Random_Seed;
    static const std::map<Opcode, int> Opcode_To_Cost;

    static std::map<Opcode, int> Create_Map(){
        std::map<Opcode, int> Local_Map;
        std::string fName = "./config/opcode.txt";
        std::ifstream fHandle;
        fHandle.open(fName.c_str());
        if(!fHandle.is_open()){
            ERROR("Failed to open %s! \n", fName.c_str());
        }
        std::string Item_Key;
        int Item_Val;
        while(!fHandle.eof()){
            fHandle >> Item_Key;
            fHandle >> Item_Val;
            if(Item_Key == "MULADD"){
                Local_Map[MULADD] = Item_Val;
            }
            else if(Item_Key == "MULSUB"){
                Local_Map[MULSUB] = Item_Val;
            }
            else if(Item_Key == "ADDADD"){
                Local_Map[ADDADD] = Item_Val;
            }
            else if(Item_Key == "ADDSUB"){
                Local_Map[ADDSUB] = Item_Val;
            }
            else if(Item_Key == "SUBSUB"){
                Local_Map[SUBSUB] = Item_Val;
            }
            else if(Item_Key == "PHI"){
                Local_Map[PHI] = Item_Val;
            }
            else if(Item_Key == "RSFAND"){
                Local_Map[RSFAND] = Item_Val;
            }
            else if(Item_Key == "LSFADD"){
                Local_Map[LSFADD] = Item_Val;
            }
            else if(Item_Key == "ABS"){
                Local_Map[ABS] = Item_Val;
            }
            else if(Item_Key == "GT"){
                Local_Map[GT] = Item_Val;
            }
            else if(Item_Key == "LET"){
                Local_Map[LET] = Item_Val;
            }
            else if(Item_Key == "ANDAND"){
                Local_Map[ANDAND] = Item_Val;
            }
            else{
                ERROR("Unknown opcode in %s!\n", fName.c_str());
            }
        }
        fHandle.close();
        return Local_Map;
    }
};

int Opcode_To_Int(const Opcode &Inst_Opcode);
Opcode Str_To_Opcode(const std::string &Opcode_Str);
int OP_Compute(const Opcode &Inst_Opcode, const int &Src_Val0, const int &Src_Val1, const int &Src_Val2);
int Get_Opcode_Cost(const Opcode &Inst_Opcode);

#endif
