//
// Created by yedi zhang on 2023/8/16.
//

#ifndef BNNQUANALYST_ENCODER_SYLVAN_H
#define BNNQUANALYST_ENCODER_SYLVAN_H

#include <sys/timeb.h>
#include "Query.h"
#include "BNNetwork.h"
#include "map"
#include "sylvan_obj.hpp"

//using namespace sylvan;
typedef sylvan::Bdd sylvanBdd;
typedef sylvan::BDD sylvanBDD;

class Encoder_Sylvan {
public:
    BNNetwork *_network;
    Query *_query;
    bool _ifIP;
    bool _ifDC;
    int _level;
    int _input_size;
    sylvanBdd _input_BDD;
    bool _if_Set_IR = false;
    int _worker = 1;
//    struct timeb enc_begin;
    struct timeb inter_begin;
    struct timeb inter_end;
    struct timeb out_begin;
    struct timeb out_end;
    struct timeb integration_begin;
public:
    Encoder_Sylvan(BNNetwork *network, Query *query, bool ifIP, bool ifDC, int level);

    void setInputRegion(sylvanBdd inputRegion);

    void test(bool ifITE);

    vector<sylvan::Bdd> *encode2targetBDD(vector<int> target_class);

    vector<sylvan::Bdd> *encode2targetBDD_opt_0(vector<int> target_class);

    vector<sylvan::Bdd> *encode2targetBDD_opt_1(vector<int> target_class); // dichAlgorithm as in the paper

    vector<sylvan::Bdd> *encode2targetBDD_opt_2(vector<int> target_class);

    sylvanBdd dichEncoding(vector<sylvanBdd> *bdd_vec, sylvanBdd inputBDD, int start, int end);

    sylvanBdd
    encodeInterBlockDich(BNNBlock blk, sylvanBdd inputBDD, int start_var, int start, int end); // dich as algo in paper

    sylvanBdd encodeInterBlockPure(BNNBlock blk, int start_var, sylvanBdd inputBDD); // no dich no parallel

    vector<sylvanBdd> *
    encodeOutBlockPure(BNNBlock blk, int start_var, sylvanBdd inputADD, vector<int> target_class);

    vector<sylvan::Bdd> *
    addIntegration(vector<sylvanBdd> *DD_blks_vec, vector<sylvanBdd> *output_vec, vector<sylvanBdd> *cubes_vec);

    vector<sylvanBdd> generate_ith_row(vector<int> *vars, int row_index, int bound, int begin_var,
                                       vector<sylvanBdd> res_last_vec);

    vector<sylvanBdd>
    generate_ith_row_reversed(vector<int> *vars, int row_index, int bound, int begin_var,
                              vector<sylvanBdd> res_last_vec);

    vector<sylvanBdd> generate_ith_row_modified(vector<int> *vars, int row_index, int bound,
                                                vector<sylvanBdd> res_last_vec);

    sylvanBdd Cudd_bddCCEncoding(vector<int> *vars, int bound, int begin_var, bool if_reverse);

    sylvanBdd Cudd_bddCCITEEncoding(vector<int> *vars, int lastIndex, int bound, int begin_var,
                                    map<string, sylvanBdd> *_BDD_cache_int);

    sylvanBdd Cudd_bddCCEncoding_modified(vector<int> *vars, int bound);

    sylvanBdd computeAndSetCUDD(int start, int end);

    sylvanBdd Sylvan_iteCCEncoding(vector<int> *vars, int lastIndex, int bound, int begin_var,
                                   map<string, sylvanBdd> *_BDD_cache_int);

};

sylvan::Bdd bddCCEncoding(vector<int> *vars, int bound, int begin_var, bool if_reverse);

sylvan::Bdd bddCCEncodingModified(vector<int> *vars, int bound);

#endif //BNNQUANALYST_ENCODER_SYLVAN_H
