// =====================================================================================================================
// Algorithm Description:
// Implementation of the scheduling algorithm
//
// Update:
// Nov 29th 2011,
// Feb 24th 2012, Change the Scheduling algorithm due to New PE structure
// Mar 27th 2012, Pipeline the ALU operation
// May 29th 2012, Support 400MHz-16bit PE based CGRA
// July 17th 2012, Add IO model for accurate scheduling
// July 18th 2012, Add operation transmission history and reuse data stored in intermediate PE
// Sep 9th 2014, Update coding style and remove useless functionality. 
//
// Author:
// Cheng Liu
// st.liucheng@gmail.com, liucheng@eee.hku.hk
// E.E.E department, The University of Hong Kong
//
// =====================================================================================================================

#include "Scheduler.h"

Scheduler::Scheduler(Data_Flow_Graph * _DFG, Coarse_Grain_Recon_Arch* _CGRA){

    Load_Parameters();
    Init(_DFG, _CGRA);

}

void Scheduler::Load_Parameters(){

    std::string Config_fName = "./config/configure.txt";
    std::ifstream Config_fHandle(Config_fName.c_str());
    if(!Config_fHandle.is_open()){
        DEBUG1("Failed to open configure.txt!");
    }

    while(!Config_fHandle.eof()){
        std::string Config_Item_Key;
        Config_fHandle >> Config_Item_Key;
        if(Config_Item_Key == "IO_Placement_Scheme"){
            std::string Config_Item_Val;
            Config_fHandle >> Config_Item_Val;
            if(Config_Item_Val == "Sequential_Placement"){
                IO_Placement_Scheme = Sequential_Placement;
            }
            else if(Config_Item_Val == "Interleaving_Placement"){
                IO_Placement_Scheme = Interleaving_Placement;
            }
            else{
                DEBUG1("Unknown IO Placement Scheme!\n");
            }
        }
        else if(Config_Item_Key == "List_Scheduling_Strategy"){
            std::string Config_Item_Val;
            Config_fHandle >> Config_Item_Val;
            if(Config_Item_Val == "PE_Pref"){
                List_Scheduling_Strategy = PE_Pref;
            }
            else if(Config_Item_Val == "OP_Pref"){
                List_Scheduling_Strategy = OP_Pref;
            }
            else if(Config_Item_Val == "PE_OP_Combined"){
                List_Scheduling_Strategy = PE_OP_Combined;
            }
            else{
                DEBUG1("Unknown scheduling strategy!\n");
            }
        }
        else if(Config_Item_Key == "Load_Balance_Factor"){
            Config_fHandle >> Load_Balance_Factor;
        }
        else if(Config_Item_Key == "PE_Sel_Strategy"){
            std::string Config_Item_Val;
            Config_fHandle >> Config_Item_Val;
            if(Config_Item_Val == "Least_Recent_Used"){
                PE_Sel_Strategy = Least_Recent_Used;
            }
            else if(Config_Item_Val == "Least_Ready_OP_Attached"){
                PE_Sel_Strategy = Least_Ready_OP_Attached;
            }
        }
    }

}

void Scheduler::Init(Data_Flow_Graph* _DFG, Coarse_Grain_Recon_Arch* _CGRA){
    
    DFG = _DFG;
    CGRA = _CGRA;
    Scheduling_Complete_Time = 0;

}

void Scheduler::IO_Placing(){
    
    if(IO_Placement_Scheme == Sequential_Placement){
        std::vector<Operand*>::iterator Vit;
        for(Vit = DFG->OP_Array.begin(); Vit != DFG->OP_Array.end(); Vit++){
            if((((*Vit)->OP_Type == INCONST) || ((*Vit)->OP_Type == INVAR)) && ((*Vit)->OP_ID !=0)){
                (*Vit)->OP_Attribute.OP_Cost = 0;
                (*Vit)->OP_Attribute.OP_Exe_PE_ID = INT_MAX;
                (*Vit)->OP_Attribute.OP_Avail_Time = INT_MAX;
                (*Vit)->OP_Attribute.OP_State = In_IO_Buffer;
            }
        }
    }
    else if(IO_Placement_Scheme == Interleaving_Scheme){
        // To be added
        std::cout << "Interleaving scheme is not supported now! " << std::endl;
    }
    else {
        DEBUG1("Unknown IO placement!\n");
    }

}

void Scheduler::Scheduling(){

    std::string Trace_fName = "./result/trace.txt";
    fTrace.open(Trace_fName.c_str());
    if(!fTrace.is_open()){
        DEBUG1("Failed to open the trace.txt!");
    }

    time_t Start_Time, End_Time;
    Start_Time = clock();

    std::cout << "Operation scheduling starts!" << std::endl;
    if(List_Scheduling_Strategy == PE_Pref){
        List_Scheduling_PE_Pref();
    }
    else if(List_Scheduling_Strategy == OP_Pref){
        List_Scheduling_OP_Pref();
    }
    else if(List_Scheduling_Strategy == PE_OP_Combined){
        List_Scheduling_PE_OP_Combined();
    }
    else{
        DEBUG1("Unknown scheduling strategy!\n");
    }

    std::cout << "Operation scheduling is completed!" << std::endl;
    std::cout << "Kernel execution time: " << Scheduling_Complete_Time << " cycles" << std::endl;

    std::cout << "Start to dump the scheduling result for hardware implementation!" << std::endl;
    Scheduling_Stat();
    Data_Mem_Analysis();
    Inst_Mem_Dump_Coe();
    Inst_Mem_Dump_Mem();
    Scheduling_Result_Dump();
    IO_Buffer_Dump_Coe();
    Addr_Buffer_Dump_Mem();
    std::cout << "Scheduling result is dumpped! " << std::endl;

    End_Time = clock();
    std::cout << "Total compilation time: " << (double)(1.0*(End_Time-Start_Time)/CLOCKS_PER_SEC) << " seconds." << std::endl;

    fTrace.close();

}

void Scheduler::ListSchedulingAlgorithmPEOPTogether(){

    bool scheduling_completed=false;

    //InputOperationScheduling();
    list<int> OP_Ready_Set;
    OPReadySetInitialization(OP_Ready_Set);
    vector<int> executed_op_num;
    executed_op_num.resize(CGRA->CGRA_Scale);
    for(int i=0; i<CGRA->CGRA_Scale; i++){
        executed_op_num[i]=0;
    }

    while(!scheduling_completed){
        int selected_PE_id;
        int selected_op_id;

        //Choose an idle PE
        list<int> Candidates;
        int min_num=GLvar::maximum_operation_num;
        int max_num=0;
        for(int i=0; i<CGRA->CGRA_Scale; i++){
            if(min_num>executed_op_num[i]){
                min_num=executed_op_num[i];
            }
            if(max_num<executed_op_num[i]){
                max_num=executed_op_num[i];
            }
        }

        int std_num=min_num+(max_num-min_num)*GLvar::load_balance_factor;
        for(int i=0; i<CGRA->CGRA_Scale; i++){
            if((max_num-min_num)>min_num && executed_op_num[i]>std_num){
                continue;
            }
            else{
                Candidates.push_back(i);
            }
        }

        //selected_PE_id=LeastActivePESelection(Candidates);

        //Choose an operation that can be executed
        //selected_op_id=LeastCostOPSelection(selected_PE_id, OP_Ready_Set);
        PEOPPairSelection(selected_PE_id, selected_op_id, Candidates, OP_Ready_Set);

        executed_op_num[selected_PE_id]++;

        //Operation Execution
        vector<Vertex*>::iterator it;
        vector<int> Src_OP_IDs;
        vector<int> arrival_time;
        Src_OP_IDs.resize(3);
        arrival_time.resize(3);
        Vertex* tmp=DFG->OP_Array[selected_op_id];
        int j=0;
        for(it=tmp->parents.begin(); it!=tmp->parents.end(); it++){
            Src_OP_IDs[j]=(*it)->vertex_id;
            arrival_time[j]=FetchOP(Src_OP_IDs[j], selected_PE_id, Implementation);
            j++;
        }
        OperationExecution(selected_op_id, Src_OP_IDs, selected_PE_id, arrival_time, Implementation);

        //output operation that has not been executed should be moved to output PE
        if(DFG->OP_Array[selected_op_id]->vertex_type==OutputData){
            StoreDataInOutMem(selected_op_id);
        }

        //cout<<"ready list size="<<OP_Ready_Set.size()<<endl;
        //Update the operation that is ready for execution
        OPReadySetUpdate(OP_Ready_Set, selected_op_id);

        scheduling_completed=SchedulingIsCompleted();
    }

}

void Scheduler::Load_Balance_Filter(list<int> &Candidates){

    int Min_Executed_OP_Num = DFG->OP_Num;
    int Max_Executed_OP_Num = 0;
    std::list<int>::iterator Lit;
    for(Lit = Candidates.begin(); Lit != Candidates.end(); Lit++){
        if(Min_Executed_OP_Num > CGRA->PE_Array[*Lit]->Executed_OP_Num){
            Min_Executed_OP_Num = CGRA->PE_Array[*Lit]->Executed_OP_Num;
        }
        if(Max_Executed_OP_Num < CGRA->PE_Array[*Lit]->Executed_OP_Num){
            Max_Executed_OP_Num = CGRA->PE_Array[*Lit]->Executed_OP_Num;
        }
    }

    int Threshold_Num = Min_Executed_OP_Num + (Max_Executed_OP_Num - Min_Executed_OP_Num) * Load_Balance_Factor;
    for(Lit = Candidates.begin(); Lit != Candidates.end(); ){
        if(CGRA->PE_Array[*Lit]->Executed_OP_Num > 1.3*Min_Executed_OP_Num && DFG->OP_Array[*Lit]->Executed_OP_Num > Threshold_Num){
            Lit = Candidates.erase(Lit);
        }
        else{
            Lit++;
        }
    }

}

/* Try to remove the PEs that is recetly used from candidate list, because
 * they are more likely to be busy. */
void Scheduler::Recent_Busy_Filter(std::list<int> &Candidates){

    int Min_Active_Time = INT_MAX;
    int Max_Active_Time = 0;

    std::list<int>::iterator Lit;
    for(Lit = Candidates.begin(); Lit != Candidates.end(); Lit++){
        int Current_Active_Time = CGRA->PE_Array[*Lit]->Max_Active_Time;
        if(Current_Active_Time < Min_Active_Time){
            Min_Active_Time = Current_Active_Time;
        }
        if(Current_Active_Time > Max_Active_Time){
            Max_Active_Time = Current_Active_Time;
        }
    }

    int Threshold_Time = Min_Active_Time + Load_Balance_Factor * (Max_Active_Time - Min_Active_Time);
    for(Lit = Candidates.begin(); Lit != Candidates.end();){
        Current_Active_Time = CGRA->PE_Array[*Lit]->Max_Active_Time;
        if(Current_Active_Time <= Threshold_Time){
            Lit++;
        }
        else{
            Lit = Candidates.erase(Lit);
        }
    }

}

int Scheduler::Least_Ready_OP_Attached_Sel(const std::list<int> &Candidates, const std::list<int> &OP_Ready_Set){

    int Sel_PE_ID;
    std::vector<int> Attached_Ready_OP_Num;
    Attached_Ready_OP_Num.resize(CGRA->CGRA_Scale);
    for(int i=0; i<CGRA->CGRA_Scale; i++){
        Attached_Ready_OP_Num[i] = 0;
    }

    std::list<int>::const_iterator Lcit;
    std::vector<Operand*>::iterator Vit;
    std::list<Attach_History>::iterator Lit;
    for(Lcit = OP_Ready_Set.begin(); Lcit != OP_Ready_Set.end(); Lcit++){
        for(Vit = DFG->OP_Array[*Lcit]->OP_Parents.begin(); Vit != DFG->OP_Array[*Lcit]->OP_Parents.end(); Vit++){
            for(Lit = DFG->OP_Array[(*Vit)->OP_ID]->OP_Attach_History.begin(); Lit != DFG->OP_Array[(*Vit)->OP_ID]->OP_Attach_History.end(); Lit++){
                Attached_Ready_OP_Num[Lit->Attached_PE_ID]++;
            }
        }
    }

    int Min_Attached_OP_Num = INT_MAX;
    for(Lcit = Candidates.begin(); Lcit != Candidates.end(); Lcit++){
        if(Attached_Ready_OP_Num[*Lcit] < Min_Attached_OP_Num){
            Sel_PE_id = *Lcit;
            Min_Attached_OP_Num = Attached_Ready_OP_Num[*Lcit];
        }
    }

    return Sel_PE_ID;

}

void Scheduler::List_Scheduling_PE_Pref(){

    bool Scheduling_Completed = false;
    std::list<int> OP_Ready_Set;
    OP_Ready_Set_Init(OP_Ready_Set);

    while(!Scheduling_Completed){

        int Sel_PE_ID;
        int Sel_OP_ID;

        // Select a PE for the next operation
        std::list<int> Candidates;
        for(int i=0; i<CGRA->CGRA_Scale; i++){
            Candidates.push_back(i);
        }
        Load_Balance_Filter(Candidates);
        Recent_Busy_Filter(Candidates);
        if(PE_Sel_Strategy == Least_Recent_Used){
            Sel_PE_ID = Least_Recent_Used_Sel(Candidates);
        }
        else if(PE_Sel_Strategy == Least_Ready_OP_Attached){
            Sel_PE_ID = Least_Ready_OP_Attached_Sel(Candidates, OP_Ready_Set);
        }
        else{
            DEBUG1("Unknown PE selection strategy!\n");
        }
        CGRA->PE_Array[Sel_PE_ID]->Executed_OP_Num++;

        //Select an operation that can be executed on the pre-selected PE.
        Sel_OP_ID = Least_Cost_OP_Sel(Sel_PE_ID, OP_Ready_Set);

        //Operation Execution
        std::vector<Operand*>::iterator Vit;
        std::vector<int> Src_OP_IDs;
        std::vector<int> Src_OP_Arrival_Time;
        Src_OP_IDs.resize(3);
        Src_OP_Arrival_Time.resize(3);
        Operand* OP_Ptr = DFG->OP_Array[Sel_OP_ID];

        int i=0;
        for(Vit = OP_Ptr->OP_Parents.begin(); Vit != OP_Ptr->OP_Parents.end(); Vit++){
            Src_OP_IDs[i] = (*Vit)->OP_ID;
            Src_OP_Arrival_Time[i] = Fetch_OP(Src_OP_IDs[i], Sel_PE_ID, Impl);
            i++;
        }
        OP_Exe(Sel_OP_ID, Src_OP_IDs, Sel_PE_ID, Src_OP_Arrival_Time, Impl);

        //output operation that has been executed should be moved to output PE
        if(DFG->OP_Array[Sel_OP_ID]->OP_Type == OUTVAR || DFG->OP_Array[Sel_OP_ID]->OP_Type == IMOUT){
            Store_OP_In_IO_Buffer(Sel_OP_ID);
        }

        //Update the operation that is ready for execution
        OP_Ready_Set_Update(OP_Ready_Set, Sel_OP_ID);

        Scheduling_Completed = Is_Scheduling_Completed();

    }

}

void Scheduler::OPReadySetUpdate(list<int> &OP_Ready_Set, const int &selected_op_id){

    list<int>::iterator cit;
    for(cit=OP_Ready_Set.begin(); cit!=OP_Ready_Set.end(); cit++){
        if((*cit)==selected_op_id){
            OP_Ready_Set.erase(cit);
            break;
        }
    }

    vector<Vertex*>::iterator vit;
    for(vit=DFG->OP_Array[selected_op_id]->children.begin(); vit!=DFG->OP_Array[selected_op_id]->children.end(); vit++){
        int current_id=(*vit)->vertex_id;

        bool all_src_ready=true;
        bool already_in_set=false;
        vector<Vertex*>::iterator it;
        for(it=(*vit)->parents.begin(); it!=(*vit)->parents.end(); it++){
            list<int>::iterator opit;
            for(opit=OP_Ready_Set.begin(); opit!=OP_Ready_Set.end(); opit++){
                if(*opit==current_id){
                    already_in_set=true;
                    break;
                }
            }
            if((*it)->vertex_attribute.vertex_state==DataUnavail){
                all_src_ready=false;
                break;
            }
        }

        bool none_executed=DFG->OP_Array[current_id]->vertex_attribute.vertex_state==DataUnavail;
        if(all_src_ready && none_executed && !already_in_set){
            OP_Ready_Set.push_back(current_id);
        }
    }

}

void Scheduler::PEOPPairSelection(int &selected_PE_id, int &selected_op_id, const list<int> &Candidates, const list<int> &OP_Ready_Set){

    int min_execution_cost=GLvar::maximum_simulation_time;
    list<int>::const_iterator PE_it;
    for(PE_it=Candidates.begin(); PE_it!=Candidates.end(); PE_it++){
        list<int>::const_iterator op_it;
        int PE_free_time=CGRA->PE_Array[*PE_it]->maximum_active_time;
        for(op_it=OP_Ready_Set.begin(); op_it!=OP_Ready_Set.end(); op_it++){
            float op_cost=0;
            int dst_op=*op_it;
            vector<Vertex*>::iterator vit;
            for(vit=DFG->OP_Array[dst_op]->parents.begin(); vit!=DFG->OP_Array[dst_op]->parents.end(); vit++){
                int src_op=(*vit)->vertex_id;
                //int time_cost=FetchOP(src_op,selected_PE_id,Simulation);
                int attached_PE_id;
                int ready_time;
                if(DFG->OP_Array[src_op]->vertex_attribute.vertex_state==DataAvail){
                    attached_PE_id=NearestAttachedPE(src_op, selected_PE_id, ready_time);
                }
                else{
                    attached_PE_id=GLvar::load_PE_id;
                    ready_time=CGRA->PE_Array[attached_PE_id]->maximum_active_time;
                }
                int dist_cost=CGRA->PE_pair_distance[attached_PE_id][*PE_it];

                //Expand the distance
                if(dist_cost>1){
                    dist_cost=dist_cost*2;
                }

                if(PE_free_time>ready_time+100){
                    op_cost+=dist_cost;
                }
                else if(PE_free_time>ready_time+50){
                    op_cost+=dist_cost*2;
                }
                else{
                    op_cost+=dist_cost*4;
                }
            }
            //cout<<"PE="<<*PE_it<<" OP="<<dst_op<<" cost="<<op_cost<<endl;
            if(op_cost<min_execution_cost){
                min_execution_cost=op_cost;
                selected_op_id=dst_op;
                selected_PE_id=*PE_it;
            }
        }
    }

}


int Scheduler::Least_Cost_OP_Sel(const int &Sel_PE_ID, const std::list<int> &OP_Ready_Set){

    int Sel_OP_ID = NaN;
    std::list<int>::const_iterator Lcit;
    int Min_Exe_Cost = INT_MAX; 
    int PE_Active_Time = CGRA->PE_Array[Sel_PE_ID]->Max_Active_Time;

    for(Lcit = OP_Ready_Set.begin(); Lcit != OP_Ready_Set.end(); Lcit++){
        float OP_Exe_Cost = 0;
        std::vector<Operand*>::iterator Vit;
        for(Vit = DFG->OP_Array[*Lcit]->OP_Parents.begin(); Vit != DFG->OP_Array[*Lcit]->OP_Parents.end(); Vit++){
            int Src_OP_ID = (*Vit)->OP_ID;
            int Attached_PE_ID;
            int Src_Ready_Time;
            if(DFG->OP_Array[Src_OP_ID]->OP_Attribute.OP_State == Avail){
                Attached_PE_ID = Nearest_Attached_PE(Src_OP_ID, Sel_PE_ID, Src_Ready_Time);
            }
            else{
                Attached_PE_ID = CGRA->Load_PE_ID;
                Src_Ready_Time = CGRA->PE_Array[Attached_PE_ID]->Max_Active_Time;
            }

            int Physical_Dist = CGRA->PE_Pair_Dist[Attached_PE_ID][Sel_PE_ID];
            if(Physical_Dist > 1){
                OP_Exe_Cost = Physical_Dist * 2;
            }
            if(PE_Active_Time > (Src_Ready_Time + 100)){
                OP_Exe_Cost += Physical_Dist;
            }
            else if(PE_Active_Time > (Src_Ready_Time + 50)){
                OP_Exe_Cost += Physical_Dist * 1.5;
            }
            else{
                OP_Exe_Cost += Physical_Dist * 2;
            }

        }

        if(OP_Exe_Cost < Min_Exe_Cost){
            Min_Exe_Cost = OP_Exe_Cost;
            Sel_OP_ID = *Lcit;
        }
    }

    if(Sel_OP_ID == NaN){
        DEBUG1("Unexpected operation selection!");
    }

    return Sel_OP_ID;

}

void Scheduler::OP_Ready_Set_Init(std::list<int> &OP_Ready_Set){

    for(int i=0; i<DFG->OP_Num; i++){
        Operand* OP_Ptr = DFG->OP_Array[i];
        if((OP_Ptr->OP_Type != INCONST) && (OP_Ptr != INVAR)){
            std::vector<Operand*>::iterator OP_It;
            bool Is_Src_OP_Ready = true;
            for(OP_It = OP_Ptr->OP_Parents.begin(); OP_It != OP_Ptr->OP_Parents.end(); OP_It++){
                if((*OP_It)->OP_Attribute.OP_State == Unavail){
                    Is_Src_OP_Ready = false;
                    break;
                }
            }

            if(Is_Src_OP_Ready){
                OP_Ready_Set.push_back(i);
            }
        }
    }

}

int Scheduler::Fetch_OP(const int &Src_OP_ID, const int &Target_PE_ID, const Exe_Mode &Fun_Mode){

    int Expected_Complete_Time = 0;
    int Src_Avail_Time;
    int Src_Attached_PE_ID;

    // Const 0 is always available on each PE
    if(Src_OP_ID == 0){
        DFG->OP_Array[0]->OP_Attribute.OP_State = Avail;
        Src_Attached_PE_ID = Target_PE_ID;
    }
    else if(Src_OP_ID!=0 && DFG->OP_Array[Src_OP_ID]->OP_Attribute.OP_State == Avail){
        Src_Attached_PE_ID = Nearest_Attached_PE(Src_OP_ID, Target_PE_ID, Src_Avail_Time);
    }
    else if(DFG->OP_Array[Src_OP_ID]->OP_Attribute.OP_State == In_IO_Buffer){
        Src_Avail_Time = Load_From_IO_Buffer(Src_OP_ID, Fun_Mode);
        Src_Attached_PE_ID = CGRA->Load_PE_ID;
    }
    else{
        DEBUG1("Unexpected operation fectching!");
    }

    // Move source operand to target PE.
    if(Src_Attached_PE_ID != Target_PE_ID){
        std::list<int> Shortest_Routing_Path;
        CGRA->PossiblePath(Src_Avail_Time, Src_Attached_PE_ID, Target_PE_ID, Shortest_Routing_Path);

        //Transfer data from src to dst using this path
        fetch_cost=OperationTransmission(ready_time, Src_OP_ID, shortest_path, mode);
    }

    return fetch_cost;

}

void Scheduler::ListSchedulingAlgorithmOPFirst(){

    bool scheduling_completed=false;
    InputOperationScheduling();

    while(!scheduling_completed){
        int selected_operation_id;
        //selected_operation_id=StaticOperationSelection();
        selected_operation_id=DynamicOperationSelection();

        int selected_PE_id;
        vector<int> src_operation_ids;
        vector<int> source_ready_time; //The time that source operands arrive at target PE data memory
        vector<int> source_start_time; //The time that source operands start to transmit
        vector<list<int> > source_routing_paths; //Rotuing paths for moving source operands to target PE
        src_operation_ids.resize(INSTR_OP_NUM-1);
        source_ready_time.resize(INSTR_OP_NUM-1);
        source_start_time.resize(INSTR_OP_NUM-1);
        source_routing_paths.resize(INSTR_OP_NUM-1);

        bool input_operation_flag=DFG->OP_Array[selected_operation_id]->vertex_type==InputData;
        bool output_operation_flag=DFG->OP_Array[selected_operation_id]->vertex_type==OutputData;
        bool in_out_mem_flag=DFG->OP_Array[selected_operation_id]->vertex_attribute.vertex_state==DataInOutMem;

        //Input operation that is still in out memory will be loaded first
        if(input_operation_flag && in_out_mem_flag){
            LoadDataFromOutMem(selected_operation_id, Implementation);
        }
        //output operation and intermediate operation will be executed
        else if(!input_operation_flag){
            vector<Vertex*>::iterator it;
            Vertex* selected_vertex=DFG->OP_Array[selected_operation_id];
            for(it=selected_vertex->parents.begin(); it!=selected_vertex->parents.end(); it++){
                src_operation_ids.push_back((*it)->vertex_id);
            }

            selected_PE_id=PESelection(selected_operation_id, src_operation_ids, source_routing_paths, source_start_time);

            //tmp_op=CGRA->PE_Array[1]->Component_Trace[42]->component_activity->memory_port_op[0];
            //if(tmp_op==154){
            //cout<<"checkpoint 1: right!"<<endl;
            //}
            //else{
            //cout<<"checkpoint 1: wrong! tmp_op="<<tmp_op<<endl;
            //}

            FetchSourceOperation(selected_PE_id, src_operation_ids, source_routing_paths, source_start_time, source_ready_time);

            /*tmp_op=CGRA->PE_Array[1]->Component_Trace[3]->component_activity->memory_port_op[0];
              op_read=CGRA->PE_Array[1]->Component_Trace[3]->component_reserved->memory_read_reserved[0];
              cout<<"After source fetch"<<endl;
              if(op_read){
              cout<<"checkpoint 0: right! tmp_op="<<tmp_op<<endl;
              }
              else{
              cout<<"checkpoint 0: wrong! tmp_op="<<tmp_op<<endl;
              }*/

            //tmp_op=CGRA->PE_Array[1]->Component_Trace[42]->component_activity->memory_port_op[0];
            //if(tmp_op==154){
            //cout<<"checkpoint 2: right!"<<endl;
            //}
            //else{
            //cout<<"checkpoint 2: wrong! tmp_op="<<tmp_op<<endl;
            //}

            OperationExecution(selected_operation_id, src_operation_ids, selected_PE_id, source_ready_time, Implementation);
            /*tmp_op=CGRA->PE_Array[1]->Component_Trace[3]->component_activity->memory_port_op[0];
              op_read=CGRA->PE_Array[1]->Component_Trace[3]->component_reserved->memory_read_reserved[0];
              cout<<"After execution"<<endl;
              if(op_read){
              cout<<"checkpoint 0: right! tmp_op="<<tmp_op<<endl;
              }
              else{
              cout<<"checkpoint 0: wrong! tmp_op="<<tmp_op<<endl;
              }*/

            //output operation that has not been executed should be moved to output PE
            if(output_operation_flag){
                StoreDataInOutMem(selected_operation_id);
            }

            /*tmp_op=CGRA->PE_Array[1]->Component_Trace[3]->component_activity->memory_port_op[0];
              op_read=CGRA->PE_Array[1]->Component_Trace[3]->component_reserved->memory_read_reserved[0];
              cout<<"After store!"<<endl;
              if(op_read){
              cout<<"checkpoint 0: right! tmp_op="<<tmp_op<<endl;
              }
              else{
              cout<<"checkpoint 0: wrong! tmp_op="<<tmp_op<<endl;
              }*/

        }
        scheduling_completed=SchedulingIsCompleted();
    }

}

int Scheduler::Least_Recent_Used_Sel(const std::list<int> &Candidates){

    int Min_Active_Time = INT_MAX;
    int Sel_PE_ID = 0;
    std::list<int>::const_iterator Lcit;

    for(Lcit = Candidates.begin(); Lcit!=Candidates.end(); Lcit++){
        int Current_PE_Active_Time = CGRA->PE_Array[i]->Max_Active_Time;
        if(Current_PE_Active_Time < Min_Active_Time){
            Sel_PE_id= *Lcit;
            Min_Active_Time = Current_PE_Active_Time;
        }
    }

    return Sel_PE_ID;

}

/* --------------------------------------------------------------------
 * We should not change neither state of CGRA nor state of DFG here, 
 * because it is just a try and selection. 
 * -------------------------------------------------------------------*/
int Scheduler::PESelection(const int &target_operation_id, const vector<int> &src_operation_ids, vector<list<int> > &source_routing_paths, vector<int> &source_operation_ready_time){

    //Initial PE Candidates
    vector<int> candidate_PE_id;

    //Filter the input PE 
    for(int i=0; i<CGRA->CGRA_Scale; i++){
        //if(i!=GLvar::store_PE_id){
        candidate_PE_id.push_back(i);
        //}
    }

    //Reduce iteration times by filtering out some candidate PEs that fails to satisfy certain metric
    //PESelectionFilter(candidate_PE_id, target_operation_id, src_operation_ids, PhysicalDistanceFiltering);
    //if(candidate_PE_id.size()==0){
    //candidate_PE_id.push_back(GLvar::store_PE_id);
    //}
    if(candidate_PE_id.size()==0){
        DEBUG1("All the candidate PEs are kicked off by physical distance filter!\n");
    }
    //PESelectionFilter(candidate_PE_id, target_operation_id, src_operation_ids, MemoryUtilizationFiltering);
    PESelectionFilter(candidate_PE_id, target_operation_id, src_operation_ids, DSPutilizationFiltering);
    //PESelectionFilter(candidate_PE_id, target_operation_id, src_operation_ids, WriteMemoryUtilizationFiltering);
    //PESelectionFilter(candidate_PE_id, target_operation_id, src_operation_ids, OutputPortUtilizationFiltering);
    if(candidate_PE_id.size()==0){
        DEBUG1("All the candidate PEs are kicked off by DSP utilization filter!\n");
    }

    vector<int> execution_time_on_each_PE;
    execution_time_on_each_PE.resize(CGRA->CGRA_Scale);
    int earliest_execution_time=INT_MAX;
    int selected_PE_id=NaN;
    vector<list<int> > source_routing_paths_tmp;
    source_routing_paths_tmp.resize(INSTR_OP_NUM-1);
    vector<int> source_operation_ready_time_tmp;
    source_operation_ready_time_tmp.resize(INSTR_OP_NUM-1);
    list<int> shortest_path;
    for(int i=0; i<CGRA->CGRA_Scale; i++){
        vector<int>::iterator iter_tmp;
        bool is_candidate=false;
        for(iter_tmp=candidate_PE_id.begin(); iter_tmp!=candidate_PE_id.end(); iter_tmp++){
            if(i==*iter_tmp){
                is_candidate=true;
                break;
            }
        }
        if(is_candidate){
            vector<int> arrival_destination_time;
            arrival_destination_time.resize(INSTR_OP_NUM-1);
            for(int j=0; j<INSTR_OP_NUM-1; j++){
                int attached_PE_id;
                source_operation_ready_time_tmp[j]=DFG->OP_Array[src_operation_ids[j]]->vertex_attribute.operation_avail_time;
                attached_PE_id=NearestAttachedPE(src_operation_ids[j], i, source_operation_ready_time_tmp[j]);

                shortest_path.clear();
                CGRA->PossiblePath(source_operation_ready_time_tmp[j],attached_PE_id,i,shortest_path);
                source_routing_paths_tmp[j]=shortest_path;

                arrival_destination_time[j]=OperationTransmission(source_operation_ready_time_tmp[j], src_operation_ids[j], shortest_path, Simulation);
            }
            execution_time_on_each_PE[i]=OperationExecution(target_operation_id, src_operation_ids, i, arrival_destination_time, Simulation);

            //Get the execution time and refresh the selected routing path information that will return
            if(earliest_execution_time>execution_time_on_each_PE[i]){
                earliest_execution_time=execution_time_on_each_PE[i];
                selected_PE_id=i;

                //cout<<"="<<source_operation_ready_time_tmp[0]<<" "<<source_operation_ready_time_tmp[1]<<" "<<source_operation_ready_time[2]<<" ";
                for(int j=0; j<INSTR_OP_NUM-1; j++){
                    source_routing_paths[j]=source_routing_paths_tmp[j];
                    source_operation_ready_time[j]=source_operation_ready_time_tmp[j];
                }
            }

        }

    }

    return selected_PE_id;

}

int Scheduler::Load_From_IO_Buffer(const int &OP_ID, const Exe_Mode &Fun_Mode){

    //Load the operation from IO Buffer
    int Expected_Complete_Time = NaN;
    for(int i=0; i<INT_MAX; i++){
        bool Load_Path_Avail = CGRA->PE_Array[CGRA->Load_PE_ID]->Component_Trace[i+1]->PE_Component_Reserved->Load_Path_Reserved == false;
        bool WR_Port1_Avail = CGRA->PE_Array[CGRA->Load_PE_ID]->Component_Trace[i+2]->PE_Component_Reserved->Data_Mem_WR_Reserved[1] == false;
        bool RD_Port3_Avail = CGRA->PE_Array[CGRA->Load_PE_ID]->Component_Trace[i+2]->PE_Component_Reserved->Data_Mem_RD_Reserved[3] == false;
        bool RD_Port4_Avail = CGRA->PE_Array[CGRA->Load_PE_ID]->Component_Trace[i+2]->PE_Component_Reserved->Data_Mem_RD_Reserved[4] == false;
        bool RD_Port5_Avail = CGRA->PE_Array[CGRA->Load_PE_ID]->Component_Trace[i+2]->PE_Component_Reserved->Data_Mem_RD_Reserved[5] == false;
        if(Load_Path_Avail && WR_Port1_Avail && RD_Port3_Avail && RD_Port4_Avail && RD_Port5_Avail){
            if(Fun_Mode == Impl){

                //update corresponding PE component state
                CGRA->PE_Array[CGRA->Load_PE_ID]->Component_Trace[i+1]->PE_Component_Reserved->Load_Path_Reserved = true;
                CGRA->PE_Array[CGRA->Load_PE_ID]->Component_Trace[i+2]->PE_Component_Reserved->Data_Mem_WR_Reserved[1] = true;

                //update corresponding PE component activity
                CGRA->PE_Array[CGRA->Load_PE_ID]->Component_Trace[i]->PE_Component_Activity->Load_OP = OP_ID;
                CGRA->PE_Array[CGRA->Load_PE_ID]->Component_Trace[i+1]->PE_Component_Activity->Load_Mux = 0;
                CGRA->PE_Array[CGRA->Load_PE_ID]->Component_Trace[i+2]->PE_Component_Activity->Data_Mem_WR_Ena[1] = 1;
                CGRA->PE_Array[CGRA->Load_PE_ID]->Component_Trace[i+2]->PE_Component_Activity->Data_Mem_Port_OP[3] = OP_ID;
                CGRA->PE_Array[CGRA->Load_PE_ID]->Component_Trace[i+2]->PE_Component_Activity->Data_Mem_Port_OP[4] = OP_ID;
                CGRA->PE_Array[CGRA->Load_PE_ID]->Component_Trace[i+2]->PE_Component_Activity->Data_Mem_Port_OP[5] = OP_ID;

                //Update operation state
                DFG->OP_Array[OP_ID]->OP_Attribute.OP_State = Avail;
                DFG->OP_Array[OP_ID]->OP_Attribute.OP_Exe_PE_ID = CGRA->Load_PE_ID;
                DFG->OP_Array[OP_ID]->OP_Attribute.OP_Avail_Time = i+2;

                Attach_History Attach_Point;
                Attach_Point.Attached_Time = i+2;
                Attach_Point.Attached_PE_ID = CGRA->Load_PE_ID;
                DFG->OP_Array[OP_ID]->OP_Attach_History.push_back(Attach_Point);

                //Dump the trace
                if(GL_Var::Print_Level>10){
                    fTrace << "Load " << OP_ID << " through " << CGRA->Load_PE_ID << " at time " << i << std::endl;
                }
            }

            Expected_Complete_Time = i+2;
            break;
        }
    }

    if(Expected_Complete_Time == NaN){
        DEBUG1("Unexpected load occassion!");
    }

    return Expected_Complete_Time;

}

void Scheduler::StoreDataInOutMem(const int &operation_id){

    //Check whether the operation is executed in store_PE
    //If so, pull it out of the data memory
    int op_avail_time=DFG->OP_Array[operation_id]->vertex_attribute.operation_avail_time+1;
    int src=DFG->OP_Array[operation_id]->vertex_attribute.execution_PE_id;
    int dst=GLvar::store_PE_id;

    if(src==dst){
        FromDSTToOutMem(operation_id, op_avail_time);
    }
    else{
        //Find a routing path from src to dst
        list<int> shortest_path;
        int ready_time=DFG->OP_Array[operation_id]->vertex_attribute.operation_avail_time;
        CGRA->PossiblePath(ready_time, src, dst, shortest_path);

        //Transfer data from src to dst
        int op_arrival_time=OperationTransmission(ready_time, operation_id, shortest_path, Implementation)+1;

        //Move data from dst to outside memory
        FromDSTToOutMem(operation_id, op_arrival_time);
    }

}

void Scheduler::FromDSTToOutMem(const int &operation_id, const int &start_time){

    int op_avail_time=start_time;
    while(true){
        bool write0_avail=CGRA->PE_Array[GLvar::store_PE_id]->Component_Trace[op_avail_time]->component_reserved->memory_write_reserved[0]==false;
        bool read0_avail=CGRA->PE_Array[GLvar::store_PE_id]->Component_Trace[op_avail_time]->component_reserved->memory_read_reserved[0]==false;
        bool read1_avail=CGRA->PE_Array[GLvar::store_PE_id]->Component_Trace[op_avail_time]->component_reserved->memory_read_reserved[1]==false;
        bool read2_avail=CGRA->PE_Array[GLvar::store_PE_id]->Component_Trace[op_avail_time]->component_reserved->memory_read_reserved[2]==false;
        bool store_path_avail=CGRA->PE_Array[GLvar::store_PE_id]->Component_Trace[op_avail_time+2]->component_reserved->store_path_reserved==false;

        if(write0_avail && read0_avail && store_path_avail){
            CGRA->PE_Array[GLvar::store_PE_id]->Component_Trace[op_avail_time]->component_reserved->memory_read_reserved[0]=true;
            CGRA->PE_Array[GLvar::store_PE_id]->Component_Trace[op_avail_time]->component_activity->memory_wr_ena[0]=0;
            CGRA->PE_Array[GLvar::store_PE_id]->Component_Trace[op_avail_time]->component_activity->memory_port_op[0]=operation_id;
            CGRA->PE_Array[GLvar::store_PE_id]->Component_Trace[op_avail_time+2]->component_activity->store_op=operation_id;
            CGRA->PE_Array[GLvar::store_PE_id]->Component_Trace[op_avail_time+2]->component_activity->store_mux=0;
            CGRA->PE_Array[GLvar::store_PE_id]->Component_Trace[op_avail_time+2]->component_reserved->store_path_reserved=true;

            //Dump the trace
            if(GLvar::report_level>10){
                fTrace<<"Store "<<operation_id<<" in outside memory "<<" at time "<<op_avail_time+3<<endl;
            }

            break;
        }
        else if(write0_avail && read1_avail && store_path_avail){
            CGRA->PE_Array[GLvar::store_PE_id]->Component_Trace[op_avail_time]->component_reserved->memory_read_reserved[1]=true;
            CGRA->PE_Array[GLvar::store_PE_id]->Component_Trace[op_avail_time]->component_activity->memory_wr_ena[0]=0;
            CGRA->PE_Array[GLvar::store_PE_id]->Component_Trace[op_avail_time]->component_activity->memory_port_op[1]=operation_id;
            CGRA->PE_Array[GLvar::store_PE_id]->Component_Trace[op_avail_time+2]->component_activity->store_op=operation_id;
            CGRA->PE_Array[GLvar::store_PE_id]->Component_Trace[op_avail_time+2]->component_activity->store_mux=1;
            CGRA->PE_Array[GLvar::store_PE_id]->Component_Trace[op_avail_time+2]->component_reserved->store_path_reserved=true;

            //Dump the trace
            if(GLvar::report_level>10){
                fTrace<<"Store "<<operation_id<<" in outside memory "<<" at time "<<op_avail_time+3<<endl;
            }

            break;
        }
        else if(write0_avail && read2_avail && store_path_avail){
            CGRA->PE_Array[GLvar::store_PE_id]->Component_Trace[op_avail_time]->component_reserved->memory_read_reserved[2]=true;
            CGRA->PE_Array[GLvar::store_PE_id]->Component_Trace[op_avail_time]->component_activity->memory_wr_ena[0]=0;
            CGRA->PE_Array[GLvar::store_PE_id]->Component_Trace[op_avail_time]->component_activity->memory_port_op[2]=operation_id;
            CGRA->PE_Array[GLvar::store_PE_id]->Component_Trace[op_avail_time+2]->component_activity->store_op=operation_id;
            CGRA->PE_Array[GLvar::store_PE_id]->Component_Trace[op_avail_time+2]->component_activity->store_mux=2;
            CGRA->PE_Array[GLvar::store_PE_id]->Component_Trace[op_avail_time+2]->component_reserved->store_path_reserved=true;

            //Dump the trace
            if(GLvar::report_level>10){
                //Time when data is store in outside memory.
                fTrace<<"Store "<<operation_id<<" in outside memory "<<" at time "<<op_avail_time+3<<endl;
            }

            break;
        }
        else{
            op_avail_time++;
        }
    }
    if(last_op_store_time<(op_avail_time+3)){
        last_op_store_time=op_avail_time+3;
    }

}

void Scheduler::PESelectionFilter(vector<int> &candidate_PE_id, const int &target_operation_id, const vector<int> &src_operation_ids, const PESelectionFilteringType &filtering_type){

    int begin_time;
    int end_time;
    begin_time=INT_MAX;
    float physical_distance_acceptable_percentile=0.25;
    float utilization_acceptable_percentile=0.9;

    for(int i=0; i<INSTR_OP_NUM-1; i++){
        int src_ready_time=DFG->OP_Array[src_operation_ids[i]]->vertex_attribute.operation_avail_time;
        if(begin_time>src_ready_time){
            begin_time=src_ready_time;
        }
    }
    end_time=GLvar::maximum_simulation_time-1;
    if(filtering_type==PhysicalDistanceFiltering){
        int max_distance=0;
        int min_distance=INT_MAX;
        for(int i=0; i<CGRA->CGRA_Scale; i++){
            for(int j=0; j<CGRA->CGRA_Scale; j++){
                if(CGRA->PE_pair_distance[i][j]>max_distance && CGRA->PE_pair_distance[i][j]>0){
                    max_distance=CGRA->PE_pair_distance[i][j];
                }
                if(CGRA->PE_pair_distance[i][j]<min_distance && CGRA->PE_pair_distance[i][j]>0){
                    min_distance=CGRA->PE_pair_distance[i][j];
                }
            }
        }
        int maximum_acceptable_physical_distance;
        maximum_acceptable_physical_distance=(int)((min_distance+(max_distance-min_distance)*physical_distance_acceptable_percentile)*3);
        vector<int>::iterator iter_tmp;
        for(iter_tmp=candidate_PE_id.begin(); iter_tmp!=candidate_PE_id.end(); ){
            int total_distance=0;
            for(int i=0; i<INSTR_OP_NUM-1; i++){
                int time_tmp=0;
                int attached_PE_id=NearestAttachedPE(src_operation_ids[i], *iter_tmp, time_tmp);
                total_distance=total_distance+CGRA->PE_pair_distance[attached_PE_id][*iter_tmp];
            }
            if(total_distance>maximum_acceptable_physical_distance){
                iter_tmp=candidate_PE_id.erase(iter_tmp);
            }
            else{
                iter_tmp++;
            }
        }
    }
    else if(filtering_type==MemoryUtilizationFiltering){
        vector<float> utilization_per_PE;
        utilization_per_PE.resize(CGRA->CGRA_Scale);
        for(int i=0; i<CGRA->CGRA_Scale; i++){
            utilization_per_PE[i]=1;
        }
        vector<int>::iterator iter_tmp;
        for(iter_tmp=candidate_PE_id.begin(); iter_tmp!=candidate_PE_id.end(); iter_tmp++){
            float utilization_tmp=CGRA->PE_Array[*iter_tmp]->MemoryUtilization(begin_time, end_time);
            utilization_per_PE[*iter_tmp]=utilization_tmp;
        }
        UtilizationFilter(candidate_PE_id, utilization_per_PE, utilization_acceptable_percentile);
    }
    else if(filtering_type==DSPutilizationFiltering){
        vector<float> utilization_per_PE;
        utilization_per_PE.resize(CGRA->CGRA_Scale);
        for(int i=0; i<CGRA->CGRA_Scale; i++){
            utilization_per_PE[i]=1;
        }
        vector<int>::iterator iter_tmp;
        for(iter_tmp=candidate_PE_id.begin(); iter_tmp!=candidate_PE_id.end(); iter_tmp++){
            float utilization_tmp=CGRA->PE_Array[*iter_tmp]->DSPutilization(begin_time, end_time);
            utilization_per_PE[*iter_tmp]=utilization_tmp;
        }
        UtilizationFilter(candidate_PE_id, utilization_per_PE, utilization_acceptable_percentile);
    }
    else if(filtering_type==WriteMemoryUtilizationFiltering){
        vector<float> utilization_per_PE;
        utilization_per_PE.resize(CGRA->CGRA_Scale);
        for(int i=0; i<CGRA->CGRA_Scale; i++){
            utilization_per_PE[i]=1;
        }
        vector<int>::iterator iter_tmp;
        for(iter_tmp=candidate_PE_id.begin(); iter_tmp!=candidate_PE_id.end(); iter_tmp++){
            float utilization_tmp=CGRA->PE_Array[*iter_tmp]->WriteMemoryUtilization(begin_time, end_time);
            utilization_per_PE[*iter_tmp]=utilization_tmp;
        }
        UtilizationFilter(candidate_PE_id, utilization_per_PE, utilization_acceptable_percentile);
    }
    else if(filtering_type==OutputPortUtilizationFiltering){
        vector<float> utilization_per_PE;
        utilization_per_PE.resize(CGRA->CGRA_Scale);
        for(int i=0; i<CGRA->CGRA_Scale; i++){
            utilization_per_PE[i]=1;
        }
        vector<int>::iterator iter_tmp;
        for(iter_tmp=candidate_PE_id.begin(); iter_tmp!=candidate_PE_id.end(); iter_tmp++){
            float utilization_tmp=CGRA->PE_Array[*iter_tmp]->OutputPortUtilization(begin_time, end_time);
            utilization_per_PE[*iter_tmp]=utilization_tmp;
        }
        UtilizationFilter(candidate_PE_id, utilization_per_PE, utilization_acceptable_percentile);
    }
    else{
        DEBUG1("Unrecognized PE selection filtering type!\n");
    }

}

void Scheduler::UtilizationFilter(vector<int> &candidate_PE_id, const vector<float> &utilization_per_PE, const float &acceptable_percentile){

    float maximum_utilization=0;
    float minimum_utilization=1;
    vector<int>::iterator id_iter;
    for(id_iter=candidate_PE_id.begin(); id_iter!=candidate_PE_id.end(); id_iter++){
        float current_utilization=utilization_per_PE[*id_iter];
        if(current_utilization>maximum_utilization){
            maximum_utilization=current_utilization;
        }
        if(current_utilization<minimum_utilization){
            minimum_utilization=current_utilization;
        }
    }
    if(maximum_utilization>minimum_utilization){
        float metric_utilization=minimum_utilization+acceptable_percentile*(maximum_utilization-minimum_utilization);
        for(id_iter=candidate_PE_id.begin(); id_iter!=candidate_PE_id.end(); ){
            float current_utilization=utilization_per_PE[*id_iter];
            if(current_utilization>metric_utilization){
                id_iter=candidate_PE_id.erase(id_iter);
            }
            else{
                id_iter++;
            }
        }
    }

}

int Scheduler::OperationTransmission(const int &start_time, const int &src_operation_id, const list<int> &routing_path, const ExecutionMode &mode){

    int total_time=start_time;
    int transmission_progress_time=start_time;
    int PE_num_on_path=routing_path.size();

    /*if(mode==Implementation){
      list<int>::const_iterator it_tmp;
      cout<<"op="<<src_operation_id<<" start time="<<start_time<<" path: ";
      for(it_tmp=routing_path.begin(); it_tmp!=routing_path.end(); it_tmp++){
      cout<<(*it_tmp)<<" ";
      }
      cout<<endl;
      }*/

    //IO load brings additional mux and pipeline.
    int current_additional_pipeline=0;
    int next_additional_pipeline=0;

    if(PE_num_on_path==0){
        DEBUG1("Empty routing path!\n");
    }

    vector<int> routing_path_copy;
    routing_path_copy.resize(PE_num_on_path);
    list<int>::const_iterator list_iter;
    int id_tmp=0;
    for(list_iter=routing_path.begin(); list_iter!=routing_path.end(); list_iter++){
        routing_path_copy[id_tmp]=*list_iter;
        id_tmp++;
    }

    int child_id=NaN;
    int parent_id=NaN;
    int last_parent_id=NaN;
    int last_PE_id=NaN;
    for(int i=0; i<PE_num_on_path;){

        //Introduce the current/next_additional_pipeline variables to handle Load/Store PE
        //Which result in additional pipeline.
        int current_PE_id=routing_path_copy[i];
        if(current_PE_id==GLvar::load_PE_id || current_PE_id==GLvar::store_PE_id){
            current_additional_pipeline=1;
        }
        else{
            current_additional_pipeline=0;
        }
        int next_PE_id=NaN;
        if(i<PE_num_on_path){
            if(i==PE_num_on_path-1){
                next_PE_id=NaN;
                child_id=NaN;
                last_parent_id=parent_id;
                parent_id=NaN;
            }
            else{
                next_PE_id=routing_path_copy[i+1];
                child_id=CGRA->GetChildID(current_PE_id, next_PE_id);
                last_parent_id=parent_id;
                parent_id=CGRA->GetParentID(current_PE_id, next_PE_id);
            }

            if(next_PE_id==GLvar::load_PE_id || next_PE_id==GLvar::store_PE_id){
                next_additional_pipeline=1;
            }
            else{
                next_additional_pipeline=0;
            }

        }

        //Destination PE is exactly the same with source PE. And, therefore there is no need for data transmission.
        if(PE_num_on_path==1){
            i++;
        }
        else{
            //First transmission on the path
            if(i==0){
                bool current_PE_memory_read_avail0=CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+1]->component_reserved->memory_read_reserved[0]==false;
                bool current_PE_memory_read_avail1=CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+1]->component_reserved->memory_read_reserved[1]==false;
                bool current_PE_memory_read_avail2=CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+1]->component_reserved->memory_read_reserved[2]==false;
                bool current_PE_memory_write_avail0=CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+1]->component_reserved->memory_write_reserved[0]==false;
                bool current_PE_output_avail=CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+3]->component_reserved->PE_output_reserved[child_id]==false;
                bool next_PE_input_avail=CGRA->PE_Array[next_PE_id]->Component_Trace[transmission_progress_time+5]->component_reserved->PE_input_reserved==false;

                bool next_load_path_avail=true;
                if(next_PE_id==GLvar::load_PE_id || next_PE_id==GLvar::store_PE_id){
                    next_load_path_avail=CGRA->PE_Array[next_PE_id]->Component_Trace[transmission_progress_time+6]->component_reserved->load_path_reserved==false;
                }

                bool next_PE_memory_write_avail1=CGRA->PE_Array[next_PE_id]->Component_Trace[transmission_progress_time+6+next_additional_pipeline]->component_reserved->memory_write_reserved[1]==false;
                bool next_PE_memory_read_avail3=CGRA->PE_Array[next_PE_id]->Component_Trace[transmission_progress_time+6+next_additional_pipeline]->component_reserved->memory_read_reserved[3]==false;
                bool next_PE_memory_read_avail4=CGRA->PE_Array[next_PE_id]->Component_Trace[transmission_progress_time+6+next_additional_pipeline]->component_reserved->memory_read_reserved[4]==false;
                bool next_PE_memory_read_avail5=CGRA->PE_Array[next_PE_id]->Component_Trace[transmission_progress_time+6+next_additional_pipeline]->component_reserved->memory_read_reserved[5]==false;

                bool current_PE_memory_read_avail=(current_PE_memory_read_avail0 || current_PE_memory_read_avail1 || current_PE_memory_read_avail2) && current_PE_memory_write_avail0;
                bool next_PE_memory_write_avail=next_PE_memory_write_avail1 && next_PE_memory_read_avail3 && next_PE_memory_read_avail4 && next_PE_memory_read_avail5;

                if(current_PE_memory_read_avail && current_PE_output_avail && next_PE_input_avail && next_PE_memory_write_avail && next_load_path_avail){
                    if(mode==Implementation){

                        if(current_PE_memory_read_avail0){
                            CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+1]->component_reserved->memory_read_reserved[0]=true;
                            CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+1]->component_activity->memory_port_op[0]=src_operation_id;
                            CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+3]->component_activity->PE_output_mux[child_id]=0;
                        }
                        else if(current_PE_memory_read_avail1){
                            CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+1]->component_reserved->memory_read_reserved[1]=true;
                            CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+1]->component_activity->memory_port_op[1]=src_operation_id;
                            CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+3]->component_activity->PE_output_mux[child_id]=1;
                        }
                        else{
                            CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+1]->component_reserved->memory_read_reserved[2]=true;
                            CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+1]->component_activity->memory_port_op[2]=src_operation_id;
                            CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+3]->component_activity->PE_output_mux[child_id]=2;
                        }
                        CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+3]->component_reserved->PE_output_reserved[child_id]=true;
                        CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+1]->component_activity->memory_wr_ena[0]=0;

                        if(GLvar::report_level>10){
                            fTrace << "Move operation " << src_operation_id << " First: from " << " PE " <<current_PE_id<<" to "<< " PE " << next_PE_id << " at time " << transmission_progress_time+1 <<endl;
                        }

                    }
                    transmission_progress_time=transmission_progress_time+4;
                    i++;
                }
                else{
                    transmission_progress_time++;
                }

                /*int tmp_op=CGRA->PE_Array[1]->Component_Trace[3]->component_activity->memory_port_op[0];
                  int op_read=CGRA->PE_Array[1]->Component_Trace[3]->component_reserved->memory_read_reserved[0];
                  cout<<"After first transmission"<<endl;
                  if(op_read){
                  cout<<"checkpoint 0: right! tmp_op="<<tmp_op<<endl;
                  }
                  else{
                  cout<<"checkpoint 0: wrong! tmp_op="<<tmp_op<<endl;
                  }*/

                //int tmp_op=CGRA->PE_Array[1]->Component_Trace[670]->component_activity->memory_port_op[0];
                //if(tmp_op==1221){
                //cout<<"checkpoint 2: right!"<<endl;
                //}
                //else{
                //cout<<"checkpoint 2: wrong! tmp_op="<<tmp_op<<endl;
                //}

            }

            else if(i>0 && i<PE_num_on_path-1){

                //Important states for bypass data path 
                bool current_PE_bypass_avail=CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+1]->component_reserved->PE_bypass_reserved==false;
                bool current_PE_output_avail=CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+2]->component_reserved->PE_output_reserved[child_id]==false;
                bool next_PE_input_avail=CGRA->PE_Array[next_PE_id]->Component_Trace[transmission_progress_time+4]->component_reserved->PE_input_reserved==false;

                bool next_load_path_avail=true;
                if(next_PE_id==GLvar::load_PE_id || next_PE_id==GLvar::store_PE_id){
                    next_load_path_avail=CGRA->PE_Array[next_PE_id]->Component_Trace[transmission_progress_time+5]->component_reserved->load_path_reserved==false;
                }

                bool next_PE_memory_write_avail1=CGRA->PE_Array[next_PE_id]->Component_Trace[transmission_progress_time+5+next_additional_pipeline]->component_reserved->memory_write_reserved[1]==false;
                bool next_PE_memory_read_avail3=CGRA->PE_Array[next_PE_id]->Component_Trace[transmission_progress_time+5+next_additional_pipeline]->component_reserved->memory_read_reserved[3]==false;
                bool next_PE_memory_read_avail4=CGRA->PE_Array[next_PE_id]->Component_Trace[transmission_progress_time+5+next_additional_pipeline]->component_reserved->memory_read_reserved[4]==false;
                bool next_PE_memory_read_avail5=CGRA->PE_Array[next_PE_id]->Component_Trace[transmission_progress_time+5+next_additional_pipeline]->component_reserved->memory_read_reserved[5]==false;
                bool next_PE_memory_write_avail=next_PE_memory_write_avail1 && next_PE_memory_read_avail3 && next_PE_memory_read_avail4 && next_PE_memory_read_avail5;

                if(current_PE_bypass_avail && current_PE_output_avail && next_PE_input_avail && next_PE_memory_write_avail && next_load_path_avail){
                    if(mode==Implementation){
                        CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+1]->component_reserved->PE_bypass_reserved=true;
                        CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+2]->component_reserved->PE_output_reserved[child_id]=true;
                        CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+1]->component_activity->PE_bypass_mux=last_parent_id;
                        CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+2]->component_activity->PE_output_mux[child_id]=3;

                        if(GLvar::report_level>10){
                            fTrace << "Move operation " << src_operation_id<< " bypass: from" << " PE " << current_PE_id << " to " << " PE " << next_PE_id << " at time " << transmission_progress_time+2 <<endl;
                        }

                    }
                    transmission_progress_time=transmission_progress_time+3;
                    i++;
                }

                //Store and forward occasion
                else{
                    //If the data needs to be stored, there must be no resource confliction and we simply reserve the corresponding resources.
                    if(mode==Implementation){
                        CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+1]->component_reserved->PE_input_reserved=true;
                        CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+1]->component_activity->PE_input_mux=last_parent_id;
                        if(current_PE_id==GLvar::load_PE_id || current_PE_id==GLvar::store_PE_id){
                            CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+2]->component_reserved->load_path_reserved=true;
                            CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+2]->component_activity->load_mux=1;
                        }
                        CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+2+current_additional_pipeline]->component_reserved->memory_write_reserved[1]=true;
                        CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+2+current_additional_pipeline]->component_activity->memory_wr_ena[1]=1;
                        CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+2+current_additional_pipeline]->component_activity->memory_port_op[3]=src_operation_id;
                        CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+2+current_additional_pipeline]->component_activity->memory_port_op[4]=src_operation_id;
                        CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+2+current_additional_pipeline]->component_activity->memory_port_op[5]=src_operation_id;

                        //Keep the attach point which can be reused later
                        AttachHistory attach_point;
                        attach_point.attached_time=transmission_progress_time+2+current_additional_pipeline;
                        attach_point.attached_PE_id=current_PE_id;
                        DFG->OP_Array[src_operation_id]->attach_history.push_back(attach_point);

                        if(GLvar::report_level>10){
                            fTrace<<"Store operation "<<src_operation_id<<" from PE "<<last_parent_id<<" in "<<" PE "<<current_PE_id<<" at time "<<transmission_progress_time<<endl;
                        }
                    }

                    //Move the data from data memory to next PE
                    while(true){

                        bool current_PE_memory_read_avail0=CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+3+current_additional_pipeline]->component_reserved->memory_read_reserved[0]==false;
                        bool current_PE_memory_read_avail1=CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+3+current_additional_pipeline]->component_reserved->memory_read_reserved[1]==false;
                        bool current_PE_memory_read_avail2=CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+3+current_additional_pipeline]->component_reserved->memory_read_reserved[2]==false;
                        bool current_PE_memory_write_avail0=CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+3+current_additional_pipeline]->component_reserved->memory_write_reserved[0]==false;
                        bool current_PE_memory_read_avail=(current_PE_memory_read_avail0 || current_PE_memory_read_avail1 || current_PE_memory_read_avail2) && current_PE_memory_write_avail0;
                        bool current_PE_output_avail=CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+5+current_additional_pipeline]->component_reserved->PE_output_reserved[child_id]==false;
                        bool next_PE_input_avail=CGRA->PE_Array[next_PE_id]->Component_Trace[transmission_progress_time+7+current_additional_pipeline]->component_reserved->PE_input_reserved==false;

                        bool next_load_path_avail=true;
                        if(next_PE_id==GLvar::load_PE_id || next_PE_id==GLvar::store_PE_id){
                            next_load_path_avail=CGRA->PE_Array[next_PE_id]->Component_Trace[transmission_progress_time+8+current_additional_pipeline]->component_reserved->load_path_reserved==false;
                        }

                        bool next_PE_memory_write_avail1=CGRA->PE_Array[next_PE_id]->Component_Trace[transmission_progress_time+8+current_additional_pipeline+next_additional_pipeline]->component_reserved->memory_write_reserved[1]==false;
                        bool next_PE_memory_read_avail3=CGRA->PE_Array[next_PE_id]->Component_Trace[transmission_progress_time+8+current_additional_pipeline+next_additional_pipeline]->component_reserved->memory_read_reserved[3]==false;
                        bool next_PE_memory_read_avail4=CGRA->PE_Array[next_PE_id]->Component_Trace[transmission_progress_time+8+current_additional_pipeline+next_additional_pipeline]->component_reserved->memory_read_reserved[4]==false;
                        bool next_PE_memory_read_avail5=CGRA->PE_Array[next_PE_id]->Component_Trace[transmission_progress_time+8+current_additional_pipeline+next_additional_pipeline]->component_reserved->memory_read_reserved[5]==false;
                        bool next_PE_memory_write_avail=next_PE_memory_write_avail1 && next_PE_memory_read_avail3 && next_PE_memory_read_avail4 && next_PE_memory_read_avail5;

                        if(current_PE_memory_read_avail && current_PE_output_avail && next_PE_input_avail && next_PE_memory_write_avail && next_load_path_avail){
                            if(mode==Implementation){
                                if(current_PE_memory_read_avail0){
                                    CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+3+current_additional_pipeline]->component_reserved->memory_read_reserved[0]=true;
                                    CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+3+current_additional_pipeline]->component_activity->memory_port_op[0]=src_operation_id;
                                    CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+5+current_additional_pipeline]->component_activity->PE_output_mux[child_id]=0;
                                }
                                else if(current_PE_memory_read_avail1){
                                    CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+3+current_additional_pipeline]->component_reserved->memory_read_reserved[1]=true;
                                    CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+3+current_additional_pipeline]->component_activity->memory_port_op[1]=src_operation_id;
                                    CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+5+current_additional_pipeline]->component_activity->PE_output_mux[child_id]=1;
                                }
                                else{ 
                                    CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+3+current_additional_pipeline]->component_reserved->memory_read_reserved[2]=true;
                                    CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+3+current_additional_pipeline]->component_activity->memory_port_op[2]=src_operation_id;
                                    CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+5+current_additional_pipeline]->component_activity->PE_output_mux[child_id]=2;
                                }

                                CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+5+current_additional_pipeline]->component_reserved->PE_output_reserved[child_id]=true;
                                CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+3+current_additional_pipeline]->component_activity->memory_wr_ena[0]=0;

                                if(GLvar::report_level>10){
                                    fTrace<< " Move operation " << src_operation_id << " forward: from " << " PE " << current_PE_id << " to " << " PE " << next_PE_id << " at time " << transmission_progress_time+4+current_additional_pipeline << endl;
                                }

                            }
                            transmission_progress_time=transmission_progress_time+6+current_additional_pipeline;
                            i++;
                            break;
                        }
                        else{
                            transmission_progress_time++;
                        }

                    }
                }
            }

            //Arrive in last PE data memory
            else{
                if(mode==Implementation){
                    CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+1]->component_reserved->PE_input_reserved=true;
                    CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+1]->component_activity->PE_input_mux=last_parent_id;
                    if(current_PE_id==GLvar::load_PE_id || current_PE_id==GLvar::store_PE_id){
                        CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+2]->component_reserved->load_path_reserved=true;
                        CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+2]->component_activity->load_mux=1;
                    }
                    CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+2+current_additional_pipeline]->component_reserved->memory_write_reserved[1]=true;
                    CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+2+current_additional_pipeline]->component_activity->memory_wr_ena[1]=1;
                    CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+2+current_additional_pipeline]->component_activity->memory_port_op[3]=src_operation_id;
                    CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+2+current_additional_pipeline]->component_activity->memory_port_op[4]=src_operation_id;
                    CGRA->PE_Array[current_PE_id]->Component_Trace[transmission_progress_time+2+current_additional_pipeline]->component_activity->memory_port_op[5]=src_operation_id;

                    if(GLvar::report_level>10){
                        fTrace<< "Move operation " << src_operation_id << " last: from " << " PE " << last_PE_id << " to PE " << current_PE_id << " at time " << transmission_progress_time << endl;
                    }

                    //Keep the attach point which can be reused later
                    AttachHistory attach_point;
                    attach_point.attached_time=transmission_progress_time+2+current_additional_pipeline;
                    attach_point.attached_PE_id=current_PE_id;
                    DFG->OP_Array[src_operation_id]->attach_history.push_back(attach_point);

                    /*
                       if(src_operation_id==3 && current_PE_id==2 && last_PE_id==0){
                       cout<<"PE_input_mux["<<transmission_progress_time+1<<"]="<<last_parent_id<<endl;
                       cout<<"PE_load_mux["<<transmission_progress_time+2<<"]="<<1<<endl;
                       }*/

                }
                i++;
                total_time=transmission_progress_time+2+current_additional_pipeline;
            }
        }
        last_PE_id=current_PE_id;
    }
    return total_time;

}

int Scheduler::OperationExecution(const int &target_operation_id, const vector<int> &src_operation_ids, const int &target_PE_id, const vector<int> &arrival_time, const ExecutionMode &mode){

    int execution_time;
    int latest_arrival_time=0;
    for(int i=0; i<INSTR_OP_NUM-1; i++){
        if(latest_arrival_time<arrival_time[i]){
            latest_arrival_time=arrival_time[i];
        }
    }

    int start_time=latest_arrival_time;
    while(true){
        bool src_read_avail=true;
        src_read_avail = src_read_avail && (CGRA->PE_Array[target_PE_id]->Component_Trace[start_time+1]->component_reserved->memory_read_reserved[3]==false);
        src_read_avail = src_read_avail && (CGRA->PE_Array[target_PE_id]->Component_Trace[start_time+1]->component_reserved->memory_read_reserved[4]==false);
        src_read_avail = src_read_avail && (CGRA->PE_Array[target_PE_id]->Component_Trace[start_time+1]->component_reserved->memory_read_reserved[5]==false);
        src_read_avail = src_read_avail && (CGRA->PE_Array[target_PE_id]->Component_Trace[start_time+1]->component_reserved->memory_write_reserved[1]==false);

        bool dsp_pipeline_avail=CGRA->PE_Array[target_PE_id]->Component_Trace[start_time+3]->component_reserved->dsp_pipeline_reserved==false;
        bool memory_write_avail=CGRA->PE_Array[target_PE_id]->Component_Trace[start_time+7]->component_reserved->memory_write_reserved[0]==false;
        memory_write_avail=memory_write_avail && (CGRA->PE_Array[target_PE_id]->Component_Trace[start_time+7]->component_reserved->memory_read_reserved[0]==false);
        memory_write_avail=memory_write_avail && (CGRA->PE_Array[target_PE_id]->Component_Trace[start_time+7]->component_reserved->memory_read_reserved[1]==false);
        memory_write_avail=memory_write_avail && (CGRA->PE_Array[target_PE_id]->Component_Trace[start_time+7]->component_reserved->memory_read_reserved[2]==false);

        if(src_read_avail && dsp_pipeline_avail && memory_write_avail){
            break;
        }
        else{
            start_time++;
        }
    }
    execution_time=start_time+7;

    if(mode==Implementation){
        if(GLvar::report_level>10){
            fTrace << "Operation " << target_operation_id << " starts execution on " << " PE " << target_PE_id << " at time " << start_time+3 <<endl;
        }

        TargetPERefresh(src_operation_ids, target_operation_id, start_time, target_PE_id);
        TargetOperationRefresh(src_operation_ids, target_operation_id, target_PE_id, execution_time);

    }

    return execution_time;

}

int Scheduler::Nearest_Attached_PE(const int &Src_OP_ID, const int &Dst_PE_ID, int &Src_Ready_Time){

    int Nearest_Attached_PE_ID = NaN;
    int Min_Dist = INT_MAX;

    std::list<Attach_History>::iterator Lit;
    std::list<Attach_History>::iterator Lit_Head;
    std::list<Attach_History>::iterator Lit_Tail;
    Lit_Head = DFG->OP_Array[Src_OP_ID]->OP_Attach_History.begin();
    Lit_Tail = DFG->OP_Array[Src_OP_ID]->OP_Attach_History.end();
    for(Lit = Lit_Head; Lit != Lit_Tail; Lit++){
        int PE_ID_Tmp = (*Lit).Attached_PE_ID;
        int Dist_Tmp = CGRA->PE_Pair_Dist[PE_ID_Tmp][Dst_PE_ID];
        if(Dist_Tmp < Min_Dist){
            Min_Dist = Dist_Tmp;
            Nearest_Attached_PE_ID = PE_ID_Tmp;
            Src_Ready_Time = (*Lit).Attached_Time;
        }
    }

    // 0 is available for each PE
    if(DFG->OP_Array[Src_OP_ID]->OP_ID == 0){
        Nearest_Attached_PE_ID = Dst_PE_ID;
    }

    if(Nearest_Attached_PE_ID == NaN){
        DEBUG1("Unexpected nearest PE!");
    }

    return Nearest_Attached_PE_ID;

}

int Scheduler::DistCal(const int &src_op, const int &dst_op){
    if(src_op==0 || dst_op==0){
        return 0;
    }
    else{
        int src_PE_id=DFG->OP_Array[src_op]->vertex_attribute.execution_PE_id;
        int dst_PE_id=DFG->OP_Array[dst_op]->vertex_attribute.execution_PE_id;
        return CGRA->PE_pair_distance[src_PE_id][dst_PE_id];
    }
}

int Scheduler::DynamicOperationSelection(){

    //A single input operand is needed to make the target operation ready
    list<int> ready_input_set1; 

    //Two input operands are needed to make the target operation ready
    list<int> ready_input_set2;

    //Three input operands are needed to make the target operation ready
    list<int> ready_input_set3;

    //Operations that have all the source operands ready
    list<int> ready_operation_set;
    list<int> possible_distance;

    for(int i=0; i<DFG->vertex_num; i++){
        Vertex* vp=DFG->OP_Array[i];
        if(vp->vertex_type!=InputData && vp->vertex_attribute.vertex_state==DataUnavail){
            vector<Vertex*>::iterator it;

            //Test whether all the source operands are ready
            bool all_src_ready=true;
            for(it=vp->parents.begin(); it!=vp->parents.end(); it++){
                if((*it)->vertex_attribute.vertex_state==DataUnavail){
                    all_src_ready=false;
                    break;
                }
            }

            if(all_src_ready){
                //If source operands are in out side memory, the operands should be ready to load.
                //If the source operands are all available, the target operation is considered to be ready.
                int out_op_num=0;
                for(it=vp->parents.begin(); it!=vp->parents.end(); it++){
                    if((*it)->vertex_attribute.vertex_state==DataInOutMem){
                        out_op_num++;
                    }
                }
                if(out_op_num==0){
                    ready_operation_set.push_back(i);
                    vector<int> Src_OP_IDs;
                    for(it=vp->parents.begin(); it!=vp->parents.end(); it++){
                        Src_OP_IDs.push_back((*it)->vertex_id);
                    }
                    int sum_dist=DistCal(Src_OP_IDs[0], Src_OP_IDs[1]);
                    sum_dist+=DistCal(Src_OP_IDs[0], Src_OP_IDs[2]);
                    sum_dist+=DistCal(Src_OP_IDs[1], Src_OP_IDs[2]);
                    possible_distance.push_back(sum_dist);
                }
                else if(out_op_num==1){
                    for(it=vp->parents.begin();it!=vp->parents.end(); it++){
                        if((*it)->vertex_attribute.vertex_state==DataInOutMem){
                            ready_input_set1.push_back((*it)->vertex_id);
                        }
                    }
                }
                else if(out_op_num==2){
                    for(it=vp->parents.begin();it!=vp->parents.end(); it++){
                        if((*it)->vertex_attribute.vertex_state==DataInOutMem){
                            ready_input_set2.push_back((*it)->vertex_id);
                        }
                    }
                }
                else if(out_op_num==3){
                    for(it=vp->parents.begin();it!=vp->parents.end(); it++){
                        if((*it)->vertex_attribute.vertex_state==DataInOutMem){
                            ready_input_set3.push_back((*it)->vertex_id);
                        }
                    }
                }
                else{
                    DEBUG2("Unexpected vertex state! %d=", out_op_num);
                }
            }
        }
    }

    if(!ready_operation_set.empty()){
        list<int>::iterator it1;
        list<int>::iterator it2;
        int min_dist=INT_MAX;
        int selected_op=NaN;
        it2=possible_distance.begin();
        for(it1=ready_operation_set.begin(); it1!=ready_operation_set.end(); it1++){
            if((*it2)<min_dist){
                min_dist=(*it2);
                selected_op=(*it1);
            }
            it2++;
        }
        return selected_op;
    }
    else if(!ready_input_set1.empty()){
        return ready_input_set1.front();
    }
    else if(!ready_input_set2.empty()){
        return ready_input_set2.front();
    }
    else if(!ready_input_set3.empty()){
        return ready_input_set3.front();
    }
    else{
        DEBUG1("No operation left for scheduling!");
    }

}

int Scheduler::StaticOperationSelection(){

    list<int> candidate_operation_set;
    int highest_priority=0;

    //Find out the highest priority of operations that have not been executed or fethced.
    for(int i=0; i<DFG->vertex_num; i++){
        Vertex* vertex_tmp=DFG->OP_Array[i];
        if(vertex_tmp->vertex_attribute.vertex_state==DataInOutMem || vertex_tmp->vertex_attribute.vertex_state==DataUnavail){
            int priority_tmp=vertex_tmp->vertex_attribute.scheduling_priority;
            if(priority_tmp>highest_priority){
                highest_priority=priority_tmp;
            }
        }
    }

    //Put these operations with highest available priority in a list
    for(int i=0; i<DFG->vertex_num; i++){
        Vertex* vertex_tmp=DFG->OP_Array[i];
        if(vertex_tmp->vertex_attribute.vertex_state==DataInOutMem || vertex_tmp->vertex_attribute.vertex_state==DataUnavail){
            int priority_tmp=vertex_tmp->vertex_attribute.scheduling_priority;
            if(priority_tmp==highest_priority){
                candidate_operation_set.push_back(i);
            }
        }
    }
    if(candidate_operation_set.empty()==true){
        DEBUG1("No Candidates available before scheduling is completed!");
    }

    //Selected the operation with most children first. Note that it may require larger data memory because data will not be
    //consumed as soon as possible. If data memory is the bottleneck, chooing the operation with least children may help.
    int min_children_num=INT_MAX;
    int selected_operation_id;
    list<int>::iterator iter_tmp;
    for(iter_tmp=candidate_operation_set.begin(); iter_tmp!=candidate_operation_set.end(); iter_tmp++){
        int children_num=DFG->OP_Array[*iter_tmp]->children.size();
        if(min_children_num>children_num){
            min_children_num=children_num;
            selected_operation_id=*iter_tmp;
        }
    }
    return selected_operation_id;

}

void Scheduler::TargetPERefresh(const vector<int> &src_operation_ids, const int &target_operation_id, const int &start_time, const int &target_PE_id){

    CGRA->PE_Array[target_PE_id]->Component_Trace[start_time+1]->component_reserved->memory_read_reserved[3]=true;
    CGRA->PE_Array[target_PE_id]->Component_Trace[start_time+1]->component_reserved->memory_read_reserved[4]=true;
    CGRA->PE_Array[target_PE_id]->Component_Trace[start_time+1]->component_reserved->memory_read_reserved[5]=true;

    CGRA->PE_Array[target_PE_id]->Component_Trace[start_time+3]->component_reserved->dsp_pipeline_reserved=true;
    CGRA->PE_Array[target_PE_id]->Component_Trace[start_time+7]->component_reserved->memory_write_reserved[0]=true;

    CGRA->PE_Array[target_PE_id]->Component_Trace[start_time+1]->component_activity->memory_wr_ena[1]=0;

    OPCODE opcode_tmp=DFG->OP_Array[target_operation_id]->vertex_attribute.opcode;
    CGRA->PE_Array[target_PE_id]->Component_Trace[start_time+3]->component_activity->dsp_opcode=opcode_tmp;
    CGRA->PE_Array[target_PE_id]->Component_Trace[start_time+7]->component_activity->memory_wr_ena[0]=1;

    CGRA->PE_Array[target_PE_id]->Component_Trace[start_time+1]->component_activity->memory_port_op[3]=src_operation_ids[0];
    CGRA->PE_Array[target_PE_id]->Component_Trace[start_time+1]->component_activity->memory_port_op[4]=src_operation_ids[1];
    CGRA->PE_Array[target_PE_id]->Component_Trace[start_time+1]->component_activity->memory_port_op[5]=src_operation_ids[2];
    CGRA->PE_Array[target_PE_id]->Component_Trace[start_time+7]->component_activity->memory_port_op[0]=target_operation_id;
    CGRA->PE_Array[target_PE_id]->Component_Trace[start_time+7]->component_activity->memory_port_op[1]=target_operation_id;
    CGRA->PE_Array[target_PE_id]->Component_Trace[start_time+7]->component_activity->memory_port_op[2]=target_operation_id;

    int current_execution_time=start_time+7;
    if(CGRA->PE_Array[target_PE_id]->maximum_active_time<current_execution_time){
        CGRA->PE_Array[target_PE_id]->maximum_active_time=current_execution_time;
    }

}

void Scheduler::FetchSourceOperation(const int &target_PE_id, const vector<int> &src_operation_ids, const vector<list<int> > &source_routing_paths, const vector<int> &start_time, vector<int> &source_ready_time){

    for(int i=0; i<INSTR_OP_NUM-1; i++){
        source_ready_time[i]=OperationTransmission(start_time[i], src_operation_ids[i], source_routing_paths[i], Implementation);
    }

}

void Scheduler::TargetOperationRefresh(const vector<int> &src_operation_ids, const int &target_operation_id, const int &target_PE_id, const int &execution_time){

    DFG->OP_Array[target_operation_id]->vertex_attribute.vertex_state=DataAvail;
    DFG->OP_Array[target_operation_id]->vertex_attribute.operation_avail_time=execution_time;
    DFG->OP_Array[target_operation_id]->vertex_attribute.execution_PE_id=target_PE_id;
    int srcA=DFG->OP_Array[src_operation_ids[0]]->vertex_value;
    int srcB=DFG->OP_Array[src_operation_ids[1]]->vertex_value;
    int srcC=DFG->OP_Array[src_operation_ids[2]]->vertex_value;
    OPCODE Opcode=DFG->OP_Array[target_operation_id]->vertex_attribute.opcode;
    int Result=op_compute(Opcode, srcA, srcB, srcC);
    DFG->OP_Array[target_operation_id]->vertex_value=Result;

    AttachHistory attach_point;
    attach_point.attached_time=execution_time;
    attach_point.attached_PE_id=target_PE_id;
    DFG->OP_Array[target_operation_id]->attach_history.push_back(attach_point);

}

bool Scheduler::SchedulingIsCompleted(){

    bool scheduling_flag=true;
    //bool break_point_scheduling_flag=true;

    //Check final execution
    for(int i=0; i<DFG->vertex_num; i++){
        //Some of the input may not be used. so the input condition can be ignored.
        /*
           if(DFG->OP_Array[i]->vertex_type==InputData && DFG->OP_Array[i]->vertex_attribute.vertex_state!=DataAvail){
           scheduling_flag=false;
           break;
           }
           */
        if(DFG->OP_Array[i]->vertex_type==IntermediateData && DFG->OP_Array[i]->vertex_attribute.vertex_state!=DataAvail){
            scheduling_flag=false;
            break;
        }
        else if(DFG->OP_Array[i]->vertex_type==OutputData && DFG->OP_Array[i]->vertex_attribute.vertex_state!=DataAvail){
            scheduling_flag=false;
            break;
        }
        else{
            scheduling_flag=true;
        }
    }

    /*
    //Check the breakpoints
    for(int i=0; i<DFG->vertex_num; i++){
    if(DFG->OP_Array[i]->vertex_type==IntermediateData && DFG->OP_Array[i]->vertex_type2==AtBreakPoint && DFG->OP_Array[i]->vertex_attribute.vertex_state!=DataAvail){
    break_point_scheduling_flag=false;
    break;
    }
    else if(DFG->OP_Array[i]->vertex_type==OutputData && (DFG->OP_Array[i]->vertex_type2==AtBreakPoint || DFG->OP_Array[i]->vertex_type2==BeforeBreakPoint) && DFG->OP_Array[i]->vertex_attribute.vertex_state!=DataAvail){
    break_point_scheduling_flag=false;
    break;
    }
    else{
    break_point_scheduling_flag=true;
    }
    }

    //When the scheduling is completed, and there is no break points.
    if(scheduling_flag){
    break_point_scheduling_flag=true;
    }

    //The first time that all break points are executed, lat_op_store_time is considered to be break point execution time. 
    //When there are no breakpoints, break_point_store_time is supposed to be last_op_store_time.
    if(break_point_scheduling_flag && break_point_store_time==0){
    break_point_store_time=last_op_store_time;
    }
    */

    return scheduling_flag;

}

int Scheduler::SchedulingStat(){

    //Analyze scheduling performance
    int final_execution_time=0;
    vector<float> read_memory_utilization;
    vector<float> write_memory_utilization;
    vector<float> output_port_utilization;
    vector<float> dsp_pipeline_utilization;
    output_port_utilization.resize(CGRA->CGRA_Scale);
    read_memory_utilization.resize(CGRA->CGRA_Scale);
    write_memory_utilization.resize(CGRA->CGRA_Scale);
    dsp_pipeline_utilization.resize(CGRA->CGRA_Scale);
    for(int i=0; i<DFG->vertex_num; i++){
        int execution_time_tmp=DFG->OP_Array[i]->vertex_attribute.operation_avail_time;
        if(execution_time_tmp>final_execution_time){
            final_execution_time=execution_time_tmp;
        }
    }

    for(int i=0; i<CGRA->CGRA_Scale; i++){
        int PE_output_counter=0;
        int dsp_pipeline_counter=0;
        int read_memory_counter=0;
        int write_memory_counter=0;
        int output_degree=CGRA->PE_Array[i]->output_degree;
        for(int j=0; j<=final_execution_time; j++){
            for(int p=0; p<6; p++){
                if(CGRA->PE_Array[i]->Component_Trace[j]->component_reserved->memory_read_reserved[p]){
                    read_memory_counter++;
                }
            }
            for(int p=0; p<4; p++){
                if(CGRA->PE_Array[i]->Component_Trace[j]->component_reserved->PE_output_reserved[p]){
                    PE_output_counter++;
                }
            }
            if(CGRA->PE_Array[i]->Component_Trace[j]->component_reserved->dsp_pipeline_reserved){
                dsp_pipeline_counter++;
            }
            for(int p=0; p<2; p++){
                if(CGRA->PE_Array[i]->Component_Trace[j]->component_reserved->memory_write_reserved[p]){
                    write_memory_counter++;
                }
            }
        }
        read_memory_utilization[i]=1.0*read_memory_counter/final_execution_time;
        write_memory_utilization[i]=1.0*write_memory_counter/final_execution_time;
        output_port_utilization[i]=1.0*PE_output_counter/(final_execution_time*output_degree);
        dsp_pipeline_utilization[i]=1.0*dsp_pipeline_counter/final_execution_time;
    }

    //Print resource utilization information
    cout<<setiosflags(ios::left);
    cout<<setfill(' ')<<setw(6)<<"PE";
    cout<<setfill(' ')<<setw(15)<<"output port";
    cout<<setfill(' ')<<setw(15)<<"memory read";
    cout<<setfill(' ')<<setw(16)<<"memory write";
    cout<<setfill(' ')<<setw(18)<<"dsp pipeline";
    cout<<"\n";
    for(int i=0; i<CGRA->CGRA_Scale; i++){
        cout<<setfill(' ')<<setw(6)<<i;
        cout<<setfill(' ')<<setw(15)<<setprecision(4)<<output_port_utilization[i];
        cout<<setfill(' ')<<setw(15)<<setprecision(4)<<read_memory_utilization[i];
        cout<<setfill(' ')<<setw(16)<<setprecision(4)<<write_memory_utilization[i];
        cout<<setfill(' ')<<setw(18)<<setprecision(4)<<dsp_pipeline_utilization[i]<<"\n";
    }

    //Print link utilization information
    CGRA->LinkUtilizationAnalysis(0, final_execution_time);

    //Dump read0 port operations
    /*for(int i=0; i<CGRA->CGRA_Scale; i++){
      if(i!=5){
      continue;
      }
      for(int j=0; j<final_execution_time; j++){
      int current_output_id=CGRA->PE_Array[i]->Component_Trace[j]->component_activity->memory_read_addr[0];
      bool read_enable=CGRA->PE_Array[i]->Component_Trace[j]->component_activity->memory_read_enable[0];
      if(read_enable){
      cout<<setfill(' ')<<setw(10)<<j;
      cout<<setfill(' ')<<setw(6)<<current_output_id<<endl;
      }
      }
      }*/
    return final_execution_time;

}

void Scheduler::DataMemoryAnalysis(){

    //Analyze the data memory capacity
    vector<int> data_mem_capacity;
    data_mem_capacity.resize(CGRA->CGRA_Scale);
    for(int i=0; i<CGRA->CGRA_Scale; i++){
        data_mem_capacity[i]=0;
    }
    vector<int> birth_time;
    vector<int> die_time;
    birth_time.resize(GLvar::maximum_operation_num);
    die_time.resize(GLvar::maximum_operation_num);

    for(int i=0; i<CGRA->CGRA_Scale; i++){

        for(int j=0; j<GLvar::maximum_operation_num; j++){
            birth_time[j]=NaN;
            die_time[j]=NaN;
        }
        birth_time[0]=0;

        //Refresh the die time in memory write port
        for(int j=0; j<GLvar::maximum_simulation_time; j++){
            for(int p=0; p<2; p++){
                int wr_op;
                if(p==0){
                    wr_op=CGRA->PE_Array[i]->Component_Trace[j]->component_activity->memory_port_op[0];
                }
                else{
                    wr_op=CGRA->PE_Array[i]->Component_Trace[j]->component_activity->memory_port_op[3];
                }

                bool wr_reserve=CGRA->PE_Array[i]->Component_Trace[j]->component_reserved->memory_write_reserved[p];
                if(wr_reserve){

                    if(birth_time[wr_op]==NaN || birth_time[wr_op]>j){
                        birth_time[wr_op]=j;
                    }

                }
            }

            //Refresh the birth time & die time in memory reading port
            for(int p=0; p<6; p++){

                bool rd_reserve=CGRA->PE_Array[i]->Component_Trace[j]->component_reserved->memory_read_reserved[p];
                int rd_op=CGRA->PE_Array[i]->Component_Trace[j]->component_activity->memory_port_op[p];
                if(rd_reserve){

                    if(die_time[rd_op]==NaN || die_time[rd_op]<j){
                        die_time[rd_op]=j;
                    }

                }
            }
        }

        //Check the birth time and die time to see if there are conflictions.
        for(int j=0; j<GLvar::maximum_operation_num; j++){
            if(birth_time[j]==NaN && die_time[j]>0){

                cout<<"op is "<<j<<" , it has "<<DFG->OP_Array[j]->children.size()<<"children and "<<DFG->OP_Array[j]->parents.size()<<"parents!";
                cout<<"execution PE id is "<<DFG->OP_Array[j]->vertex_attribute.execution_PE_id<<endl;
                cout<<"executed time is "<<DFG->OP_Array[j]->vertex_attribute.operation_avail_time<<endl;
                if(DFG->OP_Array[j]->vertex_type==InputData){
                    cout<<"Input Operation"<<endl;
                }
                else if(DFG->OP_Array[j]->vertex_type==OutputData){
                    cout<<"Output operation"<<endl;
                }
                else{
                    cout<<"Intermediate operation"<<endl;
                }
                DEBUG1("Error!\n");

            }
        }

        vector<int> data_mem_trace;
        data_mem_trace.resize(GLvar::maximum_simulation_time);
        for(int j=0; j<GLvar::maximum_simulation_time; j++){
            data_mem_trace[j]=0;
        }

        int mem_counter=0;
        for(int j=0; j<GLvar::maximum_simulation_time; j++){
            for(int p=0; p<GLvar::maximum_operation_num; p++){
                if(birth_time[p]==j){
                    mem_counter++;
                }
                if(die_time[p]==j){
                    mem_counter--;
                }
            }
            data_mem_trace[j]=mem_counter;
            if(data_mem_capacity[i]<mem_counter){
                data_mem_capacity[i]=mem_counter;
            }
        }

        AddrGen(birth_time, die_time, data_mem_capacity[i], i);
    }

    //print data memory capacity of each PE
    for(int i=0; i<CGRA->CGRA_Scale; i++){
        cout<<data_mem_capacity[i]<<" ";
    }
    cout<<endl;

}

/*---------------------------------------------------------------------
  Address allocation/release
  1) When an operand is written to the data memory for the first time.
  2) When operands are constants and they are initialized into the 
  data memory. Here only op_id==0 is considered as constant and it is 
  stored in data memory[0] in all PEs. 
  3) When the data operands are referenced for the last time, the address 
  will be released.
 -----------------------------------------------------------------------*/
void Scheduler::AddrGen(const vector<int> &birth_time, const vector<int> &die_time, const int &memCapacity, const int &PE_id){

    map<int, int> OpToAddr;
    OpToAddr[0]=0;

    list<int> AddrAvail;
    for(int i=1; i<memCapacity+10; i++){
        AddrAvail.push_back(i);
    }

    //Allocate address to data initialized in data memory
    for(int i=1; i<GLvar::maximum_operation_num; i++){
        if(birth_time[i]==0){
            OpToAddr[i]=AddrAvail.front();
            AddrAvail.pop_front();
        }
    }

    //Dump the initial data image of the data memory
    //DataMemoryInit(OpToAddr, PE_id, memCapacity+1);

    for(int i=0; i<GLvar::maximum_simulation_time; i++){
        for(int p=0; p<2; p++){
            int wr_op;
            if(p==0){
                wr_op=CGRA->PE_Array[PE_id]->Component_Trace[i]->component_activity->memory_port_op[0];
            }
            else{
                wr_op=CGRA->PE_Array[PE_id]->Component_Trace[i]->component_activity->memory_port_op[3];
            }

            //Allocate address when the data is first writen into data memory
            bool wr_reserve=CGRA->PE_Array[PE_id]->Component_Trace[i]->component_reserved->memory_write_reserved[p];
            if(wr_reserve){
                if(OpToAddr.count(wr_op)==0){
                    if(p==0){
                        CGRA->PE_Array[PE_id]->Component_Trace[i]->component_activity->memory_addr[0]=AddrAvail.front();
                        CGRA->PE_Array[PE_id]->Component_Trace[i]->component_activity->memory_addr[1]=AddrAvail.front();
                        CGRA->PE_Array[PE_id]->Component_Trace[i]->component_activity->memory_addr[2]=AddrAvail.front();
                    }
                    else{
                        CGRA->PE_Array[PE_id]->Component_Trace[i]->component_activity->memory_addr[3]=AddrAvail.front();
                        CGRA->PE_Array[PE_id]->Component_Trace[i]->component_activity->memory_addr[4]=AddrAvail.front();
                        CGRA->PE_Array[PE_id]->Component_Trace[i]->component_activity->memory_addr[5]=AddrAvail.front();
                    }
                    OpToAddr[wr_op]=AddrAvail.front();
                    AddrAvail.pop_front();
                }
                else if(OpToAddr.count(wr_op)>0){
                    if(p==0){
                        CGRA->PE_Array[PE_id]->Component_Trace[i]->component_activity->memory_addr[0]=OpToAddr[wr_op];
                        CGRA->PE_Array[PE_id]->Component_Trace[i]->component_activity->memory_addr[1]=OpToAddr[wr_op];
                        CGRA->PE_Array[PE_id]->Component_Trace[i]->component_activity->memory_addr[2]=OpToAddr[wr_op];
                    }
                    else{
                        CGRA->PE_Array[PE_id]->Component_Trace[i]->component_activity->memory_addr[3]=OpToAddr[wr_op];
                        CGRA->PE_Array[PE_id]->Component_Trace[i]->component_activity->memory_addr[4]=OpToAddr[wr_op];
                        CGRA->PE_Array[PE_id]->Component_Trace[i]->component_activity->memory_addr[5]=OpToAddr[wr_op];
                    }
                }
                else{
                    DEBUG1("Unexpected write operation!");
                }
            }
        }

        list<int> op_to_release;
        list<int>::iterator it;
        for(int p=0; p<6; p++){
            bool rd_reserve=CGRA->PE_Array[PE_id]->Component_Trace[i]->component_reserved->memory_read_reserved[p];
            int rd_op=CGRA->PE_Array[PE_id]->Component_Trace[i]->component_activity->memory_port_op[p];
            if(rd_reserve){
                if(OpToAddr.count(rd_op)==0){
                    cout<<"op="<<rd_op<<endl;
                    cout<<"executed at PE "<<DFG->OP_Array[rd_op]->vertex_attribute.execution_PE_id<<endl;
                    cout<<"current PE is "<<PE_id<<endl;
                    cout<<"execution time="<<DFG->OP_Array[rd_op]->vertex_attribute.operation_avail_time<<endl;
                    cout<<"current time="<<i<<endl;
                    cout<<"port number="<<p<<endl;
                    cout<<"Bram addr="<<OpToAddr[rd_op]<<endl;
                    DEBUG1("The operation has never been stored at all!");
                }

                if(OpToAddr.count(rd_op)>0){
                    if(die_time[rd_op]==NaN){
                        DEBUG1("Unexpected cases!");
                    }

                    CGRA->PE_Array[PE_id]->Component_Trace[i]->component_activity->memory_addr[p]=OpToAddr[rd_op];
                    if(die_time[rd_op]==i){
                        bool no_replica=true;
                        for(it=op_to_release.begin(); it!=op_to_release.end(); it++){
                            if(rd_op==*it){
                                no_replica=false;
                                break;
                            }
                        }
                        if(no_replica && rd_op!=0){
                            op_to_release.push_back(rd_op);
                        }
                    }
                }
                else{
                    DEBUG1("Unexpected cases!");
                }
            }
        }

        //Put the released address back for reuse
        while(!op_to_release.empty()){
            int released_op=op_to_release.front();
            int released_addr=OpToAddr[released_op];
            AddrAvail.push_front(released_addr);
            op_to_release.pop_front();
            OpToAddr.erase(released_op);
        }
    }

}

void Scheduler::SchedulingResultCollection(vector<int> &operation_result){

    operation_result.resize(GLvar::maximum_operation_num);
    for(int i=0; i<GLvar::maximum_operation_num; i++){
        operation_result[i]=DFG->OP_Array[i]->vertex_value;
    }

}

bool Scheduler::OperationResultCheck(){

    bool all_right=true;
    vector<int> theoretical_result;
    vector<int> simulated_result;
    DFG->DFGCalculation(theoretical_result);
    SchedulingResultCollection(simulated_result);

    for(int i=0; i<GLvar::maximum_operation_num; i++){
        if(theoretical_result[i]!=simulated_result[i]){
            DEBUG1("Calculation of Operation[%d] is wrong! Theoretical result is:%d, simulated result is:%d\n",i,theoretical_result[i], simulated_result[i]);
            all_right=false;
        }
    }

    if(all_right){
        cout<<"Scheduling algorithm obtains the results as expected!"<<endl;
    }
    else{
        DEBUG2("Operation results are NOT correct!");
    }

    return all_right;

}

void Scheduler::InstructionDumpCoe(int final_execution_time){

    for(int i=0; i<CGRA->CGRA_Scale; i++){

        ostringstream os;
        os<<"./result/PE-"<<"inst-"<<i<<".coe";
        string fName=os.str();
        ofstream fHandle;
        fHandle.open(fName.c_str());
        if(!fHandle.is_open()){
            DEBUG1("Fail to create the PE-inst-.coe");
        }

        fHandle << "memory_initialization_radix=2;" << endl;
        fHandle << "memory_initialization_vector=" << endl;
        list<int> bitList;
        list<int>::reverse_iterator it;
        int dec_data;
        int width;
        for(int j=0; j<final_execution_time; j++){

            //The highest 3 bits are reserved for future extension and they keep 0 at the moment.
            fHandle << "100";

            //load-mux, 1->input from neighboring PEs. 0->input from outside memory.
            if(i==GLvar::load_PE_id || i==GLvar::store_PE_id){
                fHandle << CGRA->PE_Array[i]->Component_Trace[j]->component_activity->load_mux;
            }
            else{
                fHandle << "0";
            }

            //PE input mux
            dec_data=CGRA->PE_Array[i]->Component_Trace[j]->component_activity->PE_input_mux;
            switch (dec_data){
                case 0:
                    fHandle << "00";
                    break;
                case 1:
                    fHandle << "01";
                    break;
                case 2:
                    fHandle << "10";
                    break;
                case 3:
                    fHandle << "11";
                    break;
                default:
                    DEBUG1("Unexpected PE_input_mux value!");
                    break;
            }

            //PE bypass mux
            dec_data=CGRA->PE_Array[i]->Component_Trace[j]->component_activity->PE_bypass_mux;
            switch (dec_data){
                case 0:
                    fHandle << "00";
                    break;
                case 1:
                    fHandle << "01";
                    break;
                case 2:
                    fHandle << "10";
                    break;
                case 3:
                    fHandle << "11";
                    break;
                default:
                    DEBUG1("unexpected PE_bypass_mux value!");
                    break;
            }

            //Memory ena
            fHandle << CGRA->PE_Array[i]->Component_Trace[j]->component_activity->memory_wr_ena[1];
            fHandle << CGRA->PE_Array[i]->Component_Trace[j]->component_activity->memory_wr_ena[0];

            //Memory addr
            int port[6]={3,4,5,0,1,2}; 
            for(int l=0; l<6; l++){
                dec_data=CGRA->PE_Array[i]->Component_Trace[j]->component_activity->memory_addr[port[l]];
                width=8;
                if(dec_data==NaN){
                    dec_data=0;
                }
                while(dec_data!=1 && dec_data!=0){
                    bitList.push_back(dec_data%2);
                    dec_data=dec_data/2;
                    width--;
                }
                bitList.push_back(dec_data);
                width--;
                while(width!=0){
                    int tmp=0;
                    bitList.push_back(tmp);
                    width--;
                }
                for(it=bitList.rbegin(); it!=bitList.rend(); it++){
                    fHandle << (*it);
                }
                bitList.clear();
            }

            //dsp_opcode
            OPCODE opcode_tmp = CGRA->PE_Array[i]->Component_Trace[j]->component_activity->dsp_opcode;
            dec_data = opcode2int(opcode_tmp);
            if(dec_data==0){
                fHandle << "0000";
            }
            else if(dec_data==1){
                fHandle << "0001";
            }
            else if(dec_data==2){
                fHandle << "0010";
            }
            else if(dec_data==3){
                fHandle << "0011";
            }
            else if(dec_data==4){
                fHandle << "0100";
            }
            else if(dec_data==5){
                fHandle << "0101";
            }
            else if(dec_data==6){
                fHandle << "0110";
            }
            else if(dec_data==7){
                fHandle << "0111";
            }
            else if(dec_data==8){
                fHandle << "1000";
            }
            else if(dec_data==9){
                fHandle << "1001";
            }
            else if(dec_data==10){
                fHandle << "1010";
            }
            else if(dec_data==11){
                fHandle << "1011";
            }
            else if(dec_data==12){
                fHandle << "1100";
            }
            else if(dec_data==13){
                fHandle << "1101";
            }
            else if(dec_data==14){
                fHandle << "1110";
            }
            else{
                fHandle << "1111";
            }

            //Store mux
            if(i==GLvar::load_PE_id || i==GLvar::store_PE_id){
                dec_data=CGRA->PE_Array[i]->Component_Trace[j]->component_activity->store_mux;
                switch (dec_data){
                    case 0:
                        fHandle << "00";
                        break;
                    case 1:
                        fHandle << "01";
                        break;
                    case 2:
                        fHandle << "10";
                        break;
                    case 3:
                        fHandle << "11";
                        break;
                    default:
                        DEBUG1("unexpected store_mux value!");
                        break;
                }
            }
            else{
                fHandle << "00";
            }

            //PE output mux
            for(int l=0; l<4; l++){
                dec_data=CGRA->PE_Array[i]->Component_Trace[j]->component_activity->PE_output_mux[l];
                switch (dec_data){
                    case 0:
                        fHandle << "00";
                        break;
                    case 1:
                        fHandle << "01";
                        break;
                    case 2:
                        fHandle << "10";
                        break;
                    case 3:
                        fHandle << "11";
                        break;
                    default:
                        DEBUG1("Unexpected PE_output_mux value!");
                        break;
                }
            }

            fHandle<<endl;
        }
        fHandle.close();
    }
}

void Scheduler::Bin2Mif(const string &BinFileName, const string &HexFileName, const int &DataWidth){
    ifstream BinFileHandle;
    BinFileHandle.open(BinFileName.c_str());
    if(!BinFileHandle.is_open()){
        DEBUG1("Failed to open %s\n", BinFileName.c_str());  
    }

    ofstream HexFileHandle;
    HexFileHandle.open(HexFileName.c_str());
    if(!HexFileHandle.is_open()){
        DEBUG1("Failed to create %s\n", HexFileName.c_str());
    }

    char BinVec[100];
    int LineNum=0;
    while(BinFileHandle.getline(BinVec,100)){
        LineNum++;

        //Ignore the first two lines due to coe file format.
        if(LineNum<3){
            continue;
        }

        HexFileHandle << "0x";
        for(int k=0; k<DataWidth/4; k++){
            int id=k*4;
            HexFileHandle << Bin2Hex(&BinVec[id]);
        }
        HexFileHandle<<endl;
    }
    HexFileHandle.close();
    BinFileHandle.close();
}

void Scheduler::AddrBufferDumpMem(){
    const int BufferDepth=4096;
    const int BufferWidth=18;

    string fMemName="./result/addr-buffer.mem";
    ofstream fMemHandle;
    fMemHandle.open(fMemName.c_str());
    if(!fMemHandle.is_open()){
        DEBUG1("Fail to create addr-buffer.mem\n");
    }

    char vec[100];
    for(int i=0; i<2; i++){
        int initAddr=i*BufferDepth*BufferWidth/8;
        char hexAddr[20];
        if(i==0){
            sprintf(hexAddr, "@00000000");
        }
        else{
            sprintf(hexAddr, "@%08X", initAddr);
        }
        fMemHandle << hexAddr <<endl;

        ostringstream os;
        os<<"./result/"<<"outside-bram-addr-"<<i<<".coe";
        string fName=os.str();
        ifstream fHandle;
        fHandle.open(fName.c_str());
        if(!fHandle.is_open()){
            DEBUG1("Failed to open the outside-addr-buffer.coe");
        }

        int lineNum=0;
        while(fHandle.getline(vec,100)){
            lineNum++;
            if(lineNum<3){
                continue;
            }

            char mVec0[100];
            char mVec1[100];
            mVec0[0] = '0';
            mVec0[1] = '0';
            mVec0[2] = '0';
            mVec1[0] = '0';
            mVec1[1] = '0';
            mVec1[2] = '0';
            for(int k=3; k<12; k++){
                mVec1[k] = vec[k-3];
                mVec0[k] = vec[k+6];
            }
            for(int k=12; k<100; k++){
                mVec0[k] = vec[18];
                mVec1[k] = vec[18];
            }

            // Transform the bin to be hex
            for(int k=0; k<(BufferWidth/2+3)/4; k++){
                int id=k*4;
                fMemHandle << Bin2Hex(&mVec0[id]);
            }
            fMemHandle << endl;

            for(int k=0; k<(BufferWidth/2+3)/4; k++){
                int id=k*4;
                fMemHandle << Bin2Hex(&mVec1[id]);
            }
            fMemHandle << endl;

        }
        fHandle.close();
    }
    fMemHandle.close();
}


void Scheduler::InstructionDumpMem(){
    const int instMemDepth=INST_MEM_DEPTH;
    const int instMemWidth=72;

    string fMemName="./result/inst.mem";
    ofstream fMemHandle;
    fMemHandle.open(fMemName.c_str());
    if(!fMemHandle.is_open()){
        DEBUG1("Failed to create inst.mem\n");
    }

    char vec[100];
    for(int i=0; i<CGRA->CGRA_Scale; i++){
        int intAddr=i*instMemDepth*instMemWidth/8;

        char hexAddr[20];
        if(i==0){
            sprintf(hexAddr, "@00000000");
        }
        else{
            sprintf(hexAddr, "@%08X", intAddr);
        }
        fMemHandle << hexAddr <<endl;

        ostringstream os;
        os<<"./result/PE-"<<"inst-"<<i<<".coe";
        string fName=os.str();
        ifstream fHandle;
        fHandle.open(fName.c_str());
        if(!fHandle.is_open()){
            DEBUG1("Failed to open the PE.coe");
        }

        if(instMemDepth==1024){
            int lineNum=0;
            while(fHandle.getline(vec,100)){
                lineNum++;

                if(lineNum<3){
                    continue;
                }

                // ----------------------------------------------------------------------------------------
                // You can't initialize the ROM block correctly using the raw data. The following steps 
                // will show how the 72bit raw data should be reorganized before it goes to data2mem command. 
                // I figured it out by comparing the original data and the dumped data from bitstream. It 
                // took me quite a fucking long time.
                // ----------------------------------------------------------------------------------------

                // Exchange the higher 36bits and lower 36bits
                char mVec[instMemWidth];
                char nVec[instMemWidth];
                for(int k=0; k<instMemWidth/2; k++){
                    nVec[k]=vec[k+instMemWidth/2];
                    nVec[k+instMemWidth/2]=vec[k];
                }

                // Exchanged data: (1bit, 8bit), (1bit, 8bit), ... The first bit will be stored in BRAM
                // parity section, while the following 8bit will be stored in BRAM data section.
                // Put the 4 parity bit together and leave the rest untouched. The new format should be
                // (4bit), (8bit), (8bit), ...
                int basicLen=9;
                int kmax=instMemWidth/2/basicLen;
                for(int k=0; k<kmax; k++){
                    mVec[k]=nVec[k*basicLen];
                    mVec[k+instMemWidth/2]=nVec[k*basicLen+instMemWidth/2];
                    for(int index=1; index<basicLen; index++){
                        mVec[kmax+k*(basicLen-1)+index-1]=nVec[k*basicLen+index];
                        mVec[kmax+k*(basicLen-1)+index-1+instMemWidth/2]=nVec[k*basicLen+index+instMemWidth/2];
                    }
                }

                /*
                   if(lineNum==9 || lineNum==10){
                   cout << "new= " << mVec << endl;
                   }
                   */

                // Transform the bin to be hex
                for(int k=0; k<instMemWidth/4; k++){
                    int id=k*4;
                    fMemHandle << Bin2Hex(&mVec[id]);
                }
                fMemHandle<<endl;
            }
        }
        else if(instMemDepth==512){
            int lineNum=0;
            while(fHandle.getline(vec,100)){
                lineNum++;

                if(lineNum<3){
                    continue;
                }

                // ----------------------------------------------------------------------------------------
                // You can't initialize the ROM block correctly using the raw data. The following steps 
                // will show how the 72bit raw data should be reorganized before it goes to data2mem command. 
                // I figured it out by comparing the original data and the dumped data from bitstream. It 
                // took me quite a fucking long time.
                // ----------------------------------------------------------------------------------------

                // Exchange the higher 36bits and lower 36bits
                char mVec[instMemWidth];

                // Exchanged data: (1bit, 8bit), (1bit, 8bit), ... The first bit will be stored in BRAM
                // parity section, while the following 8bit will be stored in BRAM data section.
                // Put the 4 parity bit together and leave the rest untouched. The new format should be
                // (4bit), (8bit), (8bit), ...
                int basicLen=9;
                int kmax=instMemWidth/basicLen;
                for(int k=0; k<kmax; k++){
                    mVec[k]=vec[k*basicLen];
                    for(int index=1; index<basicLen; index++){
                        mVec[kmax+k*(basicLen-1)+index-1]=vec[k*basicLen+index];
                    }
                }

                /*
                   if(lineNum==9 || lineNum==10){
                   cout << "new= " << mVec << endl;
                   }
                   */

                // Transform the bin to be hex
                for(int k=0; k<instMemWidth/4; k++){
                    int id=k*4;
                    fMemHandle << Bin2Hex(&mVec[id]);
                }
                fMemHandle<<endl;
            }
        }
        fHandle.close();
    }
    fMemHandle.close();
}

void Scheduler::DataMemoryInit(map<int, int> &OpToAddr, const int &PE_id, const int &memory_capacity){
    //Data memory initialization
    vector<int> memory_data;
    memory_data.resize(memory_capacity);
    for(int i=0; i<memory_capacity; i++){
        memory_data[i]=0;
    }

    //Get the initial value of data memory
    for(int i=0; i<GLvar::maximum_operation_num; i++){
        if(OpToAddr.count(i)>0){
            int addr=OpToAddr[i];
            memory_data[addr]=DFG->OP_Array[i]->vertex_value;
        }
    }

    //Open a file to store initialized data of the data memory
    ostringstream os;
    os<<"./result/PE-"<<"mem-"<<PE_id<<".coe";
    string fName=os.str();
    ofstream fHandle;
    fHandle.open(fName.c_str());
    if(!fHandle.is_open()){
        DEBUG1("Fail to create the PE-mem.coe");
    }

    //Transform the decimal data to be binary data and store them in the file.
    vector<int>bitList;
    for(int i=0; i<memory_capacity; i++){
        int dec_data=memory_data[i];
        int width=16;
        while(dec_data!=1 && dec_data!=0){
            bitList.push_back(dec_data%2);
            dec_data=dec_data/2;
            width--;
        }
        bitList.push_back(dec_data);
        width--;
        while(width!=0){
            int tmp=0;
            bitList.push_back(tmp);
            width--;
        }

        vector<int>::reverse_iterator it;
        for(it=bitList.rbegin(); it!=bitList.rend(); it++){
            fHandle<<(*it);
        }
        fHandle<<endl;
        bitList.clear();
    }
}

void Scheduler::DataMemoryDumpMem(){
    const int DataMemDepth=1024;
    const int DataMemWidth=16;
    const int readWidth=20;

    string fMemName="./result/data.mem";
    ofstream fMemHandle;
    fMemHandle.open(fMemName.c_str());
    if(!fMemHandle.is_open()){
        DEBUG1("Fail to create data.mem\n");
    }

    char vec[readWidth];
    for(int i=0; i<CGRA->CGRA_Scale; i++){
        int intAddr=i*DataMemDepth*DataMemWidth/8;

        char hexAddr[20];
        if(i==0){
            sprintf(hexAddr, "@00000000");
        }
        else{
            sprintf(hexAddr, "@0000%X", intAddr);
        }
        fMemHandle << hexAddr <<endl;

        ostringstream os;
        os<<"./result/PE-"<<"mem-"<<i<<".coe";
        string fName=os.str();
        ifstream fHandle;
        fHandle.open(fName.c_str());
        if(!fHandle.is_open()){
            DEBUG1("Fail to open the PE-mem.coe");
        }

        while(fHandle.getline(vec,readWidth)){

            char mVec[DataMemWidth];
            for(int k=0; k<DataMemWidth; k++){
                mVec[k]=vec[k];
            }

            // Dump the bin to be hex
            for(int k=0; k<DataMemWidth/4; k++){
                int id=k*4;
                fMemHandle << Bin2Hex(&mVec[id]);
            }
            fMemHandle << endl;
        }
        fHandle.close();
    }
    fMemHandle.close();
}

void Scheduler::SchedulingResultDump(){
    ostringstream os;
    os<<"./result/dst-"<<"op"<<".txt";
    string fName=os.str();
    ofstream fHandle;
    fHandle.open(fName.c_str());
    if(!fHandle.is_open()){
        DEBUG1("Fail to create the dst-op.txt");
    }

    for(int i=0; i<GLvar::maximum_operation_num; i++){
        //if(DFG->OP_Array[i]->vertex_type==OutputData){
        fHandle<<DFG->OP_Array[i]->vertex_id<<" ";
        fHandle<<DFG->OP_Array[i]->vertex_value<<" ";
        fHandle<<DFG->OP_Array[i]->vertex_attribute.execution_PE_id<<" ";
        fHandle<<endl;
        //}
    }

    fHandle.close();
}

void Scheduler::LoadIOMapping(std::vector<int> &raw_data, int &row, int &col){

    std::ostringstream oss; 
    oss << "./config/" << DFG->DFG_name << "_kernel_io.txt";
    std::string fName = oss.str();
    std::ifstream fHandle;
    fHandle.open(fName.c_str());
    if(!fHandle.is_open()){
        DEBUG1("%s open error!", fName.c_str());
    }

    while(!fHandle.eof()){
        if(fHandle.fail()){
            break;
        }
        int tmp;
        fHandle >> tmp;
        raw_data.push_back(tmp);
    }

    fHandle.clear();
    fHandle.seekg(0, std::ios::beg);

    row=0;
    std::string unused;
    while(std::getline(fHandle, unused)){
        row++;
    }

    col = raw_data.size()/row;
    fHandle.close();

}

void Scheduler::OutsideAddrMemoryDumpCoe(int final_execution_time){
    // Load op->addr information to an array, each column indicates the corresponding iteration.
    // The first column represents the op id and the rest columns represent addr of different iterations.
    int row, col;
    std::vector<int> raw_data;
    LoadIOMapping(raw_data, row, col);
    std::map<int, int> opid_to_row_index; //Map kernel op id to row index of the addr array.
    for(int i=0; i<row; i++){
        opid_to_row_index[raw_data[i*col+0]] = i;
    }

    int outside_bram_num=2;
    int IO_PE[2]={0,1};
    for(int i=0; i<outside_bram_num; i++){

        ostringstream os;
        os<<"./result/outside-"<<"bram-addr-"<<i<<".coe";
        string fName=os.str();
        ofstream fHandle;
        fHandle.open(fName.c_str());
        if(!fHandle.is_open()){
            DEBUG1("Fail to create the outside-bram-addr-.coe");
        }

        fHandle << "memory_initialization_radix=2;" <<endl;
        fHandle << "memory_initialization_vector=" <<endl;
        int IO_PE_id=IO_PE[i];
        vector<unsigned int> bram_addr;
        bram_addr.resize(final_execution_time+2);
        vector<int> load_store_idle; //0->load, 1->store, 2->idle
        load_store_idle.resize(final_execution_time+2);
        for(int j=0; j<final_execution_time+1; j++){
            bram_addr[j]=0;
            load_store_idle[j]=2;
        }

        for(int kit=1; kit<col; kit++){
            for(int j=1; j<final_execution_time; j++){

                bool load_active=CGRA->PE_Array[IO_PE_id]->Component_Trace[j]->component_reserved->load_path_reserved==true;
                int load_mux=CGRA->PE_Array[IO_PE_id]->Component_Trace[j]->component_activity->load_mux;
                bool store_active=CGRA->PE_Array[IO_PE_id]->Component_Trace[j]->component_reserved->store_path_reserved==true;
                if(load_active && load_mux==0){
                    if(load_store_idle[j-1]==1){
                        DEBUG1("Unexpected load state!\n");
                    }
                    load_store_idle[j-1]=0;
                    int loaded_op = CGRA->PE_Array[IO_PE_id]->Component_Trace[j-1]->component_activity->load_op;
                    //bram_addr[j-1] = DFG->OP_Array[loaded_op]->vertex_bram_addr;
                    int row_index = opid_to_row_index[loaded_op];
                    bram_addr[j-1] = raw_data[row_index*col+kit];
                }

                if(store_active){
                    if(load_store_idle[j]==0){
                        DEBUG1("Unexpected store state!\n");
                    }
                    load_store_idle[j+2]=1;
                    int stored_op = CGRA->PE_Array[IO_PE_id]->Component_Trace[j]->component_activity->store_op;
                    //bram_addr[j+2] = DFG->OP_Array[stored_op]->vertex_bram_addr;
                    int row_index = opid_to_row_index[stored_op];
                    bram_addr[j+2] = raw_data[row_index*col+kit];
                }

            }

            list<int> bitList;
            list<int>::reverse_iterator it;
            int dec_data;
            int width; 
            for(int j=0; j<final_execution_time+2; j++){

                //The highest bit indicates the enable signal of the bram and the 
                //following 4bits represent the byte wena signals.
                if(load_store_idle[j]==0){
                    fHandle << "10";
                }
                else if(load_store_idle[j]==1){
                    fHandle << "11";
                }
                else{
                    fHandle << "00";
                }

                /* ----------------------------------------------------------------
                 * The following 3 bits i.e. [26:24] represent the 
                 * computation status.
                 * 001 kernel computation is done, and CPU will be acknowledged.
                 * 010 One iteration of the CGRA computation is done. The next iteration 
                 *     will continue after a few cycles' preparation (controlling delay).
                 * 100 CGRA computation is on going.
                 * --------------------------------------------------------------*/
                if(j==(final_execution_time+1) && kit==col-1){
                    fHandle << "001";
                }
                else if(j==(final_execution_time+1) && kit<col-1){
                    fHandle << "010";
                }
                else{
                    fHandle << "100"; // The CGRA kernel iterates only once.
                }

                //Transform decimal addr to 13-bit binary 
                dec_data=bram_addr[j];
                width=13;
                if(dec_data==NaN){
                    dec_data=0;
                }
                while(dec_data!=1 && dec_data!=0){
                    bitList.push_back(dec_data%2);
                    dec_data=dec_data/2;
                    width--;
                }
                bitList.push_back(dec_data);
                width--;
                while(width!=0){
                    int tmp=0;
                    bitList.push_back(tmp);
                    width--;
                }
                for(it=bitList.rbegin(); it!=bitList.rend(); it++){
                    fHandle << (*it);
                }
                bitList.clear();

                fHandle<<endl;
            }

            //When the kernel iterates more than once, additional 5 lines should be added.
            if(row>2 && kit<col-1){
                fHandle << "0010000000000000000" << std::endl;
                fHandle << "0010000000000000000" << std::endl;
                fHandle << "0010000000000000000" << std::endl;
                fHandle << "0010000000000000000" << std::endl;
                fHandle << "0010000000000000000" << std::endl;
                fHandle << "0010000000000000000" << std::endl;
            }

        }
        fHandle.close();

    }

    //Bin2Mif("./result/outside-bram-addr-0.coe", "./result/outside-bram-addr-0.mif", 32);
    //Bin2Mif("./result/outside-bram-addr-1.coe", "./result/outside-bram-addr-1.mif", 32);
    //Bin2Mif("./result/outside-data-memory-0.coe", "./result/outside-data-memory-0.mif", 32);
    //Bin2Mif("./result/outside-data-memory-1.coe", "./result/outside-data-memory-1.mif", 32);
    Bin2HeadFile("./result/outside-bram-addr-0.coe", "./result/src-ctrl-words.h", "SrcMemCtrlWords", 16);
    Bin2HeadFile("./result/outside-bram-addr-1.coe", "./result/result-ctrl-words.h", "ResultMemCtrlWords", 16);
    Bin2HeadFile("./result/outside-data-memory-0.coe", "./result/initialized-src.h", "Src", 32);
    Bin2HeadFile("./result/outside-data-memory-1.coe", "./result/initialized-result.h", "Result", 32);

}

char Scheduler::Bin2Hex(char* BinVec){
    char HexChar;
    if(BinVec[0]=='0' && BinVec[1]=='0' && BinVec[2]=='0' && BinVec[3]=='0'){
        HexChar = '0';
    }
    else if(BinVec[0]=='0' && BinVec[1]=='0' && BinVec[2]=='0' && BinVec[3]=='1'){
        HexChar = '1';
    }
    else if(BinVec[0]=='0' && BinVec[1]=='0' && BinVec[2]=='1' && BinVec[3]=='0'){
        HexChar = '2';
    }
    else if(BinVec[0]=='0' && BinVec[1]=='0' && BinVec[2]=='1' && BinVec[3]=='1'){
        HexChar = '3';
    }
    else if(BinVec[0]=='0' && BinVec[1]=='1' && BinVec[2]=='0' && BinVec[3]=='0'){
        HexChar = '4';
    }
    else if(BinVec[0]=='0' && BinVec[1]=='1' && BinVec[2]=='0' && BinVec[3]=='1'){
        HexChar = '5';
    }
    else if(BinVec[0]=='0' && BinVec[1]=='1' && BinVec[2]=='1' && BinVec[3]=='0'){
        HexChar = '6';
    }
    else if(BinVec[0]=='0' && BinVec[1]=='1' && BinVec[2]=='1' && BinVec[3]=='1'){
        HexChar = '7';
    }
    else if(BinVec[0]=='1' && BinVec[1]=='0' && BinVec[2]=='0' && BinVec[3]=='0'){
        HexChar = '8';
    }
    else if(BinVec[0]=='1' && BinVec[1]=='0' && BinVec[2]=='0' && BinVec[3]=='1'){
        HexChar = '9';
    }
    else if(BinVec[0]=='1' && BinVec[1]=='0' && BinVec[2]=='1' && BinVec[3]=='0'){
        HexChar = 'A';
    }
    else if(BinVec[0]=='1' && BinVec[1]=='0' && BinVec[2]=='1' && BinVec[3]=='1'){
        HexChar = 'B';
    }
    else if(BinVec[0]=='1' && BinVec[1]=='1' && BinVec[2]=='0' && BinVec[3]=='0'){
        HexChar = 'C';
    }
    else if(BinVec[0]=='1' && BinVec[1]=='1' && BinVec[2]=='0' && BinVec[3]=='1'){
        HexChar = 'D';
    }
    else if(BinVec[0]=='1' && BinVec[1]=='1' && BinVec[2]=='1' && BinVec[3]=='0'){
        HexChar = 'E';
    }
    else{
        HexChar = 'F';
    }

    return HexChar;
}

void Scheduler::Bin2HeadFile(const string &BinFileName, const string &HeadFileName, const string &ArrayName, const int &DataWidth){

    int DataNum=FileLineCount(BinFileName)-2;
    ifstream BinFileHandle;
    BinFileHandle.open(BinFileName.c_str());
    if(!BinFileHandle.is_open()){
        DEBUG1("Failed to open %s\n", BinFileName.c_str());  
    }

    ofstream HeadFileHandle;
    HeadFileHandle.open(HeadFileName.c_str());
    if(!HeadFileHandle.is_open()){
        DEBUG1("Failed to create %s\n", HeadFileName.c_str());
    }

    char BinVec[100];
    for(int i=0; i<100; i++){
        BinVec[i] = '0';
    }
    int LineNum=0;
    HeadFileHandle << "unsigned int " << ArrayName << "[" << DataNum <<"]={";
    while(BinFileHandle.getline(BinVec,100)){
        LineNum++;

        //Ignore the first two lines due to coe file format.
        if(LineNum<3){
            continue;
        }

        HeadFileHandle << "0x";
        for(int k=0; k<(DataWidth+3)/4; k++){
            int id=k*4;
            HeadFileHandle << Bin2Hex(&BinVec[id]);
        }
        if(LineNum==DataNum+2){
            HeadFileHandle << " };";
        }
        else{
            HeadFileHandle << ", ";
        }
    }
    for(int i=LineNum+1; i<=DataNum+2; i++){
        if(i==DataNum+2){
            HeadFileHandle << "0x00000000 };";
        }
        else{
            HeadFileHandle << "0x00000000, ";
        }
    }
    HeadFileHandle.close();
    BinFileHandle.close();

}

int Scheduler::FileLineCount(const string &FileName){
    int LineCnt=0;
    ifstream FileHandle;
    FileHandle.open(FileName.c_str());
    if(!FileHandle.is_open()){
        DEBUG1("File open failed!\n");
    }
    char LineVec[200];
    while(FileHandle.getline(LineVec, 200)){
        LineCnt++;
    }
    return LineCnt;
}

