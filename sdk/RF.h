#ifndef RF_H
#define RF_H

#include "DecisionTree.h"

struct minEval;

struct DR;

struct DT;

//enum Evaluation {gini, entropy, logLoss};

class RandomForest{
public:

long noTree;
long maxTree;
long activeTree;
long treePointer;
long rotateTime;
long timer;
long retain;
DecisionTree** DTrees = nullptr;

long height;
long Rebuild;
long f;
int* sparse;
double forget;
long maxFeature;
long noClasses;
Evaluation e;


RandomForest(long maxTree, long activeTree, long rotateTime, int height, long f, int* sparse, double forget, long maxFeature=0, long noClasses=2, Evaluation e=Evaluation::gini, long r=-1, long rb=2147483647);

void fit(double** data, long* result, long size);

long* fitThenPredict(double** trainData, long* trainResult, long trainSize, double** testData, long testSize);

void Rotate();

long Test(double* data);
};
#endif
