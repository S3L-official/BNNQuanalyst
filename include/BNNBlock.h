//
// Created by yedi zhang on 2023/8/16.
//
#include "Utilities.h"
#include "map"
#include "set"
#include "stack"
#include "cuddObj.hh"
using namespace std;

#ifndef BNNQUANALYST_BNNBLOCK_H
#define BNNQUANALYST_BNNBLOCK_H

class BNNBlock {
public:
    BNNBlock(vector<struct BNNBlock> vector);

    bool _ifOut;
    vector<vector<int>> _lin_weight;
    vector<vector<float>> _lin_bias;
    vector<vector<float>> _bn_weight;
    vector<vector<float>> _bn_bias;
    vector<vector<float>> _bn_mean;
    vector<vector<float>> _bn_var;
    vector<int> _input_vec;
    vector<int> _output_vec;

    int _input_size; // dimension of input vector
    int _output_size; // dimension of output vector

public:
    BNNBlock(vector<vector<int>> lin_weight, vector <vector<float>> lin_bias, vector <vector<float>> bn_weight,
             vector <vector<float>> bn_bias, vector <vector<float>> bn_mean, vector <vector<float>> bn_var);

    BNNBlock(vector<vector<int>> lin_weight, vector <vector<float>> lin_bias);

    /**
     * Encode An Internal Block of BNN into a Cardinality Constraint.
     * @param first_fresh_var
     * @param in_var_start
     * @param out_var_start
     * @return 1 if succeed.
     */
    int blkInt2CC(int first_fresh_var, int in_var_start, int out_var_start);

    /**
     * Encode the only Output Block of BNN into a Cardinality Constraint.
     * @param first_fresh_var
     * @param in_var_start
     * @param out_var_start
     * @return
     */
    int blkOut2CC(int first_fresh_var, int in_var_start, int out_var_start);

    void setInputVec(vector<int> input_vec);

    void setOutputVec(vector<int> output_vec);

    void setOutputBLK();

    void setInputSize(int intSize);

    void setOutputSize(int outSize);
};

#endif //BNNQUANALYST_BNNBLOCK_H
