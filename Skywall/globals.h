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
#include <random>
#include <chrono>

using namespace std;

int popLSB(uint64_t& bitboard);

void outputTunableJSON();
void outputTunableOptions();

vector<string> split(const string, const char);

int squareNameToValue(string);

void setupLMR();

struct TuneValue {
	string name;
	int value;
	int min;
	int max;
	int step;

	TuneValue() {
		name = "";
		value = 0;
		min = 0;
		max = 0;
		step = 0;
	}

	TuneValue(string n, int val, int mi, int ma, int s) {
		name = n;
		value = val;
		min = mi;
		max = ma;
		step = s;
	}

};

extern TuneValue rfPruningBase;
extern TuneValue rfpDepth;

extern TuneValue nmpDepth;
extern TuneValue nmpBaseReduction;
extern TuneValue nmpScaleReduction;

extern TuneValue fpDepth;
extern TuneValue fpScale;
extern TuneValue fpMargin;

extern TuneValue lmpDepth;
extern TuneValue lmpQuad;
extern TuneValue lmpScale;
extern TuneValue lmpBase;

extern TuneValue lmrBase;
extern TuneValue lmrDivisor;
extern TuneValue lmrDepth;
extern TuneValue lmrMoveCount;

extern TuneValue hstMin;
extern TuneValue hstQuad;
extern TuneValue hstLin;
extern TuneValue hstConst;

extern TuneValue qhstMin;
extern TuneValue qhstQuad;
extern TuneValue qhstLin;
extern TuneValue qhstConst;

extern TuneValue hardTC;
extern TuneValue tcMul;
extern TuneValue timeMul;
extern TuneValue incMul;

extern TuneValue aspDelta;
extern TuneValue aspWindow;

extern TuneValue iirDepth;


extern vector<TuneValue*> allTunables;