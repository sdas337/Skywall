using namespace std;

#include "move.cpp"

#include <inttypes.h>
#include <vector>
#include <stack>

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

	int nodes = 0;
	int plyCount;
	int currentPlayer;

	uint64_t occupiedBoard[3];
	int kingLocations[3];
	uint64_t attackingSquares[3];


	stack<BoardStateInformation> boardStates;

	// no piece  = 0, king = 1, pawn = 2, knight = 3, bishop = 4, rook = 5, queen = 6, white = 8, black = 16
	void setPiece(int row, int col, int piece) {
		int square = 8 * row + col;
		rawBoard[square] = piece;
		occupiedBoard[piece / 8] |= (1ull) << square;
	}

	bool isWhitePiece(int row, int col) {
		return (rawBoard[8 * row + col] & 8) == 8;
	}

	void makeMove(Move move) {
		boardStates.push(boardStates.top());

		int startSquare = move.getStartSquare();
		int targetSquare = move.getEndSquare();
		int flags = move.getFlag();
		bool capturedPiece = rawBoard[targetSquare] != 0;

		boardStates.top().fiftyMoveCount++;
		if (capturedPiece || (rawBoard[startSquare] & 7) == 2) {
			boardStates.top().fiftyMoveCount = 0;
		}

		// Actual chess move
		int movingPiece = rawBoard[startSquare];
		boardStates.top().capturedPieceType = rawBoard[targetSquare];

		rawBoard[startSquare] = 0;
		rawBoard[targetSquare] = movingPiece;

		occupiedBoard[movingPiece / 8] &= ~(1ull << startSquare);
		occupiedBoard[movingPiece / 8] |= (1ull << targetSquare);
		occupiedBoard[(movingPiece / 8 % 2) + 1] &= ~(1ull << targetSquare);

		boardStates.top().enPassantSquare = 64;

		// double pawn push
		if (flags == 7) {
			boardStates.top().enPassantSquare = targetSquare + (currentPlayer == 1 ? -8 : 8);
		}

		// Handle Castling
		else if (flags == 6) {
			if (targetSquare % 8 > 3) {	// Queenside vs Kingside
				rawBoard[(targetSquare / 8) * 8 + 5] = rawBoard[(targetSquare / 8) * 8 + 7];
				rawBoard[(targetSquare / 8) * 8 + 7] = 0;

				occupiedBoard[movingPiece / 8] &= ~(1ull << ((targetSquare / 8) * 8 + 7));
				occupiedBoard[movingPiece / 8] |= (1ull << ((targetSquare / 8) * 8 + 5));

			} {
				rawBoard[(targetSquare / 8) * 8 + 3] = rawBoard[(targetSquare / 8) * 8];
				rawBoard[(targetSquare / 8) * 8] = 0;

				occupiedBoard[movingPiece / 8] &= ~(1ull << ((targetSquare / 8) * 8));
				occupiedBoard[movingPiece / 8] |= (1ull << ((targetSquare / 8) * 8 + 3));
			}

			// Remove castling rights for that side
			boardStates.top().castlingRights[2 * currentPlayer - 2] = false;
			boardStates.top().castlingRights[2 * currentPlayer - 1] = false;
		}

		// Handle promotion
		else if (flags > 1 && flags < 6) {
			rawBoard[targetSquare] = (currentPlayer * 8) | (flags + 1);
		}
		// En passant capture
		else if (flags == 1) {
			int capturedSquare = targetSquare + (currentPlayer == 1 ? -8 : 8);
			boardStates.top().capturedPieceType = rawBoard[capturedSquare];
			rawBoard[capturedSquare] = 0;

			occupiedBoard[(movingPiece / 8 % 2) + 1] &= ~(1ull << capturedSquare);
		}

		// Remove relevant castling rights
		if ((rawBoard[targetSquare] & 7) == 1) {	// King check
			boardStates.top().castlingRights[2 * currentPlayer - 2] = false;
			boardStates.top().castlingRights[2 * currentPlayer - 1] = false;

			kingLocations[currentPlayer] = targetSquare;
		}

		if ((rawBoard[targetSquare] & 7) == 5) {	// Rook Check
			if (startSquare % 8 > 3) { // Queenside vs Kingside
				boardStates.top().castlingRights[2 * currentPlayer - 1] = false;
			}
			else {
				boardStates.top().castlingRights[2 * currentPlayer - 2] = false;
			}
		}

		plyCount++;
		currentPlayer = currentPlayer % 2 + 1;
	}

	void undoMove(Move move) {
		int startSquare = move.getStartSquare();
		int targetSquare = move.getEndSquare();
		int flags = move.getFlag();
		bool capturedPiece = rawBoard[targetSquare] != 0;

		/*if (startSquare == 16 && targetSquare == 33) {
			printf("Undoing relevant move\n");
		}*/

		int movingPiece = rawBoard[targetSquare];

		rawBoard[startSquare] = movingPiece;
		rawBoard[targetSquare] = boardStates.top().capturedPieceType;

		occupiedBoard[movingPiece / 8] &= ~(1ull << targetSquare);
		occupiedBoard[movingPiece / 8] |= (1ull << startSquare);

		if (boardStates.top().capturedPieceType) {
			occupiedBoard[(movingPiece / 8 % 2) + 1] |= (1ull << targetSquare);
		}

		if (flags == 6) {
			if (targetSquare % 8 > 3) {	// Queenside vs Kingside
				rawBoard[(targetSquare / 8) * 8 + 7] = rawBoard[(targetSquare / 8) * 8 + 5];
				rawBoard[(targetSquare / 8) * 8 + 5] = 0;

				occupiedBoard[movingPiece / 8] &= ~(1ull << ((targetSquare / 8) * 8 + 5));
				occupiedBoard[movingPiece / 8] |= (1ull << ((targetSquare / 8) * 8 + 7));
			} {
				rawBoard[(targetSquare / 8) * 8] = rawBoard[(targetSquare / 8) * 8 + 3];
				rawBoard[(targetSquare / 8) * 8 + 3] = 0;

				occupiedBoard[movingPiece / 8] &= ~(1ull << ((targetSquare / 8) * 8 + 3));
				occupiedBoard[movingPiece / 8] |= (1ull << ((targetSquare / 8) * 8 + 0));
			}
		}
		else if (flags > 1 && flags < 6) {	// Reset to pawn
			rawBoard[startSquare] = (currentPlayer * 8) | 2;
		}
		else if (flags == 1) {	// En passant
			int capturedSquare = targetSquare + (currentPlayer == 1 ? -8 : 8);
			rawBoard[capturedSquare] = boardStates.top().capturedPieceType;

			occupiedBoard[(movingPiece / 8 % 2) + 1] |= (1ull << capturedSquare);
		}

		// King location check
		if ((rawBoard[startSquare] & 7) == 1) {	// King check
			kingLocations[(currentPlayer % 2) + 1] = startSquare;
		}

		boardStates.pop();
		plyCount--;
		currentPlayer = currentPlayer % 2 + 1;

	}

	// Checking if we're in check 
	bool sideInCheck(int player) {
		int otherPlayer = player % 2 + 1;

		// perform several partial move gens beginning from other player's king location
		vector<Move> knightMoves;
		generateKnightMoves(kingLocations[player], knightMoves, player);
		
		for (Move m : knightMoves) {
			/*m.printMove();
			printf("\n");*/
			if (rawBoard[m.getEndSquare()] == (otherPlayer * 8 | 3)) {
				return true;
			}
		}

		vector<Move> diagonalMoves;
		generateSlidingMoves(kingLocations[player], 4, diagonalMoves, player);

		for (Move m : diagonalMoves) {
			if (rawBoard[m.getEndSquare()] == (otherPlayer * 8 | 4) || rawBoard[m.getEndSquare()] == (otherPlayer * 8 | 6)) {
				return true;
			}
		}

		vector<Move> straightMoves;
		generateSlidingMoves(kingLocations[player], 5, straightMoves, player);

		for (Move m : straightMoves) {
			if (rawBoard[m.getEndSquare()] == (otherPlayer * 8 | 5) || rawBoard[m.getEndSquare()] == (otherPlayer * 8 | 6)) {
				return true;
			}
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

			if (rawBoard[targetSquare] == (otherPlayer * 8 | 2)) {
				return true;
			}
		}

		return false;
		//return (attackingSquares[otherPlayer] & (1ull << kingLocations[currentPlayer])) == 1;

	}

	char prettyPiecePrint(int row, int col) {
		int pieceAtLoc = rawBoard[8 * row + col] & 7;

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
		for (int i = 7; i >= 0; i--) {
			printf("---------------------------------\n");
			for (int j = 0; j < 8; j++) {
				printf("| ");

				printf("%c ", prettyPiecePrint(i, j));
			}

			printf("|\n");
		}
		printf("---------------------------------\n");

	}

	void loadBoardFromFen(string fen) {
		bool castlingRights[4];
		int enPassantSquare = -1;
		int fiftyMoveCount = 0;

		int row = 7;
		int col = 0;

		int fenIndex = 0;
		string current = fen.substr(0, fen.find(" "));
		string remainder = fen.substr(fen.find(" "));

		while (fenIndex < current.length()) {
			char currentItem = current[fenIndex];

			if (currentItem > 65) {	// ASCII value of A aka piece letters
				switch (currentItem)
				{
				case 'k':
					setPiece(row, col, 17);
					kingLocations[2] = 8 * row + col;
					break;
				case 'p':
					setPiece(row, col, 18);
					break;
				case 'n':
					setPiece(row, col, 19);
					break;
				case 'b':
					setPiece(row, col, 20);
					break;
				case 'r':
					setPiece(row, col, 21);
					break;
				case 'q':
					setPiece(row, col, 22);
					break;
				case 'K':
					setPiece(row, col, 9);
					kingLocations[1] = 8 * row + col;
					break;
				case 'P':
					setPiece(row, col, 10);
					break;
				case 'N':
					setPiece(row, col, 11);
					break;
				case 'B':
					setPiece(row, col, 12);
					break;
				case 'R':
					setPiece(row, col, 13);
					break;
				case 'Q':
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
					setPiece(row, col, 0);
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
			int row = (int)(current[0] - 97);
			int col = (int)(current[1] - 49);
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
	}

	Board() {
		for (int i = 0; i < 8; i++) {	// Row
			for (int j = 0; j < 8; j++) {	// Col
				setPiece(i, j, 0);
			}
		}
		
		string startingBoardPos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
		//string startingBoardPos = "4R3/2r3p1/5bk1/1p1r3p/p2PR1P1/P1BK1P2/1P6/8 b - - 0 1";
		loadBoardFromFen(startingBoardPos);

		precomputeDistances();

		printf("Finished setting up board.\n");
	}


	vector<Move> generateLegalMovesV2() {	// Using attack boards
		vector<Move> allMoves = generatePseudoLegalMoves(currentPlayer);
		vector<Move> validMoves;
		int origCurrentPlayer = currentPlayer;

		for (int i = 0; i < allMoves.size(); i++) {
			Move move = allMoves[i];
			bool moveStatus = true;
			/*if (move.getEndSquare() == 18 && move.getStartSquare() == 11) {
				printf("Error move\n");
			}*/

			makeMove(move);
			
			moveStatus = sideInCheck(origCurrentPlayer);
			undoMove(move);

			if (!moveStatus) {
				validMoves.push_back(move);
			}
		}

		return validMoves;

	}

	vector<Move> generateLegalMoves() {	// Will have to improve eventually
		vector<Move> allMoves = generatePseudoLegalMoves(currentPlayer);
		vector<Move> validMoves;

		for (int i = 0; i < allMoves.size(); i++) {
			Move move = allMoves[i];
			bool moveStatus = true;
			makeMove(move);
			vector<Move> responseMoves = generatePseudoLegalMoves(currentPlayer);
			for (Move response : responseMoves) {
				if ((rawBoard[response.getEndSquare()] & 7) == 1) {
					// Invalid move due to taking a checked king
					moveStatus = false;
					break;
				}
			}
			undoMove(move);

			if (moveStatus) {
				validMoves.push_back(move);
			}
		}

		return validMoves;

	}

private:
	int directions[8] = { 8, -8, -1, 1, -7, 7, -9, 9 };
	int distanceFromEdge[64][8];
	int validKnightMoves[64][8];

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
				validKnightMoves[i][j] = -1;
			}
		}

		// Each valid knight move available
		for (int square = 0; square < 64; square++) {
			int insertionPoint = 0;
			for (int i = 0; i < 8; i++) {
				int targetSquare = square + knightIndices[i];
				if (targetSquare >= 0 && targetSquare <= 64) {
					int rank = targetSquare / 8;
					int file = targetSquare % 8;

					int distMoved = max(abs(square / 8 - rank), abs(square % 8 - file));
					if (distMoved == 2) {
						validKnightMoves[square][insertionPoint] = targetSquare;
						insertionPoint++;
					}
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

		/*for (int col = 0; col < 8; col++) {
			int square = 8 + col;
			validPawnMoveMasks[square][1] = validPawnMoveMasks[square][1] | (1ull << (square + 16));
		}*/

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

		/*for (int col = 0; col < 8; col++) {
			int square = 48 + col;
			validPawnMoveMasks[square][2] = validPawnMoveMasks[square][2] | (1ull << (square - 16));
			//printf("0x%" PRIx64 "\n", validPawnMoveMasks[square][2]);
		}
		*/
	}

	void generateKingMoves(int square, vector<Move>& moves) {
		// Need to make sure king isn't next to other king

		for (int i = 0; i < 8; i++) {
			if (distanceFromEdge[square][i] > 0) {
				int targetSquare = square + directions[i];
				int targetPiece = rawBoard[targetSquare];

				if (targetPiece != 0) {
					if ((targetPiece >> 3) == currentPlayer) {
						continue;
					}
					moves.push_back(Move(square, targetSquare, 0));
				}
				else {
					moves.push_back(Move(square, targetSquare, 0));
				}
			}
		}

		// Castling move addition

		return;
	}

	void generatePawnMovesV2(int square, vector<Move>& moves) {
		uint64_t otherOccupiedBoard = occupiedBoard[currentPlayer % 2 + 1];
		uint64_t totalOccupiedBoard = occupiedBoard[1] | occupiedBoard[2];

		uint64_t currentPawnMask = validPawnMoveMasks[square][currentPlayer];
		uint64_t currentFileMask = 0x101010101010101 << (square % 8);
		uint64_t unblockedLocationsMask = 0xffull << (square / 8 * 8);

		if (currentPlayer == 1) {
			unblockedLocationsMask = unblockedLocationsMask << 8;
		}
		else {
			unblockedLocationsMask = unblockedLocationsMask >> 8;
		}

		uint64_t validForwardPawnMoves = (currentPawnMask & currentFileMask) & ~(totalOccupiedBoard)&unblockedLocationsMask;

		if (validForwardPawnMoves != 0) {
			if (square / 8 == 1 && currentPlayer == 1) {
				int targetSquare = square + 16;
				if ((rawBoard[targetSquare] & 7) == 0)
					moves.push_back(Move(square, targetSquare, 7));
			}
			else if (square / 8 == 6 && currentPlayer == 2) {
				int targetSquare = square - 16;
				if ((rawBoard[targetSquare] & 7) == 0)
					moves.push_back(Move(square, targetSquare, 7));
			}
		}

		uint64_t captureFiles = 0;
		if (square % 8 > 0) {
			captureFiles |= (currentFileMask >> 1);
		}
		if (square % 8 < 7) {
			captureFiles |= (currentFileMask << 1);
		}
		uint64_t validCapturePawnMoves = (currentPawnMask & captureFiles) & (otherOccupiedBoard | 1ull << boardStates.top().enPassantSquare);

		uint64_t validPawnMoves = validCapturePawnMoves | validForwardPawnMoves;

		while (validPawnMoves != 0) {
			int targetSquare = popLSB(validPawnMoves);

			if (targetSquare / 8 == 0 || targetSquare / 8 == 7) {
				for (int flag = 2; flag <= 5; flag++) {
					moves.push_back(Move(square, targetSquare, flag));
				}
			}
			else {
				moves.push_back(Move(square, targetSquare, 0));
			}
		}
	}

	void generateSlidingMoves(int square, int pieceType, vector<Move>& moves, int safePlayer) {

		int startMoveTypes = 0;
		int endMoveTypes = 8;

		if (pieceType == 4) {
			startMoveTypes = 4;
		}
		else if (pieceType == 5) {
			endMoveTypes = 4;
		}

		// Generate slidingMoves
		for (int i = startMoveTypes; i < endMoveTypes; i++) {
			for (int j = 1; j <= distanceFromEdge[square][i]; j++) {
				int targetSquare = square + directions[i] * j;
				int targetPiece = rawBoard[targetSquare];

				if (targetPiece != 0) {
					if ((targetPiece / 8) == safePlayer) {
						break;
					}
					moves.push_back(Move(square, targetSquare, 0));
					break;
				}
				else {
					moves.push_back(Move(square, targetSquare, 0));
				}
			}
		}

		return;
	}

	void generateKnightMoves(int square, vector<Move>& moves, int safePlayer) {

		for (int i = 0; i < 8; i++) {
			int targetSquare = validKnightMoves[square][i];
			if (targetSquare == -1)	// Finished list of possible knight moves
				break;

			if (rawBoard[targetSquare] / 8 == safePlayer)
				continue;

			moves.push_back(Move(square, targetSquare, 0));
		}

		return;
	}

	vector<Move> generatePseudoLegalMoves(int wantedPlayer) {
		vector<Move> listOfMoves;

		for (int square = 0; square < 64; square++) {
			int pieceOnBoard = rawBoard[square];
			int pieceTypeAtBoard = pieceOnBoard & 7;

			if (pieceOnBoard / 8 == wantedPlayer) {		// This piece can move
				switch (pieceTypeAtBoard)
				{
				case 1: //King
					generateKingMoves(square, listOfMoves);
					break;
				case 2:	// Pawn
					generatePawnMovesV2(square, listOfMoves);
					break;
				case 3: // Knight
					generateKnightMoves(square, listOfMoves, currentPlayer);
					break;
				case 4: // Bishop
					generateSlidingMoves(square, 4, listOfMoves, currentPlayer);
					break;
				case 5: // Rook
					generateSlidingMoves(square, 5, listOfMoves, currentPlayer);
					break;
				case 6: // Queen
					generateSlidingMoves(square, 6, listOfMoves, currentPlayer);
					break;
				default:
					printf("ERROR. Invalid piece type for move generation. \n.");
					break;
				}
			}
		}

		attackingSquares[wantedPlayer] = 0ull;
		for (Move m : listOfMoves) {
			attackingSquares[wantedPlayer] |= (1ull << m.getEndSquare());
		}

		return listOfMoves;
	}

};