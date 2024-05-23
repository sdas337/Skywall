#include "globals.h"
#include "board.cpp"
#include "eval.cpp"

using namespace std;

#define TT_size 1048576

struct TTentry {
	uint64_t zobristHash;
	Move m;
	int depth;
	int score;
	size_t flag;

	TTentry() {
		zobristHash = 0ull;
		m = Move(0, 0, 0);
		depth = 0;
		score = 0;
		flag = 0;
	}
	TTentry(uint64_t hash, Move move, int s, int d, size_t f) {
		zobristHash = hash;
		m = move;
		depth = d;
		score = s;
		flag = f;
	}
	
};

TTentry transpositionTable[TT_size];

Move moveToPlay;
int chosenDepth;

int maxTimeForMove = 0;


int negamax(Board &board, chrono::high_resolution_clock::time_point &time, int depth, int alpha, int beta) {
	if (depth == 0) {
		board.nodes++;
		return evaluate(board);
	}

	uint64_t currentHash = board.boardStates.back().zobristHash;
	TTentry currentEntry = transpositionTable[currentHash % TT_size];

	if (currentEntry.zobristHash == currentHash && currentEntry.depth >= depth) {
		if(currentEntry.flag == 4 ||	// exact score
			(currentEntry.flag == 2 && currentEntry.score >= beta) ||	// lower bound of score, fail high
			(currentEntry.flag == 1 && currentEntry.score <= alpha))	// upper bound, fail low
			return currentEntry.score;
	}

	vector<Move> allMoves = board.generateLegalMovesV2();

	if (allMoves.size() == 0) {
		if (board.fiftyMoveCheck() || board.repeatedPositionCheck()) {
			return -10;	// small contempt for draw
		}
		return -900000 - depth;
	}

	int bestScore = -999999;
	Move bestMove = Move(0,0,0);


	for (uint8_t i = 0; i < allMoves.size(); i++) {
		Move move = allMoves[i];

		if (board.nodes & 4096 && chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - time).count() > maxTimeForMove)
			break;

		board.makeMove(move);
		int currentScore = -negamax(board, time, depth - 1, -beta, -alpha);
		board.undoMove(move);

		if (currentScore > bestScore) {
			bestScore = currentScore;
			bestMove = move;
			if (depth == chosenDepth) {
				moveToPlay = bestMove;
			}
			if (currentScore > alpha)
				alpha = currentScore;
			if (alpha >= beta)
				break;
		}
	}

	int boundType = 0;
	if (alpha >= beta) {
		boundType = 2;
	}
	else {
		if (bestScore > alpha) {
			boundType = 4;
		}
		else {
			boundType = 1;
		}
	}

	transpositionTable[currentHash % TT_size] = TTentry(currentHash, bestMove, bestScore, depth, boundType);

	return bestScore;
}

Move searchBoard(Board board, int time) {
	maxTimeForMove = time / 30;

	cout << "Time\t\tDepth\t\tBest Move\tNodes\n";

	chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();

	for (chosenDepth = 1; chosenDepth < 64; chosenDepth++) {
		int score = negamax(board, start, chosenDepth, -999999, 999999);

		auto end = chrono::high_resolution_clock::now();
		auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
		if (duration > maxTimeForMove)
			break;

		cout << duration << " ms\t\t";
		cout << chosenDepth << "\t\t";
		cout << moveToPlay.printMove() << " \t\t";

		cout << board.nodes << "\n";
	}

	return moveToPlay;
}