#pragma once

#include "globals.h"

using namespace std;

int lmrReductions[256][256];


// Sourced from Clarity
int popLSB(uint64_t& bitboard) {
    int lsb = std::countr_zero(bitboard);
    bitboard &= bitboard - 1;
    return lsb;
}

// Sourced from Clarity
void outputTunableJSON() {
    std::cout << "{\n";
    for (TuneValue* tunable : allTunables) {
        std::cout << "   \"" << tunable->name << "\": {\n";
        std::cout << "      \"value\": " << tunable->value << "," << std::endl;
        std::cout << "      \"min_value\": " << tunable->min << "," << std::endl;
        std::cout << "      \"max_value\": " << tunable->max << "," << std::endl;
        std::cout << "      \"step\": " << tunable->step << std::endl;
        std::cout << "   },\n";
    }
    std::cout << "}\n";
}

vector<string> split(const string word, const char seperator) {
    stringstream stream(word);
    string segment;
    vector<string> list;

    // every time that it can get a segment
    while (getline(stream, segment, seperator)) {
        // add it to the vector
        list.push_back(segment);
    }

    return list;
}

int squareNameToValue(string square) {
    int num = 0;

    int col = (int)(square[0]) - 97;
    int row = (int)(square[1]) - 49;

    num = 8 * row + col;

    return num;
}

void setupLMR() {
    for (int depth = 1; depth < 256; depth++) {
        for (int moveCount = 1; moveCount < 256; moveCount++) {
            //lmrReductions[depth][moveCount] = 2;
            // negligible difference between the duo

            double base = (double)lmrBase.value / 100.0;
            double div = (double)lmrDivisor.value / 100.0;
            lmrReductions[depth][moveCount] = (int)(base + log(depth) * log(moveCount) / div);
        }
    }
}
