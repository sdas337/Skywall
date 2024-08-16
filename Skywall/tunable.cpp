#pragma once

#include "globals.h"

TuneValue rfPruningBase("rfPruningBase", 61, 25.0, 150, 5);
TuneValue rfpDepth("rfpDepth", 6, 0.0, 10, 1);
TuneValue nmpDepth("nmpDepth", 1, 0.0, 10, 1);
TuneValue nmpBaseReduction("nmpBaseReduction", 3, 0.0, 10, 1);
TuneValue nmpScaleReduction("nmpScaleReduction", 3, 1.0, 10, 1);
TuneValue fpDepth("fpDepth", 7, 0.0, 12, 1);
TuneValue fpScale("fpScale", 124, 30.0, 200, 10);
TuneValue fpMargin("fpMargin", 2, 0.0, 360, 30);
TuneValue lmpDepth("lmpDepth", 2, 0.0, 12, 1);
TuneValue lmpScale("lmpScale", 2, 1.0, 15, 2);
TuneValue lmpQuad("lmpQuad", 3, 0.0, 100, 10);
TuneValue lmpBase("lmpBase", 1, 0.0, 10, 2);
TuneValue hstPruneDepth("hstPruneDepth", 6, 0.0, 12, 1);
TuneValue hstPruneScale("hstPruneScale", 2109, 0.0, 4000, 100);
TuneValue hstPruneBase("hstPruneBase", 1117, 0.0, 2000, 100);
TuneValue hstReduction("hstReduction", 7413, 0.0, 16683, 500);
TuneValue lmrBase("lmrBase", 92, 0.0, 200, 10);
TuneValue lmrDivisor("lmrDivisor", 240, 100.0, 350, 10);
TuneValue lmrDepth("lmrDepth", 1, 0.0, 12, 1);
TuneValue lmrMoveCount("lmrMoveCount", 5, 0.0, 20, 1);
TuneValue hstMin("hstMin", 2004, 200.0, 3000, 100);
TuneValue hstQuad("hstQuad", 5, 0.0, 10, 1);
TuneValue hstLin("hstLin", 111, 0.0, 200, 10);
TuneValue hstConst("hstConst", -133, -200.0, 200, 20);
TuneValue qhstMin("qhstMin", 1614, 200.0, 3000, 20);
TuneValue qhstQuad("qhstQuad", 6, 0.0, 10, 1);
TuneValue qhstLin("qhstLin", 90, 0.0, 200, 5);
TuneValue qhstConst("qhstConst", -114, -200.0, 200, 10);
TuneValue hardTC("hardTC", 2, 1.0, 30, 1);
TuneValue tcMul("tcMul", 49, 5.0, 100, 5);
TuneValue timeMul("timeMul", 7, 1.0, 100, 3);
TuneValue incMul("incMul", 74, 5.0, 100, 10);
TuneValue aspDelta("aspDelta", 105, 10.0, 150, 10);
TuneValue aspWindow("aspWindow", 82, 3.0, 150, 5);
TuneValue iirDepth("iirDepth", 3, 0.0, 10, 1);
TuneValue None("None", 0, 0.0, 0, 1);
TuneValue mvvKing("mvvKing", 7, 0.0, 2000, 10);
TuneValue mvvPawn("mvvPawn", 89, 0.0, 2000, 10);
TuneValue mvvKnight("mvvKnight", 412, 0.0, 2000, 10);
TuneValue mvvBishop("mvvBishop", 502, 0.0, 2000, 10);
TuneValue mvvRook("mvvRook", 742, 0.0, 2000, 10);
TuneValue mvvQueen("mvvQueen", 1193, 0.0, 2000, 10);
TuneValue seeKing("seeKing", 2, 0.0, 2000, 10);
TuneValue seePawn("seePawn", 108, 0.0, 2000, 10);
TuneValue seeKnight("seeKnight", 449, 0.0, 2000, 10);
TuneValue seeBishop("seeBishop", 424, 0.0, 2000, 10);
TuneValue seeRook("seeRook", 669, 0.0, 2000, 10);
TuneValue seeQueen("seeQueen", 1096, 0.0, 2000, 10);
TuneValue seePDepth("seePDepth", 7, 0.0, 12, 1);
TuneValue seeCPScale("seeCPScale", -87, -200.0, 0, 10);
TuneValue seeQPScale("seeQPScale", -38, -200.0, 0, 10);

vector<TuneValue*> seeValues = {
	&None,
	&seeKing,
	&seePawn,
	&seeKnight,
	&seeBishop,
	&seeRook,
	&seeQueen
};

vector<TuneValue*> mvvValues = {
	&None,
	&mvvKing,
	&mvvPawn,
	&mvvKnight,
	&mvvBishop,
	&mvvRook,
	&mvvQueen
};


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
	&hstPruneDepth,
	&hstPruneScale,
	&hstPruneBase,
	&hstReduction,
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
	&iirDepth,
	&None,
	&mvvKing,
	&mvvPawn,
	&mvvKnight,
	&mvvBishop,
	&mvvRook,
	&mvvQueen,
	&seeKing,
	&seePawn,
	&seeKnight,
	&seeBishop,
	&seeRook,
	&seeQueen,
	&seePDepth,
	&seeCPScale,
	&seeQPScale
};