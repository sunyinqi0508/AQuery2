#ifndef EVALUATION_H
#define EVALUATION_H

struct minEval;

enum Evaluation {gini, entropy, logLoss};

minEval giniSparse(double** data, long* result, long* d, long size, long col, long classes, long* totalT);

minEval entropySparse(double** data, long* result, long* d, long size, long col, long classes, long* totalT);

minEval giniSparseIncremental(long sizeTotal, long classes, double* newSortedData, long* newSortedResult, long* T);

minEval entropySparseIncremental(long sizeTotal, long classes, double* newSortedData, long* newSortedResult, long* T);

minEval giniDense(long max, long size, long classes, long** rem, long* d, double* record, long* totalT);

minEval entropyDense(long max, long size, long classes, long** rem, long* d, double* record, long* totalT);

minEval giniDenseIncremental(long max, double* record, long** count, long classes, long newSize, long* T);

minEval entropyDenseIncremental(long max, double* record, long** count, long classes, long newSize, long* T);

#endif
