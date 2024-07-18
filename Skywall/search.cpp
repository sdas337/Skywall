#include "globals.h"

#include "board.cpp"
#include "eval.cpp"

using namespace std;

extern int lmrReductions[256][256];

uint32_t actual_TT_Size = 1048576;

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

chrono::high_resolution_clock::time_point start;

vector<TTentry> transpositionTable;

int historyTable[2][64][64];
int qhistoryTable[2][64][64][5];

Move counterMoves[64][64];

Move killerMoves[1024][2];

Move moveToPlay;

int maxTimeForMove = 0;

struct StackEntry {
	bool inCheck;
	int evalScore;
};

StackEntry searchStack[1024];

int testSeeValues[7] = { 0, 0, 100, 300, 300, 500, 900 };

// Sourcing SEE based on clarity and stormphrax
int estimatedMoveValue(Board& board, Move m, int flag) {
	int value = seeValues[board.rawBoard[m.getEndSquare()] % 8]->value;

	if (flag > 1 && flag < 6) {
		value += seeValues[flag + 1]->value - seeValues[2]->value;
	}

	if (flag == 1) {
		value = seeValues[2]->value;
	}

	return value;
}

static bool see(Board& board, Move m, int threshhold) {
	int startSquare = m.getStartSquare();
	int endSquare = m.getEndSquare();
	int flag = m.getFlag();

	int victim = board.rawBoard[endSquare] % 8;
	int nextVictim = board.rawBoard[startSquare] % 8;

	if (flag > 1 && flag < 6) {
		nextVictim = flag + 1;
	}

	int balance = estimatedMoveValue(board, m, flag) - threshhold;

	if (balance < 0)
		return false;

	balance -= seeValues[nextVictim]->value;

	if (balance >= 0)
		return true;

	uint64_t occupied = board.occupiedBoard[1] | board.occupiedBoard[2];
	occupied = (occupied ^ (1ull << startSquare)) | (1ull << endSquare);

	if (flag == 1) {
		occupied ^= (1ull << board.boardStates.back().enPassantSquare);
	}

	int color = board.currentPlayer % 2 + 1;

	// Get pieces for revealed attackers
	uint64_t bishops = board.pieceBoards[4] | board.pieceBoards[6];
	uint64_t rooks = board.pieceBoards[5] | board.pieceBoards[6];

	uint64_t attackers = board.getAttackers(endSquare, occupied) & occupied;

	while (true) {
		uint64_t myAttackers = attackers & board.occupiedBoard[color];
		if (myAttackers == 0ull) {
			break;
		}

		uint64_t relevantAttackerBoard = 0ull;
		// find lowest value attacker. Begins from pawn and goes to queen
		for (int index = 1; index <= 6; index++) {
			nextVictim = index % 6 + 1;
			relevantAttackerBoard = myAttackers & (board.pieceBoards[nextVictim]);
			if(relevantAttackerBoard != 0) {
				break;
			}
		}

		// Make the move on the occupied board only
		occupied ^= (1ull << countr_zero(relevantAttackerBoard));

		// if the next victim is a pawn, bishop or queen (diagonal capture)
		if (nextVictim == 2 || nextVictim == 4 || nextVictim == 6) {	// update diagonal attacks
			attackers |= board.generateBishopMoves(endSquare, occupied) & bishops;
		}

		// if the next victim is a rook or queen (orthogonal capture)
		if (nextVictim == 5 || nextVictim == 6) {
			attackers |= board.generateRookMoves(endSquare, occupied) & rooks;
		}

		attackers &= occupied;

		// update balance
		balance = -balance - 1 - seeValues[nextVictim]->value;

		color = color % 2 + 1;

		if (balance >= 0) {
			// performing small legality check for king
			if (nextVictim == 1) {
				if ((attackers & board.occupiedBoard[color]) != 0) {
					color = color % 2 + 1;
				}
			}
			break;
		}
	}

	// If the color is different, you lost the exchange
	return board.currentPlayer != color;
}

int qsearch(Board& board, int depth, int plyFromRoot, int alpha, int beta) {
	uint64_t currentHash = board.boardStates.back().zobristHash;
	TTentry currentEntry = transpositionTable[currentHash % actual_TT_Size];

	bool pvNode = (beta - alpha) > 1;
	int historyIndex = plyFromRoot % 2;
	int bestScore = -999999;

	// maybe check repeated pos
	if (currentEntry.zobristHash == currentHash) {
		if (!pvNode) {	// test qsearch without
			if (currentEntry.flag == 4 ||	// exact score
				(currentEntry.flag == 2 && currentEntry.score >= beta) ||	// lower bound of score, fail high
				(currentEntry.flag == 1 && currentEntry.score <= alpha)) {	// upper bound, fail low
				board.lookups++;
				return currentEntry.score;
			}
		}
	}

	bestScore = evaluate(board);

	if (bestScore >= beta) {
		return bestScore;
	}
	alpha = max(alpha, bestScore);

	vector<Move> allMoves;
	allMoves = board.generateLegalMovesV2(true);

	vector<int> moveScores(allMoves.size());
	for (size_t i = 0; i < moveScores.size(); i++) {
		int score = 0;

		if (currentEntry.zobristHash == currentHash && allMoves[i] == currentEntry.m) {	// TT Table
			score = 8000000;
		}
		else {	// MVV-LVA
			int victim = board.rawBoard[allMoves[i].getEndSquare()] % 8;
			score += 500000 * (victim) - 20000 * (board.rawBoard[allMoves[i].getStartSquare()] % 8);
			score += qhistoryTable[historyIndex][allMoves[i].getStartSquare()][allMoves[i].getEndSquare()][victim - 2];
		}

		moveScores[i] = score;
	}

	Move bestMove = Move(0, 0, 0);
	int currentScore, origAlpha = alpha;

	for (uint8_t i = 0; i < allMoves.size(); i++) {
		if ((board.nodes & 4095) == 0 && chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - start).count() > maxTimeForMove)
			return 900000;

		// Performing selection sort based on move scores above to order
		for (uint8_t j = i + 1; j < allMoves.size(); j++) {
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

		bool seeResult = !see(board, move, 0);
		if (seeResult) {	// Ignoring bad captures during qsearch
			continue;
		}

		board.makeMove(move);
		board.nodes++;

		currentScore = -qsearch(board, depth - 1, plyFromRoot + 1, -beta, -alpha);

		board.undoMove(move);

		//cout << move.printMove() << " has a score of " << currentScore << ". Depth is " << depth << "\n";

		// new best move
		if (currentScore > bestScore) {
			bestScore = currentScore;
			bestMove = move;

			alpha = max(currentScore, alpha);

			if (alpha >= beta) {	// Fail High
				int victim = board.rawBoard[move.getEndSquare()] % 8 - 2;

				int& value = qhistoryTable[historyIndex][move.getStartSquare()][move.getEndSquare()][victim];

				int bonus = min(qhstConst.value, qhstQuad.value * depth * depth + qhstLin.value * depth + qhstConst.value);
				bonus = bonus - value * abs(bonus) / 16384;
				value += bonus;

				for (int z = 0; z < i; z++) {
					move = allMoves[z];
					int& value = qhistoryTable[historyIndex][move.getStartSquare()][move.getEndSquare()][board.rawBoard[move.getEndSquare()] % 8 - 2];

					int malus = -min(qhstConst.value, qhstQuad.value * depth * depth + qhstLin.value * depth + qhstConst.value);
					malus = malus - value * abs(malus) / 16384;
					value += malus;
				}
				break;
			}
		}
	}

	uint8_t boundType = 0;
	if (bestScore >= beta) {
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

	if (boundType == 1) {
		bestMove = transpositionTable[currentHash % actual_TT_Size].m;
	}
	if (transpositionTable[currentHash % actual_TT_Size].depth <= 0) {
		board.ttEntries++;
	}

	transpositionTable[currentHash % actual_TT_Size] = TTentry(currentHash, bestMove, bestScore, 0, boundType);

	return bestScore;
}

int negamax(Board& board, int depth, int plyFromRoot, int alpha, int beta, bool nullMovePruningAllowed, Move priorMove) {
	bool inCheck = board.sideInCheck(board.currentPlayer);
	bool qsearchStatus = depth <= 0;

	searchStack[plyFromRoot].inCheck = inCheck;

	if (qsearchStatus && !inCheck) {
		return qsearch(board, depth, plyFromRoot, alpha, beta);
	}

	uint64_t currentHash = board.boardStates.back().zobristHash;
	TTentry currentEntry = transpositionTable[currentHash % actual_TT_Size];

	bool pvNode = (beta - alpha) > 1;
	bool notRoot = plyFromRoot > 0;

	int historyIndex = plyFromRoot % 2;
	int bestScore = -999999;

	if (notRoot) {
		if (board.repeatedPositionCheck() || board.fiftyMoveCheck()) {
			return 0;
		}
	}

	if (currentEntry.zobristHash == currentHash) {
		if (!pvNode && currentEntry.depth >= depth) {
			if (currentEntry.flag == 4 ||	// exact score
				(currentEntry.flag == 2 && currentEntry.score >= beta) ||	// lower bound of score, fail high
				(currentEntry.flag == 1 && currentEntry.score <= alpha)) {	// upper bound, fail low
				board.lookups++;
				return currentEntry.score;
			}
		}
	}
	else if (depth > iirDepth.value) {	// Internal iterative reduction
		depth--;
	}

	int eval = evaluate(board);

	searchStack[plyFromRoot].evalScore = eval;
	bool improving = false;
	if (inCheck) {
		improving = false;
	}
	else if (plyFromRoot > 1 && !searchStack[plyFromRoot - 2].inCheck) {
		improving = eval > searchStack[plyFromRoot - 2].evalScore;
	}
	else if (plyFromRoot > 3 && !searchStack[plyFromRoot - 4].inCheck) {
		improving = eval > searchStack[plyFromRoot - 4].evalScore;
	}
	else {
		improving = true;
	}


	if (!pvNode && !inCheck) {	// Pruning Technique Location
		// Testing out rf pruning
		int rfPruningMargin = (rfPruningBase.value) * depth;
		if (depth <= rfpDepth.value && eval - rfPruningMargin >= beta) {
			return eval - rfPruningMargin;
		}

		if (nullMovePruningAllowed && depth >= nmpDepth.value) {
			board.makeNullMove();
			int nullMoveScore = -negamax(board, depth - nmpBaseReduction.value - depth / nmpScaleReduction.value, plyFromRoot + 1, -beta, -alpha, false, Move());
			board.undoNullMove();

			if (nullMoveScore >= beta) {
				return nullMoveScore;
			}
		}
	}

	vector<Move> allMoves;
	
	allMoves = board.generateLegalMovesV2(false);

	if (allMoves.size() == 0) {
		if (inCheck)
			return -900000 + plyFromRoot;
		return 0;
	}

	// Order moves portion
	vector<int> moveScores(allMoves.size());
	for (size_t i = 0; i < moveScores.size(); i++) {
		int score = 0;

		if (currentEntry.zobristHash == currentHash && allMoves[i] == currentEntry.m) {	// TT Table
			score = 8000000;
		}
		else if (board.isCapture(allMoves[i])) {	// MVV + SEE
			score += mvvValues[(board.rawBoard[allMoves[i].getEndSquare()] % 8)]->value;	// MVV

			if (see(board, allMoves[i], 0)) {	// good captures
				score += 500000;
			}
			else {
				score += 50000;
			}
		}
		else {
			if (killerMoves[plyFromRoot][0] == allMoves[i] || killerMoves[plyFromRoot][1] == allMoves[i]) {	// Killer Moves
				score = 100000;
			}
			else if (allMoves[i] == counterMoves[priorMove.getStartSquare()][priorMove.getEndSquare()]) {	// Counter moves
				score = 99000;
			}
			else {	// History
				score = historyTable[board.currentPlayer - 1][allMoves[i].getStartSquare()][allMoves[i].getEndSquare()];
			}
		}

		moveScores[i] = score;
	}

	Move bestMove = Move();
	int currentScore, origAlpha = alpha;

	bool futilePruning = depth <= fpDepth.value && (eval + fpScale.value * depth + fpMargin.value) <= alpha;

	int lmpMoves = 100;
	int quietNodes = 0;
	if (depth < lmpDepth.value) {
		lmpMoves =  lmpQuad.value * depth * depth / 100  + lmpScale.value * depth + lmpBase.value;
	}

	for (uint8_t i = 0; i < allMoves.size(); i++) {
		if ((board.nodes & 4095) == 0 && chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - start).count() > maxTimeForMove)
			return 900000;

		// Performing selection sort based on move scores above to order
		for (uint8_t j = i + 1; j < allMoves.size(); j++) {
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
		bool importantMoves = (board.isCapture(move) || move.isPromotion());

		// Pruning moves within the loop
		if (futilePruning && !importantMoves && i > 0) {
			break;
		}

		// See Pruning
		if (!pvNode && !inCheck) {
			if (depth < seePDepth.value && !see(board, move, depth * (board.isCapture(move) ? seeCPScale.value : seeQPScale.value)) ) {
				continue;
			}
		}

		// Late Move Pruning
		if (!importantMoves && !pvNode) {
			if (depth < lmpDepth.value) {
				if (quietNodes > lmpMoves) {
					break;
				}
			}
			quietNodes++;

			// History Pruning
			/*if (plyFromRoot > 0 && depth < 6 && moveScores[i] < -1500 * depth - 1000) {
				break;
			}*/
		}

		board.makeMove(move);

		board.nodes++;
		bool tmpCheckStatus = board.sideInCheck(board.currentPlayer);
		int extensions = 0, reductions = 0;

		// Check Extensions
		if (tmpCheckStatus)
			extensions = 1;

		int newDepth = depth - 1 + extensions;

		// Late Move Reduction
		if (!inCheck && !tmpCheckStatus && !importantMoves && i >= lmrMoveCount.value && depth > lmrDepth.value) {
			reductions = lmrReductions[depth][i];

			//reductions += !improving;
		}
		
		if (i == 0) {
			currentScore = -negamax(board, newDepth, plyFromRoot + 1, -beta, -alpha, nullMovePruningAllowed, move);
		}
		else {
			currentScore = -negamax(board, newDepth - reductions, plyFromRoot + 1, -alpha - 1, -alpha, nullMovePruningAllowed, move);

			if (currentScore > alpha && reductions > 0) {
				currentScore = -negamax(board, newDepth, plyFromRoot + 1, -alpha - 1, -alpha, nullMovePruningAllowed, move);
			}

			if (currentScore > alpha && pvNode) {
				currentScore = -negamax(board, newDepth, plyFromRoot + 1, -beta, -alpha, nullMovePruningAllowed, move);
			}
		}

		board.undoMove(move);

		//cout << move.printMove() << " has a score of " << currentScore << ". Depth is " << depth << "\n";

		// new best move
		if (currentScore > bestScore) {
			bestScore = currentScore;
			bestMove = move;

			if (!notRoot) {
				moveToPlay = bestMove;
			}

			alpha = max(currentScore, alpha);


			if (alpha >= beta) {	// Fail High
				if (!board.isCapture(move)) {
					//History and killer move location
					killerMoves[plyFromRoot][0] = killerMoves[plyFromRoot][1];
					killerMoves[plyFromRoot][1] = move;

					counterMoves[priorMove.getStartSquare()][priorMove.getEndSquare()] = move;	// counter moves

					int& value = historyTable[historyIndex][move.getStartSquare()][move.getEndSquare()];

					int bonus = min(hstMin.value, hstQuad.value * depth * depth + hstLin.value * depth + hstConst.value);
					bonus = bonus - value * abs(bonus) / 16384;
					value += bonus;

					for (int z = 0; z < i; z++) {
						move = allMoves[z];
						if (!board.isCapture(move)) {
							int& value = historyTable[historyIndex][move.getStartSquare()][move.getEndSquare()];

							int malus = -min(hstMin.value, hstQuad.value * depth * depth + hstLin.value * depth + hstConst.value);
							malus = malus - value * abs(malus) / 16384;
							value += malus;
						}
					}
				}
				break;
			}
		}
	}

	uint8_t boundType = 0;
	if (bestScore >= beta) {
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

	if (boundType == 1) {
		bestMove = transpositionTable[currentHash % actual_TT_Size].m;
	}
	if (transpositionTable[currentHash % actual_TT_Size].depth <= 0) {
		board.ttEntries++;
	}

	transpositionTable[currentHash % actual_TT_Size] = TTentry(currentHash, bestMove, bestScore, depth, boundType);
	
	//cout << "Added move " << bestMove.printMove() << " to the transposition table.\n";

	return bestScore;
}

Move searchBoard(Board &relevantBoard, int time, int inc, int maxDepth) {
	/*for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 64; j++) {
			for (int k = 0; k < 64; k++) {
				historyTable[i][j][k] = 0ull;
			}
		}
	}*/
	
	for (int i = 0; i < 1024; i++) {
		killerMoves[i][0].rawValue = 0;
		killerMoves[i][1].rawValue = 0;
		searchStack[i] = StackEntry();
	}

	for (int i = 0; i < 64; i++) {		// Reset counter moves between searches
		for (int j = 0; j < 64; j++) {
			counterMoves[i][j] = Move();
		}
	}

	moveToPlay.rawValue = 0;
	
	maxTimeForMove = time / hardTC.value;
	//maxTimeForMove = time / 30;
	int softTimeBound = (int)( ((double)tcMul.value / 100) * (time * timeMul.value / 100 + inc * incMul.value / 100));
	//softTimeBound = time / 30;

	if (maxDepth > 1) {
		cout << "Time\t\tDepth\t\tBest Move\tScore\t\tLookups\t\tTT Entries\tNodes\n";
	}

	start = chrono::high_resolution_clock::now();

	for (int chosenDepth = 1, alpha = -999999, beta = 999999; chosenDepth < maxDepth;) {
		int score = negamax(relevantBoard, chosenDepth, 0, alpha, beta, true, Move());

		auto end = chrono::high_resolution_clock::now();
		auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
		if (duration > softTimeBound)
			break;


		if (score <= alpha)
			alpha -= aspDelta.value;
		else if (score >= beta)
			beta += aspDelta.value;
		else {
			cout << duration << " ms\t\t";
			cout << chosenDepth << "\t\t";
			cout << moveToPlay.printMove() << " \t\t";
			cout << score << "\t\t";
			cout << relevantBoard.lookups << "\t\t";
			cout << relevantBoard.ttEntries << "\t\t";
			cout << relevantBoard.nodes << "\n";

			chosenDepth++;
			alpha = score - aspWindow.value;
			beta = score + aspWindow.value;

		}
	}

	return moveToPlay.rawValue == 0 ? relevantBoard.generateLegalMovesV2(false)[0] : moveToPlay;
}