//
// Created by yedi zhang on 2020/11/10.
// Implement based on BDD, output a BDD vector
//

#include "../include/Encoder_CUDD.h"
#include "math.h"
#include "cuddInt.h"
#include "cuddObj.hh"

Encoder_CUDD::Encoder_CUDD(BNNetwork *network, Query *query, bool ifIP, bool ifDC) {
    _network = network;
    _query = query;
    _ifIP = ifIP;
    _ifDC = ifDC;
    _input_size = network->get_input_size();
    _mgr = new Cudd(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);  // The manager
    cout << "This is CUDD encoding with IP= " << _ifIP << " and DC=" << _ifDC << "." << endl;
    cout << "The CUDD_MAXINDEX is: " << CUDD_MAXINDEX << endl;
}

void Encoder_CUDD::setInputRegion(BDD inputRegion) {
    this->_input_BDD = inputRegion;
    this->_if_Set_IR = true;

}

void Encoder_CUDD::testITE(bool ifITE) {
    vector<int> *instance = &(_query->_instance);
    struct timeb tm1, tm2;
    ftime(&tm1);
    if (!ifITE) {
        _input_BDD = Cudd_bddCCEncoding(instance, instance->size() - _query->_distance, 0, false);
        ftime(&tm2);
        double tmInter = ((tm2.time - tm1.time) + (tm2.millitm - tm1.millitm) / 1000.0);
        cout << "TestEncode for CUDD (cc2bdd): " << tmInter << endl;
    } else {
        map<string, BDD> *_BDD_cache_int = new map<string, BDD>;
        _input_BDD = CUDD_iteCCEncoding(instance, instance->size() - 1, instance->size() - _query->_distance, 0,
                                        _BDD_cache_int);
        delete _BDD_cache_int;
        ftime(&tm2);
        double tmInter = ((tm2.time - tm1.time) + (tm2.millitm - tm1.millitm) / 1000.0);
        cout << "TestEncode for CUDD (ITE): " << tmInter << endl;
    }
}

/*
 * Avaliable opt mode: 0,1
 * opt=0: original without input propagation;
 * opt=1: with input propagation;
 */

vector<BDD> *Encoder_CUDD::encode2targetBDD(vector<int> target_class) {
    if (_ifDC) {
        return this->encode2targetBDD_opt_0(target_class);
    } else {
        return this->encode2targetBDD_opt_1(target_class);
    }
}


vector<BDD> *Encoder_CUDD::encode2targetBDD_opt_0(vector<int> target_class) {
    int numOfInterBLK = _network->_numOfInterBLK;
    vector<BDD> *DD_blks_vec = new vector<BDD>;
    vector<BDD> *cubes_vec = new vector<BDD>;
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
                _input_BDD = _mgr->bddOne();
            } else if (distance < 0) {
                cout << "Error! Negative distance is not allowed!" << endl;
                exit(0);
            } else {
                _input_BDD = Cudd_bddCCEncoding(instance, instance->size() - distance, 0,
                                                false); /* use directly, not need CuddRef() */
            }
        }
    }

    cubes_vec->push_back(_mgr->bddOne());
    ftime(&inter_begin);
    for (int i = 0; i < numOfInterBLK; ++i) {
        BDD current_cube = computeAndSetCUDD(blks_vec[i]._input_size + start_var,
                                             blks_vec[i]._input_size + start_var + blks_vec[i]._output_size - 1);
        cubes_vec->push_back(current_cube);
        BDD DD_intBlk = encodeInterBlockDich(blks_vec[i], _input_BDD, start_var, 0, blks_vec[i]._output_size - 1);
        DD_blks_vec->push_back(DD_intBlk); /* original */
        if (_ifIP) {
            _input_BDD = DD_intBlk.ExistAbstract((*cubes_vec)[i]);
        } else {
            _input_BDD = _mgr->bddOne();
        }
        start_var = start_var + blks_vec[i]._input_size; /* original */
    }
    ftime(&inter_end);
    vector<BDD> *DD_outBlk_vec = encodeOutBlockPure(blks_vec[numOfInterBLK], start_var, _input_BDD, target_class);
    ftime(&out_end);
    return addIntegration(DD_blks_vec, DD_outBlk_vec, cubes_vec);
//    return addIntegrationOutputOne(DD_blks_vec, DD_outBlk_vec, cubes_vec);
}

/*
 * Get the final BDD for verification: opt = 1
 */
vector<BDD> *Encoder_CUDD::encode2targetBDD_opt_1(vector<int> target_class) {
    int numOfInterBLK = _network->_numOfInterBLK;
    vector<BDD> *DD_blks_vec = new vector<BDD>;
    vector<BDD> *cubes_vec = new vector<BDD>;
    vector<BNNBlock> blks_vec = _network->getBNNBlock();

    int start_var = 0;
    int in_s = blks_vec[0]._input_size;
    BDD cube_init, cube_init_tmp;
    cube_init = _mgr->bddOne();

    for (int i = 0; i < in_s; ++i) {
        BDD var = _mgr->bddVar(i);
        cube_init = var * cube_init;
    }

    cubes_vec->push_back(cube_init);

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
                _input_BDD = _mgr->bddOne();
            } else if (distance < 0) {
                cout << "Error! Negative distance is not allowed!" << endl;
                exit(0);
            } else {
                _input_BDD = Cudd_bddCCEncoding(instance, instance->size() - distance, 0,
                                                false); /* use directly, not need CuddRef() */
            }
        }
    }

    ftime(&inter_begin);
    for (int i = 0; i < numOfInterBLK; ++i) {
        int start_varr = blks_vec[i]._input_size + start_var;
        BDD current_cube = _mgr->bddOne();
        for (int t = 0; t < blks_vec[i]._output_size; ++t) {
            BDD var = _mgr->bddVar(t + start_varr);
            current_cube = var * current_cube;
        }

        cubes_vec->push_back(current_cube);
        BDD DD_intBlk = encodeInterBlockPure(blks_vec[i], start_var, _input_BDD);
        DD_blks_vec->push_back(DD_intBlk); /* original */
        _input_BDD = DD_intBlk.ExistAbstract((*cubes_vec)[i]);
        start_var = start_var + blks_vec[i]._input_size; /* original */
    }

    ftime(&inter_end);
    vector<BDD> *DD_outBlk = encodeOutBlockPure(blks_vec[numOfInterBLK], start_var, _input_BDD, target_class);
    ftime(&out_end);
    return addIntegration(DD_blks_vec, DD_outBlk, cubes_vec);
}

/*
 * This "encodeInterBlock" does not use dich, just a for-loop to compute AND.
 * */
BDD Encoder_CUDD::encodeInterBlockPure(BNNBlock blk, int start_var, BDD inputBDD) {
    BDD current_cube = _mgr->bddOne();
    BDD bddRes = inputBDD;
    for (int i = 0; i < blk._output_size; ++i) {
        /* compute cube */

        /* compute output_i_encoding */
        float c_i = -(sqrt(blk._bn_var[0][i]) / blk._bn_weight[0][i]) * blk._bn_bias[0][i] + blk._bn_mean[0][i] -
                    blk._lin_bias[0][i];
        int bound;
        BDD DD_node;

        /* when bn_weight>0 */
        if (blk._bn_weight[0][i] > 0) {
            bound = ceil((blk._input_size + c_i) / 2);
            DD_node = Cudd_bddCCEncoding(&(blk._lin_weight[i]), bound, start_var, false);

        } else if (blk._bn_weight[0][i] < 0) {
            bound = ceil((blk._input_size - c_i) / 2);
            DD_node = Cudd_bddCCEncoding(&(blk._lin_weight[i]), bound, start_var, true);
        } else {
            DD_node = blk._bn_bias[0][i] > 0 ? _mgr->bddOne() : _mgr->bddZero();
        }
        BDD y = _mgr->bddVar(blk._input_size + start_var + i);
        BDD output_i = DD_node.Xnor(y);
        bddRes = bddRes * output_i;
    }
    return bddRes;
}

BDD Encoder_CUDD::encodeInterBlockDich(BNNBlock blk, BDD inputBDD, int start_var, int start, int end) {
    BDD result;
    if (abs(start - end) == 1) {
        float c_start = -(sqrt(blk._bn_var[0][start]) / blk._bn_weight[0][start]) * blk._bn_bias[0][start] +
                        blk._bn_mean[0][start] - blk._lin_bias[0][start];
        float c_end = -(sqrt(blk._bn_var[0][end]) / blk._bn_weight[0][end]) * blk._bn_bias[0][end] +
                      blk._bn_mean[0][end] - blk._lin_bias[0][end];

        int bound_start, bound_end;

        BDD y_start = _mgr->bddVar(blk._input_size + start_var + start);
        BDD y_end = _mgr->bddVar(blk._input_size + start_var + end);
        BDD pre_half, pos_half;

        if (blk._bn_weight[0][start] > 0) {
            bound_start = ceil((blk._input_size + c_start) / 2);
            pre_half = Cudd_bddCCEncoding(&(blk._lin_weight[start]), bound_start, start_var, false);
        } else if (blk._bn_weight[0][start] < 0) {
            bound_start = ceil((blk._input_size - c_start) / 2);
            pre_half = Cudd_bddCCEncoding(&(blk._lin_weight[start]), bound_start, start_var, true);
        } else {
            pre_half = blk._bn_bias[0][start] > 0 ? _mgr->bddOne() : _mgr->bddZero();
        }

        if (blk._bn_weight[0][end] > 0) {
            bound_end = ceil((blk._input_size + c_end) / 2);
            pos_half = Cudd_bddCCEncoding(&(blk._lin_weight[end]), bound_end, start_var, false);
        } else if (blk._bn_weight[0][end] < 0) {
            bound_end = ceil((blk._input_size - c_end) / 2);
            pos_half = Cudd_bddCCEncoding(&(blk._lin_weight[end]), bound_end, start_var, true);
        } else {
            pos_half = blk._bn_bias[0][end] > 0 ? _mgr->bddOne() : _mgr->bddZero();
        }

        pre_half = pre_half.Xnor(y_start);
        pre_half = pre_half * inputBDD;
        pos_half = pos_half.Xnor(y_end);
//        pos_half = pos_half * inputBDD;
        result = pre_half * pos_half;
    } else if (abs(start - end) == 0) {
        float c_start = -(sqrt(blk._bn_var[0][start]) / blk._bn_weight[0][start]) * blk._bn_bias[0][start] +
                        blk._bn_mean[0][start] - blk._lin_bias[0][start];
        int bound_start;
        BDD y_end = _mgr->bddVar(blk._input_size + start_var + end);

        if (blk._bn_weight[0][start] > 0) {
            bound_start = ceil((blk._input_size + c_start) / 2);
            result = Cudd_bddCCEncoding(&(blk._lin_weight[start]), bound_start, start_var, false);
        } else if (blk._bn_weight[0][start] < 0) {
            bound_start = ceil((blk._input_size - c_start) / 2);
            result = Cudd_bddCCEncoding(&(blk._lin_weight[start]), bound_start, start_var, true);
        } else {
            result = blk._bn_bias[0][start] > 0 ? _mgr->bddOne() : _mgr->bddZero();
        }
        result = result.Xnor(y_end);
//        result = result * inputBDD;
    } else {
        BDD pre_half = encodeInterBlockDich(blk, inputBDD, start_var, start, (end - start) / 2 + start);
        BDD pos_half = encodeInterBlockDich(blk, inputBDD, start_var, (end - start) / 2 + start + 1, end);
        result = pre_half * pos_half;
    }
    return result;
}

/*
 * Get ADD for the output block
 */
vector<BDD> *
Encoder_CUDD::encodeOutBlockPure(BNNBlock blk, int start_var, BDD inputBDD, vector<int> target_class) {
    vector<BDD> *bdd_vec = new vector<BDD>;
    for (int t = 0; t < target_class.size(); ++t) {
        BDD tmpSingle, ddSingle;
        ddSingle = _mgr->bddOne();
        int i = target_class[t];

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

            BDD DD_node = Cudd_bddCCEncoding_modified(&vars, bound);
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


vector<BDD> *
Encoder_CUDD::addIntegrationOutputOne(vector<BDD> *DD_blks_vec, vector<BDD> *output_vec, vector<BDD> *cubes_vec) {
    int numOfInterBLK = _network->_numOfInterBLK;
    BDD final_res, final_tmp1;
    final_res = (*DD_blks_vec)[0];

    for (int i = 1; i < numOfInterBLK; ++i) {
        final_res = final_res.AndAbstract((*DD_blks_vec)[i], (*cubes_vec)[i]);
    }

    delete DD_blks_vec;

    vector<BDD> *output_res = new vector<BDD>;
    BDD outputVar = _mgr->bddVar(100);

    BDD output0 = (*output_vec)[0];
    BDD output8 = (*output_vec)[1];

//    BDD outputBDD = outputVar.Ite((*output_vec)[0], (*output_vec)[1]);

    /* compute for each single output BDD, output is a BDD vector */
    final_tmp1 = final_res.AndAbstract(output0, (*cubes_vec)[numOfInterBLK]);
    BDD final_tmp2 = final_res.AndAbstract(output8, (*cubes_vec)[numOfInterBLK]);
//    BDD final_tmpNew = final_res.AndAbstract(outputNew, (*cubes_vec)[numOfInterBLK]);
    output_res->push_back(final_tmp1);
    output_res->push_back(final_tmp2);
//    char *file = "/home/yedi/tosem21ext/test.txt";
//    write_dd(_mgr->getManager(), final_tmp1.getNode(), file);

//    output_res->push_back(final_tmpNew);
    delete output_vec;
    delete cubes_vec;
    return output_res;
}


/*
 * Integrate all blocks into several ADDs, each for a label
 */
vector<BDD> *
Encoder_CUDD::addIntegration(vector<BDD> *DD_blks_vec, vector<BDD> *output_vec, vector<BDD> *cubes_vec) {
    int numOfInterBLK = _network->_numOfInterBLK;
    BDD final_res, final_tmp1;
    final_res = (*DD_blks_vec)[0];

    for (int i = 1; i < numOfInterBLK; ++i) {
        final_res = final_res.AndAbstract((*DD_blks_vec)[i], (*cubes_vec)[i]);
    }

    delete DD_blks_vec;

    vector<BDD> *output_res = new vector<BDD>;

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
vector<BDD>
Encoder_CUDD::generate_ith_row(vector<int> *vars, int row_index, int bound, int begin_var,
                                 vector<BDD> res_last_vec) {
    /* generate one row */
    int row = bound;
    int col = vars->size() - bound + 1;

    vector<BDD> row_vec;

    BDD x_r_c;

    /* last row */
    if (row_index + 1 == row) {
        int val = (*vars)[row_index + col - 1];
        /* last col */
        x_r_c = _mgr->bddVar(begin_var + row_index + col - 1);
        row_vec.push_back(val > 0 ? x_r_c : !(x_r_c));

        for (int j = col - 2; j >= 0; j--) {
            BDD x_r_j;
            BDD xx = _mgr->bddVar(begin_var + row_index + j);
            if ((*vars)[row_index + j] > 0) {
                x_r_j = xx.Ite(_mgr->bddOne(), (row_vec)[col - j - 2]);
            } else {
                x_r_j = xx.Ite((row_vec)[col - j - 2], _mgr->bddOne());
            }
            row_vec.push_back(x_r_j);
        }
    } else { /* for other rows */
        /* last col */
        if ((*vars)[row_index + col - 1] > 0) {
            BDD xx = _mgr->bddVar(begin_var + row_index + col - 1);
            x_r_c = xx.Ite((res_last_vec)[0], _mgr->bddZero());
        } else {
            BDD xx = _mgr->bddVar(begin_var + row_index + col - 1);
            x_r_c = xx.Ite(_mgr->bddZero(), (res_last_vec)[0]);
        }

        row_vec.push_back(x_r_c);

        for (int j = col - 2; j >= 0; j--) {
            BDD x_r_j;
            BDD xx = _mgr->bddVar(begin_var + row_index + j);
            if ((*vars)[row_index + j] > 0) {
                x_r_j = xx.Ite((res_last_vec)[col - j - 1], (row_vec)[col - j - 2]);
            } else {
                x_r_j = xx.Ite((row_vec)[col - j - 2], (res_last_vec)[col - j - 1]);
            }
            row_vec.push_back(x_r_j);
        }
    }
    return row_vec;
}

/*
 * for internal block: bn_var < 0
 */
vector<BDD>
Encoder_CUDD::generate_ith_row_reversed(vector<int> *vars, int row_index, int bound, int begin_var,
                                          vector<BDD> res_last_vec) {
    /* generate one row */
    int row = bound;
    int col = vars->size() - bound + 1;

    vector<BDD> row_vec;

    BDD x_r_c;

    /* last row */
    if (row_index + 1 == row) {
        int val = (*vars)[row_index + col - 1];
        /* last col */
        x_r_c = _mgr->bddVar(begin_var + row_index + col - 1);
        row_vec.push_back(val <= 0 ? x_r_c : !(x_r_c));

        for (int j = col - 2; j >= 0; j--) {
            BDD x_r_j;
            BDD xx = _mgr->bddVar(begin_var + row_index + j);
            /* original */
            if ((*vars)[row_index + j] <= 0) {
                x_r_j = xx.Ite(_mgr->bddOne(), (row_vec)[col - j - 2]);
            } else {
                x_r_j = xx.Ite((row_vec)[col - j - 2], _mgr->bddOne());
            }
            row_vec.push_back(x_r_j);
        }
    } else { /* for other rows */
        /* last col, original */
        if ((*vars)[row_index + col - 1] <= 0) {
            BDD xx = _mgr->bddVar(begin_var + row_index + col - 1);
            x_r_c = xx.Ite((res_last_vec)[0], _mgr->bddZero());
        } else {
            BDD xx = _mgr->bddVar(begin_var + row_index + col - 1);
            x_r_c = xx.Ite(_mgr->bddZero(), (res_last_vec)[0]);
        }
        row_vec.push_back(x_r_c);

        for (int j = col - 2; j >= 0; j--) {
            BDD x_r_j;
            BDD xx = _mgr->bddVar(begin_var + row_index + j);
            if ((*vars)[row_index + j] <= 0) {
                x_r_j = xx.Ite((res_last_vec)[col - j - 1], (row_vec)[col - j - 2]);
            } else {
                x_r_j = xx.Ite((row_vec)[col - j - 2], (res_last_vec)[col - j - 1]);
            }
            row_vec.push_back(x_r_j);
        }
    }
    return row_vec;
}


/*
 * for internal block
 */
BDD
Encoder_CUDD::Cudd_bddCCEncoding(vector<int> *vars, int bound, int begin_var, bool if_reverse) {
    int row = bound;
    int col = vars->size() - bound + 1;

    if (bound <= 0) {
        return _mgr->bddOne();
    }

    if (bound > vars->size()) {
        return _mgr->bddZero();
    }

    if (bound == 1 && !if_reverse) {
        BDD bddTmp, var;
        bddTmp = _mgr->bddZero();
        for (int i = vars->size() - 1; i >= 0; i--) {
            var = _mgr->bddVar(i + begin_var);
            bddTmp = ((*vars)[i] > 0 ? var : !var) + bddTmp;
        }
        return bddTmp;
    }

    if (bound == 1 && if_reverse) {
        BDD bddTmp, var;
        bddTmp = _mgr->bddZero();
        for (int i = vars->size() - 1; i >= 0; i--) {
            var = _mgr->bddVar(i + begin_var);
            bddTmp = ((*vars)[i] <= 0 ? var : !(var)) + bddTmp;
        }
        return bddTmp;
    }

    if (bound == vars->size() && !if_reverse) {
        BDD bddTmp, var;
        bddTmp = _mgr->bddOne();
        for (int i = 0; i < bound; ++i) {
            var = _mgr->bddVar(i + begin_var);
            bddTmp = ((*vars)[i] > 0 ? var : !(var)) * bddTmp;
        }
        return bddTmp;
    }

    if (bound == vars->size() && if_reverse) {
        BDD bddTmp, var;
        bddTmp = _mgr->bddOne();
        for (int i = 0; i < bound; ++i) {
            var = _mgr->bddVar(i + begin_var);
            bddTmp = ((*vars)[i] <= 0 ? var : !(var)) * bddTmp;
        }
        return bddTmp;
    }

    /* generate last row */
    vector<BDD> row_vec;
    if (!if_reverse) {
        row_vec = generate_ith_row(vars, row - 1, bound, begin_var, row_vec);

        for (int i = row - 2; i >= 0; i--) {
            row_vec = generate_ith_row(vars, i, bound, begin_var, row_vec);
        }
    } else { //TODO: reversed version
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
vector<BDD>
Encoder_CUDD::generate_ith_row_modified(vector<int> *vars, int row_index, int bound, vector<BDD> res_last_vec) {
    /* generate one row */
    int row = bound;
    int col = vars->size() - bound + 1;

    vector<BDD> row_vec;

    BDD x_r_c;

    /* last row */
    if (row_index + 1 == row) {
        /* last col */
        int val = (*vars)[row_index + col - 1];
        x_r_c = _mgr->bddVar(abs(val));
        row_vec.push_back(val > 0 ? x_r_c : !x_r_c);

        for (int j = col - 2; j >= 0; j--) {
            BDD x_r_j;
            BDD xx = _mgr->bddVar(abs((*vars)[row_index + j]));
            if ((*vars)[row_index + j] > 0) {
                x_r_j = xx.Ite(_mgr->bddOne(), (row_vec)[col - j - 2]);
            } else {
                x_r_j = xx.Ite((row_vec)[col - j - 2], _mgr->bddOne());
            }
            row_vec.push_back(x_r_j);
        }
    } else { /* for other rows */
        /* last col */
        if ((*vars)[row_index + col - 1] > 0) {
            BDD xx = _mgr->bddVar((*vars)[row_index + col - 1]);
            x_r_c = xx.Ite((res_last_vec)[0], _mgr->bddZero());
        } else {
            BDD xx = _mgr->bddVar(-(*vars)[row_index + col - 1]);
            x_r_c = xx.Ite(_mgr->bddZero(), (res_last_vec)[0]);
        }
        row_vec.push_back(x_r_c);

        for (int j = col - 2; j >= 0; j--) {
            BDD x_r_j;
            int val = (*vars)[row_index + j];
            BDD xx = _mgr->bddVar(abs(val));
            if (val > 0) {
                x_r_j = xx.Ite((res_last_vec)[col - j - 1], (row_vec)[col - j - 2]);
            } else {
                x_r_j = xx.Ite((row_vec)[col - j - 2], (res_last_vec)[col - j - 1]);
            }
            row_vec.push_back(x_r_j);
        }
    }
    return row_vec;
}

/*
 * for output block
 */
BDD
Encoder_CUDD::Cudd_bddCCEncoding_modified(vector<int> *vars, int bound) {
    int row = bound;
    int col = vars->size() - bound + 1;

    if (bound <= 0) {
        return _mgr->bddOne();
    }

    if (bound > vars->size()) {
        return _mgr->bddZero();
    }

    if (bound == 1) {
        BDD bddTmp, var;
        bddTmp = _mgr->bddZero();
        for (int i = vars->size() - 1; i >= 0; i--) {
            int val = (*vars)[i];
            var = _mgr->bddVar(abs(val));
            bddTmp = (val > 0 ? var : !(var)) + bddTmp;
        }
        return bddTmp;
    }

    if (bound == vars->size()) {
        BDD bddTmp, var;
        bddTmp = _mgr->bddOne();
        for (int i = vars->size() - 1; i >= 0; i--) {
            int val = (*vars)[i];
            var = _mgr->bddVar(abs(val));
            bddTmp = (val > 0 ? var : !(var)) * bddTmp;
        }
        return bddTmp;
    }

    vector<BDD> row_vec;
    /* generate last row */
    row_vec = generate_ith_row_modified(vars, row - 1, bound, row_vec);
    for (int i = row - 2; i >= 0; i--) {
        row_vec = generate_ith_row_modified(vars, i, bound, row_vec);
    }

    return (row_vec)[col - 1];
}

BDD Encoder_CUDD::computeAndSetCUDD(int start, int end) {
    BDD result;
    if (abs(start - end) == 1) {
        BDD pre_half = _mgr->bddVar(start);
        BDD pos_half = _mgr->bddVar(end);
        result = pre_half * pos_half;
    } else if (abs(start - end) == 0) {
        result = _mgr->bddVar(start);
    } else {
        BDD pre_half = computeAndSetCUDD(start, (end - start) / 2 + start);
        BDD pos_half = computeAndSetCUDD((end - start) / 2 + start + 1, end);
        result = pre_half * pos_half;
    }
    return result;
}

BDD Encoder_CUDD::CUDD_iteCCEncoding(vector<int> *vars, int lastIndex, int bound, int begin_var,
                                       map<string, BDD> *_BDD_cache_int) {

    if (bound <= 0) {
        return _mgr->bddOne();
    }

    if (bound > lastIndex + 1) {
        return _mgr->bddZero();
    }

    string key = to_string(begin_var) + '_' + to_string(lastIndex) + '_' + to_string(bound);

    if (_BDD_cache_int->find(key) != _BDD_cache_int->end()) {
        return (*_BDD_cache_int)[key];
    }
    BDD bddTmp;
    BDD var;
    if (bound == 1) {
        int i;
        bddTmp = _mgr->bddZero();
        for (i = lastIndex; i >= 0; i--) {
            var = _mgr->bddVar(i + begin_var);
            bddTmp = (*vars)[i] > 0 ? var : !(var) + bddTmp; /*Perform AND Boolean operation*/
        }
    } else if (lastIndex + 1 == bound) {
        int i;
        bddTmp = _mgr->bddOne(); /*Returns the logic one constant of the manager*/
        for (i = lastIndex; i >= 0; i--) {
            var = _mgr->bddVar(i + begin_var);
            bddTmp = (*vars)[i] > 0 ? var : !(var) * bddTmp; /*Perform AND Boolean operation*/
        }
    } else {
        var = _mgr->bddVar(lastIndex + begin_var);
        BDD bddT = CUDD_iteCCEncoding(vars, lastIndex - 1, bound - 1, begin_var, _BDD_cache_int);
        BDD bddF = CUDD_iteCCEncoding(vars, lastIndex - 1, bound, begin_var, _BDD_cache_int);
        bddTmp = ((*vars)[lastIndex] > 0 ? var : !(var)).Ite(bddT, bddF);
    }
    (*_BDD_cache_int)[key] = bddTmp;
    return bddTmp;
}
