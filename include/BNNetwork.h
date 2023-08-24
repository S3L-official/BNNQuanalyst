//
// Created by yedi zhang on 2023/8/16.
//

#ifndef BNNQUANALYST_BNNETWORK_H
#define BNNQUANALYST_BNNETWORK_H
#include "Utilities.h"
#include "map"
#include "set"
#include "stack"
#include "cuddObj.hh"
#include "BNNBlock.h"

using namespace std;

class BNNetwork {
public:
    string _name;
    string _dataSet; // mnist is our dataset currently
    string _networkType; // Current only support Binarized NN
    int _numOfInterBLK;
    vector<BNNBlock> _blkList;
    string _modelPath;
    int _maxSize;
public:
    BNNetwork(string modelFolderName, string modelPath);

    BNNetwork();

    void setModelPath(string modelPath);

    void toBNNBlock();

    void toBNNBlockFriendly();

    vector<BNNBlock> &getBNNBlock();

    int get_input_size();

    int get_output_size();

    int get_Max_size(); // return the maximal size of each block (including input)


};

#endif //BNNQUANALYST_BNNETWORK_H
