using namespace std;
#include "globals.cpp"


/*
 *	16 bit move structure in the form of FFFF/SSSSSS/EEEEEE
 *
 *
 */
class Move {
public:
	int getStartSquare() {
		return (rawValue >> 6) & 0b111111;
	}

	int getEndSquare() {
		return rawValue & 0b111111;
	}

	// 1 for en passant move
	// 2 for promote to knight
	// 3 for promote to bishop
	// 4 for promote to rook
	// 5 for promote to queen
	// 6 for castling
	// 7 for pawn push up 2x
	int getFlag() {
		return rawValue >> 12;
	}

	Move() {
		rawValue = 0;
	}

	Move(int startSquare, int endSquare, int flags) {
		rawValue = (flags << 12) | (startSquare << 6) | (endSquare);
	}

	void printMove() {
		printf("%c%d%c%d", (char)(getStartSquare() % 8 + 97), (getStartSquare() / 8 + 1), (char)(getEndSquare() % 8 + 97), (getEndSquare() / 8 + 1));
	}

	uint16_t getRawValue() {
		return rawValue;
	}
private:
	uint16_t rawValue;
};