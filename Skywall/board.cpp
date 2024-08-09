#pragma once

#include "move.cpp"
#include "magics.cpp"

#include "globals.h"


using namespace std;

struct BoardStateInformation {
	bool castlingRights[4];
	int8_t enPassantSquare;
	int8_t fiftyMoveCount;
	int8_t capturedPieceType;
	uint64_t zobristHash;
};

class Board {
public:
	// A0 = 0, H0 = 7, H8 = 63
	int rawBoard[64];

	uint64_t nodes = 0;
	uint64_t lookups = 0;
	uint64_t ttEntries = 0;

	int plyCount;
	int currentPlayer;

	uint64_t occupiedBoard[3];
	uint64_t pieceBoards[8];

	int kingLocations[3];
	uint64_t attackingSquares[3];

	BoardStateInformation boardStates[512];
	int boardStateIndex;

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

	bool fiftyMoveCheck() {
		return boardStates[boardStateIndex].fiftyMoveCount >= 100;
	}

	bool repeatedPositionCheck() {	// true for if the game a repeatedPosition
		int count = 2;
		uint64_t recentHash = boardStates[boardStateIndex].zobristHash;
		for (int i = boardStateIndex - 2; i >= 0; i--) {
			if (boardStates[i].zobristHash == recentHash)
				count--;
			if (count <= 0)
				return true;
		}

		return false;
	}

	template <int playerToMove>
	bool isCapture(Move move) {
		int pieceRemovalSquare = move.getEndSquare();
		int flags = move.getFlag();
		if (flags == 1) {
			if constexpr (playerToMove == 1) {
				pieceRemovalSquare -= 8;
			}
			else {
				pieceRemovalSquare += 8;
			}
		}

		return rawBoard[pieceRemovalSquare] != 0;
	}

	template <int playerToMove>
	void makeRawMove(Move move) {
		BoardStateInformation newInfo = boardStates[boardStateIndex];

		constexpr int otherPlayer = playerToMove % 2 + 1;

		int startSquare = move.getStartSquare();
		int targetSquare = move.getEndSquare();
		int pieceRemovalSquare = targetSquare;
		int flags = move.getFlag();
		if (flags == 1 || flags == 7) {
			if constexpr (playerToMove == 1) {
				pieceRemovalSquare -= 8;
			}
			else {
				pieceRemovalSquare += 8;
			}
		}

		bool capturedPiece = rawBoard[pieceRemovalSquare] != 0;

		// Actual chess move
		int movingPiece = rawBoard[startSquare];
		newInfo.capturedPieceType = rawBoard[pieceRemovalSquare];

		if (capturedPiece) {
			rawBoard[pieceRemovalSquare] = 0;
			occupiedBoard[otherPlayer] ^= (1ull << pieceRemovalSquare);
			pieceBoards[newInfo.capturedPieceType % 8] ^= (1ull << pieceRemovalSquare);
			
			newInfo.fiftyMoveCount = -1;
			newInfo.zobristHash ^= zobPieces[otherPlayer][newInfo.capturedPieceType % 8][pieceRemovalSquare];
		}

		rawBoard[startSquare] = 0;
		rawBoard[targetSquare] = movingPiece;

		uint64_t moveBoard = (1ull << startSquare) | (1ull << targetSquare);
		
		occupiedBoard[playerToMove] ^= moveBoard;
		pieceBoards[movingPiece % 8] ^= moveBoard;

		newInfo.zobristHash ^= zobPieces[playerToMove][movingPiece % 8][startSquare];
		newInfo.zobristHash ^= zobPieces[playerToMove][movingPiece % 8][targetSquare];

		//cout << zobristHashCalc() << "\n";

		newInfo.fiftyMoveCount++;
		if ((movingPiece & 7) == 2) {
			newInfo.fiftyMoveCount = 0;
		}

		if (newInfo.enPassantSquare != 64) {
			newInfo.zobristHash ^= zobEnPassant[newInfo.enPassantSquare % 8];
		}

		newInfo.enPassantSquare = 64;

		// double pawn push
		if (flags == 7) {
			newInfo.enPassantSquare = pieceRemovalSquare;
			newInfo.zobristHash ^= zobEnPassant[pieceRemovalSquare % 8];
		}
		// Handle Castling
		else if (flags == 6) {
			if constexpr(playerToMove == 1) {
				if (targetSquare % 8 > 3) {	// White Kingside Castle
					rawBoard[5] = rawBoard[7];
					rawBoard[7] = 0;

					occupiedBoard[1] ^= 0xa0;
					pieceBoards[5] ^= 0xa0;

					newInfo.zobristHash ^= zobPieces[1][5][7];
					newInfo.zobristHash ^= zobPieces[1][5][5];
				}
				else {	// White Queenside Castle
					rawBoard[3] = rawBoard[0];
					rawBoard[0] = 0;

					occupiedBoard[1] ^= 0x9;
					pieceBoards[5] ^= 0x9;

					newInfo.zobristHash ^= zobPieces[1][5][3];
					newInfo.zobristHash ^= zobPieces[1][5][0];
				}

				// Remove castling rights for that side
				if (newInfo.castlingRights[0])
					newInfo.zobristHash ^= zobCastle[0];
				if (newInfo.castlingRights[1])
					newInfo.zobristHash ^= zobCastle[1];

				newInfo.castlingRights[0] = false;
				newInfo.castlingRights[1] = false;
			}
			else {
				if (targetSquare % 8 > 3) {	// Black Kingside Castle
					rawBoard[61] = rawBoard[63];
					rawBoard[63] = 0;

					occupiedBoard[2] ^= 0xa000000000000000ULL;
					pieceBoards[5] ^= 0xa000000000000000ULL;

					newInfo.zobristHash ^= zobPieces[2][5][63];
					newInfo.zobristHash ^= zobPieces[2][5][61];
				}
				else {	// Black Queenside Castle
					rawBoard[59] = rawBoard[56];
					rawBoard[56] = 0;

					occupiedBoard[2] ^= 0x900000000000000ULL;
					pieceBoards[5] ^= 0x900000000000000ULL;

					newInfo.zobristHash ^= zobPieces[2][5][59];
					newInfo.zobristHash ^= zobPieces[2][5][56];
				}

				// Remove castling rights for that side
				if (newInfo.castlingRights[2])
					newInfo.zobristHash ^= zobCastle[2];
				if (newInfo.castlingRights[3])
					newInfo.zobristHash ^= zobCastle[3];

				newInfo.castlingRights[2] = false;
				newInfo.castlingRights[3] = false;
			}
		}

		// Handle promotion
		else if (flags > 1 && flags < 6) {
			rawBoard[targetSquare] = (playerToMove * 8) | (flags + 1);

			pieceBoards[2] ^= (1ull << targetSquare);
			pieceBoards[flags + 1] ^= (1ull << targetSquare);

			newInfo.zobristHash ^= zobPieces[playerToMove][2][targetSquare];
			newInfo.zobristHash ^= zobPieces[playerToMove][flags + 1][targetSquare];
		}

		// Remove relevant castling rights
		if ((rawBoard[targetSquare] & 7) == 1) {	// King check
			if constexpr (playerToMove == 1) {
				if (newInfo.castlingRights[0]) {
					newInfo.zobristHash ^= zobCastle[0];
					newInfo.castlingRights[0] = false;
				}
				if (newInfo.castlingRights[1]) {
					newInfo.zobristHash ^= zobCastle[1];
					newInfo.castlingRights[1] = false;
				}
			}
			else {
				if (newInfo.castlingRights[2]) {
					newInfo.zobristHash ^= zobCastle[2];
					newInfo.castlingRights[2] = false;
				}
				if (newInfo.castlingRights[3]) {
					newInfo.zobristHash ^= zobCastle[3];
					newInfo.castlingRights[3] = false;
				}
			}

			kingLocations[playerToMove] = targetSquare;
		}

		if ((rawBoard[targetSquare] & 7) == 5) {	// Rook Check
			if (startSquare % 8 > 3) { // Queenside vs Kingside
				if constexpr (playerToMove == 1) {
					if (newInfo.castlingRights[0]) {
						newInfo.zobristHash ^= zobCastle[0];
						newInfo.castlingRights[0] = false;
					}
				}
				else {
					if (newInfo.castlingRights[2]) {
						newInfo.zobristHash ^= zobCastle[2];
						newInfo.castlingRights[2] = false;
					}
				}
			}
			else {
				if constexpr (playerToMove == 1) {
					if (newInfo.castlingRights[1]) {
						newInfo.zobristHash ^= zobCastle[1];
						newInfo.castlingRights[1] = false;
					}
				}
				else {
					if (newInfo.castlingRights[3]) {
						newInfo.zobristHash ^= zobCastle[3];
						newInfo.castlingRights[3] = false;
					}
				}
			}
		}

		plyCount++;
		newInfo.zobristHash ^= zobColor;

		boardStateIndex++;
		boardStates[boardStateIndex] = newInfo;
		
		currentPlayer = otherPlayer;
	}

	void undoRawMove(Move move) {
		int startSquare = move.getStartSquare();
		int targetSquare = move.getEndSquare();
		int pieceRemovalSquare = targetSquare;
		int flags = move.getFlag();

		int otherPlayer = currentPlayer % 2 + 1;

		BoardStateInformation formerStatus = boardStates[boardStateIndex];
		boardStateIndex--;

		if (flags == 1) {
			pieceRemovalSquare = targetSquare + (currentPlayer == 1 ? 8 : -8);
		}

		int movingPiece = rawBoard[targetSquare];

		rawBoard[startSquare] = movingPiece;
		rawBoard[targetSquare] = 0;
		rawBoard[pieceRemovalSquare] = formerStatus.capturedPieceType;

		uint64_t moveBoard = (1ull << startSquare) | (1ull << targetSquare);

		occupiedBoard[otherPlayer] ^= moveBoard;
		pieceBoards[movingPiece % 8] ^= moveBoard;
		
		if (formerStatus.capturedPieceType) {
			occupiedBoard[currentPlayer] |= (1ull << pieceRemovalSquare);
			pieceBoards[formerStatus.capturedPieceType % 8] |= (1ull << pieceRemovalSquare);
		}

		if (flags == 6) {
			if (otherPlayer == 1) {
				if (targetSquare % 8 > 3) {	// White Kingside Castle
					rawBoard[7] = rawBoard[5];
					rawBoard[5] = 0;

					occupiedBoard[otherPlayer] ^= 0xa0;
					pieceBoards[5] ^= 0xa0;
				}
				else {	// White Queenside Castle
					rawBoard[0] = rawBoard[3];
					rawBoard[3] = 0;

					occupiedBoard[otherPlayer] ^= 0x9;
					pieceBoards[5] ^= 0x9;
				}
			}
			else {
				if (targetSquare % 8 > 3) {	// Black Kingside Castle
					rawBoard[63] = rawBoard[61];
					rawBoard[61] = 0;

					occupiedBoard[otherPlayer] ^= 0xa000000000000000ULL;
					pieceBoards[5] ^= 0xa000000000000000ULL;
				}
				else {	// Black Queenside Castle
					rawBoard[56] = rawBoard[59];
					rawBoard[59] = 0;

					occupiedBoard[otherPlayer] ^= 0x900000000000000ULL;
					pieceBoards[5] ^= 0x900000000000000ULL;
				}
			}
		}
		else if (flags > 1 && flags < 6) {	// Reset to pawn
			rawBoard[startSquare] = (otherPlayer * 8) | 2;

			pieceBoards[2] |= (1ull << startSquare);	// Fixing incorrect move that occurred earlier
			pieceBoards[flags + 1] ^= (1ull << startSquare);
		}

		// King location check
		if ((rawBoard[startSquare] & 7) == 1) {	// King check
			kingLocations[otherPlayer] = startSquare;
		}

		plyCount--;
		currentPlayer = otherPlayer;
	}

	void makeMove(Move move) {
		if (currentPlayer == 1)
			makeRawMove<1>(move);
		else
			makeRawMove<2>(move);
	}

	void undoMove(Move move) {
		undoRawMove(move);
	}

	void makeNullMove() {
		BoardStateInformation newInfo = boardStates[boardStateIndex];

		newInfo.zobristHash ^= zobColor;
		//newInfo.zobristHash ^= zobEnPassant[newInfo.enPassantSquare % 8];

		boardStateIndex++;
		boardStates[boardStateIndex] = newInfo;

		currentPlayer = currentPlayer % 2 + 1;
		plyCount++;
	}

	void undoNullMove() {
		boardStateIndex--;
		currentPlayer = currentPlayer % 2 + 1;
		plyCount--;
	}

	// Checking if we're in check 
	template <int player>
	bool sideInCheck() {
		uint64_t attackers = 0ull;

		// perform several partial move gens beginning from other player's king location
		uint64_t knightMoves = generateKnightMoves<player>(kingLocations[player]);
		attackers |= knightMoves & pieceBoards[3];

		uint64_t bishopMoves = generateBishopMoves(kingLocations[player], occupiedBoard[1] | occupiedBoard[2]);
		bishopMoves &= ~occupiedBoard[player];
		
		attackers |= bishopMoves & (pieceBoards[4] | pieceBoards[6]);

		uint64_t rookMoves = generateRookMoves(kingLocations[player], occupiedBoard[1] | occupiedBoard[2]);
		rookMoves &= ~occupiedBoard[player];

		attackers |= rookMoves & (pieceBoards[5] | pieceBoards[6]);
		attackers |= generatePawnAttacks<player>(kingLocations[player]) & pieceBoards[2];

		attackers &= occupiedBoard[player % 2 + 1];

		return popcount(attackers) != 0;
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

	void setMoveFromString(Move& m, string currentMove) {
		int startSquare = squareNameToValue(currentMove.substr(0, 2));
		int endSquare = squareNameToValue(currentMove.substr(2, 4));
		int flag = 0;

		if (rawBoard[startSquare] % 8 == 2) {	// Checking if pawn
			if (endSquare == boardStates[boardStateIndex].enPassantSquare) {
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
		if (rawBoard[startSquare] % 8 == 1) {	// king move
			if (abs(endSquare % 8 - startSquare % 8) > 1) {	// Castling
				flag = 6;
			}
		}
		m.rawValue = (flag << 12) | (startSquare << 6) | (endSquare);
	}

	void loadBoardFromFen(string fen) {
		bool castlingRights[4] = { 0, 0, 0, 0 };
		int enPassantSquare = 64;
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

		int index = remainder.find(" ");
		if (index < 0) {
			current = remainder;
		}
		current = remainder.substr(0, remainder.find(" "));
		if (current[0] != '-') {
			int col = (int)(current[0] - 97);
			int row = (int)(current[1] - 49);
			enPassantSquare = row * 8 + col;
		}
		
		if (index >= 0) {
			remainder = remainder.substr(index);

			remainder = remainder.substr(1);
			current = remainder.substr(0, remainder.find(" "));
			remainder = remainder.substr(remainder.find(" "));
			fiftyMoveCount = stoi(current);
			plyCount = stoi(remainder) * 2 + currentPlayer % 2;
		}
		else {
			fiftyMoveCount = 0;
			plyCount = 0;
		}

		BoardStateInformation tmp;
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
		boardStateIndex = 0;

		calculateZobristHash();

		generateAttacksV3(1);
		generateAttacksV3(2);
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
		generateZobristNumbers();

		string startingBoardPos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
		loadBoardFromFen(startingBoardPos);

		memset(boardStates, 0, sizeof(boardStates));
		boardStateIndex = 0;

		//printf("Finished setting up board.\n");
	}

	template <int focusPlayer, bool onlyCaptures>
	int generateLegalMovesV2(Move validMoves[]) {	// Using faster check detection
		
		Move allMoves[256];
		int moveCount;

		moveCount = generatePseudoLegalMovesV3<focusPlayer>(allMoves);

		int moveIndex = 0;

		uint64_t initialBoards[2] = { occupiedBoard[1], occupiedBoard[2] };

		for (uint8_t i = 0; i < moveCount; i++) {
			Move move = allMoves[i];

			if constexpr(onlyCaptures) {
				if (!isCapture<focusPlayer>(move)) {
					continue;
				}
			}

			/*if (occupiedBoard[1] != initialBoards[0] || occupiedBoard[2] != initialBoards[1]) {
				cout << "ERROR, ERROR, ERROR. Not agreeing boards.\n";
			}*/


			makeRawMove<focusPlayer>(move);
			bool moveStatus;
			moveStatus = determineLegalBoardState<focusPlayer>();
			undoRawMove(move);

			if (moveStatus) {
				validMoves[moveIndex] = move;
				moveIndex++;
			}
		}

		return moveIndex;

	}

	template<int otherPlayer>
	bool determineLegalBoardState() {
		bool moveStatus = sideInCheck<otherPlayer>();
		int kingDistance = max(abs(kingLocations[1] / 8 - kingLocations[2] / 8), abs(kingLocations[1] % 8 - kingLocations[2] % 8));

		return !moveStatus && kingDistance > 1;
	}

	uint64_t zobristHashCalc() {
		uint64_t rawHash = 0ull;
		for (int wantedPlayer = 1; wantedPlayer < 3; wantedPlayer++) {
			uint64_t kingBoard = occupiedBoard[wantedPlayer] & pieceBoards[1];
			uint64_t pawnBoard = occupiedBoard[wantedPlayer] & pieceBoards[2];
			uint64_t knightBoard = occupiedBoard[wantedPlayer] & pieceBoards[3];
			uint64_t bishopBoard = occupiedBoard[wantedPlayer] & pieceBoards[4];
			uint64_t rookBoard = occupiedBoard[wantedPlayer] & pieceBoards[5];
			uint64_t queenBoard = occupiedBoard[wantedPlayer] & pieceBoards[6];

			rawHash ^= zobPieces[wantedPlayer][1][popLSB(kingBoard)];

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

		if (currentPlayer == 2) {
			rawHash ^= zobColor;
		}

		for (int i = 0; i < 4; i++) {
			if (boardStates[boardStateIndex].castlingRights[i]) {
				rawHash ^= zobCastle[i];
			}
		}

		if (boardStates[boardStateIndex].enPassantSquare != 64) {
			rawHash ^= zobEnPassant[boardStates[boardStateIndex].enPassantSquare % 8];
		}

		return rawHash;
	}

	uint64_t getAttackers(int square, uint64_t comboOB) {
		uint64_t attackerBoard = 0ull;

		// Valid Knight Attacks on the currentSquare
		attackerBoard |= validKnightMoves[square] & pieceBoards[3];

		// Valid Diagonal Attacks on the currentSquare
		attackerBoard |= generateBishopMoves(square, comboOB) & (pieceBoards[4] | pieceBoards[6]);

		// Valid Rook Attacks on the currentSquare
		attackerBoard |= generateRookMoves(square, comboOB) & (pieceBoards[5] | pieceBoards[6]);


		// Pawn Attacks		
		for (int player = 1; player <= 2; player++) {

			int direction;
			if (player == 1) {
				direction = -8;
			}
			else {
				direction = 8;
			}

			for (int i = direction - 1; i < direction + 2; i += 2) {
				int targetSquare = square + i;
				if (targetSquare < 0 || targetSquare > 63)
					continue;
				if (abs(targetSquare / 8 - square / 8) != 1)
					continue;
				//if (rawBoard[targetSquare] == (otherPlayer * 8 | 2)) {
				if ((occupiedBoard[player] & (pieceBoards[2]) & (1ull << targetSquare))) {
					attackerBoard |= (1ull << targetSquare);
				}
			}
		}

		// King Attacks
		attackerBoard |= validKingMoves[square] & pieceBoards[1];

		return attackerBoard;
	}

	uint64_t generateRookMoves(int square, uint64_t blockerBitboard) {
		uint64_t relevantBlockers = blockerBitboard | refinedRookMagics[square].negMask;
		uint64_t hash = relevantBlockers * refinedRookMagics[square].blackMagic;
		uint64_t tableIndex = (hash >> rookShift) + refinedRookMagics[square].tableOffset;

		return lookup_table[tableIndex];
	}

	uint64_t generateBishopMoves(int square, uint64_t blockerBitboard) {
		uint64_t relevantBlockers = blockerBitboard | refinedBishopMagics[square].negMask;
		uint64_t hash = relevantBlockers * refinedBishopMagics[square].blackMagic;
		uint64_t tableIndex = (hash >> bishopShift) + refinedBishopMagics[square].tableOffset;

		return lookup_table[tableIndex];
	}


private:
	int directions[8] = { 8, -8, -1, 1, -7, 7, -9, 9 };
	int distanceFromEdge[64][8];
	uint64_t validKnightMoves[64];
	uint64_t validKingMoves[64];

	vector<Move> examinedMovesDuringCheck;

	uint64_t validPawnMoveMasks[64][3];
	uint64_t validPawnCaptureMasks[64][3];

	uint64_t zobPieces[3][7][64];
	uint64_t zobColor;
	uint64_t zobCastle[4];
	uint64_t zobEnPassant[8];

	void generateZobristNumbers() {
		mt19937_64 generator(123456789);
		uniform_int_distribution<uint64_t> distro;

		zobColor = distro(generator);

		for (int k = 1; k < 3; k++) {
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
		boardStates[boardStateIndex].zobristHash = zobristHashCalc();
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

		// Precomputing valid pawn capture masks
		for (int square = 0; square < 64; square++) {
			uint64_t currentFileMask = 0x101010101010101ul << (square % 8);

			uint64_t captureFiles = 0;
			if (square % 8 > 0) {
				captureFiles |= (currentFileMask >> 1);
			}
			if (square % 8 < 7) {
				captureFiles |= (currentFileMask << 1);
			}

			uint64_t captureRow = 0xffull << (8 * (square / 8) + 8);
			validPawnCaptureMasks[square][1] = captureRow & captureFiles;

			// Black pawn capture row
			captureRow = 0xffull << 8 * (square / 8 - 1);
			validPawnCaptureMasks[square][2] = captureRow & captureFiles;

			//printf("0x%" PRIx64 "\n", validPawnCaptureMasks[square][1]);
		}

		// precomputing valid pawn move masks
		for (int square = 8; square < 56; square++) {
			validPawnMoveMasks[square][1] = 1ull << (square + 8);
			validPawnMoveMasks[square][2] = 1ull << (square - 8);
		}

	}

	template<int wantedPlayer> int generateKingMovesV2(int square, Move moves[], int insertionIndex) {
		int movesGenerated = 0;

		uint64_t kingMoveBitboard = validKingMoves[square] & ~occupiedBoard[wantedPlayer];

		while (kingMoveBitboard != 0) {
			int targetSquare = popLSB(kingMoveBitboard);
			moves[insertionIndex] = Move(square, targetSquare, 0);
			insertionIndex++;
			movesGenerated++;
		}
		
		if constexpr (wantedPlayer == 1) {
			if (boardStates[boardStateIndex].castlingRights[0]) {
				constexpr uint64_t relevantRowMask = 0x60;
				uint64_t emptyArea = relevantRowMask & (occupiedBoard[1] | occupiedBoard[2]);

				if (emptyArea == 0ull) {
					constexpr uint64_t areaWithoutAttacksMask = 0x70;
					generateAttacksV3(2);

					uint64_t area = areaWithoutAttacksMask & attackingSquares[2];
					if (area == 0ull) {
						if (pieceBoards[5] & occupiedBoard[1] & 0x80ull) { // Check the rook still exists
							moves[insertionIndex] = Move(4, 6, 6);
							insertionIndex++;
							movesGenerated++;
						}
					}
				}
			}
			if (boardStates[boardStateIndex].castlingRights[1]) {
				// Occupancy check
				constexpr uint64_t relevantRowMask = 0xeull;
				uint64_t emptyArea = relevantRowMask & (occupiedBoard[1] | occupiedBoard[2]);	//
				if (emptyArea == 0ull) {
					// Check that no enemy pieces are attacking there
					constexpr uint64_t areaWithoutAttacksMask = 0x1c;
					generateAttacksV3(2);

					uint64_t area = areaWithoutAttacksMask & attackingSquares[2];
					if (area == 0ull) {
						if (pieceBoards[5] & occupiedBoard[1] & 1ull) { // Check the rook still exists
							moves[insertionIndex] = Move(4, 2, 6);
							insertionIndex++;
							movesGenerated++;
						}
					}
				}
			}
		}
		else {
			if (boardStates[boardStateIndex].castlingRights[2]) {
				constexpr uint64_t relevantRowMask = 0x6000000000000000ull;
				uint64_t emptyArea = relevantRowMask & (occupiedBoard[1] | occupiedBoard[2]);

				if (emptyArea == 0ull) {
					constexpr uint64_t areaWithoutAttacksMask = 0x7000000000000000ull;
					generateAttacksV3(1);

					uint64_t area = areaWithoutAttacksMask & attackingSquares[1];
					if (area == 0ull) {
						if (pieceBoards[5] & occupiedBoard[2] & 0x8000000000000000ull) { // Check the rook still exists
							moves[insertionIndex] = Move(60, 62, 6);
							insertionIndex++;
							movesGenerated++;
						}
					}
				}
			}
			if (boardStates[boardStateIndex].castlingRights[3]) {
				// Occupancy check
				constexpr uint64_t relevantRowMask = 0xe00000000000000ull;
				uint64_t emptyArea = relevantRowMask & (occupiedBoard[1] | occupiedBoard[2]);	//
				if (emptyArea == 0ull) {
					// Check that no enemy pieces are attacking there
					constexpr uint64_t areaWithoutAttacksMask = 0x1c00000000000000ull;
					generateAttacksV3(1);

					uint64_t area = areaWithoutAttacksMask & attackingSquares[1];
					if (area == 0ull) {
						if (pieceBoards[5] & occupiedBoard[2] & 0x100000000000000ull) { // Check the rook still exists
							moves[insertionIndex] = Move(60, 58, 6);
							insertionIndex++;
							movesGenerated++;
						}
					}
				}
			}
		}

		return movesGenerated;
	}

	template<int wantedPlayer> uint64_t generatePawnAttacks(int square) {
		uint64_t otherOccupiedBoard = occupiedBoard[wantedPlayer % 2 + 1];

		uint64_t currentPawnMask = validPawnCaptureMasks[square][wantedPlayer];

		return currentPawnMask & otherOccupiedBoard;
	}

	template<int wantedPlayer> void generatePawnMovesV3(Move moves[], int &insertionIndex) {
		const uint64_t totalOccupiedBoard = occupiedBoard[1] | occupiedBoard[2];
		const uint64_t emptyBoard = ~(totalOccupiedBoard);

		const uint64_t otherOccupiedBoard = occupiedBoard[wantedPlayer % 2 + 1];

		uint64_t pawnBoard = occupiedBoard[wantedPlayer] & pieceBoards[2];

		// Single Pawn Push
		uint64_t pawnPushes = (wantedPlayer == 1 ? pawnBoard << 8 : pawnBoard >> 8) & emptyBoard;

		// Double Pawn Push
		uint64_t doublePawnPushes = (wantedPlayer == 1 ? 0xff0000ul : 0xff0000000000ul) & pawnPushes;
		doublePawnPushes = (wantedPlayer == 1 ? doublePawnPushes << 8 : doublePawnPushes >> 8) & emptyBoard;

		// Pawn Promotions From push
		uint64_t pawnPushPromotions = (wantedPlayer == 1 ? 0xff00000000000000ul : 0xfful) & pawnPushes;
		pawnPushes ^= pawnPushPromotions;	

		while (pawnPushes != 0) {
			int targetSquare = popLSB(pawnPushes);
			int startSquare = targetSquare - directions[wantedPlayer - 1];
			moves[insertionIndex] = Move(startSquare, targetSquare, 0);
			insertionIndex++;
		}

		while (doublePawnPushes != 0) {
			int targetSquare = popLSB(doublePawnPushes);
			int startSquare = targetSquare - directions[wantedPlayer - 1] * 2;
			moves[insertionIndex] = Move(startSquare, targetSquare, 7);
			insertionIndex++;
		}

		while (pawnPushPromotions != 0) {
			int targetSquare = popLSB(pawnPushPromotions);
			int startSquare = targetSquare - directions[wantedPlayer - 1];
			for (int flag = 2; flag <= 5; flag++) {
				moves[insertionIndex] = Move(startSquare, targetSquare, flag);
				insertionIndex++;
			}
		}

		// Captures
		uint64_t leftCaptures = (wantedPlayer == 1 ? pawnBoard << 7 : pawnBoard >> 9) & otherOccupiedBoard;
		leftCaptures = leftCaptures & 0x7f7f7f7f7f7f7f7ful;	// Left Captures that end up on rightmost column started on leftmost col

		uint64_t lcPromotions = (wantedPlayer == 1 ? 0xff00000000000000ul : 0xfful) & leftCaptures;
		leftCaptures ^= lcPromotions;

		uint64_t rightCaptures = (wantedPlayer == 1 ? pawnBoard << 9 : pawnBoard >> 7) & otherOccupiedBoard;
		rightCaptures = rightCaptures & 0xfefefefefefefefeul;	// Right Captures that end up on leftmost column started on rightmost col

		uint64_t rcPromotions = (wantedPlayer == 1 ? 0xff00000000000000ul : 0xfful) & rightCaptures;
		rightCaptures ^= rcPromotions;

		while (leftCaptures != 0) {
			int targetSquare = popLSB(leftCaptures);
			int startSquare = targetSquare + directions[3 * wantedPlayer + 1];
			moves[insertionIndex] = Move(startSquare, targetSquare, 0);
			insertionIndex++;
		}

		while (rightCaptures != 0) {
			int targetSquare = popLSB(rightCaptures);
			int startSquare = targetSquare + directions[7 - wantedPlayer];
			moves[insertionIndex] = Move(startSquare, targetSquare, 0);
			insertionIndex++;
		}


		while (lcPromotions != 0) {
			int targetSquare = popLSB(lcPromotions);
			int startSquare = targetSquare + directions[3 * wantedPlayer + 1];
			for (int flag = 2; flag <= 5; flag++) {
				moves[insertionIndex] = Move(startSquare, targetSquare, flag);
				insertionIndex++;
			}
		}

		while (rcPromotions != 0) {
			int targetSquare = popLSB(rcPromotions);
			int startSquare = targetSquare + directions[7 - wantedPlayer];
			for (int flag = 2; flag <= 5; flag++) {
				moves[insertionIndex] = Move(startSquare, targetSquare, flag);
				insertionIndex++;
			}
		}

		// En Passant
		if (boardStates[boardStateIndex].enPassantSquare != 64) {
			uint64_t validEnPassantAttack = generatePawnAttacks<wantedPlayer % 2 + 1>(boardStates[boardStateIndex].enPassantSquare) & pieceBoards[2];
			while (validEnPassantAttack != 0) {
				int startSquare = popLSB(validEnPassantAttack);
				moves[insertionIndex] = Move(startSquare, boardStates[boardStateIndex].enPassantSquare, 1);
				insertionIndex++;
			}
		}
	}

	template <int safePlayer> uint64_t generateKnightMoves(int square) {
		uint64_t legalMoveBoard = validKnightMoves[square];

		return legalMoveBoard & ~occupiedBoard[safePlayer];
	}


public:
	template <int wantedPlayer>
	int generatePseudoLegalMovesV3(Move listOfMoves[]) {
		int insertionIndex = 0;
		uint64_t relevantOccupiedBoard = occupiedBoard[wantedPlayer];

		uint64_t kingBoard = relevantOccupiedBoard & pieceBoards[1];
		uint64_t pawnBoard = relevantOccupiedBoard & pieceBoards[2];
		uint64_t knightBoard = relevantOccupiedBoard & pieceBoards[3];
		uint64_t bishopBoard = relevantOccupiedBoard & pieceBoards[4];
		uint64_t rookBoard = relevantOccupiedBoard & pieceBoards[5];
		uint64_t queenBoard = relevantOccupiedBoard & pieceBoards[6];

		insertionIndex += generateKingMovesV2<wantedPlayer>(popLSB(kingBoard), listOfMoves, insertionIndex);

		generatePawnMovesV3<wantedPlayer>(listOfMoves, insertionIndex);

		while (knightBoard != 0) {
			int square = popLSB(knightBoard);
			uint64_t knightMoves = generateKnightMoves<wantedPlayer>(square);

			while (knightMoves != 0) {
				int targetSquare = popLSB(knightMoves);
				listOfMoves[insertionIndex] = Move(square, targetSquare, 0);
				insertionIndex++;
			}
		}

		while (bishopBoard != 0) {
			int square = popLSB(bishopBoard);
			uint64_t bishopMoves = generateBishopMoves(square, occupiedBoard[1] | occupiedBoard[2]);
			bishopMoves &= ~occupiedBoard[wantedPlayer];

			while (bishopMoves != 0) {
				int targetSquare = popLSB(bishopMoves);
				listOfMoves[insertionIndex] = Move(square, targetSquare, 0);
				insertionIndex++;
			}

		}

		while (rookBoard != 0) {
			int square = popLSB(rookBoard);
			uint64_t rookMoves = generateRookMoves(square, occupiedBoard[1] | occupiedBoard[2]);
			rookMoves &= ~occupiedBoard[wantedPlayer];

			while (rookMoves != 0) {
				int targetSquare = popLSB(rookMoves);
				listOfMoves[insertionIndex] = Move(square, targetSquare, 0);
				insertionIndex++;
			}

		}

		while (queenBoard != 0) {
			int square = popLSB(queenBoard);
			uint64_t queenMoves = generateBishopMoves(square, occupiedBoard[1] | occupiedBoard[2]);
			queenMoves |= generateRookMoves(square, occupiedBoard[1] | occupiedBoard[2]);

			queenMoves &= ~occupiedBoard[wantedPlayer];


			while (queenMoves != 0) {
				int targetSquare = popLSB(queenMoves);
				listOfMoves[insertionIndex] = Move(square, targetSquare, 0);
				insertionIndex++;
			}
		}

		return insertionIndex;
	}
	
	void generateAttacksV3(int wantedPlayer) {
		int square = kingLocations[wantedPlayer];

		uint64_t pawnBoard = occupiedBoard[wantedPlayer] & pieceBoards[2];
		uint64_t knightBoard = occupiedBoard[wantedPlayer] & pieceBoards[3];
		uint64_t bishopBoard = occupiedBoard[wantedPlayer] & pieceBoards[4];
		uint64_t rookBoard = occupiedBoard[wantedPlayer] & pieceBoards[5];
		uint64_t queenBoard = occupiedBoard[wantedPlayer] & pieceBoards[6];

		attackingSquares[wantedPlayer] = validKingMoves[square];

		uint64_t attackBoard;

		while (pawnBoard != 0) {	// Pawn Attacks
			square = popLSB(pawnBoard);

			attackingSquares[wantedPlayer] |= validPawnCaptureMasks[square][wantedPlayer];
		}

		while (knightBoard != 0) {	// Knight Attacks
			square = popLSB(knightBoard);

			attackingSquares[wantedPlayer] |= validKnightMoves[square];
		}

		while (bishopBoard != 0) {	// Bishop Attacks
			square = popLSB(bishopBoard);
			attackBoard = generateBishopMoves(square, occupiedBoard[1] | occupiedBoard[2]);

			attackingSquares[wantedPlayer] |= attackBoard;
		}

		while (rookBoard != 0) {	// Rook Attacks
			square = popLSB(rookBoard);
			attackBoard = generateRookMoves(square, occupiedBoard[1] | occupiedBoard[2]);

			attackingSquares[wantedPlayer] |= attackBoard;
		}

		while (queenBoard != 0) {	// Queen Attacks
			square = popLSB(queenBoard);
			attackBoard = generateBishopMoves(square, occupiedBoard[1] | occupiedBoard[2]);
			attackBoard |= generateRookMoves(square, occupiedBoard[1] | occupiedBoard[2]);

			attackingSquares[wantedPlayer] |= attackBoard;
		}

	}
};