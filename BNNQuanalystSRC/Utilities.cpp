//
// Created by yedi zhang on 2020/10/27.
//
#include "../include/Utilities.h"
#include <fstream>

/**
 * Print a dd summary
 * pr = 0 : prints nothing
 * pr = 1 : prints counts of nodes and minterms
 * pr = 2 : prints counts + disjoint sum of product
 * pr = 3 : prints counts + list of nodes
 * pr > 3 : prints counts + disjoint sum of product + list of nodes
 * @param the dd node
 */
void print_dd(DdManager *gbm, DdNode *dd, int n, int pr) {
    printf("DdManager nodes: %ld | ", Cudd_ReadNodeCount(gbm)); /*Reports the number of live nodes in BDDs and ADDs*/
    printf("DdManager vars: %d | ", Cudd_ReadSize(gbm)); /*Returns the number of BDD variables in existence*/
//    printf("DdManager reorderings: %d | ",
//           Cudd_ReadReorderings(gbm)); /*Returns the number of times reordering has occurred*/
    printf("DdManager memory: %ld \n",
           Cudd_ReadMemoryInUse(gbm)); /*Returns the memory in use by the manager measured in bytes*/
    Cudd_PrintDebug(gbm, dd, n,
                    pr);  // Prints to the standard output a DD and its statistics: number of nodes, number of leaves, number of minterms.
}


//DdNode *CCEncoding(DdManager *gbm, vector<int> *vars, int lastIndex, int bound, int begin_var,
//                   map<string, DdNode *> *_BDD_cache_int) {
//
//    if (bound <= 0) {
//        DdNode *one = Cudd_ReadOne(gbm);
//        Cudd_Ref(one);
//        return one;
//    }
//
//    if (bound > lastIndex + 1) {
//        DdNode *zero = Cudd_ReadLogicZero(gbm);
//        Cudd_Ref(zero);
//        return zero;
//    }
//
//    string key = to_string(begin_var) + '_' + to_string(lastIndex) + '_' + to_string(bound);
//
//    if (_BDD_cache_int->find(key) != _BDD_cache_int->end()) {
//        return (*_BDD_cache_int)[key];
//    }
//    DdNode *bdd;
//    if (bound == 1) {
//        DdNode *var, *tmp;
//        int i;
//        bdd = Cudd_ReadLogicZero(gbm); // refer =1
//        Cudd_Ref(bdd);
//        for (i = lastIndex; i >= 0; i--) {
//            var = Cudd_bddIthVar(gbm, i + begin_var);
//            Cudd_Ref(var);
//
//            tmp = Cudd_bddOr(gbm, (*vars)[i] > 0 ? var : Cudd_Not(var), bdd); /*Perform AND Boolean operation*/
//            Cudd_Ref(tmp);
//            Cudd_RecursiveDeref(gbm, bdd);
//            Cudd_RecursiveDeref(gbm, var);
//            bdd = tmp;
//        }
//    } else if (lastIndex + 1 == bound) {
//        DdNode *var, *tmp;
//        int i;
//        bdd = Cudd_ReadOne(gbm); /*Returns the logic one constant of the manager*/
//        Cudd_Ref(bdd); /*Increases the reference count of a node*/
//        for (i = lastIndex; i >= 0; i--) {
//            var = Cudd_bddIthVar(gbm, i + begin_var);
//            Cudd_Ref(var);
//            tmp = Cudd_bddAnd(gbm, (*vars)[i] > 0 ? var : Cudd_Not(var), bdd); /*Perform AND Boolean operation*/
//            Cudd_Ref(tmp);
//            Cudd_RecursiveDeref(gbm, bdd);
//            Cudd_RecursiveDeref(gbm, var);
//            bdd = tmp;
//        }
//    } else {
//        DdNode *var;
//        var = Cudd_bddIthVar(gbm, lastIndex + begin_var);
//        Cudd_Ref(var);
//        DdNode *bddT = CCEncoding(gbm, vars, lastIndex - 1, bound - 1, begin_var, _BDD_cache_int);
//        DdNode *bddF = CCEncoding(gbm, vars, lastIndex - 1, bound, begin_var, _BDD_cache_int);
//        bdd = Cudd_bddIte(gbm, (*vars)[lastIndex] > 0 ? var : Cudd_Not(var), bddT, bddF);
//        Cudd_Ref(bdd);
//        Cudd_RecursiveDeref(gbm, var);
//    }
//    (*_BDD_cache_int)[key] = bdd;
//    return bdd;
//}

long long combo(int n, int m) {
    if (m == 0 || m == n) return 1;

    if (m == 1) return n;

    if (m >= 2) {
        long long up = 1;
        long long down = 1;
        for (int i = n; i > n - m; i--) {
            up = up * i;
        }
        for (int j = 1; j <= m; j++) down *= j;
        return up / down;
    }
}


void write_dd(DdManager *gbm, DdNode *dd, char *filename) {
    FILE *outfile; // output file pointer for .dot file
    outfile = fopen(filename, "w");
    DdNode **ddnodearray = (DdNode **) malloc(sizeof(DdNode *)); // initialize the function array
    ddnodearray[0] = dd;
    Cudd_DumpDot(gbm, 1, ddnodearray, NULL, NULL, outfile); // dump the function to .dot file
    free(ddnodearray);
    fclose(outfile); // close the file */
}


void write_ten_adds(DdManager *gbm, vector<DdNode *> *vec, int input_size, const char *relaPath) {
    for (int i = 0; i < vec->size(); ++i) {
        char filename1[strlen(relaPath)];
        sprintf(filename1, relaPath, i); /*Write .dot filename to a string*/
        write_dd(gbm, (*vec)[i], filename1);  /*Write the resulting cascade dd to a file*/
    }
}

/*
 * Todo: complete HELP.file
 */
void print_help() {
    std::cout
            << "BNNQuanalyst is a tool for quantitative verification of Binarized Neural Networks. Currently, it supports robustness verification (with CUDD/Sylvan package), maximal safe hamming distance (with CUDD/Sylvan package) and interpretability analysis (with CUDD package)."
            << std::endl;
    std::cout << "For more details, please visit: https://github.com/S3L-official/BNNQuanalyst." << std::endl;
    std::cout << "Usage: BNNQuanalyst [option] [arg]" << std::endl;
    std::cout << "Options and arguments: " << std::endl;
    std::cout << "-h: print help information (action mode)" << std::endl;
    std::cout << "-v: verify the input query (action mode)" << std::endl;
    std::cout << "-q arg: specify the query_file to verify (arg=query_file_name)" << std::endl;
    std::cout << "-IP arg: whether use input propagation strategy or not (arg=1: Yes; arg=0: No)" << std::endl;
    std::cout << "-DC arg: whether use divide-and-conquer Strategy or not (arg=1: Yes; arg=0: No)" << std::endl;
    std::cout << "-e arg: specify the engine used (args=cudd or sylvan or cudd-npaq or npaq-cudd or npaq-sylvan)" << std::endl;
    std::cout << "-l arg: specify the level of parallelization strategies (arg = 1,2,3) (only for Sylvan package)" << std::endl;
    std::cout << "-th arg: specify the number of workers used for Sylvan (arg>=1) (only for Sylvan package)" << std::endl;
}

vector<vector<float>> parseCSV(string filename, char delimiter) {
    ifstream csvfile;
    string line;
    vector<vector<float>> matrix;
    csvfile.open(filename);
//    csvfile.open("../examples/models/mnist_bnn_2_blk/blk1/lin_weight.csv");

    if (csvfile.is_open()) {
        while (getline(csvfile, line)) {
            vector<float> matrix_line;
            string token;
            istringstream tokenstream(line);
            while (getline(tokenstream, token, delimiter)) {
                matrix_line.push_back(stod(token));
            }
            matrix.push_back(matrix_line);
        }
    } else {
        throw std::runtime_error("Error opening " + filename);
    }

    return matrix;
}

vector<vector<int>> parseCSV2Int(string filename, char delimiter) {
    ifstream csvfile;
    string line;
    vector<vector<int>> matrix;
    csvfile.open(filename);
//    csvfile.open("../examples/models/mnist_bnn_2_blk/blk1/lin_weight.csv");

    if (csvfile.is_open()) {
        while (getline(csvfile, line)) {
            vector<int> matrix_line;
            string token;
            istringstream tokenstream(line);
            while (getline(tokenstream, token, delimiter)) {
                matrix_line.push_back(stoi(token));
            }
            matrix.push_back(matrix_line);
        }
    } else {
        throw std::runtime_error("Error opening " + filename);
    }

    return matrix;
}

/*
 * TODO: solve _WIN32
 */
vector<string> filename(string path, int number, bool internal_blk) {
    char separator;
#ifdef _WIN32
    separator = '\\';
#else
    separator = '/';
#endif
    string suffix;
    vector<string> files;
    if (internal_blk == true) {
        suffix = path + separator + "blk" + to_string(number) + separator;
        files.push_back(suffix + "lin_weight.csv");
        files.push_back(suffix + "lin_bias.csv");
        files.push_back(suffix + "bn_weight.csv");
        files.push_back(suffix + "bn_bias.csv");
        files.push_back(suffix + "bn_mean.csv");
        files.push_back(suffix + "bn_var.csv");
    } else {
        suffix = path + separator + "out_blk" + separator;
        files.push_back(suffix + "lin_weight.csv");
        files.push_back(suffix + "lin_bias.csv");
    }

    return files;
}

double myStr2Double(const char *src) {
    double ret = 0, sign = 1;
    char *p = (char *) src;
    if (*p == '-') {
        sign = -1;
        p++;
    }
    while (*p && (*p != '.'))//deal integer part
    {
        ret *= 10;
        ret += (*p) - '0';// char to int and accumulate
        p++;//move pointer
    }
    if (*p == '.')// floating-point part exists
    {
        double step = 0.1; //用来标识位数
        p++; //略过小数点
        while (*p)//处理小数部分，如果没有E就直接到结束
        {
            double step2 = 1;
            if (*p == 'E' || *p == 'e')//比如说323.443E-03；
            {
                p++;//跳过E或者e;
                if (*p == '+') {
                    p++;//跳过+
                    int temp = atoi(p);
                    while (temp--) {
                        step2 *= 10;
                    }
                } else if (*p == '-') {
                    p++;//跳过-
                    int temp = atoi(p);
                    while (temp--) {
                        step2 /= 10;
                    }
                }
                ret *= step2;
                break;
            }
            ret += step * ((*p) - '0');
            step /= 10;
            p++;
        }
    }
    return ret * sign; //不要忘记符号位
}