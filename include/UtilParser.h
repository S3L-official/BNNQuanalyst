//
// Created by yedi zhang on 2023/8/16.
//

#ifndef BNNQUANALYST_UTILPARSER_H
#define BNNQUANALYST_UTILPARSER_H

#include "Utilities.h"
#include "map"
#include "set"
#include "stack"
#include "cuddObj.hh"
#include "Query.h"

vector<Query> parseQueryFile(string queryFileName);

Query parseQuery(vector<string> queryElements);

void trimSpace(string &s);

vector<vector<int>> parseInstanceFile(string instanceFileName);

#endif //BNNQUANALYST_UTILPARSER_H
