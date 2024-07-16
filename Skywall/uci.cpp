#pragma once

// uci.cpp : Defines the entry point for the application..
// Responsible for UCI handling

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

	auto duration = chrono::duration_cast<chrono::nanoseconds>(stop - start);
	cout << duration.count() / 1000000 << " milliseconds\n";
	cout << testBoard.nodes << " nodes\n";
	uint64_t nodesPerSecond = (testBoard.nodes * 1000000000ull) / (duration.count());

	printf("%llu nps", nodesPerSecond);

}

void seeTest() {
	std::ifstream inputFile("../../../testFiles/SEE.txt");
	

	//                   P    N    B    R    Q    K  NONE
	//SEE_PIECE_VALUES = { 100, 300, 300, 500, 900, 0, 0 };

	int failed = 0, passed = 0;
	int lineCount = 0;

	std::string line;
	while (std::getline(inputFile, line))
	{
		std::stringstream iss(line);
		std::vector<std::string> tokens = splitString(line, '|');

		std::string fen = tokens[0];
		std::string uciMove = tokens[1];
		int gain = stoi(tokens[2]);
		bool expected = gain >= 0;

		Board tmpBoard = Board();
		tmpBoard.loadBoardFromFen(fen);
		searchBoard(tmpBoard, 1000 * 480, 0, 0);

		Move currentMove;

		tmpBoard.setMoveFromString(currentMove, uciMove);

		bool result = see(currentMove, 0);

		if (result == expected)
			passed++;
		else {
			std::cout << "FAILED " << fen << " | " << uciMove << " | Gain: " << gain << std::endl;
			failed++;
		}
		lineCount++;
	}

	inputFile.close();
	std::cout << "Passed: " << passed << std::endl;
	std::cout << "Failed: " << failed << std::endl;
}

void evalTuningTest() {
	ifstream file("../../../testFiles/uciTestFens.epd");

	if (file.is_open()) {	// Reading in all the tests
		int lineCount = 0;
		string line;
		while (getline(file, line)) {
			// using printf() in all tests for consistency

			if (lineCount > 0) {
				break;
			}

			string fenString = line.substr(0, line.find(";") - 1);
			//FENs[lineCount] = fenString;
			//printf("%s\n", fenString.c_str());

			mainBoard.loadBoardFromFen(fenString);
			cout << fenString << "; [1.0]\n";

			cout << " Eval: " << evaluate(mainBoard) << "\n";


			lineCount++;
		}
		file.close();

		printf("Finished reading in all tests\n");
	}
	
}

void bench(int depth) {
	importPerftTest();

	auto start = chrono::high_resolution_clock::now();

	for (int line = 0; line < 128; line++) {
		for (uint32_t i = 0; i < actual_TT_Size; i++)
			transpositionTable[i] = TTentry();

		printf("Position %d\n", line);
		testBoard.loadBoardFromFen(FENs[line]);

		searchBoard(testBoard, 1000 * 60 * 60, 0, depth);

		printf("\n");

		testBoard.nodes = 0ull;
	}

	auto stop = chrono::high_resolution_clock::now();

	auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
	cout << duration.count() << " milliseconds\n";

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
	
	//mainBoard.printBoard();
	return;
}

void optionHandling(string instruction) {
	vector<string> splitString = split(instruction, ' ');

	if (splitString[2] == "Hash") {
		int newSizeMB = stoi(splitString[4]);
		int newSizeBytes = newSizeMB * 1024 * 1024;

		actual_TT_Size = newSizeBytes / sizeof(TTentry);
		transpositionTable.resize(actual_TT_Size);

		// One day will do
	}
	else {	// Tunables
		for (auto toTune : allTunables) {
			if (toTune->name == splitString[2]) {
				toTune->value = stoi(splitString[4]);

				if (toTune->name == "lmrBase" || toTune->name == "lmrDivisor") {
					setupLMR();
				}
			}
		}
	}
	
}

void instructionHandling(string instruction) {
	vector<string> splitString = split(instruction, ' ');

	if (instruction == "uci") {
		cout << "id: name Skywall V0.0\n";
		cout << "id: author Waterwall\n";
		cout << "option name Hash type spin default 16 min 1 max 64\n";
		cout << "option name Threads type spin default 1 min 1 max 1\n";
		//cout << "option name SyzygyPath type string default <empty>\n";
		outputTunableOptions();
		cout << "uciok\n";
	}
	else if (splitString[0] == "isready") {
		cout << "readyok\n";
	}
	else if (splitString[0] == "ucinewgame") {
		mainBoard = Board();
		for(uint32_t i = 0; i < actual_TT_Size; i++)
			transpositionTable[i] = TTentry();
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
			time = wtime;
		}
		else {
			time = btime;
		}

		Move result = searchBoard(mainBoard, time, inc, 64);

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
	else if (splitString[0] == "setoption") {
		optionHandling(instruction);
	}
}

void uciHandling() {

	string instruction;
	while (true) {
		//ofstream file("../../debugLogs.txt", ofstream::out | ofstream::app);
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
	setupLMR();

	transpositionTable.resize(actual_TT_Size);

	//evalTuningTest();
	//seeTest();

	//testing();
	//outputTunableJSON();
	//uciHandling();

	//bench(10);

	//bench(15);

	//mainBoard.loadBoardFromFen("8/8/2k5/3p4/2pK2P1/8/8/8 w - - 0 1");
	//cout << "Evaluation is " << evaluate(mainBoard) << "\n";
	//cout << "Evaluation is " << speedEval(mainBoard) << "\n";

	//initEvalTables();

	//testEval();

	//searchBoard(mainBoard, 1000 * 480, 0, 15);

	movegenBenchmark();

	return 0;
}
