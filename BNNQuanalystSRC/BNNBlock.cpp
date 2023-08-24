//
// Created by yedi zhang on 2020/11/8.
//

#include "../include/BNNBlock.h"

BNNBlock::BNNBlock(vector<vector<int>> lin_weight, vector<vector<float>> lin_bias, vector<vector<float>> bn_weight,
                   vector<vector<float>> bn_bias, vector<vector<float>> bn_mean, vector<vector<float>> bn_var) {
    _lin_weight = lin_weight;
    _lin_bias = lin_bias;
    _bn_weight = bn_weight;
    _bn_bias = bn_bias;
    _bn_mean = bn_mean;
    _bn_var = bn_var;
    _ifOut = false;
    _input_size = _lin_weight[0].size();
    _output_size = _lin_weight.size();
}

BNNBlock::BNNBlock(vector<vector<int>> lin_weight, vector<vector<float>> lin_bias) {
    _lin_weight = lin_weight;
    _lin_bias = lin_bias;
    _ifOut = true;
    _input_size = _lin_weight[0].size();
    _output_size = _lin_weight.size();
}

void BNNBlock::setInputVec(vector<int> input_vec) {
    _input_vec = input_vec;
}

void BNNBlock::setOutputVec(vector<int> output_vec) {
    _output_vec = output_vec;
}
void BNNBlock::setOutputBLK(){
    _ifOut=true;
}

void BNNBlock::setInputSize(int intSize){
    _input_size=intSize;
}

void BNNBlock::setOutputSize(int outSize){
    _output_size=outSize;
}
