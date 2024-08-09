#pragma once
#include "globals.h"


using namespace std;

/*
 *	16 bit move structure in the form of FFFF/SSSSSS/EEEEEE
 *
 *
 */
class Move {
public:
	constexpr int getStartSquare() {
		return (rawValue >> 6) & 0b111111;
	}

	constexpr int getEndSquare() {
		return rawValue & 0b111111;
	}

	// 1 for en passant move
	// 2 for promote to knight
	// 3 for promote to bishop
	// 4 for promote to rook
	// 5 for promote to queen
	// 6 for castling
	// 7 for pawn push up 2x
	constexpr int getFlag() {
		return rawValue >> 12;
	}

	constexpr Move() {
		rawValue = 0;
	}

	Move(int startSquare, int endSquare, int flags) {
		rawValue = (flags << 12) | (startSquare << 6) | (endSquare);
	}


	string printMove() {
		string result = "";
		result += (char)(getStartSquare() % 8 + 97);
		result += (char)(getStartSquare() / 8 + 49);
		result += (char)(getEndSquare() % 8 + 97);
		result += (char)(getEndSquare() / 8 + 49);
		return result;
	}

	uint16_t getRawValue() {
		return rawValue;
	}

	bool isPromotion() {
		return getFlag() > 1 && getFlag() < 6;
	}

	uint16_t rawValue;

private:

};

bool operator==(const Move &lhs, const Move &rhs) {
	return lhs.rawValue == rhs.rawValue;
}