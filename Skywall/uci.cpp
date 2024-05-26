﻿#pragma once

// uci.cpp : Defines the entry point for the application.

#include "uci.h"
#include "globals.h"

#include "perft.cpp"
#include "search.cpp"


using namespace std;

Board mainBoard;

void testing() {
	importPerftTest();
	printf("Beginning mass perft test.\n");
	auto start = chrono::high_resolution_clock::now();
	completePerftTest();
	//perftTest();
	auto stop = chrono::high_resolution_clock::now();

	auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
	cout << duration.count() << " milliseconds\n";
	cout << testBoard.nodes << " nodes\n";
	uint64_t nodesPerSecond = (testBoard.nodes * 1000ull) / (duration.count());

	printf("%llu nps", nodesPerSecond);

}

void positionHandling(vector<string> &instruction) {
	auto movesIndex = find(instruction.begin(), instruction.end(), "moves");

	if (instruction[1] == "startpos") {
		mainBoard.loadBoardFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	}
	else {
		string fenString;
		for (size_t i = 1; i < instruction.size(); i++)
			fenString += instruction[i] + " ";
		mainBoard.loadBoardFromFen(fenString);
	}

	if (movesIndex != instruction.end()) {	// moves within the command
		for (auto it = movesIndex + 1; it != instruction.end(); ++it) {
			string currentMove = *it;
			int flag = 0;
			int startSquare = squareNameToValue(currentMove.substr(0, 2));
			int endSquare = squareNameToValue(currentMove.substr(2, 4));

			if (mainBoard.rawBoard[startSquare] % 8 == 2) {	// Pawn Move
				if (endSquare == mainBoard.boardStates.back().enPassantSquare) {
					flag = 1;
				}
				else if (currentMove.size() == 5) {
					if (currentMove[4] == 'n')
						flag = 2;
					if (currentMove[4] == 'b')
						flag = 3;
					if (currentMove[4] == 'r')
						flag = 4;
					if (currentMove[4] == 'q')
						flag = 5;
				}
				else if (abs(endSquare / 8 - startSquare / 8) > 1) {	// Double Pawn Push
					flag = 7;
				}
			}

			if (mainBoard.rawBoard[startSquare] % 8 == 1) {	// king move
				if (abs(endSquare % 8 - startSquare % 8) > 1) {	// Castling
					flag = 6;
				}
			}

			Move toMake(startSquare, endSquare, flag);

			mainBoard.makeMove(toMake);
		}
	}
	
	mainBoard.printBoard();
	return;
}

void instructionHandling(string instruction) {
	vector<string> splitString = split(instruction, ' ');

	if (instruction == "uci") {
		cout << "id: name Skywall V0.0\n";
		cout << "id: author Waterwall\n";
		cout << "option name Hash type spin default 1 min 1 max 1\n";
		cout << "option name Threads type spin default 1 min 1 max 64\n";
		cout << "option name SyzygyPath type string default <empty>\n";
		cout << "uciok\n";
	}
	else if (splitString[0] == "isready") {
		cout << "readyok\n";
	}
	else if (splitString[0] == "ucinewgame") {
		mainBoard = Board();
	}
	else if (splitString[0] == "position") {
		positionHandling(splitString);
	}
	else if (splitString[0] == "go") {
		int wtime = 0, btime = 0;
		int maxNodes = 0, depth = 0, inc = 0;

		for (size_t i = 1; i < splitString.size(); i++) {
			if (splitString[i] == "wtime") {
				wtime = stoi(splitString[i+1]);
			}
			if (splitString[i] == "btime") {
				btime = stoi(splitString[i + 1]);
			}
			if (splitString[i] == "winc" && mainBoard.currentPlayer == 1) {
				inc = stoi(splitString[i + 1]);
			}
			if (splitString[i] == "binc" && mainBoard.currentPlayer == 2) {
				inc = stoi(splitString[i + 1]);
			}
			if (splitString[i] == "depth") {
				depth = stoi(splitString[i + 1]);
			}
			if (splitString[i] == "maxNodes") {
				maxNodes = stoi(splitString[i + 1]);
			}
		}

		int time = 0;

		if (mainBoard.currentPlayer == 1) {
			time = wtime + inc;
		}
		else {
			time = btime + inc;
		}

		Move result = searchBoard(mainBoard, time);

		if (result.getRawValue() == 0) {
			ofstream file("../../../testFiles/debugLogs.txt", ofstream::out | ofstream::app);
			vector<Move> allMoves = mainBoard.generateLegalMovesV2(false);
			for (Move m : allMoves) {
				file << result.printMove() + "\n";
			}

			file.close();

		}

		cout << "bestmove " + result.printMove();
		if (result.getFlag() > 1 && result.getFlag() < 7) {
			if (result.getFlag() == 2)
				cout << "n";
			if (result.getFlag() == 3)
				cout << "b";
			if (result.getFlag() == 4)
				cout << "r";
			if (result.getFlag() == 5)
				cout << "q";
		}


		cout << "\n";
	}
}

void uciHandling() {

	string instruction;
	while (true) {
		//ofstream file("../../../testFiles/debugLogs.txt", ofstream::out | ofstream::app);
		getline(cin, instruction);
		//file << instruction << "\n";
		//file.close();


		if (instruction == "quit") {
			break;
		}

		instructionHandling(instruction);

	}
}

int main()
{
	initEvalTables();

	//testing();
	uciHandling();

	//mainBoard.loadBoardFromFen("8/p4p2/6k1/8/5p2/8/3q4/K7 b - - 7 118");

	//cout << "Evaluation is " << evaluate(mainBoard) << "\n";

	//searchBoard(mainBoard, 60000 * 60);

	return 0;
}
