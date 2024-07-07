#pragma once

#include "globals.h"

TuneValue rfPruningBase("rfPruningBase", 62, 25, 150, 5);
TuneValue rfpDepth("rfpDepth", 5, 0, 10, 1);

TuneValue nmpDepth("nmpDepth", 3, 0, 10, 1);
TuneValue nmpBaseReduction("nmpBaseReduction", 3, 0, 10, 1);
TuneValue nmpScaleReduction("nmpScaleReduction", 3, 1, 10, 1);

TuneValue fpDepth("fpDepth", 7, 0, 12, 1);
TuneValue fpScale("fpScale", 125, 30, 200, 10);
TuneValue fpMargin("fpMargin", 10, 0, 360, 30);

TuneValue lmpDepth("lmpDepth", 1, 0, 12, 1);
TuneValue lmpQuad("lmpQuad", 4, 0, 100, 10);
TuneValue lmpScale("lmpScale", 3, 1, 15, 2);
TuneValue lmpBase("lmpBase", 2, 0, 10, 2);

TuneValue lmrBase("lmrBase", 88, 0, 200, 10);
TuneValue lmrDivisor("lmrDivisor", 246, 100, 350, 10);
TuneValue lmrDepth("lmrDepth", 1, 0, 12, 1);
TuneValue lmrMoveCount("lmrMoveCount", 5, 0, 20, 1);

TuneValue hstMin("hstMin", 2008, 200, 3000, 100);
TuneValue hstQuad("hstQuad", 5, 0, 10, 1);
TuneValue hstLin("hstLin", 120, 0, 200, 10);
TuneValue hstConst("hstConst", -126, -200, 200, 20);

TuneValue qhstMin("qhstMin", 1620, 200, 3000, 20);
TuneValue qhstQuad("qhstQuad", 6, 0, 10, 1);
TuneValue qhstLin("qhstLin", 87, 0, 200, 5);
TuneValue qhstConst("qhstConst", -111, -200, 200, 10);

TuneValue hardTC("hardTC", 4, 1, 30, 1);
TuneValue tcMul("tcMul", 50, 5, 100, 5);
TuneValue timeMul("timeMul", 4, 1, 100, 3);
TuneValue incMul("incMul", 66, 5, 100, 10);

TuneValue aspDelta("aspDelta", 90, 10, 150, 10);
TuneValue aspWindow("aspWindow", 80, 3, 150, 5);

TuneValue iirDepth("iirDepth", 4, 0, 10, 1);


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
	&lmpQuad,
	&lmpBase,
	&lmrBase,
	&lmrDivisor,
	&lmrDepth,
	&lmrMoveCount,
	&hstMin,
	&hstQuad,
	&hstLin,
	&hstConst,
	&qhstMin,
	&qhstQuad,
	&qhstLin,
	&qhstConst,
	&hardTC,
	&tcMul,
	&timeMul,
	&incMul,
	&aspDelta,
	&aspWindow,
	&iirDepth
};