/*
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <numeric>
#include <stdexcept>
#include <deque>
#include <iterator>
*/

#include <map>
#include <set>
#include <vector>
#include <iostream>

#include "document.h"
#include "read_input_functions.h"
#include "string_processing.h"
#include "search_server.h"
#include "request_queue.h"
#include "paginator.h"

using namespace std;

template <typename T>
ostream& operator<< (ostream& is, const vector<T>& vec) {
    bool flag = false;
    is << "[";
    for (const auto& elem : vec) {
        if (flag) {
            is << ", ";
        }
        is << elem;
        flag = true;
    }
    is << "]";
    return is;
}

template <typename T>
ostream& operator<< (ostream& is, const set<T>& vec) {
    bool flag = false;
    is << "{";
    for (const auto& elem : vec) {
        if (flag) {
            is << ", ";
        }
        is << elem;
        flag = true;
    }
    is << "}";
    return is;
}

template <typename T, typename Y>
ostream& operator<< (ostream& is, const map<T, Y>& mapa) {
    bool flag = false;
    size_t num = mapa.size();
    size_t i = 1;
    is << "{"s;
    for (const auto& [key, value] : mapa) {
        if (flag) {
            is << " "s;
        }
        flag = true;
        is << key << ": "s << value;
        if (i < num) {
            is << ","s;
        }
        ++i;
    }

    is << "}";
    return is;
}

int main() {
    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);
    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});
   
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    
    request_queue.AddFindRequest("curly dog"s);
    request_queue.AddFindRequest("big collar"s);
    request_queue.AddFindRequest("sparrow"s);
    cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;
    return 0;
}