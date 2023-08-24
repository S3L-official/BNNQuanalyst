//
// Created by yedi zhang on 2020/10/27.
//

/*
 * Header file of utilities.cpp
 */

#ifndef EVNN_UTILITIES_H
#define EVNN_UTILITIES_H

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
#include "algorithm"
//#include "sylvan.h"
//#include "sylvan_obj.hpp"

#ifndef _DEBUG
#define Asert(exp, message) if (!(exp))  {  printf("Assertion failed: %s\n", #exp); \
        printf("Message: %s\n",  message); \
        printf("line: %d\n", __LINE__); \
        printf("file: %s\n", __FILE__); \
        exit(EXIT_FAILURE); \
    }
#else
#define Asert(exp, message)
#endif


using namespace std;


/**
  @brief Type of engine.
*/
typedef enum {
    CUDD,
    SYLVAN,
    NPAQ_SYLVAN,
    NPAQ_CUDD,
    CUDD_NPAQ
} ENGINE;


void print_dd(DdManager *gbm, DdNode *dd, int n, int pr);

//DdNode *CCEncoding(DdManager *gbm, vector<int> *vars, int lastIndex, int bound, int begin_var,
//                   map<string, DdNode *> *_BDD_cache_int);

long long combo(int a, int b);

void write_dd(DdManager *gbm, DdNode *dd, char *filename);

void write_ten_adds(DdManager *gbm, vector<DdNode *> *vec, int input_size, const char *relaPath);

void print_help();

vector<vector<float>> parseCSV(string filename, char delimiter);

vector<vector<int>> parseCSV2Int(string filename, char delimiter);

vector<string> filename(string path, int number, bool internal_blk = true);

double myStr2Double(const char *src);

#endif //EVNN_UTILITIES_H
