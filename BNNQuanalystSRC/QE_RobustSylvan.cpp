//
// Created by yedi zhang on 2020/12/29.
//
//#include <time.h>
//#include <sys/timeb.h>
#include "../include/QueryEngine.h"
#include "../include/BNNetwork.h"
#include "../include/Encoder_Sylvan.h"
#include "math.h"

//typedef sylvan::BDD sylvanBDD;
using namespace sylvan;

QE_RobustSylvan::QE_RobustSylvan(Query *q, Options *opt, BNNetwork *bnn, string relaPath) : _bnn() {
    _query = q;
    _opt = opt;
    _bnn = bnn;
    _result = new vector<double>;
    _time = new vector<double>;
}

QE_RobustSylvan::QE_RobustSylvan() : _bnn() {

}

void QE_RobustSylvan::test(bool ifITE) {
    Encoder_Sylvan encoder(_bnn, _query, _opt->_IP, _opt->_DC, _opt->_level);
    encoder.test(ifITE);
    delete _result;
    delete _time;
}

int QE_RobustSylvan::forwardPredict(vector<int> input) {
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

//    for (int i = 0; i < outputSize; ++i) {
    for (int j = 0; j < blk_output._output_size; ++j) {
        float a = 0.0;
        for (int k = 0; k < blk_output._input_size; ++k) {
            a = a + i_lin_weight[j][k] * cur_input[k];
        }
        float rr = a + i_lin_bias[0][j];
        results.push_back(rr);
    }
//    }
    vector<float>::iterator biggest = max_element(begin(results), end(results));
    int cls = distance(begin(results), biggest);
    return cls;
}

void QE_RobustSylvan::encode2DD() {
    Encoder_Sylvan encoder(_bnn, _query, _opt->_IP, _opt->_DC, _opt->_level);
    struct timeb tmb;
    ftime(&tmb);
    if (_query->_propertyID == "target_robustness") {
        _BddVec = encoder.encode2targetBDD(_query->_target_class);
    } else {
        cout << "Wrong property type! Here should be a target_robustness query." << endl;
        exit(0);
    }

    struct timeb tmb1;
    ftime(&tmb1);

    cout << endl << "Encoding done. "
         << ((tmb1.time - tmb.time) + (tmb1.millitm - tmb.millitm) / 1000.0) << " seconds"
         << endl << endl;
    _encodingTime = ((tmb1.time - tmb.time) + (tmb1.millitm - tmb.millitm) / 1000.0);

    double partInternalBefore = ((encoder.inter_begin.time - tmb.time) +
                                 (encoder.inter_begin.millitm - tmb.millitm) / 1000.0);
    cout << "Time used for Part-A (Before encoding internal blocks): " << partInternalBefore
         << " seconds. Accounting for "
         << 100 * partInternalBefore / _encodingTime << "%." << endl;


    double partInternal = ((encoder.inter_end.time - encoder.inter_begin.time) +
                           (encoder.inter_end.millitm - encoder.inter_begin.millitm) / 1000.0);
    double partOutputBlock = ((encoder.out_end.time - encoder.out_begin.time) +
                              (encoder.out_end.millitm - encoder.out_begin.millitm) / 1000.0);
    if (_opt->_level == 4) {
        partInternal = partInternal - partOutputBlock;
    }

    cout << "Time used for Part-B (Encoding all the internal blocks): " << partInternal << " seconds. Accounting for "
         << 100 * partInternal / _encodingTime << "%." << endl;

    cout << "Time used for Part-C (Encoding the output block): " << partOutputBlock << " seconds. Accounting for "
         << 100 * partOutputBlock / _encodingTime << "%." << endl;


    double partIntegration = ((tmb1.time - encoder.integration_begin.time) +
                              (tmb1.millitm - encoder.integration_begin.millitm) / 1000.0);
    cout << "Time used for Part-D (ExistAbstract): " << partIntegration << " seconds. Accounting for "
         << 100 * partIntegration / _encodingTime << "%." << endl;

    _time->push_back(_encodingTime);
    _time->push_back(partInternalBefore);
    _time->push_back(partInternal);
    _time->push_back(partOutputBlock);
    _time->push_back(partIntegration);
}

void QE_RobustSylvan::set_benign_class(int _ins) {
    _benign_class = _ins;
    _if_set_binign = true;
}

void QE_RobustSylvan::solve() {
    struct timeb tmb;
    ftime(&tmb);
    if (_query->_propertyID == "target_robustness") {
        if (_query->_ifHD) {
            int cls = forwardPredict(_query->_instance);
            cout << "The predicted class is: " << cls << endl;
            double predictMinterm = (*_BddVec)[cls].SatCount(_bnn->get_input_size());
            int ins_size = _bnn->get_input_size();
            int dis = _query->_distance;
            long long standard = 0;
            for (int i = 0; i <= dis; ++i) {
                standard += combo(ins_size, i);
            }
            long long advNum = standard - predictMinterm;
            struct timeb tmb1;
            ftime(&tmb1);
            _solvingTime = (double) ((tmb1.time - tmb.time) + (tmb1.millitm - tmb.millitm) / 1000.0);
            cout << "Solve done. " << _solvingTime << " seconds" << endl;
            cout << "The number of adversarial examples is: " << advNum << endl;
            cout << "******** Here shows all the classes ********" << endl;
            bool ifHasOne = false;
            long long nodeSum = 0;
            for (int i = 0; i < _BddVec->size(); ++i) { /* compute adv numbers for each class */
                double target_result = (*_BddVec)[i].SatCount(_bnn->get_input_size());
                _result->push_back(target_result);
                long long nodeNum = (*_BddVec)[i].NodeCount();
                if (nodeNum == 1) {
                    ifHasOne = true;
                } else {
                    nodeSum = nodeSum + nodeNum;
                }
                printf("For target class %d: nodes: %ld | minterms: %lf \n", i, nodeNum, target_result);
            }
            struct timeb tmb2;
            ftime(&tmb2);
            double computeAll = (double) ((tmb2.time - tmb1.time) + (tmb2.millitm - tmb1.millitm) / 1000.0);
            cout << "Compute done. " << computeAll << " seconds | Total nodes: "
                 << (ifHasOne ? nodeSum + 1 : nodeSum) << endl;
        } else {
            struct timeb tmb1;
            ftime(&tmb1);
            cout << "******** Here shows all the classes ********" << endl;
            bool ifHasOne = false;
            long long nodeSum = 0;
            for (int i = 0; i < _BddVec->size(); ++i) { /* compute adv numbers for each class */
                double target_result = (*_BddVec)[i].SatCount(_bnn->get_input_size());
                _result->push_back(target_result);
                long long nodeNum = (*_BddVec)[i].NodeCount();
                if (nodeNum == 1) {
                    ifHasOne = true;
                } else {
                    nodeSum = nodeSum + nodeNum;
                }
                printf("For target class %d: nodes: %ld | minterms: %lf \n", i, nodeNum, target_result);
            }
            struct timeb tmb2;
            ftime(&tmb2);
            double computeAll = (double) ((tmb2.time - tmb1.time) + (tmb2.millitm - tmb1.millitm) / 1000.0);
            cout << "Compute done. " << computeAll << " seconds | Total nodes: "
                 << (ifHasOne ? nodeSum + 1 : nodeSum) << endl;
        }
    } else {
        cout << "Wrong query type!" << endl;
        exit(0);
    }
}

void QE_RobustSylvan::printVerificationResult() {
    if (_query->_propertyID == "target_robustness") {
        cout << "the counter-example's number is: " << (*_result)[0] << endl;
    } else if (_query->_propertyID == "local_robustness") {
        exit(0);
    }
}

/*
 * write and print for debug usage
 */
void QE_RobustSylvan::writeDD2File() {
//    int input_size = _bnn->get_input_size();
//    const char *relaPathNew = "../outputGraph/CUDD/graphFinal_CUDD_%d.dot";
//    write_ten_adds(_gbm, _addVec, input_size, relaPathNew);
}

/*
 * store log for script_running
 */
void QE_RobustSylvan::logPrint(char *filename) {
    int input_size = _bnn->get_input_size();
    FILE *outfile;
    outfile = fopen(filename, "w+");
    fprintf(outfile,
            "Encoding Time: %lf\n", _encodingTime); /*Reports the number of live nodes in BDDs and ADDs*/
    fprintf(outfile,
            "Solving Time: %lf\n", _solvingTime); /*Reports the number of live nodes in BDDs and ADDs*/
    fprintf(outfile,
            "All Time: %lf\n", _encodingTime + _solvingTime); /*Reports the number of live nodes in BDDs and ADDs*/
    if (_opt->_modeSingleOutput == 1) {
        cout << "currently not support for encoding into a single ADD";
        exit(0);
    } else {
        for (int i = 0; i < _BddVec->size(); ++i) {
            if (_query->_propertyID == "target_robustness") {
                fprintf(outfile, "Output for class %d: ", _query->_target_class[i]);
            } else if (_query->_propertyID == "local_robustness") {
                fprintf(outfile, "Output for class %d: ", i);
            }
            sylvan::Bdd bdd = (*_BddVec)[i];
            int node = bdd.NodeCount();
            double mint = bdd.SatCount(_bnn->get_input_size());
            fprintf(outfile, "%d nodes %d leaves %lf minterms \n", node, node == 1 ? 1 : 2, mint);
        }
    }

    fprintf(outfile, "\n\n");
    fprintf(outfile, "Time usd for Encoding (in total): %lf minterms \n", (*_time)[0]);
    fprintf(outfile, "Time for Part-A (Before encoding internal blocks): %lf minterms \n", (*_time)[1]);
    fprintf(outfile, "Time for Part-B (Encoding all the internal blocks): %lf minterms \n", (*_time)[2]);
    fprintf(outfile, "Time for Part-C (Encoding the output block): %lf minterms \n", (*_time)[3]);
    fprintf(outfile, "Time for Part-D (ExistAbstract): %lf minterms \n\n\n", (*_time)[4]);
    fprintf(outfile, "Time for Solving: %lf minterms \n", _solvingTime);
    fclose(outfile); // close the file */
}

void QE_RobustSylvan::quit() {
    delete _BddVec;
    delete _result;
    delete _time;
    //delete or release encoder
}
