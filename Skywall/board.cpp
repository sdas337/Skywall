#pragma once

#include "move.cpp"
#include "magics.cpp"

#include "globals.h"


using namespace std;

struct BoardStateInformation {
	int enPassantSquare;
	bool castlingRights[4];
	int fiftyMoveCount;
	int capturedPieceType;
	uint64_t zobristHash;
	uint64_t occupiedBoard[2];
	uint64_t pieceBoards[6];
};

class Board {
public:
	// A0 = 0, H0 = 7, H8 = 63
	int rawBoard[64];

	uint64_t nodes = 0;
	uint64_t lookups = 0;
	uint64_t ttEntries = 0;

	int plyCount;
	int currentPlayer;		// Changing so that 0 is white, 1 is black

	int kingLocations[2];
	uint64_t attackingSquares[2];

	vector<BoardStateInformation> boardStates;

	// no piece  = 0, king = 1, pawn = 2, knight = 3, bishop = 4, rook = 5, queen = 6, white = 8, black = 16
	void setPiece(int row, int col, int piece) {
		int square = 8 * row + col;

		rawBoard[square] = piece;
		boardStates.back().occupiedBoard[piece / 8 - 1] |= (1ull) << square;
		boardStates.back().pieceBoards[piece % 8 - 1] |= (1ull) << square;
	}

	void removePiece(int row, int col) {
		int square = 8 * row + col;
		boardStates.back().occupiedBoard[0] &= ~(1ull << square);
		boardStates.back().occupiedBoard[1] &= ~(1ull << square);
		rawBoard[square] = 0;

		for(int i = 0; i < 6; i++)
			boardStates.back().pieceBoards[i] &= ~((1ull) << square);
	}

	bool isWhitePiece(int row, int col) {
		return boardStates.back().occupiedBoard[0] & (1ull << (8 * row + col));
	}

	bool fiftyMoveCheck() {
		return boardStates.back().fiftyMoveCount >= 100;
	}

	bool repeatedPositionCheck() {	// true for if the game a repeatedPosition
		int count = 2;
		uint64_t recentHash = boardStates.back().zobristHash;
		for (int i = boardStates.size() - 2; i >= 0; i--) {
			if (boardStates[i].zobristHash == recentHash)
				count--;
			if (count <= 0)
				return true;
		}

		return false;
	}

	bool isCapture(Move move) {
		int pieceRemovalSquare = move.getEndSquare();
		int flags = move.getFlag();
		if (flags == 1 || flags == 7) {
			pieceRemovalSquare += (currentPlayer == 0 ? -8 : 8);
		}

		return rawBoard[pieceRemovalSquare] != 0;
	}

	void makeMove(Move move) {
		BoardStateInformation newInfo = boardStates.back();

		int startSquare = move.getStartSquare();
		int targetSquare = move.getEndSquare();
		int pieceRemovalSquare = targetSquare;
		int flags = move.getFlag();
		if (flags == 1 || flags == 7) {
			pieceRemovalSquare = targetSquare + (currentPlayer == 0 ? -8 : 8);
		}
		bool capturedPiece = rawBoard[pieceRemovalSquare] != 0;

		// Actual chess move
		int movingPiece = rawBoard[startSquare];
		newInfo.capturedPieceType = rawBoard[pieceRemovalSquare];

		rawBoard[startSquare] = 0;
		rawBoard[pieceRemovalSquare] = 0;
		rawBoard[targetSquare] = movingPiece;

		newInfo.occupiedBoard[movingPiece / 8 - 1] &= ~(1ull << startSquare);
		newInfo.occupiedBoard[movingPiece / 8 - 1] |= (1ull << targetSquare);
		newInfo.occupiedBoard[(movingPiece / 8) % 2] &= ~(1ull << pieceRemovalSquare);

		newInfo.pieceBoards[movingPiece % 8 - 1] &= ~(1ull << startSquare);
		if (capturedPiece) {
			newInfo.pieceBoards[newInfo.capturedPieceType % 8 - 1] &= ~(1ull << pieceRemovalSquare);
		}
		newInfo.pieceBoards[movingPiece % 8 - 1] |= (1ull << targetSquare);


		newInfo.zobristHash ^= zobPieces[currentPlayer][movingPiece % 8][startSquare];
		newInfo.zobristHash ^= zobPieces[currentPlayer][movingPiece % 8][targetSquare];

		newInfo.fiftyMoveCount++;
		if (capturedPiece || (rawBoard[startSquare] & 7) == 2) {
			newInfo.fiftyMoveCount = 0;
			newInfo.zobristHash ^= zobPieces[currentPlayer][newInfo.capturedPieceType % 8][targetSquare];
		}


		newInfo.enPassantSquare = 64;

		// double pawn push
		if (flags == 7) {
			newInfo.enPassantSquare = pieceRemovalSquare;
			newInfo.zobristHash ^= zobEnPassant[pieceRemovalSquare % 8];
		}

		// Handle Castling
		else if (flags == 6) {
			if (targetSquare % 8 > 3) {	// Queenside vs Kingside
				rawBoard[(targetSquare / 8) * 8 + 5] = rawBoard[(targetSquare / 8) * 8 + 7];
				rawBoard[(targetSquare / 8) * 8 + 7] = 0;

				newInfo.occupiedBoard[movingPiece / 8 - 1] &= ~(1ull << ((targetSquare / 8) * 8 + 7));
				newInfo.occupiedBoard[movingPiece / 8 - 1] |= (1ull << ((targetSquare / 8) * 8 + 5));

				newInfo.pieceBoards[4] &= ~(1ull << ((targetSquare / 8) * 8 + 7));
				newInfo.pieceBoards[4] |= (1ull << ((targetSquare / 8) * 8 + 5));
			} else {
				rawBoard[(targetSquare / 8) * 8 + 3] = rawBoard[(targetSquare / 8) * 8];
				rawBoard[(targetSquare / 8) * 8] = 0;

				newInfo.occupiedBoard[movingPiece / 8 - 1] &= ~(1ull << ((targetSquare / 8) * 8));
				newInfo.occupiedBoard[movingPiece / 8 - 1] |= (1ull << ((targetSquare / 8) * 8 + 3));

				newInfo.pieceBoards[4] &= ~(1ull << ((targetSquare / 8) * 8));
				newInfo.pieceBoards[4] |= (1ull << ((targetSquare / 8) * 8 + 3));
			}

			// Remove castling rights for that side
			newInfo.castlingRights[2 * currentPlayer] = false;
			newInfo.castlingRights[2 * currentPlayer + 1] = false;

			newInfo.zobristHash ^= zobCastle[2 * currentPlayer];
			newInfo.zobristHash ^= zobCastle[2 * currentPlayer + 1];
		}

		// Handle promotion
		else if (flags > 1 && flags < 6) {
			rawBoard[targetSquare] = (8 + currentPlayer * 8) | (flags + 1);

			newInfo.pieceBoards[1] &= ~(1ull << targetSquare);
			newInfo.pieceBoards[flags] |= (1ull << targetSquare);

			newInfo.zobristHash ^= zobPieces[currentPlayer][2][targetSquare];
			newInfo.zobristHash ^= zobPieces[currentPlayer][flags + 1][targetSquare];
		}

		// Remove relevant castling rights
		if ((rawBoard[targetSquare] & 7) == 1) {	// King check
			newInfo.castlingRights[2 * currentPlayer] = false;
			newInfo.castlingRights[2 * currentPlayer + 1] = false;

			newInfo.zobristHash ^= zobCastle[2 * currentPlayer];
			newInfo.zobristHash ^= zobCastle[2 * currentPlayer + 1];

			kingLocations[currentPlayer] = targetSquare;
		}

		if ((rawBoard[targetSquare] & 7) == 5) {	// Rook Check
			if (startSquare % 8 > 3) { // Queenside vs Kingside
				newInfo.castlingRights[2 * currentPlayer] = false;
				newInfo.zobristHash ^= zobCastle[2 * currentPlayer];
			}
			else {
				newInfo.castlingRights[2 * currentPlayer + 1] = false;
				newInfo.zobristHash ^= zobCastle[2 * currentPlayer + 1];
			}
		}

		plyCount++;

		if (boardStates.back().zobristHash == newInfo.zobristHash) {
			printf("ERROR, ERROR. ZOBRIST HASH DID NOT CHANGE AFTER A MOVE.\n");
		}

		newInfo.zobristHash ^= zobColor;

		boardStates.push_back(newInfo);
		currentPlayer = (currentPlayer + 1) % 2;
	}

	void undoMove(Move move) {
		int startSquare = move.getStartSquare();
		int targetSquare = move.getEndSquare();
		int pieceRemovalSquare = targetSquare;
		int flags = move.getFlag();

		BoardStateInformation formerStatus = boardStates.back();
		boardStates.erase(boardStates.end()-1);

		if (flags == 1) {
			pieceRemovalSquare = targetSquare + (currentPlayer == 0 ? 8 : -8);
		}

		int movingPiece = rawBoard[targetSquare];

		rawBoard[startSquare] = movingPiece;
		rawBoard[targetSquare] = 0;
		rawBoard[pieceRemovalSquare] = formerStatus.capturedPieceType;

		/*occupiedBoard[movingPiece / 8 - 1] &= ~(1ull << targetSquare);
		occupiedBoard[movingPiece / 8 - 1] |= (1ull << startSquare);

		pieceBoards[movingPiece % 8 - 1] &= ~(1ull << targetSquare);
		pieceBoards[movingPiece % 8 - 1] |= (1ull << startSquare);
		
		if (formerStatus.capturedPieceType) {
			occupiedBoard[(movingPiece / 8) % 2] |= (1ull << pieceRemovalSquare);
			pieceBoards[formerStatus.capturedPieceType % 8 - 1] |= (1ull << pieceRemovalSquare);
		}*/

		if (flags == 6) {
			if (targetSquare % 8 > 3) {	// Queenside vs Kingside
				rawBoard[(targetSquare / 8) * 8 + 7] = rawBoard[(targetSquare / 8) * 8 + 5];
				rawBoard[(targetSquare / 8) * 8 + 5] = 0;

				/*occupiedBoard[movingPiece / 8 - 1] &= ~(1ull << ((targetSquare / 8) * 8 + 5));
				occupiedBoard[movingPiece / 8 - 1] |= (1ull << ((targetSquare / 8) * 8 + 7));

				pieceBoards[4] |= (1ull << ((targetSquare / 8) * 8 + 7));
				pieceBoards[4] &= ~(1ull << ((targetSquare / 8) * 8 + 5));*/

			} else {
				rawBoard[(targetSquare / 8) * 8] = rawBoard[(targetSquare / 8) * 8 + 3];
				rawBoard[(targetSquare / 8) * 8 + 3] = 0;

				/*occupiedBoard[movingPiece / 8 - 1] &= ~(1ull << ((targetSquare / 8) * 8 + 3));
				occupiedBoard[movingPiece / 8 - 1] |= (1ull << ((targetSquare / 8) * 8 + 0));

				pieceBoards[4] |= (1ull << ((targetSquare / 8) * 8));
				pieceBoards[4] &= ~(1ull << ((targetSquare / 8) * 8 + 3));*/
			}
		}
		else if (flags > 1 && flags < 6) {	// Reset to pawn
			rawBoard[startSquare] = (8 + (currentPlayer + 1) % 2 * 8) | 2;

			/*pieceBoards[1] |= (1ull << startSquare);	// Fixing incorrect move that occurred earlier
			pieceBoards[flags] &= ~(1ull << startSquare);*/
		}

		// King location check
		if ((rawBoard[startSquare] & 7) == 1) {	// King check
			kingLocations[(currentPlayer + 1) % 2] = startSquare;
		}

		plyCount--;
		currentPlayer = (currentPlayer + 1) % 2;

	}

	void makeNullMove() {
		BoardStateInformation newInfo = boardStates.back();

		newInfo.zobristHash ^= zobColor;
		newInfo.zobristHash ^= zobEnPassant[newInfo.enPassantSquare % 8];

		newInfo.capturedPieceType = 0;

		boardStates.push_back(newInfo);

		currentPlayer = (currentPlayer + 1) % 2;
		plyCount++;
	}

	void undoNullMove() {
		BoardStateInformation formerStatus = boardStates.back();
		boardStates.erase(boardStates.end() - 1);

		currentPlayer = (currentPlayer + 1) % 2;
		plyCount--;
	}

	// Checking if we're in check 
	bool sideInCheck(int player) {
		BoardStateInformation currentStatus = boardStates.back();
		int otherPlayer = (player + 1) % 2;

		// perform several partial move gens beginning from other player's king location
		uint64_t knightMoves = generateKnightMoves(kingLocations[player], player);
		if (knightMoves & currentStatus.occupiedBoard[otherPlayer] & currentStatus.pieceBoards[2]) {
			return true;
		}

		uint64_t bishopMoves = generateBishopMoves(kingLocations[player], player);
		if (bishopMoves & currentStatus.occupiedBoard[otherPlayer] & (currentStatus.pieceBoards[3] | currentStatus.pieceBoards[5])) {
			return true;
		}
		
		uint64_t rookMoves = generateRookMoves(kingLocations[player], player);
		if (rookMoves & currentStatus.occupiedBoard[otherPlayer] & (currentStatus.pieceBoards[4] | currentStatus.pieceBoards[5])) {
			return true;
		}

		// Pawn positions
		int direction;
		if (otherPlayer == 1) {		// If I'm checking that white is in check from a pawn
			direction = 8;
		}
		else {	// If I'm checking that black is in check from a pawn
			direction = -8;
		}

		for (int i = direction - 1; i < direction + 2; i += 2) {
			int targetSquare = kingLocations[player] + i;
			if (targetSquare < 0 || targetSquare > 63)
				continue;
			if (abs(targetSquare / 8 - kingLocations[player] / 8) != 1)
				continue;
			if( (currentStatus.occupiedBoard[otherPlayer] & currentStatus.pieceBoards[1] & (1ull << targetSquare)) ) {
				return true;
			}
		}

		return false;
	}

	char prettyPiecePrint(int row, int col, int pieceAtLoc) {
		if (isWhitePiece(row, col)) {
			if (pieceAtLoc == 0)
				return ' ';
			if (pieceAtLoc == 1)
				return 'K';
			if (pieceAtLoc == 2)
				return 'P';
			if (pieceAtLoc == 3)
				return 'N';
			if (pieceAtLoc == 4)
				return 'B';
			if (pieceAtLoc == 5)
				return 'R';
			if (pieceAtLoc == 6)
				return 'Q';
			return 'Z';
		}
		else {			// no piece  = 0, king = 1, pawn = 2, knight = 3, bishop = 4, rook = 5, queen = 6, white = 8, black = 16
			if (pieceAtLoc == 0)
				return ' ';
			if (pieceAtLoc == 1)
				return 'k';
			if (pieceAtLoc == 2)
				return 'p';
			if (pieceAtLoc == 3)
				return 'n';
			if (pieceAtLoc == 4)
				return 'b';
			if (pieceAtLoc == 5)
				return 'r';
			if (pieceAtLoc == 6)
				return 'q';
		}
		return 'z';
	}

	void printBoard() {
		//ofstream file("../../../testFiles/boardStates.txt");
		BoardStateInformation currentStatus = boardStates.back();
		int toPrintBoard[64];

		for (int i = 0; i < 64; i++) {
			toPrintBoard[i] = 0;
		}

		for (int i = 1; i < 7; i++) {
			for (int color = 1; color <= 2; color++) {
				uint64_t currentPieceTypeBitboard = currentStatus.pieceBoards[i - 1] & currentStatus.occupiedBoard[color - 1];
				while (currentPieceTypeBitboard != 0) {
					int targetSquare = popLSB(currentPieceTypeBitboard);
					toPrintBoard[targetSquare] = (8 * color) + i;
				}
			}
		}


		for (int i = 7; i >= 0; i--) {
			//file << "---------------------------------\n";
			printf("---------------------------------\n");
			for (int j = 0; j < 8; j++) {
				int targetSquare = 8 * i + j;
				//file << "| ";
				//file << prettyPiecePrint(i, j, toPrintBoard[targetSquare] % 8) << " ";

				printf("| ");
				printf("%c ", prettyPiecePrint(i, j, toPrintBoard[targetSquare] % 8));
			}
			//file << "|\n";
			printf("|\n");
		}
		//file << "---------------------------------\n";
		printf("---------------------------------\n");

		//file.close();
	}

	void loadBoardFromFen(string fen) {
		bool castlingRights[4] = { 0, 0, 0, 0 };
		int enPassantSquare = -1;
		int fiftyMoveCount = 0;

		int row = 7;
		int col = 0;

		size_t fenIndex = 0;
		string current = fen.substr(0, fen.find(" "));
		string remainder = fen.substr(fen.find(" "));

		while (fenIndex < current.length()) {
			char currentItem = current[fenIndex];

			if (currentItem > 65) {	// ASCII value of A aka piece letters
				switch (currentItem)
				{
				case 'k':
					removePiece(row, col);
					setPiece(row, col, 17);
					kingLocations[1] = 8 * row + col;
					break;
				case 'p':
					removePiece(row, col);
					setPiece(row, col, 18);
					break;
				case 'n':
					removePiece(row, col);
					setPiece(row, col, 19);
					break;
				case 'b':
					removePiece(row, col);
					setPiece(row, col, 20);
					break;
				case 'r':
					removePiece(row, col);
					setPiece(row, col, 21);
					break;
				case 'q':
					removePiece(row, col);
					setPiece(row, col, 22);
					break;
				case 'K':
					removePiece(row, col);
					setPiece(row, col, 9);
					kingLocations[0] = 8 * row + col;
					break;
				case 'P':
					removePiece(row, col);
					setPiece(row, col, 10);
					break;
				case 'N':
					removePiece(row, col);
					setPiece(row, col, 11);
					break;
				case 'B':
					removePiece(row, col);
					setPiece(row, col, 12);
					break;
				case 'R':
					removePiece(row, col);
					setPiece(row, col, 13);
					break;
				case 'Q':
					removePiece(row, col);
					setPiece(row, col, 14);
					break;
				default:
					printf("ERROR, FEN IMPORT.\n");
					break;
				}
				col++;
			} 
			else if (currentItem ==  47){
				row--;
				col = 0;
			} else {
				int number = (int)(currentItem - 48);

				for (int k = 0; k < number; k++) {
					removePiece(row, col);
					col++;
				}
			}
			fenIndex++;
		}

		remainder = remainder.substr(1);
		current = remainder.substr(0, remainder.find(" "));
		remainder = remainder.substr(remainder.find(" "));
		currentPlayer = (current[0] == 'w') ? 0 : 1;

		fenIndex = 0;	// Castling
		remainder = remainder.substr(1);
		current = remainder.substr(0, remainder.find(" "));
		remainder = remainder.substr(remainder.find(" "));
		while (fenIndex < current.length()) {
			if (current[fenIndex] == 'K')
				castlingRights[0] = true;
			else if (current[fenIndex] == 'Q')
				castlingRights[1] = true;
			else if (current[fenIndex] == 'k')
				castlingRights[2] = true;
			else if (current[fenIndex] == 'q')
				castlingRights[3] = true;
			fenIndex++;
		}

		// en passant
		remainder = remainder.substr(1);
		current = remainder.substr(0, remainder.find(" "));
		remainder = remainder.substr(remainder.find(" "));
		if (current[0] != '-') {
			int col = (int)(current[0] - 97);
			int row = (int)(current[1] - 49);
			enPassantSquare = row * 8 + col;
		}

		remainder = remainder.substr(1);
		current = remainder.substr(0, remainder.find(" "));
		remainder = remainder.substr(remainder.find(" "));
		fiftyMoveCount = stoi(current);	
		plyCount = stoi(remainder) * 2 + (currentPlayer + 1) % 2;

		BoardStateInformation tmp = boardStates.back();
		tmp.enPassantSquare = enPassantSquare;
		tmp.castlingRights[0] = castlingRights[0];
		tmp.castlingRights[1] = castlingRights[1];
		tmp.castlingRights[2] = castlingRights[2];
		tmp.castlingRights[3] = castlingRights[3];
		tmp.fiftyMoveCount = fiftyMoveCount;
		tmp.capturedPieceType = 0;
		tmp.zobristHash = 0ull;

		//Manually calculate zobristHash
		boardStates[0] = tmp;

		calculateZobristHash();

		generateAttacksV2(0);
		generateAttacksV2(1);
	}

	Board() {
		BoardStateInformation tmp;
		tmp.enPassantSquare = 0;
		tmp.castlingRights[0] = true;
		tmp.castlingRights[1] = true;
		tmp.castlingRights[2] = true;
		tmp.castlingRights[3] = true;
		tmp.fiftyMoveCount = 0;
		tmp.capturedPieceType = 0;
		tmp.zobristHash = 0ull;

		boardStates.push_back(tmp);

		examinedMovesDuringCheck.resize(15);
		for (int i = 0; i < 8; i++) {	// Row
			for (int j = 0; j < 8; j++) {	// Col
				setPiece(i, j, 0);
			}
		}

		precomputeDistances();
		generateMagics();
		generateZobristNumbers();

		string startingBoardPos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
		loadBoardFromFen(startingBoardPos);

		printf("Finished setting up board.\n");
	}

	vector<Move> generateLegalMovesV2(bool onlyCaptures) {	// Using faster check detection
		vector<Move> allMoves(256);

		int moveCount = generatePseudoLegalMovesV3(allMoves, currentPlayer);
		vector<Move> validMoves(256);
		int origCurrentPlayer = currentPlayer;

		int moveIndex = 0;

		for (uint8_t i = 0; i < moveCount; i++) {
			
			Move move = allMoves[i];

			if (onlyCaptures && !isCapture(move)) {
				continue;
			}

			bool moveStatus = true;
			makeMove(move);
			moveStatus = sideInCheck(origCurrentPlayer);
			int kingDistance = max(abs(kingLocations[0] / 8 - kingLocations[1] / 8), abs(kingLocations[0] % 8 - kingLocations[1] % 8));
			undoMove(move);

			if (!moveStatus) {
				if (kingDistance > 1) {
					validMoves[moveIndex] = move;
					moveIndex++;
				}
			}
		}

		validMoves.resize(moveIndex);
		return validMoves;

	}


private:
	int directions[8] = { 8, -8, -1, 1, -7, 7, -9, 9 };
	int distanceFromEdge[64][8];
	uint64_t validKnightMoves[64];
	uint64_t validKingMoves[64];

	vector<Move> examinedMovesDuringCheck;

	uint64_t validPawnMoveMasks[64][3];
	uint64_t zobPieces[3][7][64];
	uint64_t zobColor;
	uint64_t zobCastle[4];
	uint64_t zobEnPassant[8];

	void generateZobristNumbers() {
		mt19937_64 generator(123456789);
		uniform_int_distribution<uint64_t> distro;

		zobColor = distro(generator);

		for (int k = 0; k < 2; k++) {
			for (int i = 0; i < 7; i++) {
				for (int j = 0; j < 64; j++) {
					zobPieces[k][i][j] = distro(generator);
				}
			}
		}

		for (int i = 0; i < 4; i++)
			zobCastle[i] = distro(generator);

		for (int i = 0; i < 8; i++)
			zobEnPassant[i] = distro(generator);
		
	}

	void calculateZobristHash() {
		uint64_t rawHash = 0ull;
		BoardStateInformation currentState = boardStates.back();

		for (int wantedPlayer = 0; wantedPlayer < 2; wantedPlayer++) {
			uint64_t pawnBoard = currentState.occupiedBoard[wantedPlayer] & currentState.pieceBoards[1];
			uint64_t knightBoard = currentState.occupiedBoard[wantedPlayer] & currentState.pieceBoards[2];
			uint64_t bishopBoard = currentState.occupiedBoard[wantedPlayer] & currentState.pieceBoards[3];
			uint64_t rookBoard = currentState.occupiedBoard[wantedPlayer] & currentState.pieceBoards[4];
			uint64_t queenBoard = currentState.occupiedBoard[wantedPlayer] & currentState.pieceBoards[5];

			rawHash ^= zobPieces[wantedPlayer][1][kingLocations[wantedPlayer]];

			while (pawnBoard != 0) {
				int square = popLSB(pawnBoard);
				rawHash ^= zobPieces[wantedPlayer][2][square];				
			}

			while (knightBoard != 0) {
				int square = popLSB(knightBoard);
				rawHash ^= zobPieces[wantedPlayer][3][square];
			}

			while (bishopBoard != 0) {
				int square = popLSB(bishopBoard);
				rawHash ^= zobPieces[wantedPlayer][4][square];
			}

			while (rookBoard != 0) {
				int square = popLSB(rookBoard);
				rawHash ^= zobPieces[wantedPlayer][5][square];
			}

			while (queenBoard != 0) {
				int square = popLSB(queenBoard);
				rawHash ^= zobPieces[wantedPlayer][6][square];
			}
		}

		if (currentPlayer == 1) {
			rawHash ^= zobColor;
		}

		for (int i = 0; i < 4; i++) {
			if (boardStates.back().castlingRights[i]) {
				rawHash ^= zobCastle[i];
			}
		}

		boardStates.back().zobristHash = rawHash;
	}

	void precomputeDistances() {
		for (int rank = 0; rank < 8; rank++) {
			for (int file = 0; file < 8; file++) {
				int index = 8 * rank + file;
				distanceFromEdge[index][0] = 7 - rank;
				distanceFromEdge[index][1] = rank;
				distanceFromEdge[index][2] = file;
				distanceFromEdge[index][3] = 7 - file;

				distanceFromEdge[index][4] = min(rank, 7 - file);
				distanceFromEdge[index][5] = min(7 - rank, file);
				distanceFromEdge[index][6] = min(rank, file);
				distanceFromEdge[index][7] = min(7 - rank, 7 - file);
			}
		}

		int knightIndices[8] = { -17, -15, -10, -6, 6, 10, 15, 17 };
		for (int i = 0; i < 64; i++) {
			for (int j = 0; j < 8; j++) {
				validKnightMoves[i] = 0ull;
				validKingMoves[i] = 0ull;
			}
		}

		// Each valid knight move available
		for (int square = 0; square < 64; square++) {
			for (int i = 0; i < 8; i++) {
				int targetSquare = square + knightIndices[i];
				if (targetSquare >= 0 && targetSquare < 64) {
					int rank = targetSquare / 8;
					int file = targetSquare % 8;

					int distMoved = max(abs(square / 8 - rank), abs(square % 8 - file));
					if (distMoved == 2) {
						validKnightMoves[square] |= (1ull << targetSquare);
					}
				}
			}
		}

		// Each valid King move available
		for (int square = 0; square < 64; square++) {
			for (int i = 0; i < 8; i++) {
				if (distanceFromEdge[square][i] > 0) {
					int targetSquare = square + directions[i];
					validKingMoves[square] |= (1ull << targetSquare);
				}
			}
		}

		// precomputing valid white pawn move masks
		for (int row = 1; row < 7; row++) {
			int square = row * 8;
			validPawnMoveMasks[square][0] = 3ull << (square + 8);

			for (int col = 1; col < 7; col++) {
				square = row * 8 + col;
				validPawnMoveMasks[square][0] = 7ull << (square + 7);
			}

			square = row * 8 + 7;
			validPawnMoveMasks[square][0] = 3ull << (square + 7);
		}

		// precomputing valid black pawn move masks
		for (int row = 6; row > 0; row--) {
			int square = row * 8;
			validPawnMoveMasks[square][1] = 3ull << (square - 8);
			//printf("0x%" PRIx64 "\n", validPawnMoveMasks[square][2]);

			for (int col = 1; col < 7; col++) {
				square = row * 8 + col;
				validPawnMoveMasks[square][1] = 7ull << (square - 9);
				//printf("0x%" PRIx64 "\n", validPawnMoveMasks[square][2]);
			}

			square = row * 8 + 7;
			validPawnMoveMasks[square][1] = 3ull << (square - 9);
			//printf("0x%" PRIx64 "\n", validPawnMoveMasks[square][2]);
		}

	}

	int generateKingMovesV2(int square, vector<Move>& moves, int insertionIndex, int wantedPlayer) {
		int movesGenerated = 0;
		BoardStateInformation currentState = boardStates.back();

		uint64_t kingMoveBitboard = validKingMoves[square] & ~currentState.occupiedBoard[wantedPlayer];

		while (kingMoveBitboard != 0) {
			int targetSquare = popLSB(kingMoveBitboard);
			moves[insertionIndex] = Move(square, targetSquare, 0);
			insertionIndex++;
			movesGenerated++;
		}

		int otherPlayer = (wantedPlayer + 1) % 2;
		
		if (boardStates.back().castlingRights[wantedPlayer * 2]) {	// King side castling right exists
			// Occupancy check
			uint64_t relevantRowMask = 0x6060606060606060 & (0xffull << (56 * wantedPlayer));
			uint64_t emptyArea = relevantRowMask & (currentState.occupiedBoard[0] | currentState.occupiedBoard[1]);	//
			if (emptyArea == 0ull) {
				// Check that no enemy pieces are attacking there
				uint64_t areaWithoutAttacksMask = (relevantRowMask | (1ull << square));
				generateAttacksV2(otherPlayer);

				uint64_t area = areaWithoutAttacksMask & attackingSquares[otherPlayer];
				if (area == 0ull) {
					if(currentState.pieceBoards[4] & currentState.occupiedBoard[wantedPlayer] & (1ull << ((56 * wantedPlayer) + 7))) { // Check the rook still exists
						moves[insertionIndex] = Move(square, square + 2, 6);
						insertionIndex++;
						movesGenerated++;
					}
				}
			}
		}
		if (boardStates.back().castlingRights[wantedPlayer * 2 + 1]) {	// Queen side castling right exists
			// Occupancy check
			uint64_t relevantRowMask = 0x0e0e0e0e0e0e0e0e & (0xffull << (56 * wantedPlayer));
			uint64_t emptyArea = relevantRowMask & (currentState.occupiedBoard[0] | currentState.occupiedBoard[1]);	//
			if (emptyArea == 0ull) {
				// Check that no enemy pieces are attacking there
				uint64_t areaWithoutAttacksMask = relevantRowMask & ~(1ull << (square - 3)) | (1ull << square);
				generateAttacksV2(otherPlayer);

				uint64_t area = areaWithoutAttacksMask & attackingSquares[otherPlayer];
				if (area == 0ull) {
					if (currentState.pieceBoards[4] & currentState.occupiedBoard[wantedPlayer] & (1ull << (56 * wantedPlayer))) { // Check the rook still exists
						moves[insertionIndex] = Move(square, square - 2, 6);
						insertionIndex++;
						movesGenerated++;
					}
				}
			}
		}

		return movesGenerated;
	}

	uint64_t generatePawnCaptures(int square, int wantedPlayer) {
		BoardStateInformation currentState = boardStates.back();

		uint64_t otherOccupiedBoard = currentState.occupiedBoard[(wantedPlayer + 1) % 2];
		uint64_t totalOccupiedBoard = currentState.occupiedBoard[wantedPlayer] | otherOccupiedBoard;

		uint64_t currentPawnMask = validPawnMoveMasks[square][wantedPlayer];
		uint64_t currentFileMask = 0x101010101010101 << (square % 8);

		uint64_t captureFiles = 0;
		if (square % 8 > 0) {
			captureFiles |= (currentFileMask >> 1);
		}
		if (square % 8 < 7) {
			captureFiles |= (currentFileMask << 1);
		}
		uint64_t validCapturePawnMoves = (currentPawnMask & captureFiles) & (otherOccupiedBoard | 1ull << boardStates.back().enPassantSquare);

		return validCapturePawnMoves;
	}

	int generatePawnMovesV2(int square, vector<Move>& moves, int insertionIndex, int wantedPlayer) {
		int movesGenerated = 0;
		BoardStateInformation currentState = boardStates.back();

		uint64_t otherOccupiedBoard = currentState.occupiedBoard[(wantedPlayer + 1) % 2];
		uint64_t totalOccupiedBoard = currentState.occupiedBoard[wantedPlayer] | otherOccupiedBoard;

		uint64_t currentPawnMask = validPawnMoveMasks[square][wantedPlayer];
		uint64_t currentFileMask = 0x101010101010101 << (square % 8);
		uint64_t unblockedLocationsMask = 0xffull << (square / 8 * 8);

		if (wantedPlayer == 0) {
			unblockedLocationsMask = unblockedLocationsMask << 8;
		}
		else {
			unblockedLocationsMask = unblockedLocationsMask >> 8;
		}

		uint64_t validForwardPawnMoves = (currentPawnMask & currentFileMask) & ~(totalOccupiedBoard) & unblockedLocationsMask;

		if (validForwardPawnMoves != 0) {
			if (square / 8 == 1 && wantedPlayer == 0) {
				int targetSquare = square + 16;
				if((totalOccupiedBoard & (1ull << targetSquare)) == 0) {
					moves[insertionIndex] = Move(square, targetSquare, 7);
					insertionIndex++;
					movesGenerated++;
				}
			}
			else if (square / 8 == 6 && wantedPlayer == 1) {
				int targetSquare = square - 16;
				if ((totalOccupiedBoard & (1ull << targetSquare)) == 0) {
					moves[insertionIndex] = Move(square, targetSquare, 7);
					insertionIndex++;
					movesGenerated++;
				}
			}
		}

		uint64_t validPawnMoves = generatePawnCaptures(square, wantedPlayer) | validForwardPawnMoves;

		while (validPawnMoves != 0) {
			int targetSquare = popLSB(validPawnMoves);

			if (targetSquare / 8 == 0 || targetSquare / 8 == 7) {
				for (int flag = 2; flag <= 5; flag++) {
					moves[insertionIndex] = Move(square, targetSquare, flag);
					insertionIndex++;
					movesGenerated++;
				}
			}
			else {
				if (targetSquare == boardStates.back().enPassantSquare) {
					moves[insertionIndex] = Move(square, targetSquare, 1);
					insertionIndex++;
					movesGenerated++;
				}
				else {
					moves[insertionIndex] = Move(square, targetSquare, 0);
					insertionIndex++;
					movesGenerated++;
				}
			}
		}
		return movesGenerated;
	}
	
	uint64_t generateRookMoves(int square, int safePlayer) {
		BoardStateInformation currentState = boardStates.back();

		uint64_t slidingMoveBitboard = 0ull;
		uint64_t blockerBitboard = currentState.occupiedBoard[0] | currentState.occupiedBoard[1];

		uint64_t relevantBlockers = blockerBitboard | refinedRookMagics[square].negMask;
		uint64_t hash = relevantBlockers * refinedRookMagics[square].blackMagic;
		uint64_t tableIndex = (hash >> rookShift) + refinedRookMagics[square].tableOffset;

		if (lookup_table[tableIndex] == 0) {
			lookup_table[tableIndex] = generateSlidingMoveBitboard(square, 5, blockerBitboard);
			printf("ERROR, IMPOSSIBLE. Rook MAGIC BOARD GEN FINDING A NEW POSITION\n");
		}
		slidingMoveBitboard = lookup_table[tableIndex];

		slidingMoveBitboard &= ~currentState.occupiedBoard[safePlayer];

		return slidingMoveBitboard;
	}

	uint64_t generateBishopMoves(int square, int safePlayer) {
		BoardStateInformation currentState = boardStates.back();

		uint64_t slidingMoveBitboard = 0ull;
		uint64_t blockerBitboard = currentState.occupiedBoard[0] | currentState.occupiedBoard[1];

		uint64_t relevantBlockers = blockerBitboard | refinedBishopMagics[square].negMask;
		uint64_t hash = relevantBlockers * refinedBishopMagics[square].blackMagic;
		uint64_t tableIndex = (hash >> bishopShift) + refinedBishopMagics[square].tableOffset;

		if (lookup_table[tableIndex] == 0) {
			//lookup_table[tableIndex] = generateSlidingMoveBitboard(square, 5, blockerBitboard);
			printf("ERROR, IMPOSSIBLE. BISHOP BOARD GEN FINDING A NEW POSITION\n");
		}
		slidingMoveBitboard = lookup_table[tableIndex];

		slidingMoveBitboard &= ~currentState.occupiedBoard[safePlayer];

		return slidingMoveBitboard;
	}

	uint64_t generateKnightMoves(int square, int safePlayer) {
		uint64_t legalMoveBoard = validKnightMoves[square];

		return legalMoveBoard & ~boardStates.back().occupiedBoard[safePlayer];
	}

	int generatePseudoLegalMovesV3(vector<Move>& listOfMoves, int wantedPlayer) {
		int insertionIndex = 0;
		BoardStateInformation currentState = boardStates.back();

		uint64_t relevantOccupiedBoard = currentState.occupiedBoard[wantedPlayer];

		uint64_t kingBoard = relevantOccupiedBoard & currentState.pieceBoards[0];
		uint64_t pawnBoard = relevantOccupiedBoard & currentState.pieceBoards[1];
		uint64_t knightBoard = relevantOccupiedBoard & currentState.pieceBoards[2];
		uint64_t bishopBoard = relevantOccupiedBoard & currentState.pieceBoards[3];
		uint64_t rookBoard = relevantOccupiedBoard & currentState.pieceBoards[4];
		uint64_t queenBoard = relevantOccupiedBoard & currentState.pieceBoards[5];

		insertionIndex += generateKingMovesV2(popLSB(kingBoard), listOfMoves, insertionIndex, wantedPlayer);

		while (pawnBoard != 0) {
			int square = popLSB(pawnBoard);
			insertionIndex += generatePawnMovesV2(square, listOfMoves, insertionIndex, wantedPlayer);
		}

		while (knightBoard != 0) {
			int square = popLSB(knightBoard);
			uint64_t knightMoves = generateKnightMoves(square, wantedPlayer);

			while (knightMoves != 0) {
				int targetSquare = popLSB(knightMoves);
				listOfMoves[insertionIndex] = Move(square, targetSquare, 0);
				insertionIndex++;
			}
		}

		while (bishopBoard != 0) {
			int square = popLSB(bishopBoard);
			uint64_t bishopMoves = generateBishopMoves(square, wantedPlayer);

			while (bishopMoves != 0) {
				int targetSquare = popLSB(bishopMoves);
				listOfMoves[insertionIndex] = Move(square, targetSquare, 0);
				insertionIndex++;
			}

		}

		while (rookBoard != 0) {
			int square = popLSB(rookBoard);
			uint64_t rookMoves = generateRookMoves(square, wantedPlayer);

			while (rookMoves != 0) {
				int targetSquare = popLSB(rookMoves);
				listOfMoves[insertionIndex] = Move(square, targetSquare, 0);
				insertionIndex++;
			}

		}

		while (queenBoard != 0) {
			int square = popLSB(queenBoard);
			uint64_t queenMoves = generateBishopMoves(square, wantedPlayer);
			queenMoves |= generateRookMoves(square, wantedPlayer);

			while (queenMoves != 0) {
				int targetSquare = popLSB(queenMoves);
				listOfMoves[insertionIndex] = Move(square, targetSquare, 0);
				insertionIndex++;
			}
		}

		return insertionIndex;
	}
	
	void generateAttacksV2(int wantedPlayer) {
		BoardStateInformation currentState = boardStates.back();

		uint64_t relevantOccupiedBoard = currentState.occupiedBoard[wantedPlayer];

		uint64_t pawnBoard = relevantOccupiedBoard & currentState.pieceBoards[1];
		uint64_t knightBoard = relevantOccupiedBoard & currentState.pieceBoards[2];
		uint64_t bishopBoard = relevantOccupiedBoard & currentState.pieceBoards[3];
		uint64_t rookBoard = relevantOccupiedBoard & currentState.pieceBoards[4];
		uint64_t queenBoard = relevantOccupiedBoard & currentState.pieceBoards[5];

		attackingSquares[wantedPlayer] = 0ull;

		while (pawnBoard != 0) {
			int square = popLSB(pawnBoard);

			uint64_t currentPawnMask = validPawnMoveMasks[square][wantedPlayer];
			uint64_t currentFileMask = 0x101010101010101 << (square % 8);

			uint64_t captureFiles = 0;
			if (square % 8 > 0) {
				captureFiles |= (currentFileMask >> 1);
			}
			if (square % 8 < 7) {
				captureFiles |= (currentFileMask << 1);
			}
			uint64_t pawnAttacks = (currentPawnMask & captureFiles);
			attackingSquares[wantedPlayer] |= pawnAttacks;
		}

		while (knightBoard != 0) {
			int square = popLSB(knightBoard);
			uint64_t knightMoves = generateKnightMoves(square, wantedPlayer);

			attackingSquares[wantedPlayer] |= knightMoves;
		}

		while (bishopBoard != 0) {
			int square = popLSB(bishopBoard);
			uint64_t bishopMoves = generateBishopMoves(square, wantedPlayer);

			attackingSquares[wantedPlayer] |= bishopMoves;
		}

		while (rookBoard != 0) {
			int square = popLSB(rookBoard);
			uint64_t rookMoves = generateRookMoves(square, wantedPlayer);
			
			attackingSquares[wantedPlayer] |= rookMoves;
		}

		while (queenBoard != 0) {
			int square = popLSB(queenBoard);
			uint64_t queenMoves = generateBishopMoves(square, wantedPlayer);
			queenMoves |= generateRookMoves(square, wantedPlayer);

			attackingSquares[wantedPlayer] |= queenMoves;
		}
	}
};