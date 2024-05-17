// Skywall.cpp : Defines the entry point for the application.
//

#include "Skywall.h"
#include "board.cpp"

#include <fstream>

#include <chrono>

using namespace std;

vector<pair<int, uint64_t>> perftTestResults[128];
string FENs[128];

Board testBoard;

int testDepth;

int DEBUG = 0;

uint64_t moveChecker(int depth) {
	uint64_t nodes = 0;
	if (depth == 0) {
		testBoard.nodes++;
		return 1;
	}

	
	vector<Move> allMoves = testBoard.generateLegalMovesV2();
	uint64_t origBoard1 = testBoard.pieceBoards[2];
	uint64_t origBoard2 = testBoard.pieceBoards[3];


	for (uint8_t i = 0; i < allMoves.size(); i++) {
		Move move = allMoves[i];
		
		/*if (move.getStartSquare() == 54 && move.getEndSquare() == 38 && depth == 2) {
			printf("Examine Here\n");
		}
		if (move.getStartSquare() == 11 && move.getEndSquare() == 27 && depth == 3) {
			printf("Examine Here\n");
		}*/

		testBoard.makeMove(move);
		
		uint64_t movesMade = moveChecker(depth - 1);
		nodes += movesMade;

		testBoard.undoMove(move);

		if (testBoard.pieceBoards[2] != origBoard1 || testBoard.pieceBoards[3] != origBoard2) {
			printf("ERROR, ERROR. Make unmake not resetting to same board position. Iteration %d within the generated moves.\n", i);
			move.printMove();
			printf("\n %llu, %llu.\n", origBoard1, origBoard2);
		}

		if (DEBUG == 1 && depth == testDepth) {
			move.printMove();
			printf(" - %llu\n", movesMade);

		}
	}
	return nodes;
}

void importPerftTest() {
	ifstream file("../../../testFiles/standard.epd");

	if (file.is_open()) {	// Reading in all the tests
		int lineCount = 0;
		string line;
		while (getline(file, line)) {
			// using printf() in all tests for consistency

			string fenString = line.substr(0, line.find(";") - 1);
			FENs[lineCount] = fenString;
			//printf("%s\n", fenString.c_str());

			line = line.substr(line.find(";") + 1);
			vector<pair<int, uint64_t>> oneFEN;
			int64_t numTests = count(line.begin(), line.end(), ';');
			for (int i = 0; i < numTests + 1; i++) {
				string movesAtDepth = line.substr(0, line.find(";") - 1);
				uint64_t numMoves = stoi(movesAtDepth.substr(3));
				int depth = stoi(movesAtDepth.substr(1, 2));
				//printf("%d %d\n", depth, numMoves);
				line = line.substr(line.find(";") + 1);

				oneFEN.emplace_back(depth, numMoves);
			}
			perftTestResults[lineCount] = oneFEN;

			// PerftTest at each depth for the given pos


			lineCount++;
		}
		file.close();

		printf("Finished reading in all tests\n");
	}
}

void perftTest() {
	string customPos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	customPos = FENs[1];
	//customPos = "rnbqkbnr/pppppppp/8/8/8/3P4/PPP1PPPP/RNBQKBNR b KQkq - 0 1";
	//customPos = "k7/8/8/8/6Pp/8/K7/8 w - - 0 2";
	//customPos = "k7/8/8/8/6Pp/K7/8/8 b - - 1 2";
	//customPos = "k7/8/8/8/6P1/K6p/8/8 w - - 0 3";

	testDepth = 5;
	testBoard.loadBoardFromFen(customPos);

	// breaks here
	uint64_t result = moveChecker(testDepth);
	

	printf("%llu legal moves\n", result);

	//testBoard.printBoard();


	return;
}

void completePerftTest() {
	// sourced from Ethereal. Need to fully fix up down the line for full perft test.
	int testCount = 0;
	int passedTestCount = 0;

	int lastTest = 128;
	//lastTest = 2;
	for (int line = 0; line < lastTest; line++) {
		testBoard.loadBoardFromFen(FENs[line]);

		//testBoard.precomputeDistances();
		int pairCount = perftTestResults[line].size();
		for(int z = 0; z < pairCount; z++) {
			int depth = perftTestResults[line][z].first;
			uint64_t moveCount = perftTestResults[line][z].second;

			if (depth > 6) {
				continue;
			}

			testDepth = depth;

			uint64_t result = moveChecker(testDepth);

			if (moveCount == result) {
				printf("Position %d at depth %d passed.\n", line, depth);
				passedTestCount++;
			}
			else {
				printf("Position %d at depth %d failed. %llu moves generated vs %llu moves actual\n", line, depth, result, moveCount);
			}

			testCount++;
		}
		printf("\n");
	}

	printf("Total tests: %d / %d\n", passedTestCount, testCount);
	
}

int main()
{
	importPerftTest();
	auto start = chrono::high_resolution_clock::now();
	
	//completePerftTest();

	perftTest();
	
	auto stop = chrono::high_resolution_clock::now();

	auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
	cout << duration.count() << " milliseconds\n";
	cout << testBoard.nodes << " nodes\n";
	uint64_t nodesPerSecond = (testBoard.nodes * 1000ull) / (duration.count());

	printf("%llu nps", nodesPerSecond);

	return 0;
}
