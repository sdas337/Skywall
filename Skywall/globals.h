#pragma once

#include <vector>
#include <set>
#include <iostream>
#include <sstream> 
#include <fstream>

#include <inttypes.h>
#include <stack>
#include <string>
#include <algorithm>


using namespace std;

int popLSB(uint64_t& bitboard);

vector<string> split(const string, const char);

int squareNameToValue(string);