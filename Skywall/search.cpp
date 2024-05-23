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
	uint64_t currentHash = board.boardStates.back().zobristHash;
	TTentry currentEntry = transpositionTable[currentHash % TT_size];

	bool qsearch = (depth <= 0);
	int newDepth = depth - 1;

	if (qsearch) {
		int score = evaluate(board);
		board.nodes++;
		return score;
	}

	if (currentEntry.zobristHash == currentHash && currentEntry.depth >= depth) {
		if (currentEntry.flag == 4 ||	// exact score
			(currentEntry.flag == 2 && currentEntry.score >= beta) ||	// lower bound of score, fail high
			(currentEntry.flag == 1 && currentEntry.score <= alpha))	// upper bound, fail low
			board.lookups++;
			return currentEntry.score;
	}

	vector<Move> allMoves = board.generateLegalMovesV2(qsearch);

	if (!qsearch && allMoves.size() == 0) {
		if (board.fiftyMoveCheck() || board.repeatedPositionCheck()) {
			return -10;	// small contempt for draw
		}
		return -900000 - depth;
	}

	/*if (qsearch) {
		int score = evaluate(board);

		if (score >= beta)
			return score;
		alpha = max(alpha, score);
	}*/


	// Order moves portion
	vector<int> moveScores(allMoves.size());
	for (size_t i = 0; i < moveScores.size(); i++) {
		int score = 0;

		if (currentEntry.zobristHash == currentHash && allMoves[i] == currentEntry.m)
			score = 1000000;

		if (board.isCapture(allMoves[i]))
			score += 500 * (board.rawBoard[allMoves[i].getEndSquare()] % 8 - board.rawBoard[allMoves[i].getStartSquare()] % 8);

		moveScores[i] = score;
	}

	int bestScore = -999999, currentScore;
	Move bestMove = Move(0,0,0);

	// Check Extensions
	//if (board.sideInCheck(board.currentPlayer))
		//newDepth++;

	for (uint8_t i = 0; i < allMoves.size(); i++) {

		if (board.nodes & 4096 && chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - time).count() > maxTimeForMove)
			break;

		// Performing selection sort based on move scores above to order
		for (size_t j = i + 1; j < allMoves.size(); j++) {
			if (moveScores[j] > moveScores[i]) {
				int tmp = moveScores[j];
				moveScores[j] = moveScores[i];
				moveScores[i] = tmp;

				Move swap = allMoves[i];
				allMoves[i] = allMoves[j];
				allMoves[j] = swap;
			}
		}

		Move move = allMoves[i];

		board.makeMove(move);

		//board.nodes++;
		
		if (i == 0) {
			currentScore = -negamax(board, time, newDepth, -beta, -alpha);
		}
		else {
			currentScore = -negamax(board, time, newDepth-1, -alpha - 1, -alpha);
			if (currentScore > alpha && currentScore < beta) {
				currentScore = -negamax(board, time, newDepth, -beta, -alpha);
			}
		}

		board.undoMove(move);

		if (currentScore > bestScore) {
			bestScore = currentScore;
			bestMove = move;
			if (depth == chosenDepth) {
				moveToPlay = bestMove;
			}
			alpha = max(currentScore, alpha);
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

	if (currentEntry.depth == 0) {
		board.ttEntries++;
	}

	transpositionTable[currentHash % TT_size] = TTentry(currentHash, bestMove, alpha, depth, boundType);

	return alpha;
}

Move searchBoard(Board board, int time) {
	maxTimeForMove = time / 30;

	cout << "Time\t\tDepth\t\tBest Move\tLookups\t\tTT Entries\tNodes\n";

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
		cout << board.lookups << "\t\t";
		cout << board.ttEntries << "\t\t";
		cout << board.nodes << "\n";
	}

	return moveToPlay;
}