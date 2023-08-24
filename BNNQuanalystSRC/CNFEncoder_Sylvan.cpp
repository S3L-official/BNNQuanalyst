//
// Created by yedi zhang on 2022/4/23.
//

#include "CNFEncoder.h"
#include "sylvan_obj.hpp"
#include "sylvan.h"
#include "UtilLace.h"

using namespace sylvan;

CNFEncoder_Sylvan::CNFEncoder_Sylvan(BNNetwork *network, Query *query, int worker) {
    _network = network;
    _query = query;
    _input_size = network->get_input_size();
    _worker = worker;
    size_t deque_size = 0; // default value for the size of task deques for the workers
    lace_start(_worker, deque_size);
    sylvan_set_sizes(1LL << 22, 1LL << 30, 1LL << 18, 1LL << 27); //28: 268435456
//    sylvan_set_sizes(1LL << 22, 1LL << 33, 1LL << 18, 1LL << 30); //28: 268435456
    sylvan_init_package();
    sylvan_init_bdd();
//    cout << "This is NPAQ-Sylvan encoding." << endl;
}

void CNFEncoder_Sylvan::parseDimacsFile_Sylvan(string dimacsFileName, vector<int> target_class) {
    vector<vector<int>> instance;
    ifstream dimacs_file;
    string line;
    vector<Query> queryList;
    Bdd cube_remain = sylvan_true;
    vector<Bdd> *bdd_vec = new vector<Bdd>;
    dimacs_file.open(dimacsFileName);
    set<int> input_variables;
    vector<int> other_variables;
    int numOfVars = 0;
    int numOfClauses = 0;
    int low_input_var = 0;
    int high_input_var = 0;
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
                        Bdd varBdd = sylvan_ithvar(var);
                        cube_remain = cube_remain * varBdd;
                    }
                } else {
                    continue;
                }
            } else if (line[0] == 'x') {// this is xor lines, now we begin to cnf lines
                string clause(line, 2);
                Bdd curBdd = this->encodeXLineSylvan(this->getVars_Sylvan(clause));
                bdd_vec->push_back(curBdd);
            } else { // this is cnf lines, now we begin to cnf lines
                Bdd curBdd = this->encodeCNFLineSylvan(this->getVars_Sylvan(line));
                bdd_vec->push_back(curBdd);
            }
        }
        Bdd resultBdd = this->encodeSylvan(bdd_vec);
        struct timeb tmb2;
        ftime(&tmb2);
        cout <<  "CNF2BDD (Sylvan) Encoding done. " << ((tmb2.time - tmb1.time) + (tmb2.millitm - tmb1.millitm) / 1000.0)
             << " seconds" << endl;
        Bdd target_bdds = sylvan_project(resultBdd.GetBDD(), (cube_remain).GetBDD());
        double minterm = target_bdds.SatCount(_network->get_input_size() );
        struct timeb tmb3;
        ftime(&tmb3);
        cout << "CNF2BDD (Sylvan) Solving done. " << ((tmb3.time - tmb2.time) + (tmb3.millitm - tmb2.millitm) / 1000.0)
             << " seconds" << endl;
        cout << "The minterm is: " << minterm << endl;
        delete bdd_vec;
    } else {
        throw std::runtime_error("Error opening instance File");
    }
}

Bdd CNFEncoder_Sylvan::encodeSylvan(vector<sylvanBdd> *bdd_vec) {
    Bdd base = (*bdd_vec)[0];
    for (int i = 1; i < bdd_vec->size(); ++i) {
//        cout << "This is " << i << "'th bdd." << endl;
        base = base * (*bdd_vec)[i];
    }
    return base;
}

vector<int> CNFEncoder_Sylvan::getVars_Sylvan(string line) {
    vector<int> varsElements;
    string token;
    istringstream tokenstream(line);
    while (getline(tokenstream, token, ' ')) {
        varsElements.push_back(stoi(token));
    }
    return varsElements;
}

Bdd CNFEncoder_Sylvan::encodeCNFLineSylvan(vector<int> var_vec) {
    Bdd base = sylvan_false;
    for (int i = 0; i < var_vec.size(); ++i) {
        int val = var_vec[i];
        if (val < 0) {
            Bdd base_next = sylvan_ithvar(-val);
            base = base + (!base_next);
        } else if (val == 0) {
            break;
        } else {
            Bdd base_next = sylvan_ithvar(val);
            base = base + base_next;
        }
    }
    return base;
}

Bdd CNFEncoder_Sylvan::encodeXLineSylvan(vector<int> var_vec) {
    Bdd base;
    int val = var_vec[0];
    assert(val != 0);
    if (val < 0) {
        base = sylvan_ithvar(-val);
        base = !base;
    } else {
        base = sylvan_ithvar(val);
    }

    for (int i = 1; i < var_vec.size(); ++i) {
        int val = var_vec[i];
        if (val == 0) {
            break;
        } else if (val < 0) {
            Bdd base_next = sylvan_ithvar(val);
            base = base.Xor(!base_next);
        } else {
            Bdd base_next = sylvan_ithvar(val);
            base = base.Xor(base_next);
        }
    }
    return base;
}

Bdd CNFEncoder_Sylvan::encodeDichSylvan(vector<Bdd> *bdd_vec, int start, int end) {
//    return sylvan_true;
    if (abs(start - end) == 1) {
        Bdd pre_half = (*bdd_vec)[start];
        Bdd pos_half = (*bdd_vec)[end];
        Bdd res = pre_half * pos_half;
        return res;
    } else if (abs(start - end) == 0) {
        return (*bdd_vec)[start];
    } else {
        Bdd pre_half = this->encodeDichSylvan(bdd_vec, start, (end - start) / 2 + start);
        Bdd pos_half = this->encodeDichSylvan(bdd_vec, (end - start) / 2 + start + 1, end);
        Bdd res = pre_half * pos_half;
        return res;
    }
}

Bdd CNFEncoder_Sylvan::andSetSylvan(int start, int end) {
    Bdd result;
    if (abs(start - end) == 1) {
        Bdd pre_half = sylvan_ithvar(start);
        Bdd pos_half = sylvan_ithvar(end);
        result = pre_half * pos_half;
    } else if (abs(start - end) == 0) {
        return result;
    } else {
        Bdd pre_half = this->andSetSylvan(start, (end - start) / 2 + start);
        Bdd pos_half = this->andSetSylvan((end - start) / 2 + start + 1, end);
        result = pre_half * pos_half;
    }
    return result;
}

void CNFEncoder_Sylvan::quit() {
    sylvan_quit();
    lace_stop();
}