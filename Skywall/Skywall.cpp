// Skywall.cpp : Defines the entry point for the application.
//

#include "Skywall.h"
#include "board.cpp"

#include <fstream>

#include <chrono>

using namespace std;

Board testBoard;

int testDepth;

uint64_t moveChecker(int depth) {
	uint64_t nodes = 0;
	if (depth == 0) {
		testBoard.nodes++;
		return 1;
	}

	vector<Move> allMoves = testBoard.generateLegalMovesV2();

	/*if (allMoves.size() < 20) {
		printf("Less than 20 possible moves\n");
	}*/

	for (int i = 0; i < allMoves.size(); i++) {
		Move move = allMoves[i];
		testBoard.makeMove(move);
		/*for (int j = 0; j < 2 - depth; j++) {
			printf("\t");
		}
		move.printMove();
		printf("\n");*/
		uint64_t movesMade = moveChecker(depth - 1);
		nodes += movesMade;

		testBoard.undoMove(move);

		if (depth == testDepth) {
			move.printMove();
			printf(" - %d\n", movesMade);

		}

		/*if (testBoard.rawBoard[49] != 18) {
			printf("Warning\n");
		}*/
	}
	return nodes;
}

void perftTest() {

	int perftTestResults[128][6];
	
	// sourced from Ethereal. Need to fully fix up down the line for full perft test.

	/*ifstream file("../../../testFiles/standard.epd");

	if (file.is_open()) {
		int lineCount = 0;
		string line;
		while (getline(file, line)) {
			// using printf() in all tests for consistency

			string fenString = line.substr(0, line.find(";") - 1);
			//printf("%s\n", fenString.c_str());

			line = line.substr(line.find(";") + 1);
			for (int i = 1; i <= 6; i++) {
				string movesAtDepth = line.substr(0, line.find(";") - 1);
				int numMoves = stoi(movesAtDepth.substr(3));
				printf("%d\n", numMoves);
				line = line.substr(line.find(";") + 1);

				perftTestResults[lineCount][i - 1] = numMoves;
			}

			// PerftTest at each depth for the given pos


			lineCount++;
		}
		file.close();
	}
	*/


	//string customPos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	//customPos = "rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR b KQkq - 0 1";
	//customPos = "rnbqkbnr/p1pppppp/8/1p6/3P4/8/PPP1PPPP/RNBQKBNR w KQkq - 0 1";
	//customPos = "rnbqkbnr/p1pppppp/8/1p6/3P4/8/PPPKPPPP/RNBQ1BNR b kq - 0 1";
	//customPos = "rnbqkbnr/p1pppppp/8/8/1p1P4/8/PPPKPPPP/RNBQ1BNR w kq - 0 1";

	testDepth = 5;
	//testBoard.loadBoardFromFen(customPos);

	// breaks here
	uint64_t result = moveChecker(testDepth);
	

	printf("%llu legal moves\n", result);

	//testBoard.printBoard();


	return;
}

int main()
{
	auto start = chrono::high_resolution_clock::now();
	for (int i = 0; i < 10; i++) {
		perftTest();
	}
	
	auto stop = chrono::high_resolution_clock::now();

	auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
	cout << duration.count() << " milliseconds\n";

	return 0;
}
