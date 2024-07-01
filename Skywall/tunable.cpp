#pragma once

#include "globals.h"

TuneValue rfPruningBase("rfPruningBase", 57, 25, 150, 5);
TuneValue rfpDepth("rfpDepth", 5, 0, 10, 1);

TuneValue nmpDepth("nmpDepth", 3, 0, 10, 1);
TuneValue nmpBaseReduction("nmpBaseReduction", 3, 0, 10, 1);
TuneValue nmpScaleReduction("nmpScaleReduction", 5, 1, 10, 1);

TuneValue fpDepth("fpDepth", 6, 0, 12, 1);
TuneValue fpScale("fpScale", 141, 30, 200, 10);
TuneValue fpMargin("fpMargin", 22, 0, 360, 30);

TuneValue lmpDepth("lmpDepth", 3, 0, 12, 1);
TuneValue lmpScale("lmpScale", 8, 1, 15, 2);
TuneValue lmpBase("lmpBase", 1, 0, 10, 2);

TuneValue lmrBase("lmrBase", 73, 0, 200, 10);
TuneValue lmrDivisor("lmrDivisor", 237, 100, 350, 10);
TuneValue lmrDepth("lmrDepth", 1, 0, 12, 1);
TuneValue lmrMoveCount("lmrMoveCount", 5, 0, 20, 1);

TuneValue hstMin("hstMin", 2025, 200, 3000, 100);
TuneValue hstQuad("hstQuad", 4, 0, 10, 1);
TuneValue hstLin("hstLin", 109, 0, 200, 10);
TuneValue hstConst("hstConst", -100, -200, 200, 20);

TuneValue hardTC("hardTC", 4, 1, 30, 1);
TuneValue tcMul("tcMul", 48, 5, 100, 5);
TuneValue timeMul("timeMul", 5, 1, 100, 3);
TuneValue incMul("incMul", 59, 5, 100, 10);

TuneValue aspDelta("aspDelta", 84, 10, 150, 10);
TuneValue aspWindow("aspWindow", 74, 3, 150, 5);


vector<TuneValue *> allTunables = {
	&rfPruningBase,
	&rfpDepth,
	&nmpDepth,
	&nmpBaseReduction,
	&nmpScaleReduction,
	&fpDepth,
	&fpScale,
	&fpMargin,
	&lmpDepth,
	&lmpScale,
	&lmpBase,
	&lmrBase,
	&lmrDivisor,
	&lmrDepth,
	&lmrMoveCount,
	&hstMin,
	&hstQuad,
	&hstLin,
	&hstConst,
	&hardTC,
	&tcMul,
	&timeMul,
	&incMul,
	&aspDelta,
	&aspWindow
};