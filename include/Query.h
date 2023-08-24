//
// Created by yedi zhang on 2023/8/16.
//

#ifndef BNNQUANALYST_QUERY_H
#define BNNQUANALYST_QUERY_H

#include "Utilities.h"
#include "map"
#include "set"
#include "stack"
#include "cuddObj.hh"

using namespace std;

class Query {
public:
    string _propertyID;
    string _modelName1;
    string _modelName2;
    vector<int> _instance;
    bool _ifHD;
    int _distance;
    vector<int> _target_class;
    vector<int> _trigger_set;
    string _robustness_judge; /* hamming or fix-set */
    string _instance_name;
    string _target_class_name;
    string _fixSet_name;
    double _tolerance;
    int _PI_num;
public:
    Query(string modelName, int target_class); /* GR */

    Query(string modelName1, vector<int> instance, int distance); /* LR for hamming */
    Query(string modelName1, vector<int> instance, int distance, int target_class); /* LR for hamming */

    Query(string modelName1, vector<int> instance, vector<int> set); /* LR for set */

//    Query(string modelName1, vector<int> instance, int distance, int target_class); /* LR for set */
    Query(string modelName1, vector<int> instance, int distance, vector<int> target_class); /* LR for set */

    Query(string modelName1, vector<int> instance, vector<int> set, vector<int> target_class); /* LR for set */

    Query(string modelName1, vector<int> TAP_trigger);

    string getIdentifier();

    string getModelName1();

    string getModelName2();

    int getDistance();

    void setInstanceName(string inst);

    void setTargetClassName(string inst);

    void setFixSetName(string inst);

    void setIdentifier(string newID);

    void setExplainerParas(int num);

    void setTolerance(double t);
};


#endif //BNNQUANALYST_QUERY_H
