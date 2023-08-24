//
// Created by yedi zhang on 2020/11/9.
//
#include "math.h"
#include "../include/UtilParser.h"

extern string relaPath;

vector<vector<int>> parseInstanceFile(string instanceFileName) {
    ifstream instancefile;
    string line;
    vector<int> instancePixels;
    vector<vector<int>> instanceList;
    instancefile.open(instanceFileName);

    if (instancefile.is_open()) {
        while (getline(instancefile, line)) {
            if (line[0] == '#' | line[0] == '\n' | line == "") {
                continue;
            }
            string token;
            istringstream tokenstream(line);
            while (getline(tokenstream, token, ' ')) {
                instancePixels.push_back(stoi(token));
            }
            instanceList.push_back(instancePixels);
        }
    } else {
        throw std::runtime_error("Error opening " + instanceFileName);
    }

    return instanceList;
}

vector<Query> parseQueryFile(string queryFileName) {
    vector<vector<int>> instance;
    ifstream queryfile;
    string line;
    vector<Query> queryList;

    queryfile.open(queryFileName);

    if (queryfile.is_open()) {
        while (getline(queryfile, line)) {
            if (line[0] == '#' | line[0] == '\n' | line == "") {
                continue;
            }
            vector<string> queryElements;
            string token;
            istringstream tokenstream(line);
            while (getline(tokenstream, token, ':')) {
                queryElements.push_back(token);
            }
            Query q = parseQuery(queryElements);
            queryList.push_back(q);
        }
    } else {
        throw std::runtime_error("Error opening Query File");
    }

    return queryList;
}

void trimSpace(string &s) {
    if (!s.empty()) {
        s.erase(0, s.find_first_not_of(" "));
        s.erase(s.find_last_not_of(" ") + 1);
    }
}

Query parseQuery(vector<string> queryElements) {
    assert(queryElements.size() == 2);
    string str = queryElements[1];
    size_t found = str.find(';');
    string queryTrim1 = str.substr(0, found);
    string queryTrim2 = queryTrim1.substr(queryTrim1.find('<') + 1, queryTrim1.find('>') - 2);
    istringstream tokenstream(queryTrim2);
    string token;
    vector<string> q;
    while (getline(tokenstream, token, ',')) {
        q.push_back(token);
    }

    for (int i = 0; i < queryElements.size(); ++i) {
        trimSpace(queryElements[i]);
    }
    for (int i = 0; i < q.size(); ++i) {
        trimSpace(q[i]);
    }
    if ((queryElements[0]) == "local_robustness") {
        assert(q.size() == 4);
        vector<int> instance = parseInstanceFile(relaPath + "benchmarks/inputs/" + q[1] + ".txt")[0];
        string type = q[2];
        if (type == "hamming") {
            Query query(q[0], instance, stoi(q[3]));
            query.setInstanceName(q[1]);
            return query;
        } else if (type == "set") {
            vector<int> set = parseInstanceFile(relaPath + "benchmarks/inputs/" + q[3] + ".txt")[0];
            Query query(q[0], instance, set);
            query.setInstanceName(q[1]);
            query.setFixSetName(q[3]);
            return query;
        } else {
            cout << "error local robustness type for judging, please set: 'hamming' or 'set'";
        }
    } else if ((queryElements[0]) == "target_robustness") {
        assert(q.size() == 5);
        vector<int> instance = parseInstanceFile(relaPath + "benchmarks/inputs/" + q[1] + ".txt")[0];
        vector<int> target_set = parseInstanceFile(relaPath + "benchmarks/inputs/" + q[4] + ".txt")[0];
        string type = q[2];
        if (type == "hamming") {
            Query query(q[0], instance, stoi(q[3]), target_set);
            query.setInstanceName(q[1]);
            query.setTargetClassName(q[4]);
            query._ifHD = true;
            return query;
        } else if (type == "set") {
            vector<int> set = parseInstanceFile(relaPath + "benchmarks/inputs/" + q[3] + ".txt")[0];
            Query query(q[0], instance, set, target_set);
            query.setInstanceName(q[1]);
            query.setFixSetName(q[3]);
            query.setTargetClassName(q[4]);
            query._ifHD = false;
            return query;
        } else {
            cout << "error local robustness type for judging, please set: 'hamming' or 'set'";
        }
    } else if (queryElements[0] == "compute_SD") {
        assert(q.size() == 4);
        vector<int> instance = parseInstanceFile(relaPath + "benchmarks/inputs/" + q[1] + ".txt")[0];
        Query query(q[0], instance, stoi(q[2])); /* same to hamming target_robust*/
        query.setInstanceName(q[1]);
        query.setIdentifier("compute_SD");
        query.setTolerance(stod(q[3]));
        return query;
    } else if (queryElements[0] == "PI_explain") {
        assert(q.size() == 6);
        vector<int> instance = parseInstanceFile(relaPath + "benchmarks/inputs/" + q[1] + ".txt")[0];
        vector<int> target_class = parseInstanceFile(relaPath + "benchmarks/inputs/" + q[4] + ".txt")[0];
        string type = q[2];
        if (type == "hamming") {
            Query query(q[0], instance, stoi(q[3]), target_class);
            query.setInstanceName(q[1]);
            query.setTargetClassName(q[4]);
            query.setIdentifier("PI_explain");
            query.setExplainerParas(stoi(q[5]));
            return query;
        } else if (type == "set") {
            vector<int> set = parseInstanceFile(relaPath + "benchmarks/inputs/" + q[3] + ".txt")[0];
            Query query(q[0], instance, set, target_class);
            query.setInstanceName(q[1]);
            query.setFixSetName(q[3]);
            query.setTargetClassName(q[4]);
            query.setIdentifier("PI_explain");
            query.setExplainerParas(stoi(q[5]));
            return query;
        } else {
            cout << "error local robustness type for judging, please set: 'hamming' or 'set'";
        }
    } else if (queryElements[0] == "EF_explain") {
        assert(q.size() == 5);
        vector<int> instance = parseInstanceFile(relaPath + "benchmarks/inputs/" + q[1] + ".txt")[0];
        vector<int> target_class = parseInstanceFile(relaPath + "benchmarks/inputs/" + q[4] + ".txt")[0];
        string type = q[2];
        if (type == "hamming") {
            Query query(q[0], instance, stoi(q[3]), target_class);
            query.setInstanceName(q[1]);
            query.setTargetClassName(q[4]);
            query.setIdentifier("EF_explain");
            return query;
        } else if (type == "set") {
            vector<int> set = parseInstanceFile(relaPath + "benchmarks/inputs/" + q[3] + ".txt")[0];
            Query query(q[0], instance, set, target_class);
            query.setInstanceName(q[1]);
            query.setFixSetName(q[3]);
            query.setTargetClassName(q[4]);
            query.setIdentifier("EF_explain");
            return query;
        } else {
            cout << "error local robustness type for judging, please set: 'hamming' or 'set'";
        }
    } else {
        cout << "WRONG query_input file format!" << endl;
        exit(0);
    }
}