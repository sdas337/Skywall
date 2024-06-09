#pragma once

#include "board.cpp"

vector<pair<int, uint64_t>> perftTestResults[128];
string FENs[128];

Board testBoard;

int testDepth;

int DEBUG = 1;

uint64_t moveChecker(int depth, bool testingCaptures) {
	uint64_t nodes = 0;
	if (depth == 0) {
		testBoard.nodes++;
		return 1;
	}

	vector<Move> allMoves = testBoard.generateLegalMovesV2(testingCaptures);

	for (uint8_t i = 0; i < allMoves.size(); i++) {
		Move move = allMoves[i];

		testBoard.makeMove(move);

		uint64_t movesMade = moveChecker(depth - 1, testingCaptures);
		nodes += movesMade;

		testBoard.undoMove(move);


		if (DEBUG == 1 && depth == testDepth) {
			cout << move.printMove() << " - " << movesMade << "\n";
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
	string customPos = "r1b4r/ppq1nppp/4p3/2k1P3/3QB3/P4N2/2P2PPP/R1B1R1K1 b - - 1 16";
	
	testDepth = 4;
	testBoard.loadBoardFromFen(FENs[1]);

	// breaks here
	uint64_t result = moveChecker(testDepth, true);


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
		for (int z = 0; z < pairCount; z++) {
			int depth = perftTestResults[line][z].first;
			uint64_t moveCount = perftTestResults[line][z].second;

			if (depth > 6) {
				continue;
			}

			testDepth = depth;

			uint64_t result = moveChecker(testDepth, false);

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

void movegenBenchmark() {
	importPerftTest();

	auto start = chrono::high_resolution_clock::now();
	for (int line = 0; line < 128; line++) {
		testBoard.loadBoardFromFen(FENs[line]);

		for (int repCount = 0; repCount < 524288; repCount++) {
			vector<Move> allMoves = testBoard.generateLegalMovesV2(false);
		}
	}
	auto stop = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);

	cout << "Movegen Time: " << duration.count() << " microseconds\n";
	double moveGenSpeed = (67108864.0) / (duration.count());
	printf("Movegen speed: %f gps\n\n\n", moveGenSpeed);

	start = chrono::high_resolution_clock::now();
	double moveCount = 0;
	for (int line = 0; line < 128; line++) {
		testBoard.loadBoardFromFen(FENs[line]);

		vector<Move> allMoves = testBoard.generateLegalMovesV2(false);
		for (int repCount = 0; repCount < 524288; repCount++) {
			for (Move move : allMoves) {
				testBoard.makeMove(move);
				moveCount++;
				testBoard.undoMove(move);
			}
		}
	}
	stop = chrono::high_resolution_clock::now();
	duration = chrono::duration_cast<chrono::microseconds>(stop - start);

	cout << "MakeMove Time: " << duration.count() << " microseconds\n";
	double makeMoveTime = moveCount / (duration.count());
	printf("MakeMove speed: %f mps", makeMoveTime);
}
