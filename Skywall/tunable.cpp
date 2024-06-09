#pragma once

#include "globals.h"

TuneValue rfPruningBase("rfPruningBase", 95, 25, 150, 5);
TuneValue rfpDepth("rfpDepth", 5, 0, 10, 1);

TuneValue nmpDepth("nmpDepth", 3, 0, 10, 1);
TuneValue nmpBaseReduction("nmpBaseReduction", 3, 0, 10, 1);
TuneValue nmpScaleReduction("nmpScaleReduction", 5, 1, 10, 1);

TuneValue fpDepth("fpDepth", 8, 0, 12, 1);
TuneValue fpScale("fpScale", 150, 30, 200, 10);
TuneValue fpMargin("fpMargin", 0, 0, 360, 30);

TuneValue lmpDepth("lmpDepth", 5, 0, 12, 1);
TuneValue lmpScale("lmpScale", 7, 1, 15, 2);
TuneValue lmpBase("lmpBase", 2, 0, 10, 2);

TuneValue lmrBase("lmrBase", 77, 0, 200, 10);
TuneValue lmrDivisor("lmrDivisor", 226, 100, 350, 10);
TuneValue lmrDepth("lmrDepth", 2, 0, 12, 1);
TuneValue lmrMoveCount("lmrMoveCount", 6, 0, 20, 1);

TuneValue hstMin("hstMin", 1896, 200, 3000, 100);
TuneValue hstQuad("hstQuad", 4, 0, 10, 1);
TuneValue hstLin("hstLin", 120, 0, 200, 10);
TuneValue hstConst("hstConst", -120, -200, 200, 20);

TuneValue hardTC("hardTC", 2, 1, 30, 1);
TuneValue tcMul("tcMul", 60, 5, 100, 5);
TuneValue timeMul("timeMul", 5, 1, 100, 3);
TuneValue incMul("incMul", 75, 5, 100, 10);

TuneValue aspDelta("aspDelta", 70, 10, 150, 10);
TuneValue aspWindow("aspWindow", 15, 3, 150, 5);


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