//
// Created by yedi zhang on 2023/8/16.
//

#ifndef BNNQUANALYST_Encoder_CUDD_H
#define BNNQUANALYST_Encoder_CUDD_H

#include <sys/timeb.h>
#include "Query.h"
#include "BNNetwork.h"
#include "map"
#include "cuddObj.hh"

class Encoder_CUDD {
public:
    BNNetwork *_network;
    Query *_query;
    bool _ifIP;
    bool _ifDC;
    int _level = 0;
    int _input_size = 100;
    Cudd *_mgr;
    BDD _input_BDD;
    bool _if_Set_IR = false;
    struct timeb inter_begin;
    struct timeb inter_end;
    struct timeb out_end;
    bool ifSingleOutput = false;
public:

    Encoder_CUDD(BNNetwork *network, Query *query, bool ifIP = true, bool ifDC = true);

    void setInputRegion(BDD inputRegion);

    void setSingleOutput();

    void testITE(bool ifITE); // true for ITE, false for cc2bdd

    vector<BDD> *encode2targetBDD(vector<int> target_class);

    vector<BDD> *encode2targetBDD_opt_0(vector<int> target_class);

    vector<BDD> *encode2targetBDD_opt_1(vector<int> target_class);

    BDD encodeInterBlockPure(BNNBlock blk, int start_var, BDD inputBDD);

    BDD encodeInterBlockDich(BNNBlock blk, BDD inputBDD, int start_var, int start, int end);

    vector<BDD> *
    encodeOutBlockPure(BNNBlock blk, int start_var, BDD inputADD, vector<int> target_class);

    vector<BDD> *encodeOutBlockPureSingle(BNNBlock blk, int start_var, BDD inputBDD, vector<int> target_class);

    vector<BDD> *
    addIntegration(vector<BDD> *DD_blks_vec, vector<BDD> *output_vec, vector<BDD> *cubes_vec);

    vector<BDD> *
    addIntegrationOutputOne(vector<BDD> *DD_blks_vec, vector<BDD> *output_vec, vector<BDD> *cubes_vec);

    vector<BDD> *addIntegrationSingle(vector<BDD> *DD_blks_vec, vector<BDD> *output_vec, vector<BDD> *cubes_vec);

    vector<BDD> generate_ith_row(vector<int> *vars, int row_index, int bound, int begin_var,
                                 vector<BDD> res_last_vec);

    vector<BDD>
    generate_ith_row_reversed(vector<int> *vars, int row_index, int bound, int begin_var,
                              vector<BDD> res_last_vec);

    vector<BDD> generate_ith_row_modified(vector<int> *vars, int row_index, int bound,
                                          vector<BDD> res_last_vec);

    BDD Cudd_bddCCEncoding(vector<int> *vars, int bound, int begin_var, bool if_reverse);

    BDD Cudd_bddCCEncoding_modified(vector<int> *vars, int bound);

    BDD computeAndSetCUDD(int start, int end);

    BDD
    CUDD_iteCCEncoding(vector<int> *vars, int lastIndex, int bound, int begin_var, map<string, BDD> *_BDD_cache_int);
};

#endif //BNNQUANALYST_Encoder_CUDD_H
