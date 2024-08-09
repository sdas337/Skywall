#pragma once

// uci.cpp : Defines the entry point for the application..
// Responsible for UCI handling

#include "globals.h"

#include "perft.cpp"
#include "search.cpp"


using namespace std;

void testing() {
	importPerftTest();
	printf("Beginning mass perft test.\n");
	auto start = chrono::high_resolution_clock::now();
	//completePerftTest();
	perftTest();
	auto stop = chrono::high_resolution_clock::now();

	auto duration = chrono::duration_cast<chrono::nanoseconds>(stop - start);
	cout << duration.count() / 1000000 << " milliseconds\n";
	cout << testBoard.nodes << " nodes\n";
	uint64_t nodesPerSecond = (testBoard.nodes * 1000000000ull) / (duration.count());

	printf("%llu nps", nodesPerSecond);

}

void seeTest(Search& currentSearch) {
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

		currentSearch.resetSearch();

		Move currentMove;

		currentSearch.board.setMoveFromString(currentMove, uciMove);

		bool result = currentSearch.see(currentMove, 0);

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

void evalTuningTest(Search &currentSearch) {
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

			currentSearch.board.loadBoardFromFen(fenString);
			cout << fenString << "; [1.0]\n";

			cout << " Eval: " << evaluate2(currentSearch.board) << "\n";

			lineCount++;
		}
		file.close();

		printf("Finished reading in all tests\n");
	}
	
}

void bench(Search &currentSearch, int depth) {
	importPerftTest();

	uint64_t totalNodes = 0ull;

	auto start = chrono::high_resolution_clock::now();

	for (int line = 0; line < 128; line++) {
		currentSearch.resetSearch();;

		printf("Position %d\n", line);
		currentSearch.board.loadBoardFromFen(FENs[line]);

		currentSearch.searchBoard(1000 * 60 * 60, 0, depth);

		printf("\n");

		totalNodes += currentSearch.board.nodes;
		currentSearch.board.nodes = 0ull;
	}

	auto stop = chrono::high_resolution_clock::now();

	auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
	cout << duration.count() << " milliseconds\n";

	cout << totalNodes << " nodes\n";

	uint64_t moveGenSpeed = (totalNodes * 1000) / (duration.count());
	printf("%llu nps\n", moveGenSpeed);

}

void positionHandling(Search& currentSearch, vector<string> &instruction) {
	auto movesIndex = find(instruction.begin(), instruction.end(), "moves");

	if (instruction[1] == "startpos") {
		currentSearch.board.loadBoardFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	}
	else {
		string fenString;
		for (size_t i = 1; i < instruction.size(); i++)
			fenString += instruction[i] + " ";
		currentSearch.board.loadBoardFromFen(fenString);
	}

	if (movesIndex != instruction.end()) {	// moves within the command
		for (auto it = movesIndex + 1; it != instruction.end(); ++it) {
			string currentMove = *it;
			int flag = 0;
			int startSquare = squareNameToValue(currentMove.substr(0, 2));
			int endSquare = squareNameToValue(currentMove.substr(2, 4));

			if (currentSearch.board.rawBoard[startSquare] % 8 == 2) {	// Pawn Move
				if (endSquare == currentSearch.board.boardStates[currentSearch.board.boardStateIndex].enPassantSquare) {
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

			if (currentSearch.board.rawBoard[startSquare] % 8 == 1) {	// king move
				if (abs(endSquare % 8 - startSquare % 8) > 1) {	// Castling
					flag = 6;
				}
			}

			Move toMake(startSquare, endSquare, flag);

			currentSearch.board.makeMove(toMake);
		}
	}
	
	//mainBoard.printBoard();
	return;
}

void optionHandling(Search& currentSearch, string instruction) {
	vector<string> splitString = split(instruction, ' ');

	if (splitString[2] == "Hash") {
		int newSizeMB = stoi(splitString[4]);
		int newSizeBytes = newSizeMB * 1024 * 1024;

		currentSearch.actual_TT_Size = newSizeBytes / sizeof(TTentry);
		currentSearch.transpositionTable.resize(currentSearch.actual_TT_Size);

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

void instructionHandling(Search &currentSearch, string instruction) {
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
		currentSearch.resetSearch();
	}
	else if (splitString[0] == "position") {
		positionHandling(currentSearch, splitString);
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
			if (splitString[i] == "winc" && currentSearch.board.currentPlayer == 1) {
				inc = stoi(splitString[i + 1]);
			}
			if (splitString[i] == "binc" && currentSearch.board.currentPlayer == 2) {
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

		if (currentSearch.board.currentPlayer == 1) {
			time = wtime;
		}
		else {
			time = btime;
		}

		Move result = currentSearch.searchBoard(time, inc, 64);

		/*if (result.getRawValue() == 0) {
			ofstream file("../../../testFiles/debugLogs.txt", ofstream::out | ofstream::app);
			vector<Move> allMoves = mainBoard.generateLegalMovesV2(false);
			for (Move m : allMoves) {
				file << result.printMove() + "\n";
			}

			file.close();

		}*/

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
		optionHandling(currentSearch, instruction);
	}
}

void uciHandling(Search &currentSearch) {

	string instruction;
	while (true) {
		//ofstream file("../../debugLogs.txt", ofstream::out | ofstream::app);
		getline(cin, instruction);
		//file << instruction << "\n";
		//file.close();


		if (instruction == "quit") {
			break;
		}

		instructionHandling(currentSearch, instruction);

	}
}

int main()
{
	Search currentSearch;

	setupLMR();

	//evalTuningTest();
	//seeTest();

	testing();
	//outputTunableJSON();

	//uciHandling(currentSearch);

	//bench(10);

	//bench(currentSearch, 12);

	//mainBoard.loadBoardFromFen("8/8/2k5/3p4/2pK2P1/8/8/8 w - - 0 1");
	//cout << "Evaluation is " << evaluate(mainBoard) << "\n";
	//cout << "Evaluation is " << speedEval(mainBoard) << "\n";

	//initEvalTables();

	//testEval();

	//searchBoard(mainBoard, 1000 * 480, 0, 15);

	//movegenBenchmark();

	return 0;
}
