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

class Move {
	public:
		int getStartSquare();
		int getEndSquare();
		int getFlag();
		Move();
		Move(int startSquare, int endSquare, int flags);
		string printMove();
		uint16_t getRawValue();
		bool isPromotion();
		uint16_t rawValue;
};

bool operator==(const Move& lhs, const Move& rhs);

struct BoardStateInformation {
	bool castlingRights[4];
	int8_t enPassantSquare;
	int8_t fiftyMoveCount;
	int8_t capturedPieceType;
	uint64_t zobristHash;
};

class Board {
public:
	// A0 = 0, H0 = 7, H8 = 63
	int rawBoard[64];

	uint64_t nodes = 0;
	uint64_t lookups = 0;
	uint64_t ttEntries = 0;

	int plyCount;
	int currentPlayer;

	uint64_t occupiedBoard[3];
	uint64_t pieceBoards[8];

	int kingLocations[3];
	uint64_t attackingSquares[3];

	vector<BoardStateInformation> boardStates;

	char prettyPiecePrint(int row, int col, int pieceAtLoc);
	void printBoard();
	void setMoveFromString(Move& m, string currentMove);
	void loadBoardFromFen(string fen);

	Board();

	void setPiece(int row, int col, int piece);
	void removePiece(int row, int col);
	bool isWhitePiece(int row, int col);
	bool fiftyMoveCheck();
	bool repeatedPositionCheck();
	bool isCapture(Move move);

	void makeRawMove(Move move);
	void undoRawMove(Move move);
	void makeMove(Move move);
	void undoMove(Move move);
	void makeNullMove();
	void undoNullMove();

	uint64_t zobristHashCalc();

	bool sideInCheck(int player);
	vector<Move> generateLegalMovesV2(bool onlyCaptures);
	uint64_t getAttackers(int square, uint64_t comboOB);
	uint64_t generateRookMoves(int square, uint64_t blockerBitboard);
	uint64_t generateBishopMoves(int square, uint64_t blockerBitboard);

	int generatePseudoLegalMovesV3(vector<Move>& listOfMoves, int wantedPlayer);
	void generateAttacksV3(int wantedPlayer);

private:
	int directions[8] = { 8, -8, -1, 1, -7, 7, -9, 9 };
	int distanceFromEdge[64][8];
	uint64_t validKnightMoves[64];
	uint64_t validKingMoves[64];

	vector<Move> examinedMovesDuringCheck;

	uint64_t validPawnMoveMasks[64][3];
	uint64_t validPawnCaptureMasks[64][3];

	uint64_t zobPieces[3][7][64];
	uint64_t zobColor;
	uint64_t zobCastle[4];
	uint64_t zobEnPassant[8];

	void generateZobristNumbers();
	void calculateZobristHash();

	void precomputeDistances();
	int generateKingMovesV2(int square, vector<Move>& moves, int insertionIndex, int wantedPlayer);
	uint64_t generatePawnAttacks(int square, int wantedPlayer);
	int generatePawnMovesV2(int square, vector<Move>& moves, int insertionIndex, int wantedPlayer);
	uint64_t generateKnightMoves(int square, int safePlayer);
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

extern TuneValue seePDepth;
extern TuneValue seeCPScale;
extern TuneValue seeQPScale;


extern vector<TuneValue*> mvvValues;
extern vector<TuneValue*> seeValues;

extern vector<TuneValue*> allTunables;