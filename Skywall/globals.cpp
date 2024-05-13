using namespace std;

#include <string>

// Sourced from Clarity
int popLSB(uint64_t& bitboard) {
    int lsb = std::countr_zero(bitboard);
    bitboard &= bitboard - 1;
    return lsb;
}

