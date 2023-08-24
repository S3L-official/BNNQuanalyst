//
// Created by yedi zhang on 2023/8/16.
//

#ifndef BNNQUANALYST_CNFEncoder_H
#define BNNQUANALYST_CNFEncoder_H

#include <sys/timeb.h>
#include "Query.h"
#include "BNNetwork.h"
#include "map"
#include "cuddObj.hh"
//#include "sylvan.h"
//#include "sylvan_obj.hpp"
#include "Encoder_CUDD.h"
#include "Encoder_Sylvan.h"

class CNFEncoder_CUDD {
public:
    BNNetwork *_network;
    Query *_query;
    int _input_size = 100;
    Cudd *_mgr;
public:
    CNFEncoder_CUDD(BNNetwork *network, Query *query);

    BDD encodeCUDD(vector<BDD> *bdd_vec);

    void parseDimacsFile_CUDD(string dimacsFileName, vector<int> target_class);

    vector<int> getVars_CUDD(string line);

    void quit();

    BDD encodeCNFLineCUDD(Cudd *mgr, vector<int> var_vec);

    BDD encodeXLineCUDD(Cudd *mgr, vector<int> var_vec);

    BDD encodeDichCUDD(vector<BDD> *bdd_vec, int start, int end);

    BDD andSetCUDD(Cudd *mgr, int start, int end);
};

class CNFEncoder_Sylvan {
public:
    BNNetwork *_network;
    Query *_query;
    int _input_size = 100;
    Cudd *_mgr;
    int _worker;
public:
    CNFEncoder_Sylvan(BNNetwork *network, Query *query, int worker);

    void parseDimacsFile_Sylvan(string dimacsFileName, vector<int> target_class);

    vector<int> getVars_Sylvan(string line);

    void quit();

    sylvanBdd encodeCNFLineSylvan(vector<int> var_vec);

    sylvanBdd encodeXLineSylvan(vector<int> var_vec);

    sylvanBdd encodeDichSylvan(vector<sylvanBdd> *bdd_vec, int start, int end);

    sylvanBdd encodeSylvan(vector<sylvanBdd> *bdd_vec);

    sylvanBdd andSetSylvan(int start, int end);
};

#endif //BNNQUANALYST_CNFEncoder_H
