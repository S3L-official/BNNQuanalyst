//
// Created by yedi zhang on 2020/11/8.
//
#include "../include/BNNetwork.h"
#include "sstream"
#include "assert.h"

BNNetwork::BNNetwork(string modelFolderName, string modelPath) {
    assert(modelFolderName != "");
    _name = modelFolderName;
    vector<string> res;
    stringstream input(modelFolderName);
    string temp;
    while (getline(input, temp, '_')) {
        res.push_back(temp);
    }

    _dataSet = res[0];
    _networkType = res[1];
    _numOfInterBLK = stoi(res[2]);
    _maxSize = 0;
    setModelPath(modelPath);
    toBNNBlock();
}

void BNNetwork::setModelPath(string modelPath) {
    _modelPath = modelPath;
}


void BNNetwork::toBNNBlock() {

    for (int i = 1; i <= _numOfInterBLK; ++i) {
        vector<string> fileName = filename(_modelPath + _name, i, true);
        vector<vector<int>> lin_weight = parseCSV2Int(fileName[0], ',');
        vector<vector<float>> lin_bias = parseCSV(fileName[1], ',');
        vector<vector<float>> bn_weight = parseCSV(fileName[2], ',');
        vector<vector<float>> bn_bias = parseCSV(fileName[3], ',');
        vector<vector<float>> bn_mean = parseCSV(fileName[4], ',');
        vector<vector<float>> bn_var = parseCSV(fileName[5], ',');
        _blkList.push_back(BNNBlock(lin_weight, lin_bias, bn_weight, bn_bias, bn_mean, bn_var));
        _maxSize = _maxSize > _blkList[i - 1]._input_size ? _maxSize : _blkList[i - 1]._input_size;

    }

    vector<string> fileName = filename(_modelPath + "/" + _name, 0, false);
    vector<vector<int>> lin_weight = parseCSV2Int(fileName[0], ',');
    vector<vector<float>> lin_bias = parseCSV(fileName[1], ',');
    _blkList.push_back(BNNBlock(lin_weight, lin_bias));
    _maxSize = _maxSize > _blkList[_numOfInterBLK]._input_size ? _maxSize : _blkList[_numOfInterBLK]._input_size;
}


void BNNetwork::toBNNBlockFriendly() {

    for (int i = 1; i <= _numOfInterBLK + 1; ++i) {
        vector<string> fileName = filename(_modelPath + _name, i, true);
        vector<vector<int>> lin_weight = parseCSV2Int(fileName[0], ',');
        vector<vector<float>> lin_bias = parseCSV(fileName[1], ',');
        vector<vector<float>> bn_weight = parseCSV(fileName[2], ',');
        vector<vector<float>> bn_bias = parseCSV(fileName[3], ',');
        vector<vector<float>> bn_mean = parseCSV(fileName[4], ',');
        vector<vector<float>> bn_var = parseCSV(fileName[5], ',');
        _blkList.push_back(BNNBlock(lin_weight, lin_bias, bn_weight, bn_bias, bn_mean, bn_var));
        _maxSize = _maxSize > _blkList[i - 1]._input_size ? _maxSize : _blkList[i - 1]._input_size;
    }
}

vector<BNNBlock> &BNNetwork::getBNNBlock() {
    return _blkList;
}


int BNNetwork::get_input_size() {
    return _blkList[0]._input_size;
}

int BNNetwork::get_output_size() {
    return _blkList[_numOfInterBLK]._output_size;
}

int BNNetwork::get_Max_size() {
    return _maxSize;
}
