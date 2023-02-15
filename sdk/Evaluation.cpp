#include "Evaluation.h"
#include <cfloat>
#include <math.h>
#include <cstdio>

struct minEval{
        double value;
        int* values;

	double eval;
        long left; // how many on its left
        double* record;
        long max;
        long** count;
};

minEval giniSparse(double** data, long* result, long* d, long size, long col, long classes, long* totalT){
	double max = data[d[size-1]][col];
	minEval ret;
	ret.eval = DBL_MAX;

	long i, j;
	long count[classes];
        long total = 0;
        for(i=0; i<classes; i++)count[i]=0;
        double gini1, gini2;
        double c;
        long l, r;
        for(i=0; i<size-1; i++){
                c = data[d[i]][col];
                if(c==max)break;
                count[result[d[i]]]++;
                total++;
                if(c==data[d[i+1]][col])continue;
                gini1 = 1.0;
                gini2 = 1.0;
                for(j=0;j<classes;j++){
                        l = count[j];
                        r = totalT[j]-l;
                        gini1 -= pow((double)l/total, 2);
                        gini2 -= pow((double)r/(size-total), 2);
                }
                gini1 = gini1*total/size + gini2*(size-total)/size;
                if(ret.eval>gini1){
                        ret.eval = gini1;
                        ret.value = c;
                        ret.left = total;
                }
        }
	return ret;
}

minEval entropySparse(double** data, long* result, long* d, long size, long col, long classes, long* totalT){
        double max = data[d[size-1]][col];
        minEval ret;
        ret.eval = DBL_MAX;

        long i, j;
        long count[classes];
        long total = 0;
        for(i=0; i<classes; i++)count[i]=0;
        double entropy1, entropy2;
        double c;
        long l, r;
        for(i=0; i<size-1; i++){
                c = data[d[i]][col];
                if(c==max)break;
                count[result[d[i]]]++;
                total++;
                if(c==data[d[i+1]][col])continue;
                entropy1 = 0;
                entropy2 = 0;
                for(j=0;j<classes;j++){
                        l = count[j];
                        r = totalT[j]-l;
                        if(l!=0)entropy1 -= ((double)l/total)*log((double)l/total);
                        if(r!=0)entropy2 -= ((double)r/(size-total))*log((double)r/(size-total));
                }
                entropy1 = entropy1*total/size + entropy2*(size-total)/size;
                if(ret.eval>entropy1){
                        ret.eval = entropy1;
                        ret.value = c;
                        ret.left = total;
                }
        }
        return ret;
}

minEval giniSparseIncremental(long sizeTotal, long classes, double* newSortedData, long* newSortedResult, long* T){
	long l, r, i, j;
	minEval ret;
	ret.eval = DBL_MAX;
        double gini1, gini2;
        long count[classes];
        long total = 0;
        for(i=0; i<classes; i++)count[i]=0;
        double c, max=newSortedData[sizeTotal-1]; // largest value 

        for(i=0; i<sizeTotal; i++){
                c = newSortedData[i];
                if(c==max)break;
                count[newSortedResult[i]]++;
                total++;
                if(c==newSortedData[i+1])continue;
                gini1 = 1.0;
                gini2 = 1.0;
                for(j=0;j<classes;j++){
                        l = count[j];
                        r = T[j]-l;
                        gini1 -= pow((double)l/total, 2);
                        gini2 -= pow((double)r/(sizeTotal-total), 2);
                }
                gini1 = (gini1*total)/sizeTotal + (gini2*(sizeTotal-total))/sizeTotal;
                if(ret.eval>gini1){
                        ret.eval = gini1;
                        ret.value = c;
                }
        }
	return ret;
}

minEval entropySparseIncremental(long sizeTotal, long classes, double* newSortedData, long* newSortedResult, long* T){
        long l, r, i, j;
        minEval ret;
        ret.eval = DBL_MAX;
        double e1, e2;
        long count[classes];
        long total = 0;
        for(i=0; i<classes; i++)count[i]=0;
        double c, max=newSortedData[sizeTotal-1]; // largest value 

        for(i=0; i<sizeTotal; i++){
                c = newSortedData[i];
                if(c==max)break;
                count[newSortedResult[i]]++;
                total++;
                if(c==newSortedData[i+1])continue;
                e1 = 0;
                e2 = 0;
                for(j=0;j<classes;j++){
                        l = count[j];
                        r = T[j]-l;
                        if(l!=0)e1 -= ((double)l/total)*log((double)l/total);
                        if(r!=0)e2 -= ((double)r/(sizeTotal-total))*log((double)r/(sizeTotal-total));
                }
                e1 = e1*total/sizeTotal + e2*(sizeTotal-total)/sizeTotal;
                if(ret.eval>e1){
                        ret.eval = e1;
                        ret.value = c;
                }
        }
        return ret;
}

minEval giniDense(long max, long size, long classes, long** rem, long* d, double* record, long* totalT){
	minEval ret;
	ret.eval = DBL_MAX;

	double gini1, gini2;
	long *t, *t2, *r, *r2, i, j;
        for(i=0;i<max;i++){
                t = rem[i];
                if(i>0){
                        t2 = rem[i-1];
                        for(j=0;j<=classes;j++){
                                t[j]+=t2[j];
                        }
                }
                if(t[classes]>=size)break;
                gini1 = 1.0;
                gini2 = 1.0;
                for(j=0;j<classes;j++){
                        long l, r;
                        l = t[j];
                        r = totalT[j]-l;
                        gini1 -= pow((double)l/t[classes], 2);
                        gini2 -= pow((double)r/(size-t[classes]), 2);
                }
                gini1 = (gini1*t[classes])/size + (gini2*(size-t[classes]))/size;
                if(gini1<ret.eval){
                        ret.eval = gini1;
                        ret.value = record[i];
                        ret.left = t[classes];
                }
	}
	return ret;
}

minEval entropyDense(long max, long size, long classes, long** rem, long* d, double* record, long* totalT){
	minEval ret;
	ret.eval = DBL_MAX;

	double entropy1, entropy2;
	long *t, *t2, *r, *r2, i, j;
        for(i=0;i<max;i++){
                t = rem[i];
                if(i>0){
                        t2 = rem[i-1];
                        for(j=0;j<=classes;j++){
                                t[j]+=t2[j];
                        }
                }
                if(t[classes]>=size)break;
                entropy1 = 0;
                entropy2 = 0;
                for(j=0;j<classes;j++){
                        long l, r;
                        l = t[j];
                        r = totalT[j]-l;
                        if(l!=0)entropy1 -= ((double)l/t[classes])*log((double)l/t[classes]);
                        if(r!=0)entropy2 -= ((double)r/(size-t[classes]))*log((double)r/(size-t[classes]));
                }
                entropy1 = entropy1*t[classes]/size + entropy2*(size-t[classes])/size;
                if(entropy1<ret.eval){
                        ret.eval = entropy1;
                        ret.value = record[i];
                        ret.left = t[classes];
                }
        }
	return ret;
}
