//
// Created by yedi zhang on 2021/8/3.
//

#ifndef BNNQUANALYST_UTILLACE_H
#define BNNQUANALYST_UTILLACE_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include "iostream"
#include "sstream"
#include "assert.h"
#include "cudd.h"
#include <fstream>
#include <dirent.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include "map"
#include "math.h"
#include "algorithm"
#include "lace.h"
#include "sylvan.h"
//#include "sylvan_obj.hpp"

//typedef sylvan::Bdd sylvanBdd;
//typedef sylvan::BDD sylvanBDD;
typedef BNNBlock *myBlock;

using namespace sylvan;

TASK_DECL_2(sylvan::BDD, computeAndSetBDD, int, int)

#define  computeAndSetBDD(start, end) RUN(computeAndSetBDD,start,end)

TASK_DECL_4(sylvanBDD, dichEnc, vector<sylvanBdd> *, sylvanBdd, int, int)

#define dichEnc(bdd_vec, inputBdd, start, end) RUN(dichEnc,bdd_vec,inputBdd,start, end)

TASK_DECL_5(vector<sylvan::Bdd>*, parallelInterBlk, Encoder_Sylvan*, vector<BNNBlock>*, vector<int>*,
            vector<sylvanBdd>*, sylvanBdd)

#define parallelInterBlk(encoder, blkVec, targetCls, cubes_vec, inputBdd) RUN(parallelInterBlk,encoder, blkVec,targetCls,cubes_vec,inputBdd)

TASK_DECL_5(sylvan::BDD, encInterBlk, myBlock, sylvan::Bdd, int, int, int)

#define encInterBlk(blk, inputBDD, start_var, start, end) RUN(encInterBlk,blk, inputBDD, start_var, start, end)

TASK_DECL_4(sylvan::BDD, encOutputBlkClassPure, myBlock, sylvan::Bdd, int, int)

#define encOutputBlkClassPure(blk, inputBDD, start_var, targetCls) RUN(encOutputBlkClassPure,blk, inputBDD,start_var, targetCls)

TASK_DECL_4(vector<sylvan::Bdd>*, encOutputBlk, myBlock, sylvan::Bdd, int, vector<int>*)

#define encOutputBlk(blk, inputBDD, start_var, targetClsVec) RUN(encOutputBlk,blk, inputBDD,start_var,targetClsVec)

#endif //BNNQUANALYST_UTILLACE_H
