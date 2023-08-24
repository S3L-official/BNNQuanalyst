//
// Created by yedi zhang on 2020/11/9.
//

#include "../include/QueryEngine.h"
#include "../include/CNFEncoder.h"
#include <time.h>
#include <sys/timeb.h>
#include "../include/Utilities.h"
#include "signal.h"
#include "sylvan_obj.hpp"
#include "sylvan.h"
#include "../include/UtilParser.h"
#include <sys/wait.h>
#include "unistd.h"

extern string relaPath;
using namespace sylvan;

QueryEngine::QueryEngine(Options *opts, string relaPath) {
    string queryFileName = opts->_queryFileName;
    _queries = parseQueryFile(relaPath + "benchmarks/queries/" + queryFileName);
    _engine = opts->_engineType;
    _opts = opts;
}

bool QueryEngine::checkOneQuery(Query query) {
    bool result = false;
    BNNetwork bnn = BNNetwork(query._modelName1, relaPath + "benchmarks/models/");
    switch (_engine) {
        case CUDD_NPAQ: {
            if (query._propertyID == "target_robustness") {
                QE_Robust QE_Robust(&query, _opts, &bnn, relaPath + "models/");
                string instanceName = query._instance_name;
                string digit = instanceName.substr(instanceName.size() - 1, instanceName.size());
                cout << "******************* Now we begin to encode (CUDD) *******************" << endl;
                QE_Robust.encode2DD();
                cout << "*********************** Encode Finished **********************" << endl;
                cout << "\n";
                cout << "******************** Now we begin to transfer into CNF format *******************" << endl;
                QE_Robust.transfer2CNF();
                cout << "************************* Transfer Finished *********************" << endl << endl << endl;
                QE_Robust.quit();
            } else {
                cout << "Currently only support target_robustness with cudd-npaq query engine!" << endl;
            }
            break;
        }
        case CUDD: {
            if (query._propertyID == "target_robustness") {
                QE_Robust QE_Robust(&query, _opts, &bnn, relaPath + "models/");
                string instanceName = query._instance_name;
                string digit = instanceName.substr(instanceName.size() - 1, instanceName.size());
                cout << "******************* Now we begin to encode (CUDD) *******************" << endl;
                QE_Robust.encode2DD();
                cout << "*********************** Encode Finished **********************" << endl;
                cout << "\n";

                cout << "******************** Now we begin to solve *******************" << endl;
                QE_Robust.solve();
                cout << "************************* Solve Finished *********************" << endl << endl << endl;

                QE_Robust.quit();
                break;
            } else if (query._propertyID == "compute_SD") {
                QE_SDComputer cptr(&query, _opts, &bnn, relaPath + "models/");
                cout << "******************** Now we begin to solve Maximal Distance (CUDD) *******************"
                     << endl;
                cout << "\n";
                cptr.solve();
                cout << "************************* Solve Finished *********************" << endl;
                cout << "\n\n";
            } else if (query._propertyID == "PI_explain") {
                QE_Interpreter interpreter(&query, _opts, &bnn, relaPath + "models/");
                vector<bool> ifSucc = interpreter.solve();
                string dis = (query._robustness_judge == "hamming") ? "_HD_0" + to_string(query._distance) : "_FS";
                string log_name =
                        relaPath + "outputs/PI_explain/" + query._modelName1 +
                        "_" +
                        query._instance_name + "_" + query._robustness_judge + "_" +
                        ((query._robustness_judge == "hamming") ? to_string(query._distance)
                                                                : query._fixSet_name)
                        + "_to_" +
                        (query._target_class_name) + "_PI_explain_";
                if (query._PI_num != 0) {
                    int real_index = 0;
                    for (int i = 0; i < interpreter._ADDVec->size(); ++i) {
                        if (!ifSucc[i]) {
                            continue;
                        }
                        for (int j = 0; j < (*interpreter._exp_vec_vec)[real_index]->size(); ++j) {
                            string log_name_new =
                                    log_name + "Target_" + to_string(query._target_class[i]) + "_PI_" +
                                    to_string(j + 1) +
                                    ".txt";
                            char *cstr = new char[log_name_new.length() + 1];
                            strcpy(cstr, log_name_new.c_str());
                            FILE *outfile;
                            outfile = fopen(cstr, "w+");
//                            for (int q = 0; q < Cudd_ReadSize(interpreter._gbm); q++) {
                            for (int q = 0; q < query._instance.size(); q++) {
                                switch ((*(*interpreter._exp_vec_vec)[real_index])[j][q]) {
                                    case 0:
                                        fprintf(outfile, "%d ", -q);
                                        break;
                                    case 1:
                                        fprintf(outfile, "%d ", q);
                                        break;
                                    case 2:
                                        break;
                                    default:
                                        cout << "???" << endl;
                                }
                            }
                            delete[] cstr;
                        }
                        real_index++;
                    }
                    cout << "We have written the explanations into the folder: ./outputs/PI_explain." << endl;
                } else {
                    cout << "Currently, we cannot find a prime implicant explanation." << endl;
                }

                interpreter.quit();
                return result;
            } else if (query._propertyID == "EF_explain") {
                QE_Interpreter interpreter(&query, _opts, &bnn, relaPath + "models/");
                vector<bool> ifSucc = interpreter.solve();
                string log_name =
                        relaPath + "outputs/EF_explain/" + query._modelName1 + "_" +
                        query._instance_name + "_" + query._robustness_judge + "_" +
                        ((query._robustness_judge == "hamming") ? to_string(query._distance)
                                                                : query._fixSet_name)
                        + "_to_target_" +
                        (query._target_class_name) + "_EF_explain_";
                int real_index = 0;
                for (int i = 0; i < interpreter._ADDVec->size(); ++i) {
                    if (!ifSucc[i]) {
                        continue;
                    }
                    string log_name_new = log_name + "Target_" + to_string(query._target_class[i]) + ".txt";
                    char *cstr = new char[log_name_new.length() + 1];
                    strcpy(cstr, log_name_new.c_str());
                    FILE *outfile;
                    outfile = fopen(cstr, "w+");
                    for (int q = 0; q < Cudd_ReadSize(interpreter._gbm); q++) {
                        switch ((*(*interpreter._exp_vec_vec)[real_index])[0][q]) {
                            case 0:
                                fprintf(outfile, "%d ", -q);
                                break;
                            case 1:
                                fprintf(outfile, "%d ", q);
                                break;
                            case 2:
                                break;
                            default:
                                cout << "???" << endl;
                        }
                    }
                    real_index++;
                    delete[] cstr;
                }

                cout << "We have written the explanations into the folder: ./outputs/EF_explain." << endl;
                interpreter.quit();
                return result;
            } else {
                cout << "We currently only check target_robustness, PI_explain, EF_explain queries!" << endl;
                exit(0);
            }
            break;
        }
        case SYLVAN: {
            if (query._propertyID == "target_robustness") {
                QE_RobustSylvan QE_RobustSylvan(&query, _opts, &bnn, relaPath + "models/");
                string instanceName = query._instance_name;
                string digit = instanceName.substr(instanceName.size() - 1, instanceName.size());
                cout << "******************* Now we begin to encode (Sylvan) *******************" << endl;
                int n_workers = _opts->_worker;
                size_t deque_size = 0;
                // Initialize the Lace framework for <n_workers> workers.
                lace_start(n_workers, deque_size);
                // user can pre-define different table size for 'sylvan_set_sizes()'
                sylvan_set_sizes(1LL << 22, 1LL << 30, 1LL << 18, 1LL << 27);
//                sylvan_set_sizes(1LL << 22, 1LL << 33, 1LL << 18, 1LL << 30); //28: 268435456
                sylvan_init_package();
                sylvan_init_bdd();
                QE_RobustSylvan.encode2DD();
                cout << "*********************** Encode Finished **********************" << endl;
                cout << "\n\n";
                cout << "******************** Now we begin to solve *******************" << endl;
                QE_RobustSylvan.solve();
                cout << "************************* Solve Finished *********************" << endl;
                cout << "\n\n";
                QE_RobustSylvan.quit();
                sylvan_quit();
                lace_stop();
                break;
            } else if (query._propertyID == "compute_SD") {
                QE_SDComputerSylvan cptr(&query, _opts, &bnn, relaPath + "models/");
                cout << "******************** Now we begin to solve Maximal Distance (Sylvan) *******************"
                     << endl;
                cout << "\n\n";
                int n_workers = _opts->_worker; // automatically detect number of workers
                size_t deque_size = 0; // default value for the size of task deques for the workers
                // Initialize the Lace framework for <n_workers> workers.
                lace_start(n_workers, deque_size);
                sylvan_set_sizes(1LL << 22, 1LL << 30, 1LL << 18, 1LL << 27);
//                sylvan_set_sizes(1LL << 22, 1LL << 33, 1LL << 18, 1LL << 30); //28: 268435456
                sylvan_init_package();
                sylvan_init_bdd();
                cptr.solve();
                cout << "************************* Solve Finished *********************" << endl;
                cout << "\n\n";
                cptr.quit();
                return result;
            } else {
                cout << "Currently only support target_robustness and compute_SD queries with Sylvan package!" << endl;
            }
            break;
        }
        case NPAQ_CUDD: {
            if (query._propertyID == "target_robustness") {
                cout << "******************* Now we begin to encode (NPAQ-CUDD) *******************" << endl;
                vector<string> resVec;
                stringstream input(query._modelName1);
                string temp;
                while (getline(input, temp, '_')) {
                    resVec.push_back(temp);
                }
                int numOfBlk = stoi(resVec[2]);
                string arch = to_string(numOfBlk) + "blk";
                for (int i = 0; i < numOfBlk; ++i) {
                    arch = arch + "_" + resVec[5 + i];
                }
                int HD = query._distance;
                string instance_name = query._instance_name;
                vector<string> resVec2;
                stringstream input2(instance_name);
                string temp2;
                while (getline(input2, temp2, '_')) {
                    resVec2.push_back(temp2);
                }
                string dimacs_name = relaPath + "benchmarks/CNFSets_NPAQ/card-" + bnn._dataSet + "-" +
                                     to_string(bnn._blkList[0]._input_size) + "-bnn_" + arch + "-robustness-perturb_" +
                                     to_string(HD) + "-id_" + resVec2[1] + ".dimacs";
                cout << "the dimacs_name is " << dimacs_name << endl;
                CNFEncoder_CUDD cnfEncoder(&bnn, &query);
                cnfEncoder.parseDimacsFile_CUDD(dimacs_name, query._target_class);
                cout << "Now we finished the CNF2BDD!" << endl;
                cnfEncoder.quit();
            } else {
                cout << "Currently only support target_robustness with npaq-cudd query engine!" << endl;
            }
            break;
        }
        case NPAQ_SYLVAN: {
            if (query._propertyID == "target_robustness") {
                cout << "******************* Now we begin to encode (NPAQ-SYLVAN) *******************" << endl;
                vector<string> resVec;
                stringstream input(query._modelName1);
                string temp;
                while (getline(input, temp, '_')) {
                    resVec.push_back(temp);
                }
                int numOfBlk = stoi(resVec[2]);
                string arch = to_string(numOfBlk) + "blk";
                for (int i = 0; i < numOfBlk; ++i) {
                    arch = arch + "_" + resVec[5 + i];
                }
                int HD = query._distance;
                string instance_name = query._instance_name;
                vector<string> resVec2;
                stringstream input2(instance_name);
                string temp2;
                while (getline(input2, temp2, '_')) {
                    resVec2.push_back(temp2);
                }
                string dimacs_name = relaPath + "benchmarks/CNFSets_NPAQ/card-" + bnn._dataSet + "-" +
                                     to_string(bnn._blkList[0]._input_size) + "-bnn_" + arch + "-robustness-perturb_" +
                                     to_string(HD) + "-id_" + resVec2[1] + ".dimacs";
                cout << "the dimacs_name is " << dimacs_name << endl;
                CNFEncoder_Sylvan cnfEncoderSylvan(&bnn, &query, _opts->_worker);
                cnfEncoderSylvan.parseDimacsFile_Sylvan(dimacs_name, query._target_class);
                cout << "Now we finished the CNF2BDD!" << endl;
                cnfEncoderSylvan.quit();
            } else {
                cout << "Currently only support target_robustness with npaq-sylvan query engine!" << endl;
            }
            break;
        }
        default:
            result = true;
    }
    return result;
}

