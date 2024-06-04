#include <vector>
#include <set>
#include <iostream>
#include <sstream> 
#include <fstream>

#include <inttypes.h>
#include <stack>
#include <string>
#include <algorithm>
#include <random>
#include <chrono>

using namespace std;

int popLSB(uint64_t& bitboard);

vector<string> split(const string, const char);

int squareNameToValue(string);

void setupLMR();