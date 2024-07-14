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

constexpr int32_t S(const int32_t mg, const int32_t eg)
{
	//return (eg << 16) + mg;
	return static_cast<int32_t>(static_cast<uint32_t>(eg) << 16) + mg;
}

constexpr int32_t extract_eg(const int32_t eval) {
	return (eval + 0x8000) >> 16;
}

constexpr int32_t extract_mg(const int32_t eval) {
	return ((short)eval);
}

// trim from start (in place)
inline void ltrim(std::string& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
		}));
}

// trim from end (in place)
inline void rtrim(std::string& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
		}).base(), s.end());
}

inline void trim(std::string& s) {
	rtrim(s);
	ltrim(s);
}

inline std::vector<std::string> splitString(std::string& str, char delimiter)
{
	trim(str);
	if (str == "") return std::vector<std::string>{};

	std::vector<std::string> strSplit;
	std::stringstream ss(str);
	std::string token;

	while (getline(ss, token, delimiter))
	{
		trim(token);
		strSplit.push_back(token);
	}

	return strSplit;
}

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