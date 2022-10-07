#include "DecisionTree.h"
#include "aquery.h"
// __AQ_NO_SESSION__
#include "../server/table.h"

DecisionTree* dt = nullptr;
long pt = 0;
double** data = nullptr;
long* result = nullptr;

__AQEXPORT__(bool) newtree(int height, long f, ColRef<int> sparse, double forget, long maxf, long noclasses, Evaluation e, long r, long rb){
	if(sparse.size!=f)return 0;
	int* issparse = (int*)malloc(f*sizeof(int));
	for(long i=0; i<f; i++){
		issparse[i] = sparse.container[i];
	}
	if(maxf<0)maxf=f;
	dt = new DecisionTree(height, f, issparse, forget, maxf, noclasses, e, r, rb);
	return 1;
}

__AQEXPORT__(bool) additem(ColRef<double>X, long y, long size){
	long j = 0;
	if(size>0){
		free(data);
		free(result);
		pt = 0;
		data=(double**)malloc(size*sizeof(double*));
		result=(long*)malloc(size*sizeof(long));
	}
	data[pt] = (double*)malloc(X.size*sizeof(double));
	for(j=0; j<X.size; j++){
		data[pt][j]=X.container[j];
	}
	result[pt] = y;
	return 1;
}
__AQEXPORT__(bool) fit(){
	if(pt<=0)return 0;
	dt->fit(data, result, pt);
	return 1;
}

__AQEXPORT__(ColRef_storage) predict(){
	int* result = (int*)malloc(pt*sizeof(int));
	for(long i=0; i<pt; i++){
		result[i]=dt->Test(data[i], dt->DTree);
	}
	ColRef_storage ret(result, pt, pt, "prediction", 0);
	return ret;
}


