//
// Created by yedi zhang on 2020/11/9.
//

#include "../include/Query.h"

Query::Query(string modelName, vector<int> instance, int distance) {
    _modelName1 = modelName;
    _instance = instance;
    _distance = distance;
    _propertyID = "local_robustness";
    _robustness_judge = "hamming";

}

Query::Query(string modelName, vector<int> instance, int distance, int target_class) {
    _modelName1 = modelName;
    _instance = instance;
    _distance = distance;
    _target_class = {target_class};
    _propertyID = "local_robustness";
    _robustness_judge = "hamming";

}

Query::Query(string modelName, vector<int> instance, vector<int> set) {
    _modelName1 = modelName;
    _instance = instance;
    _trigger_set = set;
    _propertyID = "local_robustness";
    _robustness_judge = "fix-set";
}

Query::Query(string modelName, vector<int> instance, int distance, vector<int> target_class) {
    _modelName1 = modelName;
    _instance = instance;
    _distance = distance;
    _target_class = target_class;
    _propertyID = "target_robustness";
    _robustness_judge = "hamming";
}

Query::Query(string modelName, vector<int> instance, vector<int> set, vector<int> target_class) {
    _modelName1 = modelName;
    _instance = instance;
    _trigger_set = set;
    _target_class = target_class;
    _propertyID = "target_robustness";
    _robustness_judge = "fix-set";
}

string Query::getIdentifier() {
    return _propertyID;
}

string Query::getModelName1() {
    return _modelName1;
}

string Query::getModelName2() {
    return _modelName2;
}


int Query::getDistance() {
    return _distance;
}

void Query::setInstanceName(string inst) {
    _instance_name = inst;
}

void Query::setTargetClassName(string inst) {
    _target_class_name = inst;
}

void Query::setFixSetName(string inst) {
    _fixSet_name = inst;
}

void Query::setIdentifier(string newID) {
    this->_propertyID = newID;
}

void Query::setExplainerParas(int num) {
    this->_PI_num = num; /* default 0 for all PI */
}

void Query::setTolerance(double t) {
    _tolerance = t;
}