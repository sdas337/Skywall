#include "globals.h"
#include "eval.h"

#include "search.h"

using namespace std;

extern Board testBoard;

uint64_t moveChecker(int depth, bool testingCaptures);

void importPerftTest();

void perftTest();

void completePerftTest();

void movegenBenchmark();

void seeTest();
void evalTuningTest();
void bench(int depth);
