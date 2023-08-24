//
// Created by yedi zhang on 2020/11/4.
//

#include <stdio.h>
#include "iostream"
#include "cuddObj.hh"
#include "unistd.h"
#include <string.h>
#include "Options.h"
#include "QueryEngine.h"

using namespace std;

string relaPath;

int main(int argc, char *argv[]) {
    Options opts(argc, argv);
    char *path = NULL;

    path = getcwd(NULL, 0);
    relaPath = string(path) + "/";
    free(path);

    QueryEngine queryEngine(&opts, relaPath);
    int queryNum = queryEngine._queries.size();
    if (queryNum == 1) {
        cout << "There are " << queryNum << " query in total." << endl;
    } else {
        cout << "There are " << queryNum << " queries in total." << endl;
    }
    for (int i = 0; i < queryNum; ++i) {
        cout << endl << "Now checking query " << i + 1 << "......" << endl << endl;
        Query query = queryEngine._queries[i];
        queryEngine.checkOneQuery(query);
    }

    return 0;
}