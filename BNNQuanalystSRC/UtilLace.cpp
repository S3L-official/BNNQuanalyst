//
// Created by yedi zhang on 2021/8/3.
//

#include "../include/Encoder_Sylvan.h"
#include "../include/BNNetwork.h"
#include "../include/UtilLace.h"

vector<sylvanBdd> generate_ith_row(vector<int> *vars, int row_index, int bound, int begin_var,
                                   vector<sylvanBdd> res_last_vec) {
    /* generate one row */
    int row = bound;
    int col = vars->size() - bound + 1;
    vector<sylvanBdd> row_vec;

    sylvanBdd x_r_c;

    /* last row */
    if (row_index + 1 == row) {
        int val = (*vars)[row_index + col - 1];
        /* last col */
        x_r_c = sylvan_ithvar(begin_var + row_index + col - 1);
        row_vec.push_back(val > 0 ? x_r_c : !(x_r_c));

        for (int j = col - 2; j >= 0; j--) {
            sylvanBdd x_r_j;
            sylvanBdd var = sylvanBdd::bddVar(begin_var + row_index + j);
            if ((*vars)[row_index + j] > 0) {
                x_r_j = var.Ite(sylvan_true, (row_vec)[col - j - 2]);
            } else {
                x_r_j = var.Ite((row_vec)[col - j - 2], sylvan_true);
            }
            row_vec.push_back(x_r_j);
        }
    } else { /* for other rows */
        /* last col */
        sylvanBdd var = sylvanBdd::bddVar(begin_var + row_index + col - 1);
        if ((*vars)[row_index + col - 1] > 0) {
            x_r_c = var.Ite((res_last_vec)[0], sylvan_false);
        } else {
            x_r_c = var.Ite(sylvan_false, (res_last_vec)[0]);
        }

        row_vec.push_back(x_r_c);

        for (int j = col - 2; j >= 0; j--) {
            sylvanBdd x_r_j;
            sylvanBdd var = sylvanBdd::bddVar(begin_var + row_index + j);
            if ((*vars)[row_index + j] > 0) {
                x_r_j = var.Ite((res_last_vec)[col - j - 1], (row_vec)[col - j - 2]);
            } else {
                x_r_j = var.Ite((row_vec)[col - j - 2], (res_last_vec)[col - j - 1]);
            }
            row_vec.push_back(x_r_j);
        }
    }
    return row_vec;
}

vector<sylvanBdd> generate_ith_row_reversed(vector<int> *vars, int row_index, int bound, int begin_var,
                                            vector<sylvanBdd> res_last_vec) {
    /* generate one row */
    int row = bound;
    int col = vars->size() - bound + 1;
    vector<sylvanBdd> row_vec;

    sylvanBdd x_r_c;

    /* last row */
    if (row_index + 1 == row) {
        int val = (*vars)[row_index + col - 1];
        /* last col */
        x_r_c = sylvan_ithvar(begin_var + row_index + col - 1);
        row_vec.push_back(val <= 0 ? x_r_c : !(x_r_c));

        for (int j = col - 2; j >= 0; j--) {
            sylvanBdd x_r_j;
            sylvanBdd var = sylvanBdd::bddVar(begin_var + row_index + j);
            if ((*vars)[row_index + j] <= 0) {
                x_r_j = var.Ite(sylvan_true, (row_vec)[col - j - 2]);
            } else {
                x_r_j = var.Ite((row_vec)[col - j - 2], sylvan_true);
            }
            row_vec.push_back(x_r_j);
        }
    } else { /* for other rows */
        /* last col */
        sylvanBdd var = sylvanBdd::bddVar(begin_var + row_index + col - 1);
        if ((*vars)[row_index + col - 1] <= 0) {
            x_r_c = var.Ite((res_last_vec)[0], sylvan_false);
        } else {
            x_r_c = var.Ite(sylvan_false, (res_last_vec)[0]);
        }

        row_vec.push_back(x_r_c);

        for (int j = col - 2; j >= 0; j--) {
            sylvanBdd x_r_j;
            sylvanBdd var = sylvanBdd::bddVar(begin_var + row_index + j);
            if ((*vars)[row_index + j] <= 0) {
                x_r_j = var.Ite((res_last_vec)[col - j - 1], (row_vec)[col - j - 2]);
            } else {
                x_r_j = var.Ite((row_vec)[col - j - 2], (res_last_vec)[col - j - 1]);
            }
            row_vec.push_back(x_r_j);
        }
    }
    return row_vec;
}

sylvanBdd bddCCEncoding(vector<int> *vars, int bound, int begin_var, bool if_reverse) {
    int row = bound;
    int col = vars->size() - bound + 1;

    if (bound <= 0) {
        return sylvan_true;
    }

    if (bound > vars->size()) {
        return sylvan_false;
    }

    if (bound == 1 && !if_reverse) {
        sylvanBdd bddTmp, var;
        bddTmp = sylvan_false;
        for (int i = vars->size() - 1; i >= 0; i--) {
            var = sylvanBdd::bddVar(i + begin_var);
            bddTmp = ((*vars)[i] > 0 ? var : !(var)) + bddTmp;
        }
        return bddTmp;
    }

    if (bound == 1 && if_reverse) {
        sylvanBdd bddTmp, var;
        bddTmp = sylvan_false;
        for (int i = vars->size() - 1; i >= 0; i--) {
            var = sylvanBdd::bddVar(i + begin_var);
            bddTmp = ((*vars)[i] <= 0 ? var : !(var)) + bddTmp;
        }
        return bddTmp;
    }

    if (bound == vars->size() && !if_reverse) {
        sylvanBdd bddTmp, var;
        bddTmp = sylvan_true;
        for (int i = 0; i < bound; ++i) {
            var = sylvanBdd::bddVar(i + begin_var);
            bddTmp = ((*vars)[i] > 0 ? var : !(var)) * bddTmp;
        }
        return bddTmp;
    }

    if (bound == vars->size() && if_reverse) {
        sylvanBdd bddTmp, var;
        bddTmp = sylvan_true;
        for (int i = 0; i < bound; ++i) {
            var = sylvanBdd::bddVar(i + begin_var);
            bddTmp = ((*vars)[i] <= 0 ? var : !(var)) * bddTmp;
        }
        return bddTmp;
    }

    /* generate last row */
    vector<sylvanBdd> row_vec;
    if (!if_reverse) {
        row_vec = generate_ith_row(vars, row - 1, bound, begin_var, row_vec);
        for (int i = row - 2; i >= 0; i--) {
            row_vec = generate_ith_row(vars, i, bound, begin_var, row_vec);
        }
    } else {
        row_vec = generate_ith_row_reversed(vars, row - 1, bound, begin_var, row_vec);

        for (int i = row - 2; i >= 0; i--) {
            row_vec = generate_ith_row_reversed(vars, i, bound, begin_var, row_vec);
        }
    }
    return (row_vec)[col - 1];
}

vector<sylvanBdd>
generate_ith_row_modified(vector<int> *vars, int row_index, int bound, vector<Bdd> res_last_vec) {
    /* generate one row */
    int row = bound;
    int col = vars->size() - bound + 1;

    vector<sylvanBdd> row_vec;

    sylvanBdd x_r_c;

    /* last row */
    if (row_index + 1 == row) {
        /* last col */
        int val = (*vars)[row_index + col - 1];
        x_r_c = sylvan_ithvar(abs(val));
        row_vec.push_back(val > 0 ? x_r_c : !(x_r_c));

        for (int j = col - 2; j >= 0; j--) {
            sylvanBdd x_r_j;
            sylvanBdd xx = sylvan_ithvar(abs((*vars)[row_index + j]));
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
            sylvanBdd xx = sylvan_ithvar((*vars)[row_index + col - 1]);
            x_r_c = xx.Ite((res_last_vec)[0], sylvan_false);
        } else {
            sylvanBdd xx = sylvan_ithvar(-(*vars)[row_index + col - 1]);
            x_r_c = xx.Ite(sylvan_false, (res_last_vec)[0]);
        }
        row_vec.push_back(x_r_c);

        for (int j = col - 2; j >= 0; j--) {
            sylvanBdd x_r_j;
            int val = (*vars)[row_index + j];
            sylvanBdd xx = sylvan_ithvar(abs(val));
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

sylvanBdd bddCCEncodingModified(vector<int> *vars, int bound) {
    int row = bound;
    int col = vars->size() - bound + 1;

    if (bound <= 0) {
        return sylvan_true;
    }

    if (bound > vars->size()) {
        return sylvan_false;
    }

    if (bound == 1) {
        sylvanBdd bddTmp, var;
        bddTmp = sylvan_false;
        for (int i = vars->size() - 1; i >= 0; i--) {
            int val = (*vars)[i];
            var = sylvan_ithvar(abs(val));
            bddTmp = (val > 0 ? var : !(var)) + bddTmp;
        }
        return bddTmp;
    }

    if (bound == vars->size()) {
        sylvanBdd bddTmp, var;
        bddTmp = sylvan_true;
        for (int i = vars->size() - 1; i >= 0; i--) {
            int val = (*vars)[i];
            var = sylvan_ithvar(abs(val));
            bddTmp = (val > 0 ? var : !(var)) * bddTmp;
        }
        return bddTmp;
    }

    vector<sylvanBdd> row_vec;
    /* generate last row */
    row_vec = generate_ith_row_modified(vars, row - 1, bound, row_vec);
    for (int i = row - 2; i >= 0; i--) {
        row_vec = generate_ith_row_modified(vars, i, bound, row_vec);
    }
    return (row_vec)[col - 1];
}

TASK_IMPL_2(sylvanBDD, computeAndSetBDD, int, start, int, end) {
    sylvanBdd result;
    if (abs(start - end) == 1) {
        sylvanBdd pre_half = Bdd::bddVar(start);
        sylvanBdd pos_half = Bdd::bddVar(end);
        result = pre_half * pos_half;
    } else if (abs(start - end) == 0) {
        result = Bdd::bddVar(start);
    } else {
        bdd_refs_spawn(SPAWN(computeAndSetBDD, start, (end - start) / 2 + start));
        sylvanBdd pos_half = CALL(computeAndSetBDD, (end - start) / 2 + start + 1, end);
        sylvanBdd pre_half = bdd_refs_sync(SYNC(computeAndSetBDD));
        result = pre_half * pos_half;
    }
    return result.GetBDD();
}

TASK_IMPL_4(sylvanBDD, dichEnc, vector<sylvanBdd> *, bdd_vec, sylvanBdd, inputBDD, int, start, int, end) {
    sylvanBdd result;
    if (abs(start - end) == 1) {
        sylvanBdd pre_half = (*bdd_vec)[start];
        sylvanBdd pos_half = (*bdd_vec)[end];
        result = pre_half * inputBDD * pos_half;
    } else if (abs(start - end) == 0) {
        result = (*bdd_vec)[start];
    } else {
        bdd_refs_spawn(SPAWN(dichEnc, bdd_vec, inputBDD, start, (end - start) / 2 + start));
        sylvanBdd pos_half = CALL(dichEnc, bdd_vec, inputBDD, (end - start) / 2 + start + 1, end);
        sylvanBdd pre_half = bdd_refs_sync(SYNC(dichEnc));
        result = pre_half * pos_half;
    }
    return result.GetBDD();
}

/* use encodeInterBlk and then output: all the blocks parallel, for opt = 4 */
TASK_IMPL_5(vector<sylvanBdd>*, parallelInterBlk, Encoder_Sylvan*, encoder, vector<BNNBlock>*, blk_vec, vector<int>*,
            targetCls, vector<sylvanBdd>*, cubes_vec, sylvanBdd, inputBdd) {

    vector<sylvanBdd> *bdd_vec_inter = new vector<sylvanBdd>; // input block and output block

    int size_internal = blk_vec->size() - 1;

    for (int i = 0; i < size_internal; ++i) {
        bdd_vec_inter->push_back(sylvan_false);
    }

    int start_var = 0;
    sylvanBdd inputregion = inputBdd;
    for (int t = 0; t < size_internal-1; ++t) { // unit+1
        bdd_refs_spawn(SPAWN(encInterBlk, &(*blk_vec)[t], inputregion, start_var, 0, (*blk_vec)[t]._output_size - 1));
        start_var = start_var + (*blk_vec)[t]._input_size;
        inputregion = sylvan_true;
    }

    (*bdd_vec_inter)[size_internal - 1] = CALL(encInterBlk, &(*blk_vec)[size_internal - 1], sylvan_true, start_var,
                                               0, (*blk_vec)[size_internal - 1]._output_size - 1);

    start_var = start_var + (*blk_vec)[size_internal - 1]._input_size;
    for (int t = size_internal - 2; t >= 0; t--) { // unit+1
        (*bdd_vec_inter)[t] = bdd_refs_sync(SYNC(encInterBlk));
    }

    ftime(&encoder->out_begin);
    vector<sylvanBdd> *bdd_vec_out = encOutputBlk(&(*blk_vec)[size_internal], sylvan_true, start_var, targetCls);
    ftime(&encoder->out_end);

    for (int i = 0; i < bdd_vec_out->size(); ++i) {
        bdd_vec_inter->push_back((*bdd_vec_out)[i]);
    }
    delete bdd_vec_out;

    /* compute the cube vectors */
    int cube_start_var = 0;
    for (int key = 0; key < size_internal - 1; ++key) {
        int in_size = (*blk_vec)[key]._input_size;
        int out_size = (*blk_vec)[key]._output_size;
        bdd_refs_spawn(SPAWN(computeAndSetBDD, in_size + cube_start_var, in_size + cube_start_var + out_size - 1));
        cube_start_var = cube_start_var + in_size;
    }

    (*cubes_vec)[size_internal] = CALL(computeAndSetBDD, (*blk_vec)[size_internal - 1]._input_size + cube_start_var,
                                           (*blk_vec)[size_internal - 1]._input_size + cube_start_var +
                                           (*blk_vec)[size_internal - 1]._output_size - 1);
    for (int t = size_internal-2; t >=0; t--) { // unit+1
        (*cubes_vec)[t+1] = bdd_refs_sync(SYNC(computeAndSetBDD));
    }
    return bdd_vec_inter;
}


/* parallel for internal blocks: opt = 2,3 */
TASK_IMPL_5(sylvanBDD, encInterBlk, myBlock, blk, Bdd, inputBDD, int, start_var, int, start, int, end) {
    Bdd result;
    if (abs(start - end) == 1) {
        float c_start = -(sqrt(blk->_bn_var[0][start]) / blk->_bn_weight[0][start]) * blk->_bn_bias[0][start] +
                        blk->_bn_mean[0][start] - blk->_lin_bias[0][start];
        float c_end = -(sqrt(blk->_bn_var[0][end]) / blk->_bn_weight[0][end]) * blk->_bn_bias[0][end] +
                      blk->_bn_mean[0][end] - blk->_lin_bias[0][end];

        int bound_start, bound_end;

        Bdd y_start = sylvan_ithvar(blk->_input_size + start_var + start);
        Bdd y_end = sylvan_ithvar(blk->_input_size + start_var + end);
        Bdd pre_half, pos_half;

        if (blk->_bn_weight[0][start] > 0) {
            bound_start = ceil((blk->_input_size + c_start) / 2);
            pre_half = bddCCEncoding(&(blk->_lin_weight[start]), bound_start, start_var, false);
        } else if (blk->_bn_weight[0][start] < 0) {
            bound_start = ceil((blk->_input_size - c_start) / 2);
            pre_half = bddCCEncoding(&(blk->_lin_weight[start]), bound_start, start_var, true);
        } else {
            pre_half = blk->_bn_bias[0][start] > 0 ? sylvan_true : sylvan_false;
        }
        if (blk->_bn_weight[0][end] > 0) {
            bound_end = ceil((blk->_input_size + c_end) / 2);
            pos_half = bddCCEncoding(&(blk->_lin_weight[end]), bound_end, start_var, false);
        } else if (blk->_bn_weight[0][end] < 0) {
            bound_end = ceil((blk->_input_size - c_end) / 2);
            pos_half = bddCCEncoding(&(blk->_lin_weight[end]), bound_end, start_var, true);
        } else {
            pos_half = blk->_bn_bias[0][end] > 0 ? sylvan_true : sylvan_false;
        }

        pre_half = pre_half.Xnor(y_start);
        pos_half = pos_half.Xnor(y_end);
        pre_half = pre_half * inputBDD;
//        pos_half = pos_half * inputBDD;
        result = pre_half * pos_half;
    } else if (abs(start - end) == 0) {
        float c_start = -(sqrt(blk->_bn_var[0][start]) / blk->_bn_weight[0][start]) * blk->_bn_bias[0][start] +
                        blk->_bn_mean[0][start] - blk->_lin_bias[0][start];
        int bound_start = ceil((blk->_input_size + c_start) / 2);
        Bdd y_end = sylvan_ithvar(blk->_input_size + start_var + end);

        if (blk->_bn_weight[0][start] > 0) {
            bound_start = ceil((blk->_input_size + c_start) / 2);
            result = bddCCEncoding(&(blk->_lin_weight[start]), bound_start, start_var, false);
        } else if (blk->_bn_weight[0][start] < 0) {
            bound_start = ceil((blk->_input_size - c_start) / 2);
            result = bddCCEncoding(&(blk->_lin_weight[start]), bound_start, start_var, true);
        } else {
            result = blk->_bn_bias[0][start] > 0 ? sylvan_true : sylvan_false;
        }
        result = result.Xnor(y_end);
//        result = result * inputBDD;
    } else {
        bdd_refs_spawn(SPAWN(encInterBlk, blk, inputBDD, start_var, start, (end - start) / 2 + start));
        Bdd pos_half = CALL(encInterBlk, blk, inputBDD, start_var, (end - start) / 2 + start + 1, end);
        Bdd pre_half = bdd_refs_sync(SYNC(encInterBlk));
        result = pre_half * pos_half;
    }
    return result.GetBDD();
}




TASK_IMPL_4(sylvanBDD, encOutputBlkClassPure, myBlock, blk, Bdd, inputBDD, int, start_var, int, targetCls) {
    int i = targetCls;
    Bdd tmpSingle, ddSingle; /* f_i1 and ... and f_in*/
    ddSingle = sylvan_true;

    for (int j = 0; j < blk->_output_size; ++j) {
        if (i == j) { continue; }
        vector<int> vars;
        int sum_w_i = 0;
        int w_i_minus_num = 0;
        for (int k = 0; k < blk->_input_size; ++k) {
            if (blk->_lin_weight[i][k] > 0 & blk->_lin_weight[j][k] < 0) {
                vars.push_back(k + start_var);
            } else if (blk->_lin_weight[i][k] < 0 & blk->_lin_weight[j][k] > 0) {
                vars.push_back(-k - start_var);
                w_i_minus_num++;
            }
            sum_w_i = sum_w_i + blk->_lin_weight[i][k] - blk->_lin_weight[j][k];
        }

        int bound;

        float fl = (sum_w_i + blk->_lin_bias[0][j] - blk->_lin_bias[0][i]) / 4;
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
        Bdd DD_node = bddCCEncodingModified(&vars, bound);
        if ((j == 0 && i != 0) || (i == 0 && j == 1)) {
            tmpSingle = DD_node * inputBDD; /* Perform first AND Boolean operation */
        } else {
            tmpSingle = DD_node * ddSingle; /* Perform AND Boolean operation */
        }
        ddSingle = tmpSingle;
    }
    return ddSingle.GetBDD();
}


/* for parallel the output blocks: for opt= 2,3,4,5
 * encOutputBlk also deals with cc encoding
 * encOutputBlkClass: do cc-encoding and "AND" simutaneously
 * encOutputBlkClassNew: first cc-encoding, then AND with dichEnc, most fast
 * encOutputBlkClassPure: not use dichEnc
 * */
TASK_IMPL_4(vector<sylvanBdd>*, encOutputBlk, myBlock, blk, Bdd, inputBDD, int, start_var, vector<int>*, targetClsVec) {
    vector<sylvanBdd> *bdd_vec = new vector<sylvanBdd>;
    for (int t = 0; t < targetClsVec->size(); ++t) {
        bdd_vec->push_back(sylvan_false);
    }
    int clsSize = targetClsVec->size() - 1;
    for (int t = 0; t < clsSize; ++t) {
        int i = (*targetClsVec)[t];
        bdd_refs_spawn(SPAWN(encOutputBlkClassPure, blk, inputBDD, start_var, i));
    }
    sylvanBDD lst = CALL(encOutputBlkClassPure, blk, inputBDD, start_var, (*targetClsVec)[clsSize]);
    (*bdd_vec)[clsSize] = lst;
    for (int t = clsSize - 1; t >= 0; t--) {
        int i = (*targetClsVec)[t];
        (*bdd_vec)[i] = bdd_refs_sync(SYNC(encOutputBlkClassPure));
    }
    return bdd_vec;
}


//sylvanBdd dichEncoding(vector<sylvanBdd> *bdd_vec, sylvanBdd inputBDD, int start, int end) {
//    if (abs(start - end) == 1) {
//        sylvanBdd pre_half = (*bdd_vec)[start];
//        sylvanBdd pos_half = (*bdd_vec)[end];
//        sylvanBdd res = pre_half * inputBDD * pos_half;
//        return res;
//    } else if (abs(start - end) == 0) {
//        return (*bdd_vec)[start];
//    } else {
//        sylvanBdd pre_half = dichEncoding(bdd_vec, inputBDD, start, (end - start) / 2 + start);
//        sylvanBdd pos_half = dichEncoding(bdd_vec, inputBDD, (end - start) / 2 + start + 1, end);
//        sylvanBdd res = pre_half * pos_half;
//        return res;
//    }
//}