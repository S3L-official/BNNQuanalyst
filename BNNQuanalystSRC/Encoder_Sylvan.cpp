//
// Created by yedi zhang on 2021/5/31.
// Implement based on Sylvan
// Currently only support for Targeted Classes, not for all BDD classes (since the former includes the latter)
// The functions whose name is same to CUDD is just the same function, except implemented with sylvan-lib.
// The functions with different names, e.g., ended with "Parallel" is designed to add additional parallel algorithm.
//

#include "../include/Encoder_Sylvan.h"
#include "math.h"
#include "sylvan.h"
#include "sylvan_obj.hpp"
#include <thread>
#include "lace.h"
#include "../include/UtilLace.h"


Encoder_Sylvan::Encoder_Sylvan(BNNetwork *network, Query *query, bool ifIP, bool ifDC, int level) {
    _network = network;
    _query = query;
    _ifIP = ifIP;
    _ifDC = ifDC;
    _level = level;
    _input_size = network->get_input_size();
    _worker = lace_workers();
    cout << "This is Sylvan-based encoding with IP = " << _ifIP << ", DC = " << _ifDC << ", ParaLevel = " << _level
         << " and Thread = " << _worker << "." << endl;
}

void Encoder_Sylvan::setInputRegion(Bdd inputRegion) {
    _input_BDD = inputRegion;
    _if_Set_IR = true;
}

void Encoder_Sylvan::test(bool ifITE) {
    vector<int> *instance = &(_query->_instance);
    struct timeb tm1, tm2;
    ftime(&tm1);
    if (!ifITE) {
        _input_BDD = Cudd_bddCCEncoding(instance, instance->size() - _query->_distance, 0, false);
        ftime(&tm2);
        double tmInter = ((tm2.time - tm1.time) + (tm2.millitm - tm1.millitm) / 1000.0);
        cout << "TestEncode for Sylvan (cc2bdd): " << tmInter << endl;
    } else {
        map<string, Bdd> *_BDD_cache_int = new map<string, Bdd>;
        _input_BDD = Sylvan_iteCCEncoding(instance, instance->size() - 1, instance->size() - _query->_distance, 0,
                                          _BDD_cache_int);
        delete _BDD_cache_int;
        ftime(&tm2);
        double tmInter = ((tm2.time - tm1.time) + (tm2.millitm - tm1.millitm) / 1000.0);
        cout << "TestEncode for Sylvan (ITE): " << tmInter << endl;
    }
}

/*
 * opt mode: 0,1,2
 * opt=0: para level L1 or L2, dich, with/without IP;
 * opt=1: para level L1 or L2, no-dich, with/without IP;
 * opt=2: para level L3, without IP, with/without DC
 */
vector<sylvanBdd> *Encoder_Sylvan::encode2targetBDD(vector<int> target_class) {
    int numOfInterblk = _network->_numOfInterBLK;
    if (_level >= 3 && numOfInterblk >= 2) {
        return this->encode2targetBDD_opt_2(target_class);
    } else if (_ifDC) {
        return this->encode2targetBDD_opt_0(target_class);
    } else {
        return this->encode2targetBDD_opt_1(target_class);
    }
}

/*
 * Get the BDD vector for verification: opt = 0 (basic version)
 * D&C, with or without IP, L1 or L2
 */
vector<sylvanBdd> *Encoder_Sylvan::encode2targetBDD_opt_0(vector<int> target_class) {
//    cout << "Now we jump into opt 0!" << endl;

    int numOfInterBLK = _network->_numOfInterBLK;
    vector<Bdd> *DD_blks_vec = new vector<Bdd>;
    vector<Bdd> *cubes_vec = new vector<Bdd>;
    vector<BNNBlock> blks_vec = _network->getBNNBlock();

    int start_var = 0;

    if (!_if_Set_IR) {
        vector<int> *instance = &(_query->_instance);
        if (_query->_robustness_judge == "fix-set") {
            vector<int> set = _query->_trigger_set;
            vector<int> fix_set_vars;
            int set_index = 0;
            for (int i = 0; i < instance->size(); ++i) {
                if (set_index < set.size() && i == set[set_index]) {
                    set_index++;
                    continue;
                } else {
                    fix_set_vars.push_back((*instance)[i] > 0 ? i : -i);
                }
            }
            assert(set_index == set.size());
            _input_BDD = Cudd_bddCCEncoding_modified(&fix_set_vars,
                                                     fix_set_vars.size()); /* use directly, not need CuddRef() */
        } else { /* hamming distance by default */
            int distance = _query->_distance;
            if (distance == _input_size) {
                _input_BDD = sylvan_true;
            } else if (distance < 0) {
                cout << "Error! Negative distance is not allowed!" << endl;
                exit(0);
            } else {
                _input_BDD = Cudd_bddCCEncoding(instance, instance->size() - distance, 0,
                                                false); /* use directly, not need CuddRef() */
            }
        }
    }

    cubes_vec->push_back(sylvan_true);

    ftime(&inter_begin);
    for (int i = 0; i < numOfInterBLK; ++i) {
        Bdd current_cube;
        if (_level >= 2) {
            current_cube = computeAndSetBDD(blks_vec[i]._input_size + start_var,
                                            blks_vec[i]._input_size + start_var + blks_vec[i]._output_size - 1);
        } else {
            current_cube = computeAndSetCUDD(blks_vec[i]._input_size + start_var,
                                             blks_vec[i]._input_size + start_var + blks_vec[i]._output_size - 1);
        }
        cubes_vec->push_back(current_cube);
        Bdd DD_intBlk;
        if (_level == 2) {
            DD_intBlk = encInterBlk(&blks_vec[i], _input_BDD, start_var, 0, blks_vec[i]._output_size - 1);
        } else {
            DD_intBlk = encodeInterBlockDich(blks_vec[i], _input_BDD, start_var, 0, blks_vec[i]._output_size - 1);
        }
        DD_blks_vec->push_back(DD_intBlk); /* original */
        if (_ifIP) {
            _input_BDD = sylvan_project(DD_intBlk.GetBDD(), (*cubes_vec)[i + 1].GetBDD());
        } else {
            _input_BDD = sylvan_true;
        }
        start_var = start_var + blks_vec[i]._input_size; /* original */
    }
    ftime(&inter_end);
    ftime(&out_begin);
    vector<Bdd> *DD_outBlk_vec;
    if (_level == 2) {
        DD_outBlk_vec = encOutputBlk(&blks_vec[numOfInterBLK], _input_BDD, start_var, &target_class);
    } else {
        DD_outBlk_vec = encodeOutBlockPure(blks_vec[numOfInterBLK], start_var, _input_BDD, target_class);
    }
    ftime(&out_end);
    ftime(&integration_begin);
    return addIntegration(DD_blks_vec, DD_outBlk_vec, cubes_vec);
}


/*
 * without D&C, with or without IP, L1 (without D&C, cannot use L2 either)
 * */
vector<sylvanBdd> *Encoder_Sylvan::encode2targetBDD_opt_1(vector<int> target_class) {
//    cout << "Now we jump into opt 1!" << endl;

    int numOfInterBLK = _network->_numOfInterBLK;
    vector<Bdd> *DD_blks_vec = new vector<Bdd>;
    vector<Bdd> *cubes_vec = new vector<Bdd>;
    vector<BNNBlock> blks_vec = _network->getBNNBlock();

    int start_var = 0;

    if (!_if_Set_IR) {
        vector<int> *instance = &(_query->_instance);
        if (_query->_robustness_judge == "fix-set") {
            vector<int> set = _query->_trigger_set;
            vector<int> fix_set_vars;
            int set_index = 0;
            for (int i = 0; i < instance->size(); ++i) {
                if (set_index < set.size() && i == set[set_index]) {
                    set_index++;
                    continue;
                } else {
                    fix_set_vars.push_back((*instance)[i] > 0 ? i : -i);
                }
            }
            assert(set_index == set.size());
            _input_BDD = Cudd_bddCCEncoding_modified(&fix_set_vars,
                                                     fix_set_vars.size()); /* use directly, not need CuddRef() */
        } else { /* hamming distance by default */
            int distance = _query->_distance;
            if (distance == _input_size) {
                _input_BDD = sylvan_true;
            } else if (distance < 0) {
                cout << "Error! Negative distance is not allowed!" << endl;
                exit(0);
            } else {
                _input_BDD = Cudd_bddCCEncoding(instance, instance->size() - distance, 0,
                                                false); /* use directly, not need CuddRef() */
            }
        }
    }

    cubes_vec->push_back(sylvan_true);

    ftime(&inter_begin);
    for (int i = 0; i < numOfInterBLK; ++i) {
        struct timeb tm1, tm2, tm3, tm4;
        ftime(&tm1);
        int start_varr = blks_vec[i]._input_size + start_var;
        Bdd current_cube = sylvan_true;
        for (int t = 0; t < blks_vec[i]._output_size; ++t) {
            Bdd var = Bdd::bddVar(t + start_varr);
            current_cube = var * current_cube;
        }
        cubes_vec->push_back(current_cube);
        ftime(&tm2);
        Bdd DD_intBlk = encodeInterBlockPure(blks_vec[i], start_var, _input_BDD);
        DD_blks_vec->push_back(DD_intBlk); /* original */
        ftime(&tm3);
        _input_BDD = sylvan_project(DD_intBlk.GetBDD(), (*cubes_vec)[i + 1].GetBDD());
        ftime(&tm4);
        start_var = start_var + blks_vec[i]._input_size; /* original */
        double tmAndSet = ((tm2.time - tm1.time) + (tm2.millitm - tm1.millitm) / 1000.0);
        double tmInter = ((tm3.time - tm2.time) + (tm3.millitm - tm2.millitm) / 1000.0);
        double tmExtr = ((tm4.time - tm3.time) + (tm4.millitm - tm3.millitm) / 1000.0);
        cout << "**************** The time for computeAndSet " << i + 1 << " is: " << tmAndSet
             << " ****************" << endl;
        cout << "**************** The time for encode internal block " << i + 1 << " is: " << tmInter
             << " ****************" << endl;
        cout << "**************** The time for sylvan_project in internal block " << i + 1 << " is: " << tmExtr
             << " ****************" << endl;
    }
    ftime(&inter_end);
    ftime(&out_begin);
    vector<Bdd> *DD_outBlk_vec = encodeOutBlockPure(blks_vec[numOfInterBLK], start_var, _input_BDD, target_class);
    ftime(&out_end);
    ftime(&integration_begin);
    return addIntegration(DD_blks_vec, DD_outBlk_vec, cubes_vec);
}

/*
 * L3, without IP, with or without D&C
 * */

vector<sylvanBdd> *Encoder_Sylvan::encode2targetBDD_opt_2(vector<int> target_class) {
//    cout << "Now we jump into opt 2!" << endl;
    int numOfInterBLK = _network->_numOfInterBLK;
    vector<Bdd> *cubes_vec = new vector<Bdd>;
    vector<BNNBlock> blks_vec = _network->getBNNBlock();

    for (int i = 0; i <= numOfInterBLK; ++i) {
        cubes_vec->push_back(sylvan_true);
    }

    if (!_if_Set_IR) {
        vector<int> *instance = &(_query->_instance);
        if (_query->_robustness_judge == "fix-set") {
            int distance = _query->_distance;
            vector<int> set = _query->_trigger_set;
            vector<int> fix_set_vars;
            int set_index = 0;
            for (int i = 0; i < instance->size(); ++i) {
                if (set_index < set.size() && i == set[set_index]) {
                    set_index++;
                    continue;
                } else {
                    fix_set_vars.push_back((*instance)[i] > 0 ? i : -i);
                }
            }
            assert(set_index == set.size());
            _input_BDD = Cudd_bddCCEncoding_modified(&fix_set_vars,
                                                     fix_set_vars.size()); /* use directly, not need CuddRef() */
        } else { /* hamming distance by default */
            int distance = _query->_distance;
            _input_BDD = Cudd_bddCCEncoding(instance, instance->size() - distance, 0,
                                            false); /* use directly, not need CuddRef() */
        }

    }

    ftime(&inter_begin);
    vector<Bdd> *bddVec = parallelInterBlk(this, &blks_vec, &target_class, cubes_vec, _input_BDD);
    vector<Bdd> *DD_blks_vec = new vector<Bdd>;
    vector<Bdd> *DD_outBlk_vec = new vector<Bdd>;
    for (int i = 0; i < numOfInterBLK; ++i) {
        DD_blks_vec->push_back((*bddVec)[i]);
    }
    for (int i = 0; i < target_class.size(); ++i) {
        DD_outBlk_vec->push_back((*bddVec)[numOfInterBLK + i]);
    }
    ftime(&inter_end);
    ftime(&integration_begin);
    delete bddVec;
    return addIntegration(DD_blks_vec, DD_outBlk_vec, cubes_vec);

}

/*
 * This "encodeInterBlock" does not use dich, just a for-loop to compute AND.
 * */
Bdd Encoder_Sylvan::encodeInterBlockPure(BNNBlock blk, int start_var, Bdd inputBDD) {
    Bdd bddRes = inputBDD;
    for (int i = 0; i < blk._output_size; ++i) {
        /* compute cube */

        /* compute output_i_encoding */
        float c_i = -(sqrt(blk._bn_var[0][i]) / blk._bn_weight[0][i]) * blk._bn_bias[0][i] + blk._bn_mean[0][i] -
                    blk._lin_bias[0][i];

        int bound;
        Bdd DD_node;

        /* when bn_weight>0 */
        if (blk._bn_weight[0][i] > 0) {
            bound = ceil((blk._input_size + c_i) / 2);
            DD_node = Cudd_bddCCEncoding(&(blk._lin_weight[i]), bound, start_var, false);

        } else if (blk._bn_weight[0][i] < 0) {
            bound = ceil((blk._input_size - c_i) / 2);
            DD_node = Cudd_bddCCEncoding(&(blk._lin_weight[i]), bound, start_var, true);
        } else {
            DD_node = blk._bn_bias[0][i] > 0 ? sylvan_true : sylvan_false;
        }
        Bdd y = sylvan_ithvar(blk._input_size + start_var + i);
        Bdd output_i = DD_node.Xnor(y);
        bddRes = bddRes * output_i;
    }
    return bddRes;
}

/*
 * This "dichEncoding" is totally same to CUDD.
 * */
Bdd Encoder_Sylvan::dichEncoding(vector<Bdd> *bdd_vec, Bdd inputBDD, int start, int end) {
    if (abs(start - end) == 1) {
        Bdd pre_half = (*bdd_vec)[start] * inputBDD;
        Bdd pos_half = (*bdd_vec)[end] * inputBDD;
        Bdd res = pre_half * pos_half;
        return res;
    } else if (abs(start - end) == 0) {
        return (*bdd_vec)[start] * inputBDD;
    } else {
        Bdd pre_half = dichEncoding(bdd_vec, inputBDD, start, (end - start) / 2 + start);
        Bdd pos_half = dichEncoding(bdd_vec, inputBDD, (end - start) / 2 + start + 1, end);
        Bdd res = pre_half * pos_half;
        return res;
    }
}

/*
 * This "encodeInterBlock" is totally same to CUDD.
 * */
Bdd Encoder_Sylvan::encodeInterBlockDich(BNNBlock blk, Bdd inputBDD, int start_var, int start, int end) {
    Bdd result;
    if (abs(start - end) == 1) {
        float c_start = -(sqrt(blk._bn_var[0][start]) / blk._bn_weight[0][start]) * blk._bn_bias[0][start] +
                        blk._bn_mean[0][start] - blk._lin_bias[0][start];
        float c_end = -(sqrt(blk._bn_var[0][end]) / blk._bn_weight[0][end]) * blk._bn_bias[0][end] +
                      blk._bn_mean[0][end] - blk._lin_bias[0][end];

        int bound_start, bound_end;
        Bdd y_start = sylvan_ithvar(blk._input_size + start_var + start);
        Bdd y_end = sylvan_ithvar(blk._input_size + start_var + end);
        Bdd pre_half, pos_half;

        if (blk._bn_weight[0][start] > 0) {
            bound_start = ceil((blk._input_size + c_start) / 2);
            pre_half = Cudd_bddCCEncoding(&(blk._lin_weight[start]), bound_start, start_var, false);
        } else if (blk._bn_weight[0][start] < 0) {
            bound_start = ceil((blk._input_size - c_start) / 2);
            pre_half = Cudd_bddCCEncoding(&(blk._lin_weight[start]), bound_start, start_var, true);
        } else {
            pre_half = blk._bn_bias[0][start] > 0 ? sylvan_true : sylvan_false;
        }

        if (blk._bn_weight[0][end] > 0) {
            bound_end = ceil((blk._input_size + c_end) / 2);
            pos_half = Cudd_bddCCEncoding(&(blk._lin_weight[end]), bound_end, start_var, false);
        } else if (blk._bn_weight[0][end] < 0) {
            bound_end = ceil((blk._input_size - c_end) / 2);
            pos_half = Cudd_bddCCEncoding(&(blk._lin_weight[end]), bound_end, start_var, true);
        } else {
            pos_half = blk._bn_bias[0][end] > 0 ? sylvan_true : sylvan_false;
        }
        pre_half = pre_half.Xnor(y_start);
        pre_half = pre_half * inputBDD;
        pos_half = pos_half.Xnor(y_end);
        result = pre_half * pos_half;
    } else if (abs(start - end) == 0) {
        float c_start = -(sqrt(blk._bn_var[0][start]) / blk._bn_weight[0][start]) * blk._bn_bias[0][start] +
                        blk._bn_mean[0][start] - blk._lin_bias[0][start];
        int bound_start;
        Bdd y_end = sylvan_ithvar(blk._input_size + start_var + end);

        if (blk._bn_weight[0][start] > 0) {
            bound_start = ceil((blk._input_size + c_start) / 2);
            result = Cudd_bddCCEncoding(&(blk._lin_weight[start]), bound_start, start_var, false);
        } else if (blk._bn_weight[0][start] < 0) {
            bound_start = ceil((blk._input_size - c_start) / 2);
            result = Cudd_bddCCEncoding(&(blk._lin_weight[start]), bound_start, start_var, true);
        } else {
            result = blk._bn_bias[0][start] > 0 ? sylvan_true : sylvan_false;
        }
        result = result.Xnor(y_end);
    } else {
        Bdd pre_half = encodeInterBlockDich(blk, inputBDD, start_var, start, (end - start) / 2 + start);
        Bdd pos_half = encodeInterBlockDich(blk, inputBDD, start_var, (end - start) / 2 + start + 1, end);
        result = pre_half * pos_half;
    }
    return result;
}


/* This is totally same with CUDD-version */
vector<Bdd> *
Encoder_Sylvan::encodeOutBlockPure(BNNBlock blk, int start_var, Bdd inputBDD, vector<int> target_class) {
    vector<Bdd> *bdd_vec = new vector<Bdd>;
    for (int t = 0; t < target_class.size(); ++t) {
        int i = target_class[t];
        Bdd tmpSingle, ddSingle; /* f_i1 and ... and f_in*/
        ddSingle = sylvan_true;

        for (int j = 0; j < blk._output_size; ++j) {
            if (i == j) { continue; }
            vector<int> vars;
            int sum_w_i = 0;
            int w_i_minus_num = 0;
            for (int k = 0; k < blk._input_size; ++k) {
                if (blk._lin_weight[i][k] > 0 & blk._lin_weight[j][k] < 0) {
                    vars.push_back(k + start_var);
                } else if (blk._lin_weight[i][k] < 0 & blk._lin_weight[j][k] > 0) {
                    vars.push_back(-k - start_var);
                    w_i_minus_num++;
                }
                sum_w_i = sum_w_i + blk._lin_weight[i][k] - blk._lin_weight[j][k];
            }

            int bound;

            float fl = (sum_w_i + blk._lin_bias[0][j] - blk._lin_bias[0][i]) / 4;
            float fl_abs = abs(fl);
            float frac, inte;
            frac = modf(fl_abs, &inte);

            if (j < i) {
                if (frac > 0) {
                    bound = ceil(fl) + w_i_minus_num;
                } else if (frac == 0) { /* fl is an integer */
                    bound = ceil(fl) + 1 + w_i_minus_num;
                } else {
                    cout << "Error bound in output block!" << endl;
                    exit(0);
                }
            } else {
                bound = ceil(fl) + w_i_minus_num;
            }

            Bdd DD_node = Cudd_bddCCEncoding_modified(&vars, bound);
            if ((j == 0 && i != 0) || (i == 0 && j == 1)) {
                tmpSingle = DD_node * inputBDD; /* Perform first AND Boolean operation */
            } else {
                tmpSingle = DD_node * ddSingle; /* Perform AND Boolean operation */
            }
            ddSingle = tmpSingle;
        }
        bdd_vec->push_back(ddSingle);
    }
    /* compute final single MTBDD with terminal added */
    return bdd_vec;
}

vector<Bdd> *
Encoder_Sylvan::addIntegration(vector<Bdd> *DD_blks_vec, vector<Bdd> *output_vec,
                                 vector<Bdd> *cubes_vec) {
    int numOfInterBLK = _network->_numOfInterBLK;
    Bdd final_res, final_tmp1;
    struct timeb tmb1, tmb2, tmb3;
    ftime(&tmb1);
    final_res = (*DD_blks_vec)[0];
    for (int i = 1; i < numOfInterBLK; ++i) {
        final_res = final_res.AndAbstract((*DD_blks_vec)[i], (*cubes_vec)[i]);
    }

    ftime(&tmb2);
    delete DD_blks_vec;

    vector<Bdd> *output_res = new vector<Bdd>;

    /* compute for each single output BDD, output is a BDD vector */
    for (int i = 0; i < output_vec->size(); ++i) {
        final_tmp1 = final_res.AndAbstract((*output_vec)[i], (*cubes_vec)[numOfInterBLK]);
        output_res->push_back(final_tmp1);
    }
    delete output_vec;
    delete cubes_vec;
    return output_res;
}

/*
 * for internal block: bn_var > 0
 */
vector<Bdd>
Encoder_Sylvan::generate_ith_row(vector<int> *vars, int row_index, int bound, int begin_var,
                                   vector<Bdd> res_last_vec) {
    /* generate one row */
    int row = bound;
    int col = vars->size() - bound + 1;

    vector<Bdd> row_vec;

    Bdd x_r_c;

    /* last row */
    if (row_index + 1 == row) {
        int val = (*vars)[row_index + col - 1];
        /* last col */
        x_r_c = sylvan_ithvar(begin_var + row_index + col - 1);
        row_vec.push_back(val > 0 ? x_r_c : !(x_r_c));

        for (int j = col - 2; j >= 0; j--) {
            Bdd x_r_j;
            Bdd xx = sylvan_ithvar(begin_var + row_index + j);
            if ((*vars)[row_index + j] > 0) {
                x_r_j = xx.Ite(sylvan_true, (row_vec)[col - j - 2]);
            } else {
                x_r_j = xx.Ite((row_vec)[col - j - 2], sylvan_true);
            }
            row_vec.push_back(x_r_j);
        }
    } else { /* for other rows */
        /* last col */
        if ((*vars)[row_index + col - 1] > 0) {
            Bdd xx = sylvan_ithvar(begin_var + row_index + col - 1);
            x_r_c = xx.Ite((res_last_vec)[0], sylvan_false);
        } else {
            Bdd xx = sylvan_ithvar(begin_var + row_index + col - 1);
            x_r_c = xx.Ite(sylvan_false, (res_last_vec)[0]);
        }

        row_vec.push_back(x_r_c);

        for (int j = col - 2; j >= 0; j--) {
            Bdd x_r_j;
            Bdd xx = sylvan_ithvar(begin_var + row_index + j);
            if ((*vars)[row_index + j] > 0) {
                x_r_j = xx.Ite((res_last_vec)[col - j - 1],
                               (row_vec)[col - j - 2]);
            } else {
                x_r_j = xx.Ite((row_vec)[col - j - 2],
                               (res_last_vec)[col - j - 1]);
            }
            row_vec.push_back(x_r_j);
        }
    }
    return row_vec;
}


/*
 * for internal block: bn_var < 0
 */
vector<Bdd>
Encoder_Sylvan::generate_ith_row_reversed(vector<int> *vars, int row_index, int bound, int begin_var,
                                            vector<Bdd> res_last_vec) {
    /* generate one row */
    int row = bound;
    int col = vars->size() - bound + 1;

    vector<Bdd> row_vec;

    Bdd x_r_c;

    /* last row */
    if (row_index + 1 == row) {
        int val = (*vars)[row_index + col - 1];
        /* last col */
        x_r_c = sylvan_ithvar(begin_var + row_index + col - 1);
        row_vec.push_back(val <= 0 ? x_r_c : !(x_r_c));

        for (int j = col - 2; j >= 0; j--) {
            Bdd x_r_j;
            Bdd xx = sylvan_ithvar(begin_var + row_index + j);
            /* original */
            if ((*vars)[row_index + j] <= 0) {
                x_r_j = xx.Ite(sylvan_true, (row_vec)[col - j - 2]);
            } else {
                x_r_j = xx.Ite((row_vec)[col - j - 2], sylvan_true);
            }
            row_vec.push_back(x_r_j);
        }
    } else { /* for other rows */
        /* last col, original */
        if ((*vars)[row_index + col - 1] <= 0) {
            Bdd xx = sylvan_ithvar(begin_var + row_index + col - 1);
            x_r_c = xx.Ite((res_last_vec)[0], sylvan_false);
        } else {
            Bdd xx = sylvan_ithvar(begin_var + row_index + col - 1);
            x_r_c = xx.Ite(sylvan_false, (res_last_vec)[0]);
        }
        row_vec.push_back(x_r_c);

        for (int j = col - 2; j >= 0; j--) {
            Bdd x_r_j;
            Bdd xx = sylvan_ithvar(begin_var + row_index + j);
            if ((*vars)[row_index + j] <= 0) {
                x_r_j = xx.Ite((res_last_vec)[col - j - 1],
                               (row_vec)[col - j - 2]);
            } else {
                x_r_j = xx.Ite((row_vec)[col - j - 2],
                               (res_last_vec)[col - j - 1]);
            }
            row_vec.push_back(x_r_j);
        }
    }
    return row_vec;
}

/*
 * for output block
 */
vector<Bdd>
Encoder_Sylvan::generate_ith_row_modified(vector<int> *vars, int row_index, int bound, vector<Bdd> res_last_vec) {
    /* generate one row */
    int row = bound;
    int col = vars->size() - bound + 1;

    vector<Bdd> row_vec;

    Bdd x_r_c;

    /* last row */
    if (row_index + 1 == row) {
        /* last col */
        int val = (*vars)[row_index + col - 1];
        x_r_c = sylvan_ithvar(abs(val));
        row_vec.push_back(val > 0 ? x_r_c : !(x_r_c));

        for (int j = col - 2; j >= 0; j--) {
            Bdd x_r_j;
            Bdd xx = sylvan_ithvar(abs((*vars)[row_index + j]));
            if ((*vars)[row_index + j] > 0) {
                x_r_j = xx.Ite(sylvan_true, (row_vec)[col - j - 2]);
            } else {
                x_r_j = xx.Ite((row_vec)[col - j - 2], sylvan_true);
            }
            row_vec.push_back(x_r_j);
        }
    } else { /* for other rows */
        /* last col */
        if ((*vars)[row_index + col - 1] > 0) {
            Bdd xx = sylvan_ithvar((*vars)[row_index + col - 1]);
            x_r_c = xx.Ite((res_last_vec)[0], sylvan_false);
        } else {
            Bdd xx = sylvan_ithvar(-(*vars)[row_index + col - 1]);
            x_r_c = xx.Ite(sylvan_false, (res_last_vec)[0]);
        }
        row_vec.push_back(x_r_c);

        for (int j = col - 2; j >= 0; j--) {
            Bdd x_r_j;
            int val = (*vars)[row_index + j];
            Bdd xx = sylvan_ithvar(abs(val));
            if (val > 0) {
                x_r_j = xx.Ite((res_last_vec)[col - j - 1],
                               (row_vec)[col - j - 2]);
            } else {
                x_r_j = xx.Ite((row_vec)[col - j - 2],
                               (res_last_vec)[col - j - 1]);
            }
            row_vec.push_back(x_r_j);
        }
    }
    return row_vec;
}


/*
 * for internal block
 */
Bdd
Encoder_Sylvan::Cudd_bddCCEncoding(vector<int> *vars, int bound, int begin_var,
                                     bool if_reverse) {
    int row = bound;
    int col = vars->size() - bound + 1;

    if (bound <= 0) {
        return sylvan_true;
    }

    if (bound > vars->size()) {
        return sylvan_false;
    }

    if (bound == 1 && !if_reverse) {
        Bdd bddTmp, var;
        bddTmp = sylvan_false;
        for (int i = vars->size() - 1; i >= 0; i--) {
            var = sylvan_ithvar(i + begin_var);
            bddTmp = ((*vars)[i] > 0 ? var : !var) + bddTmp;
        }
        return bddTmp;
    }

    if (bound == 1 && if_reverse) {
        Bdd bddTmp, var;
        bddTmp = sylvan_false;
        for (int i = vars->size() - 1; i >= 0; i--) {
            var = sylvan_ithvar(i + begin_var);
            bddTmp = ((*vars)[i] <= 0 ? var : !var) + bddTmp;
        }
        return bddTmp;
    }

    if (bound == vars->size() && !if_reverse) {
        Bdd bddTmp, var;
        bddTmp = sylvan_true;
        for (int i = 0; i < bound; ++i) {
            var = sylvan_ithvar(i + begin_var);
            bddTmp = ((*vars)[i] > 0 ? var : !(var)) * bddTmp;
        }
        return bddTmp;
    }

    if (bound == vars->size() && if_reverse) {
        Bdd bddTmp, var;
        bddTmp = sylvan_true;
        for (int i = 0; i < bound; ++i) {
            var = sylvan_ithvar(i + begin_var);
            bddTmp = ((*vars)[i] <= 0 ? var : !(var)) * bddTmp;
        }
        return bddTmp;
    }

    /* generate last row */
    vector<Bdd> row_vec;
    if (!if_reverse) {
        row_vec = generate_ith_row(vars, row - 1, bound, begin_var, row_vec);

        for (int i = row - 2; i >= 0; i--) {
            row_vec = generate_ith_row(vars, i, bound, begin_var, row_vec);
        }
    } else { //reversed version
        row_vec = generate_ith_row_reversed(vars, row - 1, bound, begin_var, row_vec);

        for (int i = row - 2; i >= 0; i--) {
            row_vec = generate_ith_row_reversed(vars, i, bound, begin_var, row_vec);
        }
    }
    return (row_vec)[col - 1];
}

/*
 * for output block
 */
Bdd
Encoder_Sylvan::Cudd_bddCCEncoding_modified(vector<int> *vars, int bound) {
    int row = bound;
    int col = vars->size() - bound + 1;

    if (bound <= 0) {
        return sylvan_true;
    }

    if (bound > vars->size()) {
        return sylvan_false;
    }

    if (bound == 1) {
        Bdd bddTmp, var;
        bddTmp = sylvan_false;
        for (int i = vars->size() - 1; i >= 0; i--) {
            int val = (*vars)[i];
            var = sylvan_ithvar(abs(val));
            bddTmp = (val > 0 ? var : !(var)) + bddTmp;
        }
        return bddTmp;
    }

    if (bound == vars->size()) {
        Bdd bddTmp, var;
        bddTmp = sylvan_true;
        for (int i = vars->size() - 1; i >= 0; i--) {
            int val = (*vars)[i];
            var = sylvan_ithvar(abs(val));
            bddTmp = (val > 0 ? var : !(var)) * bddTmp;
        }
        return bddTmp;
    }

    vector<Bdd> row_vec;
    /* generate last row */
    row_vec = generate_ith_row_modified(vars, row - 1, bound, row_vec);
    for (int i = row - 2; i >= 0; i--) {
        row_vec = generate_ith_row_modified(vars, i, bound, row_vec);
    }
    return (row_vec)[col - 1];
}

Bdd
Encoder_Sylvan::Cudd_bddCCITEEncoding(vector<int> *vars, int lastIndex, int bound, int begin_var,
                                        map<string, Bdd> *_BDD_cache_int) {
    if (bound <= 0) {
        return sylvan_true;
    }

    if (bound > lastIndex + 1) {
        return sylvan_false;
    }

    string key = to_string(begin_var) + '_' + to_string(lastIndex) + '_' + to_string(bound);

    if (_BDD_cache_int->find(key) != _BDD_cache_int->end()) {
        return (*_BDD_cache_int)[key];
    }
    Bdd bddTmp;
    Bdd var;
    if (bound == 1) {
        int i;
        bddTmp = sylvan_false;
        for (i = lastIndex; i >= 0; i--) {
            var = sylvan_ithvar(i + begin_var);
            bddTmp = (*vars)[i] > 0 ? var : !(var) + bddTmp; /*Perform AND Boolean operation*/
        }
    } else if (lastIndex + 1 == bound) {
        int i;
        bddTmp = sylvan_true; /*Returns the logic one constant of the manager*/
        for (i = lastIndex; i >= 0; i--) {
            var = sylvan_ithvar(i + begin_var);
            bddTmp = (*vars)[i] > 0 ? var : !(var) * bddTmp; /*Perform AND Boolean operation*/
        }
    } else {
        var = sylvan_ithvar(lastIndex + begin_var);
        Bdd bddT = Cudd_bddCCITEEncoding(vars, lastIndex - 1, bound - 1, begin_var, _BDD_cache_int);
        Bdd bddF = Cudd_bddCCITEEncoding(vars, lastIndex - 1, bound, begin_var, _BDD_cache_int);
        bddTmp = ((*vars)[lastIndex] > 0 ? var : !(var)).Ite(bddT, bddF);
    }
    (*_BDD_cache_int)[key] = bddTmp;
    return bddTmp;
}

Bdd Encoder_Sylvan::computeAndSetCUDD(int start, int end) {
    Bdd result;
    if (abs(start - end) == 1) {
        Bdd pre_half = Bdd::bddVar(start);
        Bdd pos_half = Bdd::bddVar(end);
        result = pre_half * pos_half;
    } else if (abs(start - end) == 0) {
        result = Bdd::bddVar(start);
    } else {
        Bdd pre_half = computeAndSetCUDD(start, (end - start) / 2 + start);
        Bdd pos_half = computeAndSetCUDD((end - start) / 2 + start + 1, end);
        result = pre_half * pos_half;
    }
    return result;
}

Bdd Encoder_Sylvan::Sylvan_iteCCEncoding(vector<int> *vars, int lastIndex, int bound, int begin_var,
                                           map<string, Bdd> *_BDD_cache_int) {

    if (bound <= 0) {
        return sylvan_true;
    }

    if (bound > lastIndex + 1) {
        return sylvan_false;
    }

    string key = to_string(begin_var) + '_' + to_string(lastIndex) + '_' + to_string(bound);

    if (_BDD_cache_int->find(key) != _BDD_cache_int->end()) {
        return (*_BDD_cache_int)[key];
    }
    Bdd bddTmp;
    Bdd var;
    if (bound == 1) {
        int i;
        bddTmp = sylvan_false;
        for (i = lastIndex; i >= 0; i--) {
            var = Bdd::bddVar(i + begin_var);
            bddTmp = (*vars)[i] > 0 ? var : !(var) + bddTmp; /*Perform AND Boolean operation*/
        }
    } else if (lastIndex + 1 == bound) {
        int i;
        bddTmp = sylvan_true; /*Returns the logic one constant of the manager*/
        for (i = lastIndex; i >= 0; i--) {
            var = Bdd::bddVar(i + begin_var);
            bddTmp = (*vars)[i] > 0 ? var : !(var) * bddTmp; /*Perform AND Boolean operation*/
        }
    } else {
        var = Bdd::bddVar(lastIndex + begin_var);
        Bdd bddT = Sylvan_iteCCEncoding(vars, lastIndex - 1, bound - 1, begin_var, _BDD_cache_int);
        Bdd bddF = Sylvan_iteCCEncoding(vars, lastIndex - 1, bound, begin_var, _BDD_cache_int);
        bddTmp = ((*vars)[lastIndex] > 0 ? var : !(var)).Ite(bddT, bddF);
    }
    (*_BDD_cache_int)[key] = bddTmp;
    return bddTmp;
}