#pragma once

#include "globals.h"

#include "board.cpp"

#include "eval.h"

vector<pair<int, uint64_t>> perftTestResults[128];
string FENs[128];

Board testBoard;

int testDepth;

int DEBUG = 0;

uint64_t moveChecker(int depth, bool testingCaptures) {
	uint64_t nodes = 0;

	if (depth == 0) {
		testBoard.nodes++;
		return 1;
	}

	vector<Move> allMoves = testBoard.generateLegalMovesV2(testingCaptures);

	for (uint8_t i = 0; i < allMoves.size(); i++) {
		Move move = allMoves[i];

		//cout << testBoard.zobristHashCalc() << "\n";

		testBoard.makeRawMove(move);

		/*if (move.getEndSquare() == 35 && move.getStartSquare() == 51) {
			cout << "Here\n";
		}*/

		uint64_t movesMade = moveChecker(depth - 1, testingCaptures);
		nodes += movesMade;

		testBoard.undoRawMove(move);

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
	
	testDepth = 5;
	testBoard.loadBoardFromFen(FENs[1]);
	//testBoard.loadBoardFromFen("rnbqkbnr/pppppppp/8/8/1P6/8/P1PPPPPP/RNBQKBNR b KQkq b3 0 1");
	//testBoard.loadBoardFromFen("rnbqkbnr/1ppppppp/p7/8/1P6/8/P1PPPPPP/RNBQKBNR w KQkq - 0 2");
	//testBoard.loadBoardFromFen("rnbqkbnr/1ppppppp/p7/1P6/8/8/P1PPPPPP/RNBQKBNR b KQkq - 0 2");
	//testBoard.loadBoardFromFen("rnbqkbnr/1pp1pppp/p7/1P1p4/8/8/P1PPPPPP/RNBQKBNR w KQkq d6 0 3");

	// breaks here
	uint64_t result = moveChecker(testDepth, false);


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
		size_t pairCount = perftTestResults[line].size();
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
		//cout << line << "\n";
		testBoard.loadBoardFromFen(FENs[line]);

		for (int repCount = 0; repCount < 5242880; repCount++) {
			vector<Move> allMoves(256);
			int index = 0;
			uint64_t pawnBoard = testBoard.occupiedBoard[1] & testBoard.pieceBoards[2];

			while (pawnBoard != 0) {
				int square = popLSB(pawnBoard);
				index += testBoard.generatePawnMovesV2(square, allMoves, index, 1);
			}

			pawnBoard = testBoard.occupiedBoard[2] & testBoard.pieceBoards[2];

			while (pawnBoard != 0) {
				int square = popLSB(pawnBoard);
				index += testBoard.generatePawnMovesV2(square, allMoves, index, 2);
			}

			//vector<Move> allMoves = testBoard.generateLegalMovesV2(false);

		}
	}
	auto stop = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);

	cout << "New Movegen Time: " << duration.count() << " microseconds\n";
	double moveGenSpeed = (671088640.0) / (duration.count());
	printf("New Movegen speed: %f gps\n\n\n", moveGenSpeed);

	start = chrono::high_resolution_clock::now();
	for (int line = 0; line < 128; line++) {
		//cout << line << "\n";
		testBoard.loadBoardFromFen(FENs[line]);

		for (int repCount = 0; repCount < 5242880; repCount++) {
			vector<Move> allMoves(256);
			int index = 0;
			testBoard.generatePawnMovesV3(allMoves, index, 1);
			testBoard.generatePawnMovesV3(allMoves, index, 2);
			//vector<Move> allMoves = testBoard.generateLegalMovesV2(false);

		}
	}
	stop = chrono::high_resolution_clock::now();
	duration = chrono::duration_cast<chrono::microseconds>(stop - start);

	cout << "Old Movegen Time: " << duration.count() << " microseconds\n";
	moveGenSpeed = (671088640.0) / (duration.count());
	printf("Old Movegen speed: %f gps\n\n\n", moveGenSpeed);

	start = chrono::high_resolution_clock::now();
	double moveCount = 0;
	for (int line = 0; line < 128; line++) {
		testBoard.loadBoardFromFen(FENs[line]);

		vector<Move> allMoves = testBoard.generateLegalMovesV2(false);
		for (int repCount = 0; repCount < 32678; repCount++) {
			for (Move move : allMoves) {
				testBoard.makeRawMove(move);
				moveCount++;
				testBoard.undoRawMove(move);
			}
		}
	}
	stop = chrono::high_resolution_clock::now();
	duration = chrono::duration_cast<chrono::microseconds>(stop - start);

	cout << "MakeMove Time: " << duration.count() << " microseconds\n";
	double makeMoveTime = moveCount / (duration.count());
	printf("MakeMove speed: %f mps", makeMoveTime);
}
