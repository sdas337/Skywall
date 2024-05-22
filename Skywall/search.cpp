#include "globals.h"
#include "board.cpp"

using namespace std;

Move searchBoard(Board board, int time) {
	Move bestMove;

	vector<Move> allMoves = board.generateLegalMovesV2();

	bestMove = allMoves[0];

	return bestMove;
}