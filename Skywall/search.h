#include "globals.h"

extern int lmrReductions[256][256];

struct TTentry {
	uint64_t zobristHash;
	Move m;
	uint8_t depth;
	uint8_t flag;
	int score;

	TTentry() {
		zobristHash = 0ull;
		m = Move(0, 0, 0);
		depth = 0;
		score = 0;
		flag = 0;
	}
	TTentry(uint64_t hash, Move move, int s, uint8_t d, uint8_t f) {
		zobristHash = hash;
		m = move;
		depth = d;
		score = s;
		flag = f;
	}

};

extern uint32_t actual_TT_Size;
extern vector<TTentry> transpositionTable;

int estimatedMoveValue(Board& board, Move m, int flag);
bool see(Board& board, Move m, int threshhold);
int qsearch(Board& board, int depth, int plyFromRoot, int alpha, int beta);
int negamax(Board& board, int depth, int plyFromRoot, int alpha, int beta, bool nullMovePruningAllowed, Move priorMove);

Move searchBoard(Board& relevantBoard, int time, int inc, int maxDepth);