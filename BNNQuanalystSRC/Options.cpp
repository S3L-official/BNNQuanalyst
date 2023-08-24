//
// Created by yedi zhang on 2020/11/8.
//

#include "../include/Options.h"
#include <string.h>

using namespace std;

Options::Options(int argc, char *argv[]) {
    assert(argv != NULL);

    for (int i = 0; i < argc; i++) {
        assert(argv[i] != NULL);
    }
    int index = 1;
    while (index < argc) {
        if (!strcmp(argv[index], "-h")) {
            print_help();
            exit(0);
        } else if (!strcmp(argv[index], "-v")) {
            index++;
        } else if (!strcmp(argv[index], "-q")) {
            _queryFileName = argv[index + 1];
            index = index + 2;
        } else if (!strcmp(argv[index], "-IP")) {
            int modeReorder = stoi(argv[index + 1]);
            _IP = (modeReorder == 1);
            index = index + 2;
        } else if (!strcmp(argv[index], "-DC")) {
            int modeReorder = stoi(argv[index + 1]);
            _DC = (modeReorder == 1);
            index = index + 2;
        } else if (!strcmp(argv[index], "-e")) {
            string engine = argv[index + 1];
            if (engine == "cudd") {
                _engineType = CUDD;
            } else if (engine == "sylvan") {
                _engineType = SYLVAN;
            } else if (engine == "npaq-sylvan") { // first use npaq to get CNF then encode CNF into BDD using sylvan
                _engineType = NPAQ_SYLVAN;
            } else if (engine == "npaq-cudd") { // first use npaq to get CNF then encode CNF into BDD using cudd
                _engineType = NPAQ_CUDD;
            } else if (engine ==
                       "cudd-npaq") { // first use cudd to get BDD then encode CNF, finally use npaq do the model counting
                _engineType = CUDD_NPAQ;
            } else {
                _engineType = CUDD;
            }
            index = index + 2;
        } else if (!strcmp(argv[index], "-l")) {
            int modeOpt = stoi(argv[index + 1]);
            _level = modeOpt;
            index = index + 2;
        } else if (!strcmp(argv[index], "-th")) {
            int worker = stoi(argv[index + 1]);
            _worker = worker;
            index = index + 2;
        } else {
            cout << "WRONG arguments' input. Please enter the options and arguments w.r.t. the help document: " << endl;
            cout << "BNNQuanalyst -h" << endl;
            exit(0);
        }
    }
}