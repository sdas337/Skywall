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
};

class Board {
public:
	// A0 = 0, H0 = 7, H8 = 63
	int rawBoard[64];

	uint64_t nodes = 0;
	int plyCount;
	int currentPlayer;

	uint64_t occupiedBoard[3];
	uint64_t pieceBoards[7];

	int kingLocations[3];
	uint64_t attackingSquares[3];

	stack<BoardStateInformation> boardStates;

	// no piece  = 0, king = 1, pawn = 2, knight = 3, bishop = 4, rook = 5, queen = 6, white = 8, black = 16
	void setPiece(int row, int col, int piece) {
		int square = 8 * row + col;
		rawBoard[square] = piece;
		occupiedBoard[piece / 8] |= (1ull) << square;
		pieceBoards[piece % 8] |= (1ull) << square;
	}

	void removePiece(int row, int col) {
		int square = 8 * row + col;
		occupiedBoard[1] &= ~(1ull << square);
		occupiedBoard[2] &= ~(1ull << square);
		rawBoard[square] = 0;

		for(int i = 1; i < 7; i++)
			pieceBoards[i] &= ~((1ull) << square);
	}

	bool isWhitePiece(int row, int col) {
		return occupiedBoard[1] & (1ull << (8 * row + col));
	}

	void makeMove(Move move) {
		BoardStateInformation newInfo = boardStates.top();

		int startSquare = move.getStartSquare();
		int targetSquare = move.getEndSquare();
		int pieceRemovalSquare = targetSquare;
		int flags = move.getFlag();
		if (flags == 1 || flags == 7) {
			pieceRemovalSquare = targetSquare + (currentPlayer == 1 ? -8 : 8);
		}

		bool capturedPiece = rawBoard[pieceRemovalSquare] != 0;

		newInfo.fiftyMoveCount++;
		if (capturedPiece || (rawBoard[startSquare] & 7) == 2) {	
			newInfo.fiftyMoveCount = 0;
		}

		// Actual chess move
		int movingPiece = rawBoard[startSquare];
		newInfo.capturedPieceType = rawBoard[pieceRemovalSquare];

		rawBoard[startSquare] = 0;
		rawBoard[pieceRemovalSquare] = 0;
		rawBoard[targetSquare] = movingPiece;

		occupiedBoard[movingPiece / 8] &= ~(1ull << startSquare);
		occupiedBoard[movingPiece / 8] |= (1ull << targetSquare);
		occupiedBoard[(movingPiece / 8 % 2) + 1] &= ~(1ull << pieceRemovalSquare);

		pieceBoards[movingPiece % 8] &= ~(1ull << startSquare);
		pieceBoards[newInfo.capturedPieceType % 8] &= ~(1ull << pieceRemovalSquare);
		pieceBoards[movingPiece % 8] |= (1ull << targetSquare);


		newInfo.enPassantSquare = 64;

		// double pawn push
		if (flags == 7) {
			newInfo.enPassantSquare = pieceRemovalSquare;
		}

		// Handle Castling
		else if (flags == 6) {
			if (targetSquare % 8 > 3) {	// Queenside vs Kingside
				rawBoard[(targetSquare / 8) * 8 + 5] = rawBoard[(targetSquare / 8) * 8 + 7];
				rawBoard[(targetSquare / 8) * 8 + 7] = 0;

				occupiedBoard[movingPiece / 8] &= ~(1ull << ((targetSquare / 8) * 8 + 7));
				occupiedBoard[movingPiece / 8] |= (1ull << ((targetSquare / 8) * 8 + 5));

				pieceBoards[5] &= ~(1ull << ((targetSquare / 8) * 8 + 7));
				pieceBoards[5] |= (1ull << ((targetSquare / 8) * 8 + 5));

			} else {
				rawBoard[(targetSquare / 8) * 8 + 3] = rawBoard[(targetSquare / 8) * 8];
				rawBoard[(targetSquare / 8) * 8] = 0;

				occupiedBoard[movingPiece / 8] &= ~(1ull << ((targetSquare / 8) * 8));
				occupiedBoard[movingPiece / 8] |= (1ull << ((targetSquare / 8) * 8 + 3));

				pieceBoards[5] &= ~(1ull << ((targetSquare / 8) * 8));
				pieceBoards[5] |= (1ull << ((targetSquare / 8) * 8 + 3));
			}

			// Remove castling rights for that side
			newInfo.castlingRights[2 * currentPlayer - 2] = false;
			newInfo.castlingRights[2 * currentPlayer - 1] = false;
		}

		// Handle promotion
		else if (flags > 1 && flags < 6) {
			rawBoard[targetSquare] = (currentPlayer * 8) | (flags + 1);

			pieceBoards[2] &= ~(1ull << targetSquare);
			pieceBoards[flags + 1] |= (1ull << targetSquare);
		}

		// Remove relevant castling rights
		if ((rawBoard[targetSquare] & 7) == 1) {	// King check
			newInfo.castlingRights[2 * currentPlayer - 2] = false;
			newInfo.castlingRights[2 * currentPlayer - 1] = false;

			kingLocations[currentPlayer] = targetSquare;
		}

		if ((rawBoard[targetSquare] & 7) == 5) {	// Rook Check
			if (startSquare % 8 > 3) { // Queenside vs Kingside
				newInfo.castlingRights[2 * currentPlayer - 2] = false;
			}
			else {
				newInfo.castlingRights[2 * currentPlayer - 1] = false;
			}
		}

		plyCount++;

		boardStates.push(newInfo);
		currentPlayer = currentPlayer % 2 + 1;
	}

	void undoMove(Move move) {
		int startSquare = move.getStartSquare();
		int targetSquare = move.getEndSquare();
		int pieceRemovalSquare = targetSquare;
		int flags = move.getFlag();

		BoardStateInformation formerStatus = boardStates.top();
		boardStates.pop();

		if (flags == 1) {
			pieceRemovalSquare = targetSquare + (currentPlayer == 1 ? 8 : -8);
		}

		int movingPiece = rawBoard[targetSquare];

		rawBoard[startSquare] = movingPiece;
		rawBoard[targetSquare] = 0;
		rawBoard[pieceRemovalSquare] = formerStatus.capturedPieceType;

		occupiedBoard[movingPiece / 8] &= ~(1ull << targetSquare);
		occupiedBoard[movingPiece / 8] |= (1ull << startSquare);

		pieceBoards[movingPiece % 8] &= ~(1ull << targetSquare);
		pieceBoards[movingPiece % 8] |= (1ull << startSquare);
		
		if (formerStatus.capturedPieceType) {
			occupiedBoard[(movingPiece / 8 % 2) + 1] |= (1ull << pieceRemovalSquare);
			pieceBoards[formerStatus.capturedPieceType % 8] |= (1ull << pieceRemovalSquare);
		}

		if (flags == 6) {
			if (targetSquare % 8 > 3) {	// Queenside vs Kingside
				rawBoard[(targetSquare / 8) * 8 + 7] = rawBoard[(targetSquare / 8) * 8 + 5];
				rawBoard[(targetSquare / 8) * 8 + 5] = 0;

				occupiedBoard[movingPiece / 8] &= ~(1ull << ((targetSquare / 8) * 8 + 5));
				occupiedBoard[movingPiece / 8] |= (1ull << ((targetSquare / 8) * 8 + 7));

				pieceBoards[5] |= (1ull << ((targetSquare / 8) * 8 + 7));
				pieceBoards[5] &= ~(1ull << ((targetSquare / 8) * 8 + 5));

			} else {
				rawBoard[(targetSquare / 8) * 8] = rawBoard[(targetSquare / 8) * 8 + 3];
				rawBoard[(targetSquare / 8) * 8 + 3] = 0;

				occupiedBoard[movingPiece / 8] &= ~(1ull << ((targetSquare / 8) * 8 + 3));
				occupiedBoard[movingPiece / 8] |= (1ull << ((targetSquare / 8) * 8 + 0));

				pieceBoards[5] |= (1ull << ((targetSquare / 8) * 8));
				pieceBoards[5] &= ~(1ull << ((targetSquare / 8) * 8 + 3));
			}
		}
		else if (flags > 1 && flags < 6) {	// Reset to pawn
			rawBoard[startSquare] = ((currentPlayer % 2 + 1) * 8) | 2;

			pieceBoards[2] |= (1ull << startSquare);	// Fixing incorrect move that occurred earlier
			pieceBoards[flags + 1] &= ~(1ull << startSquare);
		}

		// King location check
		if ((rawBoard[startSquare] & 7) == 1) {	// King check
			kingLocations[(currentPlayer % 2) + 1] = startSquare;
		}

		plyCount--;

		currentPlayer = currentPlayer % 2 + 1;

	}

	// Checking if we're in check 
	bool sideInCheck(int player) {
		int otherPlayer = player % 2 + 1;

		// perform several partial move gens beginning from other player's king location
		uint64_t knightMoves = generateKnightMoves(kingLocations[player], player);
		if (knightMoves & occupiedBoard[otherPlayer] & pieceBoards[3]) {
			return true;
		}

		uint64_t bishopMoves = generateBishopMoves(kingLocations[player], player);
		if (bishopMoves & occupiedBoard[otherPlayer] & (pieceBoards[4] | pieceBoards[6])) {
			return true;
		}
		
		uint64_t rookMoves = generateRookMoves(kingLocations[player], player);
		if (rookMoves & occupiedBoard[otherPlayer] & (pieceBoards[5] | pieceBoards[6])) {
			return true;
		}

		// Pawn positions
		int direction;
		if (otherPlayer == 1) {
			direction = -8;
		}
		else {
			direction = 8;
		}

		for (int i = direction - 1; i < direction + 2; i += 2) {
			int targetSquare = kingLocations[player] + i;
			if (targetSquare < 0 || targetSquare > 63)
				continue;
			if (abs(targetSquare / 8 - kingLocations[player] / 8) != 1)
				continue;
			//if (rawBoard[targetSquare] == (otherPlayer * 8 | 2)) {
			if( (occupiedBoard[otherPlayer] & (pieceBoards[2]) & (1ull << targetSquare)) ) {
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

		int toPrintBoard[64];

		for (int i = 0; i < 64; i++) {
			toPrintBoard[i] = 0;
		}

		for (int i = 1; i < 7; i++) {
			for (int color = 1; color <= 2; color++) {
				uint64_t currentPieceTypeBitboard = pieceBoards[i] & occupiedBoard[color];
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
					kingLocations[2] = 8 * row + col;
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
					kingLocations[1] = 8 * row + col;
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
		currentPlayer = (current[0] == 'w') ? 1 : 2;

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
		plyCount = stoi(remainder) * 2 + currentPlayer % 2;

		BoardStateInformation tmp;
		tmp.enPassantSquare = enPassantSquare;
		tmp.castlingRights[0] = castlingRights[0];
		tmp.castlingRights[1] = castlingRights[1];
		tmp.castlingRights[2] = castlingRights[2];
		tmp.castlingRights[3] = castlingRights[3];
		tmp.fiftyMoveCount = fiftyMoveCount;
		tmp.capturedPieceType = 0;

		boardStates.push(tmp);

		generateAttacksV2(1);
		generateAttacksV2(2);
	}

	Board() {
		examinedMovesDuringCheck.resize(15);
		for (int i = 0; i < 8; i++) {	// Row
			for (int j = 0; j < 8; j++) {	// Col
				setPiece(i, j, 0);
			}
		}

		for (int i = 0; i < 7; i++) {
			pieceBoards[i] = 0ull;
		}

		precomputeDistances();
		generateMagics();

		string startingBoardPos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
		loadBoardFromFen(startingBoardPos);

		printf("Finished setting up board.\n");
	}

	vector<Move> generateLegalMovesV2() {	// Using faster check detection
		vector<Move> allMoves(256);

		int moveCount = generatePseudoLegalMovesV3(allMoves, currentPlayer);
		vector<Move> validMoves(256);
		int origCurrentPlayer = currentPlayer;

		int moveIndex = 0;

		for (uint8_t i = 0; i < moveCount; i++) {
			Move move = allMoves[i];
			bool moveStatus = true;
			makeMove(move);
			moveStatus = sideInCheck(origCurrentPlayer);
			int kingDistance = max(abs(kingLocations[1] / 8 - kingLocations[2] / 8), abs(kingLocations[1] % 8 - kingLocations[2] % 8));
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
			validPawnMoveMasks[square][1] = 3ull << (square + 8);

			for (int col = 1; col < 7; col++) {
				square = row * 8 + col;
				validPawnMoveMasks[square][1] = 7ull << (square + 7);
			}

			square = row * 8 + 7;
			validPawnMoveMasks[square][1] = 3ull << (square + 7);
		}

		// precomputing valid black pawn move masks
		for (int row = 6; row > 0; row--) {
			int square = row * 8;
			validPawnMoveMasks[square][2] = 3ull << (square - 8);
			//printf("0x%" PRIx64 "\n", validPawnMoveMasks[square][2]);

			for (int col = 1; col < 7; col++) {
				square = row * 8 + col;
				validPawnMoveMasks[square][2] = 7ull << (square - 9);
				//printf("0x%" PRIx64 "\n", validPawnMoveMasks[square][2]);
			}

			square = row * 8 + 7;
			validPawnMoveMasks[square][2] = 3ull << (square - 9);
			//printf("0x%" PRIx64 "\n", validPawnMoveMasks[square][2]);
		}

	}

	int generateKingMovesV2(int square, vector<Move>& moves, int insertionIndex, int wantedPlayer) {
		int movesGenerated = 0;

		uint64_t kingMoveBitboard = validKingMoves[square] & ~occupiedBoard[wantedPlayer];

		while (kingMoveBitboard != 0) {
			int targetSquare = popLSB(kingMoveBitboard);
			moves[insertionIndex] = Move(square, targetSquare, 0);
			insertionIndex++;
			movesGenerated++;
		}

		int otherPlayer = wantedPlayer % 2 + 1;
		
		if (boardStates.top().castlingRights[wantedPlayer * 2 - 2]) {	// King side castling right exists
			// Occupancy check
			uint64_t relevantRowMask = 0x6060606060606060 & (0xffull << (56 * wantedPlayer - 56));
			uint64_t emptyArea = relevantRowMask & (occupiedBoard[1] | occupiedBoard[2]);	//
			if (emptyArea == 0ull) {
				// Check that no enemy pieces are attacking there
				uint64_t areaWithoutAttacksMask = (relevantRowMask | (1ull << square));
				generateAttacksV2(otherPlayer);

				uint64_t area = areaWithoutAttacksMask & attackingSquares[otherPlayer];
				if (area == 0ull) {
					if(pieceBoards[5] & occupiedBoard[wantedPlayer] & (1ull << ((56 * wantedPlayer - 56) + 7))) { // Check the rook still exists
						moves[insertionIndex] = Move(square, square + 2, 6);
						insertionIndex++;
						movesGenerated++;
					}
				}
			}
		}
		if (boardStates.top().castlingRights[wantedPlayer * 2 - 1]) {	// Queen side castling right exists
			// Occupancy check
			uint64_t relevantRowMask = 0x0e0e0e0e0e0e0e0e & (0xffull << (56 * wantedPlayer - 56));
			uint64_t emptyArea = relevantRowMask & (occupiedBoard[1] | occupiedBoard[2]);	//
			if (emptyArea == 0ull) {
				// Check that no enemy pieces are attacking there
				uint64_t areaWithoutAttacksMask = relevantRowMask & ~(1ull << (square - 3)) | (1ull << square);
				generateAttacksV2(otherPlayer);

				uint64_t area = areaWithoutAttacksMask & attackingSquares[otherPlayer];
				if (area == 0ull) {
					if (pieceBoards[5] & occupiedBoard[wantedPlayer] & (1ull << (56 * wantedPlayer - 56))) { // Check the rook still exists
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
		uint64_t otherOccupiedBoard = occupiedBoard[wantedPlayer % 2 + 1];
		uint64_t totalOccupiedBoard = occupiedBoard[1] | occupiedBoard[2];

		uint64_t currentPawnMask = validPawnMoveMasks[square][wantedPlayer];
		uint64_t currentFileMask = 0x101010101010101 << (square % 8);

		uint64_t captureFiles = 0;
		if (square % 8 > 0) {
			captureFiles |= (currentFileMask >> 1);
		}
		if (square % 8 < 7) {
			captureFiles |= (currentFileMask << 1);
		}
		uint64_t validCapturePawnMoves = (currentPawnMask & captureFiles) & (otherOccupiedBoard | 1ull << boardStates.top().enPassantSquare);

		return validCapturePawnMoves;
	}

	int generatePawnMovesV2(int square, vector<Move>& moves, int insertionIndex, int wantedPlayer) {
		int movesGenerated = 0;
		uint64_t otherOccupiedBoard = occupiedBoard[wantedPlayer % 2 + 1];
		uint64_t totalOccupiedBoard = occupiedBoard[1] | occupiedBoard[2];

		uint64_t currentPawnMask = validPawnMoveMasks[square][wantedPlayer];
		uint64_t currentFileMask = 0x101010101010101 << (square % 8);
		uint64_t unblockedLocationsMask = 0xffull << (square / 8 * 8);

		if (wantedPlayer == 1) {
			unblockedLocationsMask = unblockedLocationsMask << 8;
		}
		else {
			unblockedLocationsMask = unblockedLocationsMask >> 8;
		}

		uint64_t validForwardPawnMoves = (currentPawnMask & currentFileMask) & ~(totalOccupiedBoard) & unblockedLocationsMask;

		if (validForwardPawnMoves != 0) {
			if (square / 8 == 1 && wantedPlayer == 1) {
				int targetSquare = square + 16;
				if((totalOccupiedBoard & (1ull << targetSquare)) == 0) {
					moves[insertionIndex] = Move(square, targetSquare, 7);
					insertionIndex++;
					movesGenerated++;
				}
			}
			else if (square / 8 == 6 && wantedPlayer == 2) {
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
				if (targetSquare == boardStates.top().enPassantSquare) {
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
		uint64_t slidingMoveBitboard = 0ull;
		uint64_t blockerBitboard = occupiedBoard[1] | occupiedBoard[2];

		uint64_t relevantBlockers = blockerBitboard | refinedRookMagics[square].negMask;
		uint64_t hash = relevantBlockers * refinedRookMagics[square].blackMagic;
		uint64_t tableIndex = (hash >> rookShift) + refinedRookMagics[square].tableOffset;

		if (lookup_table[tableIndex] == 0) {
			lookup_table[tableIndex] = generateSlidingMoveBitboard(square, 5, blockerBitboard);
			printf("ERROR, IMPOSSIBLE. Rook MAGIC BOARD GEN FINDING A NEW POSITION\n");
		}
		slidingMoveBitboard = lookup_table[tableIndex];

		slidingMoveBitboard &= ~occupiedBoard[safePlayer];

		return slidingMoveBitboard;
	}

	uint64_t generateBishopMoves(int square, int safePlayer) {
		uint64_t slidingMoveBitboard = 0ull;
		uint64_t blockerBitboard = occupiedBoard[1] | occupiedBoard[2];

		uint64_t relevantBlockers = blockerBitboard | refinedBishopMagics[square].negMask;
		uint64_t hash = relevantBlockers * refinedBishopMagics[square].blackMagic;
		uint64_t tableIndex = (hash >> bishopShift) + refinedBishopMagics[square].tableOffset;

		if (lookup_table[tableIndex] == 0) {
			//lookup_table[tableIndex] = generateSlidingMoveBitboard(square, 5, blockerBitboard);
			printf("ERROR, IMPOSSIBLE. BISHOP BOARD GEN FINDING A NEW POSITION\n");
		}
		slidingMoveBitboard = lookup_table[tableIndex];

		slidingMoveBitboard &= ~occupiedBoard[safePlayer];

		return slidingMoveBitboard;
	}

	uint64_t generateKnightMoves(int square, int safePlayer) {
		uint64_t legalMoveBoard = validKnightMoves[square];

		return legalMoveBoard & ~occupiedBoard[safePlayer];
	}

	int generatePseudoLegalMovesV3(vector<Move>& listOfMoves, int wantedPlayer) {
		int insertionIndex = 0;
		uint64_t relevantOccupiedBoard = occupiedBoard[wantedPlayer];

		uint64_t kingBoard = relevantOccupiedBoard & pieceBoards[1];
		uint64_t pawnBoard = relevantOccupiedBoard & pieceBoards[2];
		uint64_t knightBoard = relevantOccupiedBoard & pieceBoards[3];
		uint64_t bishopBoard = relevantOccupiedBoard & pieceBoards[4];
		uint64_t rookBoard = relevantOccupiedBoard & pieceBoards[5];
		uint64_t queenBoard = relevantOccupiedBoard & pieceBoards[6];

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
		uint64_t pawnBoard = occupiedBoard[wantedPlayer] & pieceBoards[2];
		uint64_t knightBoard = occupiedBoard[wantedPlayer] & pieceBoards[3];
		uint64_t bishopBoard = occupiedBoard[wantedPlayer] & pieceBoards[4];
		uint64_t rookBoard = occupiedBoard[wantedPlayer] & pieceBoards[5];
		uint64_t queenBoard = occupiedBoard[wantedPlayer] & pieceBoards[6];

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