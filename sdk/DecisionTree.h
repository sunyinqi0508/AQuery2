#ifndef CART_H
#define CART_H

#include "Evaluation.h"

struct minEval;

struct DR;

struct DT;

//enum Evaluation {gini, entropy, logLoss};

class DecisionTree{
public:

DT* DTree = nullptr;
int maxHeight;
long feature;
long maxFeature;
long seed;
long classes;
int* Sparse;
double forgetRate;
Evaluation evalue;
long Rebuild;
long roundNo;
long called;
long retain;

DecisionTree(int hight, long f, int* sparse, double forget, long maxFeature, long noClasses, Evaluation e, long r, long rb);

void Stablelize();

void Free();

minEval findMinGiniDense(double** data, long* result, long* totalT, long size, long col);

minEval findMinGiniSparse(double** data, long* result, long* totalT, long size, long col, DT* current);

minEval incrementalMinGiniDense(double** data, long* result, long size, long col, long*** count, double** record, long* max, long newCount, long forgetSize, bool isRoot);

minEval incrementalMinGiniSparse(double** dataNew, long* resultNew, long sizeNew, long sizeOld, DT* current, long col, long forgetSize, bool isRoot);

long* fitThenPredict(double** trainData, long* trainResult, long trainSize, double** testData, long testSize);

void fit(double** data, long* result, long size);

void Update(double** data, long* result, long size, DT* current);

void IncrementalUpdate(double** data, long* result, long size, DT* current);

long Test(double* data, DT* root);

void print(DT* root);
};
#endif
