//
// Created by yedi zhang on 2020/11/8.
//

#ifndef BNNQuanalyst_OPTIONS_H
#define BNNQuanalyst_OPTIONS_H

#include "Utilities.h"
#include "BNNetwork.h"

using namespace std;

class Options {
public:
    char *_queryFileName;
    ENGINE _engineType; //CUDD or Sylvan
    int _modeSingleOutput = 0;
    bool _IP = true;
    bool _DC = true;
    int _level = 1; // no use for cudd engine
    int _worker = 1; // single thread by default

public:
    Options(int argc, char *argv[]);

};


#endif
