#ifndef CART_H
#define CART_H

#include "Evaluation.h"

struct minEval;

struct DR;

struct DT;

class DecisionTree
{
public:
    DT *DTree = nullptr;
    double minIG;
    long maxHeight;
    long feature;
    long maxFeature;
    bool isRF;
    long classes;
    int *Sparse;
    double forgetRate;
    double increaseRate;
    double initialIR;
    Evaluation evalue;
    long Rebuild;
    long roundNo;
    long called;
    long retain;
    long lastT;
    long lastAll;

    DecisionTree(long f, int *sparse, double forget, long maxFeature, long noClasses, Evaluation e);

    void Stablelize();

    void Free();

    minEval findMinGiniDense(double **data, long *result, long *totalT, long size, long col);

    minEval findMinGiniSparse(double **data, long *result, long *totalT, long size, long col, DT *current);

    minEval incrementalMinGiniDense(double **data, long *result, long size, long col, long ***count, double **record, long *max, long newCount, long forgetSize, double **forgottenData, long *forgottenClass);

    minEval incrementalMinGiniSparse(double **dataNew, long *resultNew, long sizeNew, long sizeOld, DT *current, long col, long forgetSize, double **forgottenData, long *forgottenClass);

    long *fitThenPredict(double **trainData, long *trainResult, long trainSize, double **testData, long testSize);

    void fit(double **data, long *result, long size);

    void Update(double **data, long *result, long size, DT *current);

    void IncrementalUpdate(double **data, long *result, long size, DT *current);

    long Test(double *data, DT *root);

    void print(DT *root);
};
#endif
