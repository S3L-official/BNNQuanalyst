//
// Created by yedi zhang on 2021/1/10.
//

#include "../include/QueryEngine.h"
#include "../include/BNNetwork.h"
#include "../include/Encoder_CUDD.h"
#include "../include/Utilities.h"
#include "math.h"

QE_SDComputer::QE_SDComputer(Query *q, Options *opt, BNNetwork *bnn, string relaPath) : _bnn() {
    _query = q;
    _opt = opt;
    _bnn = bnn;
    _SD = new vector<int>;
}

QE_SDComputer::QE_SDComputer() : _bnn() {

}

int QE_SDComputer::forwardPredict(vector<int> input) {
    vector<int> cur_input;
    vector<float> results;
    vector<BNNBlock> blks = _bnn->getBNNBlock();
    bool ifFirst = true;
    for (int i = 0; i < _bnn->_numOfInterBLK; ++i) {
        if (i == 0) {
            cur_input = input;
        }

        BNNBlock blk_i = blks[i];
        vector<vector<int>> i_lin_weight = blk_i._lin_weight;
        vector<vector<float>> i_lin_bias = blk_i._lin_bias;
        vector<vector<float>> i_bn_weight = blk_i._bn_weight;
        vector<vector<float>> i_bn_bias = blk_i._bn_bias;
        vector<vector<float>> i_bn_mean = blk_i._bn_mean;
        vector<vector<float>> i_bn_var = blk_i._bn_var;
        vector<int> result_vec;

        for (int j = 0; j < blk_i._output_size; ++j) {
            float a = 0.0;
            for (int k = 0; k < blk_i._input_size; ++k) {
                if (ifFirst) {
                    int inst = cur_input[k] > 0 ? 1 : -1;
                    a = a + i_lin_weight[j][k] * inst;
                } else {
                    a = a + i_lin_weight[j][k] * cur_input[k];
                }
            }
            float rr = (a + i_lin_bias[0][j] - i_bn_mean[0][j]) / sqrt(i_bn_var[0][j]) * i_bn_weight[0][j] +
                       i_bn_bias[0][j];
            int rr_sign = rr >= 0 ? 1 : -1;
            result_vec.push_back(rr_sign);
        }
        cur_input = result_vec;
        ifFirst = false;
    }

    int outputSize = _bnn->get_output_size();
    BNNBlock blk_output = blks[blks.size() - 1];
    vector<vector<int>> i_lin_weight = blk_output._lin_weight;
    vector<vector<float>> i_lin_bias = blk_output._lin_bias;

    for (int i = 0; i < outputSize; ++i) {
        for (int j = 0; j < blk_output._output_size; ++j) {
            float a = 0.0;
            for (int k = 0; k < blk_output._input_size; ++k) {
                a = a + i_lin_weight[j][k] * cur_input[k];
            }
            float rr = a + i_lin_bias[0][j];
            results.push_back(rr);
        }
    }
    vector<float>::iterator biggest = max_element(begin(results), end(results));
    int cls = distance(begin(results), biggest);
    return cls;
}

void QE_SDComputer::solve() {
    int input_size = _bnn->get_input_size();
    Encoder_CUDD encoder(_bnn, _query, _opt->_IP, _opt->_DC);
    _gbm = encoder._mgr->getManager();
    struct timeb tmb;
    ftime(&tmb);
    int cls = forwardPredict(_query->_instance);
    cout << "The predict class is: " << cls << endl;
    vector<int> predict_vec;
    predict_vec.push_back(cls);
    _BDDVec = encoder.encode2targetBDD(predict_vec);
    assert(_BDDVec->size() == 1);
    struct timeb tmb1;
    ftime(&tmb1);
    cout << "Encoding done. " << ((tmb1.time - tmb.time) + (tmb1.millitm - tmb.millitm) / 1000.0)
         << " seconds" << endl;
    _encodingTime = ((tmb1.time - tmb.time) + (tmb1.millitm - tmb.millitm) / 1000.0);
    double tole = _query->_tolerance;

    for (int num = 0; num < _BDDVec->size(); ++num) { /* compute SD for each target class */
        double target_result = (*_BDDVec)[num].CountMinterm(input_size);
        int dis = _query->_distance;
        long long standard = 0;
        for (int i = 0; i <= dis; ++i) {
            standard += combo(input_size, i);
        }
        double rightRatio = target_result / standard; /* right classification ratio */
        int SD = _query->_distance;

        if (rightRatio < 1 - tole) /* less then decrease r */{
            bool if_find = false;
            while (SD >= 0) {
                SD = SD - 1;
                standard = 0;
                for (int k = 0; k <= SD; ++k) {
                    standard += combo(input_size, k);
                }
                BDD new_IR = encoder.Cudd_bddCCEncoding(&_query->_instance, input_size - SD, 0, false);
                BDD prod = new_IR * (*_BDDVec)[num];
                target_result = prod.CountMinterm(input_size);

                rightRatio = target_result / standard;
                if (rightRatio >= 1 - tole) {
                    struct timeb tmb2;
                    ftime(&tmb2);
                    cout << "We find a safe distance:" << SD << endl << endl;
                    cout << "Solving done. "
                         << ((tmb2.time - tmb1.time) + (tmb2.millitm - tmb1.millitm) / 1000.0)
                         << " seconds" << endl << endl;
                    _solvingTIme = (double) ((tmb2.time - tmb1.time) + (tmb2.millitm - tmb1.millitm) / 1000.0);
                    _SD->push_back(SD);
                    if_find = true;
                    double allTime = _encodingTime + _solvingTIme;
                    cout << "All Time. " << allTime << " seconds" << endl;
                    break;
                }
            }
            if (!if_find) {
                struct timeb tmb2;
                ftime(&tmb2);
                cout << "No safe distance and wrong classification! Solving done. "
                     << ((tmb2.time - tmb1.time) + (tmb2.millitm - tmb1.millitm) / 1000.0)
                     << " seconds" << endl;
                _solvingTIme = (double) ((tmb2.time - tmb1.time) + (tmb2.millitm - tmb1.millitm) / 1000.0);
                _SD->push_back(-1);
                double allTime = _encodingTime + _solvingTIme;
                cout << "All Time. " << allTime << " seconds" << endl;
                break;
            }
        } else {
            while (SD <= input_size) {
                SD = SD + 1;
                standard += combo(input_size, SD);
                BDD new_IR_super = encoder.Cudd_bddCCEncoding(&_query->_instance, input_size - SD, 0, false);
                BDD new_IR_sub = encoder.Cudd_bddCCEncoding(&_query->_instance, SD, 0, true);
                BDD div = new_IR_super * new_IR_sub;
                encoder.setInputRegion(div);
                vector<BDD> *new_vec_bdd = encoder.encode2targetBDD(_query->_target_class);
                double sup_target_result = (*new_vec_bdd)[num].CountMinterm(input_size);
                rightRatio = (target_result + sup_target_result) / standard;
                target_result = target_result + sup_target_result;
                cout << "################## Current SD is: " << SD << " ##################" << endl;
                if (rightRatio < 1 - tole) {
                    struct timeb tmb2;
                    ftime(&tmb2);
                    cout << "We find a safe distance: " << SD - 1 << endl;
                    cout << "Solving done. " << ((tmb2.time - tmb1.time) + (tmb2.millitm - tmb1.millitm) / 1000.0)
                         << " seconds" << endl << endl;
                    _solvingTIme = (double) ((tmb2.time - tmb1.time) + (tmb2.millitm - tmb1.millitm) / 1000.0);
                    _SD->push_back(SD - 1);
                    double allTime = _encodingTime + _solvingTIme;
                    cout << "All Time. " << allTime << " seconds" << endl;
                    break;
                }
            }
        }
    }
}

/*
 * store log for script_running
 */
void QE_SDComputer::logPrint(char *filename) {
    int input_size = _bnn->get_input_size();
    FILE *outfile;
    outfile = fopen(filename, "w+");
    fprintf(outfile,
            "Encoding Time: %lf\n", _encodingTime); /*Reports the number of live nodes in BDDs and ADDs*/
    fprintf(outfile,
            "Solving Time: %lf\n", _solvingTIme); /*Reports the number of live nodes in BDDs and ADDs*/
    fprintf(outfile,
            "All Time: %lf\n", _encodingTime + _solvingTIme); /*Reports the number of live nodes in BDDs and ADDs*/
    for (int i = 0; i < _query->_target_class.size(); ++i) {
        fprintf(outfile,
                "The largest safe distance for class %d is: %d\n", _query->_target_class[i],
                (*_SD)[i]); /*Reports the number of live nodes in BDDs and ADDs*/
    }
    fprintf(outfile,
            "DdManager nodes: %ld | ", Cudd_ReadNodeCount(_gbm)); /*Reports the number of live nodes in BDDs and ADDs*/
    fprintf(outfile, "DdManager vars: %d | ", Cudd_ReadSize(_gbm)); /*Returns the number of BDD variables in existence*/
    fprintf(outfile, "DdManager memory: %ld\n",
            Cudd_ReadMemoryInUse(_gbm)); /*Returns the memory in use by the manager measured in bytes*/
    fprintf(outfile, "\n\n***************** Output information of manager *****************\n");
    Cudd_PrintInfo(_gbm, outfile);
    fclose(outfile); // close the file */
}

void QE_SDComputer::quit() {
    delete _BDDVec;
    Cudd_Quit(_gbm);
}
