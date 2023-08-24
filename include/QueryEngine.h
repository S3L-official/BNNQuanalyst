//
// Created by yedi zhang on 2020/11/9.
//

#ifndef BNNQuanalyst_VERIFIER_H
#define BNNQuanalyst_VERIFIER_H

#include "BNNetwork.h"
#include "Encoder_CUDD.h"
#include "Encoder_Sylvan.h"
#include "Options.h"
#include <time.h>
#include <sys/timeb.h>

class QueryEngine {
public:
    Options *_opts;
    ENGINE _engine;
    vector<BNNetwork> _models;
    vector<Query> _queries;

public:
    QueryEngine(Options *opt, string relaPath);

    bool checkOneQuery(Query query);
};

/* also target robustness*/
class QE_Robust {
public:
    Query *_query;
    BNNetwork *_bnn;
    Options *_opt;
    vector<DdNode *> *_ADDVec;
    vector<BDD> *_BDDVec;
    DdManager *_gbm;
    vector<double> *_result;
    vector<double> *_time;
    double _allAdv = -1;
    float _encodingTime = -1;
    float _solvingTIme = -1;
    int _benign_class = 0;
    bool _if_set_binign = false;
    bool _if_BDD2ADD = false;

public:
    QE_Robust(Query *q, Options *opt, BNNetwork *bnn, string relaPath);

    QE_Robust();

    void setMgr(DdManager *mgr);

    void encode2DD();

    void test(bool ifITE);

    int forwardPredict(vector<int> input);

    void solve();

    void transfer2CNF();

    void set_benign_class(int _ins);

    void printVerificationResult();

    void writeDD2File();

    void logPrint(char *fileName);

    void quit();

    void setBDDVec(vector<BDD> *bddVec);
};

class QE_RobustSylvan {
public:
    Query *_query;
    BNNetwork *_bnn;
    Options *_opt;
    vector<sylvan::Bdd> *_BddVec;
    vector<double> *_result;
    vector<double> *_time;
    double _allAdv = -1;
    float _encodingTime = -1;
    float _solvingTime = -1;
    int _benign_class = 0;
    bool _if_set_binign = false;
public:
    QE_RobustSylvan(Query *q, Options *opt, BNNetwork *bnn, string relaPath);

    QE_RobustSylvan();

    void encode2DD();

    void test(bool ifITE);

    int forwardPredict(vector<int> input);

    void solve();

    void set_benign_class(int _ins);

    void printVerificationResult();

    void writeDD2File();

    void logPrint(char *fileName);

    void quit();


};


class QE_SDComputer {
public:
    Query *_query;
    BNNetwork *_bnn;
    Options *_opt;
    vector<BDD> *_BDDVec;
    DdManager *_gbm;
    float _encodingTime = -1;
    float _solvingTIme = -1;
    vector<int> *_SD;
public:
    QE_SDComputer(Query *q, Options *opt, BNNetwork *bnn, string relaPath);

    QE_SDComputer();

    int forwardPredict(vector<int> input);

    void solve();

    void logPrint(char *fileName);

    void quit();
};

class QE_SDComputerSylvan {
public:
    Query *_query;
    BNNetwork *_bnn;
    Options *_opt;
    vector<sylvan::Bdd> *_BddVec;
    DdManager *_gbm;
    float _encodingTime = -1;
    float _solvingTIme = -1;
    vector<int> *_SD;
public:
    QE_SDComputerSylvan(Query *q, Options *opt, BNNetwork *bnn, string relaPath);

    QE_SDComputerSylvan();

    int forwardPredict(vector<int> input);

    void solve();

    void logPrint(char *fileName);

    void quit();
};


class QE_Interpreter {
public:
    Query *_query;
    BNNetwork *_bnn;
    Options *_opt;
    vector<BDD> *_BDDVec;
    vector<DdNode *> *_ADDVec;
    vector<vector<int *> *> *_exp_vec_vec;
    DdManager *_gbm;
    float _encodingTime = -1;
    float _solvingTIme = -1;
    int _SD;
public:
    QE_Interpreter(Query *q, Options *opt, BNNetwork *bnn, string relaPath);

    QE_Interpreter();

    vector<bool> solve();

    vector<bool> getImp(int num);

    vector<bool> getEssVars();

    void logPrint(char *fileName, int i, int j);

    void quit();
};


#endif //BNNQuanalyst_VERIFIER_H
