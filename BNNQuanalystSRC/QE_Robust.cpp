//
// Created by yedi zhang on 2020/12/29.
//
//#include <time.h>
//#include <sys/timeb.h>
#include "../include/QueryEngine.h"
#include "../include/BNNetwork.h"
#include "../include/Encoder_CUDD.h"
#include "dddmp.h"
#include "dddmpInt.h"
#include "math.h"

extern string relaPath;

QE_Robust::QE_Robust(Query *q, Options *opt, BNNetwork *bnn, string relaPath) : _bnn() {
    _query = q;
    _opt = opt;
    _bnn = bnn;
    _result = new vector<double>;
    _time = new vector<double>;
}

QE_Robust::QE_Robust() : _bnn() {

}

void QE_Robust::test(bool ifITE) {
    Encoder_CUDD encoder(_bnn, _query, _opt->_IP, _opt->_DC);
    encoder.testITE(ifITE);
    delete _result;
    delete _time;
}

int QE_Robust::forwardPredict(vector<int> input) {
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

    for (int j = 0; j < blk_output._output_size; ++j) {
        float a = 0.0;
        for (int k = 0; k < blk_output._input_size; ++k) {
            a = a + i_lin_weight[j][k] * cur_input[k];
        }
        float rr = a + i_lin_bias[0][j];
        results.push_back(rr);
    }
    vector<float>::iterator biggest = max_element(begin(results), end(results));
    int cls = distance(begin(results), biggest);
    return cls;
}

void QE_Robust::encode2DD() {
    Encoder_CUDD encoder(_bnn, _query, _opt->_IP, _opt->_DC);
    _gbm = encoder._mgr->getManager();
    struct timeb tmb;
    ftime(&tmb);

    if (_query->_propertyID == "target_robustness") {
        _BDDVec = encoder.encode2targetBDD(_query->_target_class);
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
    cout << "Time used for Part-B (Encoding all the internal blocks): " << partInternal << " seconds. Accounting for "
         << 100 * partInternal / _encodingTime << "%." << endl;

    double partOutputBlock = ((encoder.out_end.time - encoder.inter_end.time) +
                              (encoder.out_end.millitm - encoder.inter_end.millitm) / 1000.0);
    cout << "Time used for Part-C (Encoding the output block): " << partOutputBlock << " seconds. Accounting for "
         << 100 * partOutputBlock / _encodingTime << "%." << endl;

    double partIntegration = ((tmb1.time - encoder.out_end.time) +
                              (tmb1.millitm - encoder.out_end.millitm) / 1000.0);
    cout << "Time used for Part-D (ExistAbstract): " << partIntegration << " seconds. Accounting for "
         << 100 * partIntegration / _encodingTime << "%." << endl;

    _time->push_back(_encodingTime);
    _time->push_back(partInternalBefore);
    _time->push_back(partInternal);
    _time->push_back(partOutputBlock);
    _time->push_back(partIntegration);
}

void QE_Robust::transfer2CNF() {
    struct timeb tmb1;
    ftime(&tmb1);
    int cls = forwardPredict(_query->_instance);
    cout << "Note that: we encode the CNF which indicates the correct classification result." << endl;
    for (int i = 0; i < _BDDVec->size(); ++i) {
        if (i != cls) {
            continue;
        }
        string insName = _query->_instance_name;
        string modelName = _query->_modelName1;
        int dis = _query->_distance;
        string pathStr =
                relaPath + "outputs/CNFSets_CUDD/" + modelName + "_" + insName + "_HD_" + to_string(dis) + ".cnf";
        cout << "CNF outputPath: " << pathStr << endl;
        int clauses, news;
        FILE *outCnfFile;
        char *fileName = new char[pathStr.length() + 1];
        strcpy(fileName, pathStr.c_str());
        outCnfFile = fopen(fileName, "w+");
        Dddmp_cuddBddStoreCnf(_gbm, (*_BDDVec)[i].getNode(), DDDMP_CNF_MODE_MAXTERM, 0, NULL, NULL, NULL, NULL,
                              Cudd_ReadNodeCount(_gbm), -1,
                              -1, fileName, outCnfFile, &clauses,
                              &news);
    }
    struct timeb tmb2;
    ftime(&tmb2);
    float _transferTime = (double) ((tmb2.time - tmb1.time) + (tmb2.millitm - tmb1.millitm) / 1000.0);
    cout << "Transfer to CNF file done. Cost " << _transferTime << " seconds." << endl;
    cout << "We have written the transformed CNF files into the folder: ./outputs/CNFSets_CUDD." << endl;
    cout << "For model counting, please use ApproxMC embedded in NPAQ." << endl;
}

void QE_Robust::setMgr(DdManager *mgr) {
    _gbm = mgr;
}

void QE_Robust::solve() {
    _if_BDD2ADD = true;
    struct timeb tmb;
    ftime(&tmb);
    _allAdv = 0;
    if (_opt->_modeSingleOutput == 1 || _query->_propertyID != "target_robustness") {
        cout << "Wrong query or wrong verification options" << endl;
        exit(0);
    } else {
        if (_query->_ifHD) {
            int cls = forwardPredict(_query->_instance);
            cout << "The predicted class is: " << cls << endl;
            _ADDVec = new vector<DdNode *>;
            double predictMinterm = (*_BDDVec)[cls].CountMinterm(_bnn->get_input_size());
            int ins_size = _bnn->get_input_size();
            int dis = _query->_distance;
            long long standard = 0;
            for (int i = 0; i <= dis; ++i) {
                standard += combo(ins_size, i);
            }
            long long advNum = standard - predictMinterm;
            struct timeb tmb1;
            ftime(&tmb1);
            _solvingTIme = (double) ((tmb1.time - tmb.time) + (tmb1.millitm - tmb.millitm) / 1000.0);
            cout << "Solve done. " << _solvingTIme << " seconds" << endl;
            cout << "The number of adversarial examples is: " << advNum << endl;
            cout << "******** Here shows all the classes ********" << endl;
            cout << "The input size is: " << _bnn->get_input_size() << endl;
            bool ifHasOne = false;
            long long nodeSum = 0;
            double computeAll = 0;
            for (int i = 0; i < _BDDVec->size(); ++i) {
                struct timeb tmb2;
                ftime(&tmb2);
                double target_result = (*_BDDVec)[i].CountMinterm(_bnn->get_input_size());
                struct timeb tmb3;
                ftime(&tmb3);
                double computeOne = (double) ((tmb3.time - tmb2.time) + (tmb3.millitm - tmb2.millitm) / 1000.0);
                computeAll = computeAll + computeOne;
                _result->push_back(target_result);
                long long nodeNum = (*_BDDVec)[i].nodeCount();
                if (nodeNum == 1) {
                    ifHasOne = true;
                } else {
                    nodeSum = nodeSum + nodeNum;
                }
                printf("For target class %d: nodes: %ld | minterms: %lf \n", i, nodeNum, target_result);
                DdNode *add = Cudd_BddToAdd(_gbm, (*_BDDVec)[i].getNode());
                Cudd_Ref(add);

                _ADDVec->push_back(add);
            }
            cout << "Compute done. " << computeAll << " seconds | Total nodes: "
                 << (ifHasOne ? nodeSum + 1 : nodeSum) << endl;
            delete _BDDVec;
        } else {
            _ADDVec = new vector<DdNode *>;
            struct timeb tmb1;
            ftime(&tmb1);
            cout << "******** Here shows all the classes ********" << endl;
            bool ifHasOne = false;
            long long nodeSum = 0;
            for (int i = 0; i < _BDDVec->size(); ++i) {
                double target_result = (*_BDDVec)[i].CountMinterm(_bnn->get_input_size());
                _result->push_back(target_result);
                long long nodeNum = (*_BDDVec)[i].nodeCount();
                if (nodeNum == 1) {
                    ifHasOne = true;
                } else {
                    nodeSum = nodeSum + nodeNum;
                }
                printf("For target class %d: nodes: %ld | minterms: ", i, nodeNum);
                cout << target_result << endl;
                DdNode *add = Cudd_BddToAdd(_gbm, (*_BDDVec)[i].getNode());
                Cudd_Ref(add);
                _ADDVec->push_back(add);
            }
            struct timeb tmb2;
            ftime(&tmb2);
            double computeAll = (double) ((tmb2.time - tmb1.time) + (tmb2.millitm - tmb1.millitm) / 1000.0);
            cout << "Compute done. " << computeAll << " seconds | Total nodes: "
                 << (ifHasOne ? nodeSum + 1 : nodeSum) << endl;
            delete _BDDVec;
        }
    }
}

void QE_Robust::printVerificationResult() {
    if (_query->_propertyID == "target_robustness") {
        cout << "the counter-example's number is: " << (*_result)[0] << endl;
    } else if (_query->_propertyID == "local_robustness") {
        if (_opt->_modeSingleOutput == 1) {
            cout << "Not support verifiying based on a single output BDD currently!" << endl;
        } else {
            for (int i = 0; i < _ADDVec->size(); ++i) {
                cout << "the counter-example's number for class " << i << " is: " << (*_result)[i] << endl;
            }
        }
    }
}

/*
 * write and print for debug usage
 */
void QE_Robust::writeDD2File() {
    int input_size = _bnn->get_input_size();
    const char *relaPathNew = "../outputGraph/CUDD/graphFinal_CUDD_%d.dot";
    write_ten_adds(_gbm, _ADDVec, input_size, relaPathNew);
}

/*
 * store log for script_running
 */
void QE_Robust::logPrint(char *filename) {
    int input_size = _bnn->get_input_size();
    FILE *outfile;
    outfile = fopen(filename, "w+");
    fprintf(outfile,
            "Encoding Time: %lf\n", _encodingTime); /*Reports the number of live nodes in BDDs and ADDs*/
    fprintf(outfile,
            "Solving Time: %lf\n", _solvingTIme); /*Reports the number of live nodes in BDDs and ADDs*/
    fprintf(outfile,
            "All Time: %lf\n", _encodingTime + _solvingTIme); /*Reports the number of live nodes in BDDs and ADDs*/
    if (_query->_propertyID == "local_robustness") {
        fprintf(outfile,
                "The number of all Advs: %lf\n", _allAdv); /*Reports the number of live nodes in BDDs and ADDs*/
    }
    fprintf(outfile,
            "DdManager nodes: %ld | ",
            Cudd_ReadNodeCount(_gbm)); /*Reports the number of live nodes in BDDs and ADDs*/
    fprintf(outfile, "DdManager vars: %d | ",
            Cudd_ReadSize(_gbm)); /*Returns the number of BDD variables in existence*/
    fprintf(outfile, "DdManager memory: %ld\n",
            Cudd_ReadMemoryInUse(_gbm)); /*Returns the memory in use by the manager measured in bytes*/
    if (_opt->_modeSingleOutput == 1) {
        cout << "currently not support for encoding into a single ADD";
        exit(0);
    } else if (_if_BDD2ADD) {
        for (int i = 0; i < _ADDVec->size(); ++i) {
            if (_query->_propertyID == "target_robustness") {
                fprintf(outfile, "Output for class %d", _query->_target_class[i]);
            } else if (_query->_propertyID == "local_robustness") {
                fprintf(outfile, "Output for class %d", i);
            }
            Cudd_PrintDebug_Modified(_gbm, (*_ADDVec)[i], input_size, 1, outfile);
        }
    }

    fprintf(outfile, "\n\n");

    fprintf(outfile, "Time usd for Encoding (in total): %lf minterms \n", (*_time)[0]);
    fprintf(outfile, "Time for Part-A (Before encoding internal blocks): %lf minterms \n", (*_time)[1]);
    fprintf(outfile, "Time for Part-B (Encoding all the internal blocks): %lf minterms \n", (*_time)[2]);
    fprintf(outfile, "Time for Part-C (Encoding the output block): %lf minterms \n", (*_time)[3]);
    fprintf(outfile, "Time for Part-D (ExistAbstract): %lf minterms \n\n\n", (*_time)[4]);
    fprintf(outfile, "Time for Solving: %lf minterms \n\n\n", _solvingTIme);

    fprintf(outfile, "\n\n***************** Output information of manager *****************\n");
    Cudd_PrintInfo(_gbm, outfile);
    fclose(outfile); // close the file */
}

void QE_Robust::quit() {
    if (!_if_BDD2ADD) {
        delete _BDDVec;
    } else {
        for (int i = 0; i < _ADDVec->size(); ++i) {
            Cudd_RecursiveDeref(_gbm, (*_ADDVec)[i]);
        }
        delete _ADDVec;
    }
    Cudd_Quit(_gbm);
}
