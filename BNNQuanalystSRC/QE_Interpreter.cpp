//
// Created by yedi zhang on 2020/12/29.
//

#include "../include/QueryEngine.h"
#include "../include/BNNetwork.h"
#include "../include/Encoder_CUDD.h"
#include "../include/Utilities.h"

QE_Interpreter::QE_Interpreter(Query *q, Options *opt, BNNetwork *bnn, string relaPath) : _bnn() {
    _query = q;
    _opt = opt;
    _bnn = bnn;
    _exp_vec_vec = new vector<vector<int *> *>;
    _ADDVec = new vector<DdNode *>;
}

QE_Interpreter::QE_Interpreter() : _bnn() {

}

vector<bool> QE_Interpreter::solve() {
    Encoder_CUDD encoder(_bnn, _query, _opt->_IP, _opt->_DC);
    _gbm = encoder._mgr->getManager();
    struct timeb tmb;
    ftime(&tmb);
    _BDDVec = encoder.encode2targetBDD(_query->_target_class);
    struct timeb tmb1;
    ftime(&tmb1);
    cout << "Encoding done. " << ((tmb1.time - tmb.time) + (tmb1.millitm - tmb.millitm) / 1000.0)
         << " seconds" << endl << endl;
    _encodingTime = ((tmb1.time - tmb.time) + (tmb1.millitm - tmb.millitm) / 1000.0);

    struct timeb tmb2;
    ftime(&tmb2);
    for (int i = 0; i < _BDDVec->size(); ++i) {
        DdNode *bdd = (*_BDDVec)[i].getNode();
        Cudd_Ref(bdd);
        DdNode *add = Cudd_BddToAdd(_gbm, bdd);
        Cudd_Ref(add);
        Cudd_RecursiveDeref(_gbm, bdd);
        _ADDVec->push_back(add);
    }
    delete _BDDVec;
    vector<bool> ifSucc;
    if (_query->_propertyID == "PI_explain") {
        ifSucc = getImp(_query->_PI_num);
    } else if (_query->_propertyID == "EF_explain") {
        ifSucc = getEssVars();
    }
    cout << "\nSolve done. " << ((tmb2.time - tmb1.time) + (tmb2.millitm - tmb1.millitm) / 1000.0)
         << " seconds" << endl;
    _solvingTIme = (double) ((tmb2.time - tmb1.time) + (tmb2.millitm - tmb1.millitm) / 1000.0);
    return ifSucc;
}

vector<bool> QE_Interpreter::getImp(int num) {
    vector<bool> res(_ADDVec->size(), false);
    for (int i = 0; i < _ADDVec->size(); ++i) { /* return (prime) implicant(s) for each target class */
        int length;
        if (Cudd_CountMinterm(_gbm, (*_ADDVec)[i], _query->_instance.size()) == 0) {
            cout << "The BDD for class " << i << " is Zero node." << endl;
            continue;
        }
        vector<int *> *onePIvec = new vector<int *>;
        DdNode *implicant = Cudd_LargestCube(_gbm, (*_ADDVec)[i], &length);
        Cudd_Ref(implicant);
        int counting = 0;
        int *array = new int[Cudd_ReadSize(_gbm)];
        Cudd_BddToCubeArray(_gbm, implicant, array);
        onePIvec->push_back(array);
        counting++;

        if (num == 1) {
            _exp_vec_vec->push_back(onePIvec);
            res[i] = true;
            Cudd_RecursiveDeref(_gbm, implicant);
        } else {
            DdNode *lb = Cudd_bddAnd(_gbm, (*_ADDVec)[i], Cudd_Not(implicant));
            Cudd_Ref(lb);
            Cudd_RecursiveDeref(_gbm, implicant);
            while (lb != Cudd_ReadLogicZero(_gbm) && (counting < num)) {
                int *array = new int[Cudd_ReadSize(_gbm)];
                DdNode *tmp;
                int length;
                implicant = Cudd_LargestCube(_gbm, lb, &length);
                Cudd_Ref(implicant);
                if (implicant == Cudd_ReadLogicZero(_gbm)) {
                    Cudd_RecursiveDeref(_gbm, implicant);
                    cout << "Currently, we can only find " << counting << " implicants for class " << i <<
                         "!" << endl;
                    break;
                }
                Cudd_BddToCubeArray(_gbm, implicant, array);
                Cudd_RecursiveDeref(_gbm, implicant);
                onePIvec->push_back(array);
                counting++;
                tmp = Cudd_bddAnd(_gbm, lb, Cudd_Not(implicant));
                Cudd_Ref(tmp);
                Cudd_RecursiveDeref(_gbm, lb);
                lb = tmp;
            }
            _exp_vec_vec->push_back(onePIvec);
            res[i] = true;
            if (counting == num) {
                cout << "We find " << num << " implicants successfully for class " << i << "." << endl;
            }
        }
    }
    return res;
}

vector<bool> QE_Interpreter::getEssVars() {
    vector<bool> res(_ADDVec->size(), false);
    for (int i = 0; i < _ADDVec->size(); ++i) { /* return a PI for each target class */
        if (Cudd_CountMinterm(_gbm, (*_ADDVec)[i], _query->_instance.size()) == 0) {
            cout << "The BDD for class " << i << " is Zero node." << endl;
            continue;
        }
        int *array = new int[Cudd_ReadSize(_gbm)];
        DdNode *essLiterals = Cudd_FindEssential(_gbm, (*_ADDVec)[i]);
        if (essLiterals != Cudd_ReadLogicZero(_gbm)) {
            print_dd(_gbm, Cudd_BddToAdd(_gbm,essLiterals),784,1);
            Cudd_Ref(essLiterals);
            Cudd_BddToCubeArray(_gbm, essLiterals, array);
            vector<int *> *onePIvec = new vector<int *>;
            onePIvec->push_back(array);
            Cudd_RecursiveDeref(_gbm, essLiterals);
            _exp_vec_vec->push_back(onePIvec);
            res[i] = true;
            cout << "We find an essential feature explanation for class " << i << " successfully." << endl;
        } else {
            cout << "We fail to find an essential feature explanation for class " << i << "." << endl;
        }
    }
    return res;
}

void QE_Interpreter::logPrint(char *filename, int i, int j) {
    FILE *outfile;
    outfile = fopen(filename, "w+");
    for (int q = 0; q < Cudd_ReadSize(_gbm); q++) {
        switch ((*(*_exp_vec_vec)[i])[j][q]) {
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
    fclose(outfile); // close the file */
}


void QE_Interpreter::quit() {
    if (_query->_PI_num != 0) {
        for (vector<vector<int *> *>::iterator ptr = _exp_vec_vec->begin(); ptr < _exp_vec_vec->end(); ptr++) {
            for (vector<int *>::iterator ptr2 = (*ptr)->begin(); ptr2 < (*ptr)->end(); ptr2++) {
                delete *ptr2;
            }
            delete *ptr;
        }
    }
    delete _exp_vec_vec;
    for (int i = 0; i < _ADDVec->size(); ++i) {
        Cudd_RecursiveDeref(_gbm, (*_ADDVec)[i]);
    }
    delete _ADDVec;
    Cudd_Quit(_gbm);
}

