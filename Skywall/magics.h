#include "globals.h"

using namespace std;


extern uint64_t lookup_table[88507];

struct blackMagicEntries {
	uint64_t negMask;
	uint64_t blackMagic;
	uint64_t tableOffset;
};

extern int rookShift, bishopShift;

extern blackMagicEntries refinedRookMagics[64];
extern blackMagicEntries refinedBishopMagics[64];

uint64_t generateSlidingMoveBitboard(int square, int pieceType, uint64_t blockerBitboard);
void generateMagics();
