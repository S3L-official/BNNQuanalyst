//
// Created by yedi zhang on 2022/4/23.
//

#include "CNFEncoder.h"

CNFEncoder_CUDD::CNFEncoder_CUDD(BNNetwork *network, Query *query) {
    _network = network;
    _query = query;
    _input_size = network->get_input_size();
    _mgr = new Cudd(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);  // The manager
//    cout << "This is NPAQ-CUDD encoding." << endl;
}

void CNFEncoder_CUDD::parseDimacsFile_CUDD(string dimacsFileName, vector<int> target_class) {
    vector<vector<int>> instance;
    ifstream dimacs_file;
    string line;
    vector<Query> queryList;
    auto *bdd_vec = new vector<BDD>;
    dimacs_file.open(dimacsFileName);
    set<int> input_variables;
    vector<int> other_variables;
    int numOfVars = 0;
    int numOfClauses = 0;
    int low_input_var = 0;
    int high_input_var = 0;
    BDD baseBDD = _mgr->bddOne();
    struct timeb tmb1;
    ftime(&tmb1);
    if (dimacs_file.is_open()) {
        while (getline(dimacs_file, line)) {
            if (line[0] == '#' | line[0] == '\n' | line == "") {
                continue;
            }
            if (line[0] == 'p') {
                vector<string> headElements;
                string token;
                istringstream tokenstream(line);
                while (getline(tokenstream, token, ' ')) {
                    headElements.push_back(token);
                }
                numOfVars = stoi(headElements[2]);
                low_input_var = numOfVars;
            } else if (line[0] == 'c') { // comment lines
                vector<string> commentElements;
                string token;
                istringstream tokenstream(line);
                while (tokenstream >> token) {
                    commentElements.push_back(token);
                }
                if (commentElements[1] == "ind") { // 统计input variables
                    for (int i = 2; i < commentElements.size() - 1; ++i) {
                        int var = stoi(commentElements[i]);
                        input_variables.insert(var);
                        low_input_var = min(low_input_var, var);
                        high_input_var = max(high_input_var, var);
                    }
                } else {
                    continue;
                }
            } else if (line[0] == 'x') {// this is xor lines, now we begin to cnf lines
                string clause(line, 2);
                BDD curBdd = this->encodeXLineCUDD(_mgr, this->getVars_CUDD(clause));
                bdd_vec->push_back(curBdd);
            } else { // this is cnf lines, now we begin to cnf lines
                BDD curBdd = this->encodeCNFLineCUDD(_mgr, this->getVars_CUDD(line));
                bdd_vec->push_back(curBdd);
            }
        }
        BDD resultBdd = this->encodeCUDD(bdd_vec);
        struct timeb tmb2;
        ftime(&tmb2);
        cout << "CNF2BDD (CUDD) Encoding done. " << ((tmb2.time - tmb1.time) + (tmb2.millitm - tmb1.millitm) / 1000.0)
             << " seconds" << endl;
        BDD cubeLow = this->andSetCUDD(_mgr, 1, low_input_var - 1);
        BDD cubeHigh = this->andSetCUDD(_mgr, high_input_var + 1, numOfVars - target_class.size());
        BDD target_bdds = resultBdd.ExistAbstract(cubeLow * cubeHigh);
        double minterm = target_bdds.CountMinterm(_network->get_input_size() + target_class.size());
        struct timeb tmb3;
        ftime(&tmb3);
        cout << "CNF2BDD (CUDD) Solving done. " << ((tmb3.time - tmb2.time) + (tmb3.millitm - tmb2.millitm) / 1000.0)
             << " seconds" << endl;
        cout << "The minterm is: " << minterm << endl;
        delete bdd_vec;
    } else {
        throw std::runtime_error("Error opening dimacs File!");
    }
}

vector<int> CNFEncoder_CUDD::getVars_CUDD(string line) {
    vector<int> varsElements;
    string token;
    istringstream tokenstream(line);
    while (getline(tokenstream, token, ' ')) {
        varsElements.push_back(stoi(token));
    }
    return varsElements;
}

BDD CNFEncoder_CUDD::encodeCNFLineCUDD(Cudd *mgr, vector<int> var_vec) {
    BDD base = mgr->bddZero();
    for (int i = 0; i < var_vec.size(); ++i) {
        int val = var_vec[i];
        if (val < 0) {
            base = base + (!(mgr->bddVar(-val)));
        } else if (val == 0) {
            break;
        } else {
            base = base + mgr->bddVar(val);
        }
    }
    return base;
}

BDD CNFEncoder_CUDD::encodeCUDD(vector<BDD> *bdd_vec) {
    BDD base = (*bdd_vec)[0];
    for (int i = 1; i < bdd_vec->size(); ++i) {
//        cout << "This is " << i << "'th bdd." << endl;
        base = base * (*bdd_vec)[i];
    }
    return base;
}

BDD CNFEncoder_CUDD::encodeXLineCUDD(Cudd *mgr, vector<int> var_vec) {
    BDD base;
    int val = var_vec[0];
    assert(val != 0);
    if (val < 0) {
        base = !(mgr->bddVar(-val));
    } else {
        base = mgr->bddVar(val);
    }

    for (int i = 1; i < var_vec.size(); ++i) {
        int val = var_vec[i];
        if (val == 0) {
            break;
        } else if (val < 0) {
            base = base.Xor(!(mgr->bddVar(-val)));
        } else {
            base = base.Xor(mgr->bddVar(val));
        }
    }
    return base;
}

BDD CNFEncoder_CUDD::encodeDichCUDD(vector<BDD> *bdd_vec, int start, int end) {
//    return this->getMgr()->bddOne();
    if (abs(start - end) == 1) {
        BDD pre_half = (*bdd_vec)[start];
        BDD pos_half = (*bdd_vec)[end];
        BDD res = pre_half * pos_half;
        return res;
    } else if (abs(start - end) == 0) {
        return (*bdd_vec)[start];
    } else {
        BDD pre_half = this->encodeDichCUDD(bdd_vec, start, (end - start) / 2 + start);
        BDD pos_half = this->encodeDichCUDD(bdd_vec, (end - start) / 2 + start + 1, end);
        BDD res = pre_half * pos_half;
        return res;
    }
}

BDD CNFEncoder_CUDD::andSetCUDD(Cudd *mgr, int start, int end) {
    BDD result;
    if (abs(start - end) == 1) {
        BDD pre_half = mgr->bddVar(start);
        BDD pos_half = mgr->bddVar(end);
        result = pre_half * pos_half;
    } else if (abs(start - end) == 0) {
        result = mgr->bddVar(start);
    } else {
        BDD pre_half = this->andSetCUDD(mgr, start, (end - start) / 2 + start);
        BDD pos_half = this->andSetCUDD(mgr, (end - start) / 2 + start + 1, end);
        result = pre_half * pos_half;
    }
    return result;
}

void CNFEncoder_CUDD::quit() {
    Cudd_Quit(this->_mgr->getManager());
}