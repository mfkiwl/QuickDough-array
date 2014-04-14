// ============================================================================
// Fucnction Description:
// Directional graph implemented with adjacency list
// Provide better support for reading DFG which can be dumped from LLVM
//
// Version:
// 0.1      Nov 23th 2011
// 0.2      May 30th 2012
//
// Author:
// Cheng Liu
// st.liucheng@gmail.com
// E.E.E Department, The University of Hong Kong
//
// ============================================================================

#ifndef _DATA_FLOW_GRAPH_H_
#define _DATA_FLOW_GRAPH_H_

#include "Vertex.h"
#include "GlobalDef.h"
#include <cstdlib>
#include <map>
#include <sstream>

using namespace std;

class DataFlowGraph{

    public:
        int vertex_num;
        int input_vertex_num;
        int output_vertex_num;
        int im_vertex_num;

        int min_input_degree;
        int max_input_degree;
        float average_degree;

        int min_output_degree;
        int max_output_degree;

        int min_vertex_priority;
        int max_vertex_priority;
        float average_vertex_priority;

        string DFG_name;
        vector<Vertex*> DFG_vertex;

        DataFlowGraph();
        void DFGCalculation(vector<int> &operation_result);
        void OutsideDataMemoryDumpCoe();

    private:
        void LoadParameter();
        void DFGConstruct();
        bool IsVertexInDFG(const int &operation_id);
        void DFGStatistic();
        void RandomInstGen();
        void VertexPriorityAllocation();
        void VertexPriorityAnalysis();
        //void OutputDegreeAnalysis();
        //void InputDegreeAnalysis();
};

#endif
