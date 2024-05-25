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

chrono::high_resolution_clock::time_point start;
Board board;

TTentry transpositionTable[TT_size];

int historyTable[2][64][64];
int maxHistory;

Move killerMoves[1024][2];

Move moveToPlay;
int chosenDepth;

int maxTimeForMove = 0;


int negamax(int depth, int plyFromRoot, int alpha, int beta, bool nullMovePruningAllowed) {
	uint64_t currentHash = board.boardStates.back().zobristHash;
	TTentry currentEntry = transpositionTable[currentHash % TT_size];

	bool qsearch = (depth <= 0);
	bool inCheck = board.sideInCheck(board.currentPlayer);
	bool pvNode = (beta - alpha > 1);

	bool notRoot = plyFromRoot > 0;
	int historyIndex = plyFromRoot % 2;

	if (notRoot) {	// Draw check
		if (board.fiftyMoveCheck() || board.repeatedPositionCheck()) {
			return 0;
		}
	}

	if (currentEntry.zobristHash == currentHash && currentEntry.depth >= depth) {
		if (currentEntry.flag == 4 ||	// exact score
			(currentEntry.flag == 2 && currentEntry.score >= beta) ||	// lower bound of score, fail high
			(currentEntry.flag == 1 && currentEntry.score <= alpha)) {	// upper bound, fail low
				board.lookups++;
				return currentEntry.score;
		}
	}

	vector<Move> allMoves = board.generateLegalMovesV2(qsearch);

	if (!qsearch && allMoves.size() == 0) {
		return -900000 - depth;
	}

	int eval = evaluate(board);

	if (qsearch) {
		int bestScore = eval;
		if (bestScore >= beta) {
			return bestScore;
		}
		alpha = max(alpha, bestScore);
	}
	else if (!pvNode && !board.sideInCheck(board.currentPlayer)) {	// Pruning Technique Location
		
	}

	// Order moves portion
	vector<int> moveScores(allMoves.size());
	for (size_t i = 0; i < moveScores.size(); i++) {
		int score = 0;

		if (currentEntry.zobristHash == currentHash && allMoves[i] == currentEntry.m) {
			score = 8000000;
		}
		else if (board.isCapture(allMoves[i])) {
			score += 500000 * (board.rawBoard[allMoves[i].getEndSquare()] % 8) - board.rawBoard[allMoves[i].getStartSquare()] % 8;
		}
		else {
			if (killerMoves[plyFromRoot][1] == allMoves[i]) {
				score = 450000;
			}
			else {
				score = historyTable[board.currentPlayer - 1][allMoves[i].getStartSquare()][allMoves[i].getEndSquare()];
			}
		}

		moveScores[i] = score;
	}

	Move bestMove = Move(0,0,0);
	int newDepth = depth - 1, currentScore, origAlpha = alpha;
	int bestScore = -999999;

	for (uint8_t i = 0; i < allMoves.size(); i++) {
		if (board.nodes & 4096 && chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - start).count() > maxTimeForMove)
			return 900000;

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
		bool importantMoves = (board.isCapture(move) || move.getFlag() > 1 && move.getFlag() < 6);

		board.makeMove(move);
		board.nodes++;
		bool tmpCheckStatus = board.sideInCheck(board.currentPlayer);
		int extensions = 0, reductions = 0;

		// Check Extensions
		if (tmpCheckStatus)
			extensions = 1;

		// Late Move Reduction
		if (!inCheck && !tmpCheckStatus && !importantMoves && i >= 6 && depth > 2) {
			reductions = 1;
		}
		
		if (i == 0) {
			currentScore = -negamax(newDepth + extensions, plyFromRoot + 1, -beta, -alpha, nullMovePruningAllowed);
		}
		else {
			currentScore = -negamax(newDepth - 1 - reductions + extensions, plyFromRoot + 1, -alpha - 1, -alpha, nullMovePruningAllowed);

			if (currentScore > alpha && currentScore < beta) {
				currentScore = -negamax(newDepth + extensions, plyFromRoot + 1, -beta, -alpha, nullMovePruningAllowed);
			}
		}

		board.undoMove(move);

		// new best move
		if (currentScore > bestScore) {
			bestScore = currentScore;
			bestMove = move;
			if (!notRoot) {
				moveToPlay = bestMove;
			}

			alpha = max(currentScore, alpha);


			if (alpha >= beta) {	// Fail High
				if (!qsearch && !board.isCapture(move)) {
					//History and killer move location
					historyTable[historyIndex][move.getStartSquare()][move.getEndSquare()] += depth * depth;
					maxHistory = max(maxHistory, historyTable[historyIndex][move.getStartSquare()][move.getEndSquare()]);

					//killerMoves[plyFromRoot][0] = killerMoves[plyFromRoot][1];
					killerMoves[plyFromRoot][1] == move;
				}
				break;
			}
		}
	}

	int boundType = 0;
	if (alpha >= beta) {
		boundType = 2;
	}
	else {
		if (bestScore > origAlpha) {
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

Move searchBoard(Board &relevantBoard, int time) {
	
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 64; j++) {
			for (int k = 0; k < 64; k++) {
				historyTable[i][j][k] = 0ull;
			}
		}
	}
	
	for (int i = 0; i < 1024; i++) {
		//killerMoves[i][0].rawValue = 0;
		killerMoves[i][1].rawValue = 0;
	}

	maxHistory = 0ull;
	moveToPlay.rawValue = 0;
	
	maxTimeForMove = time / 30;

	cout << "Time\t\tDepth\t\tBest Move\tMax History\tLookups\t\tTT Entries\tNodes\n";

	board = relevantBoard;
	start = chrono::high_resolution_clock::now();

	for (chosenDepth = 1; chosenDepth < 64; chosenDepth++) {
		int score = negamax(chosenDepth, 0, -999999, 999999, true);

		auto end = chrono::high_resolution_clock::now();
		auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
		if (duration > maxTimeForMove)
			break;

		cout << duration << " ms\t\t";
		cout << chosenDepth << "\t\t";
		cout << moveToPlay.printMove() << " \t\t";
		cout << maxHistory << "\t\t";
		cout << board.lookups << "\t\t";
		cout << board.ttEntries << "\t\t";
		cout << board.nodes << "\n";
	}

	return moveToPlay.rawValue == 0 ? board.generateLegalMovesV2(false)[0] : moveToPlay;
}