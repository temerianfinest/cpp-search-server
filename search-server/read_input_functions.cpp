#include "read_input_functions.h"
#include <string>
#include <vector>
#include <set>
#include <iostream>
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}


