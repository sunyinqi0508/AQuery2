#include "DecisionTree.h"
#include "aquery.h"
// __AQ_NO_SESSION__
#include "../server/table.h"

DecisionTree* dt = nullptr;

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

__AQEXPORT__(bool) fit(ColRef<ColRef<double>> X, ColRef<int> y){
	if(X.size != y.size)return 0;
	double** data = (double**)malloc(X.size*sizeof(double*));
	long* result = (long*)malloc(y.size*sizeof(long));
	for(long i=0; i<X.size; i++){
		data[i] = X.container[i].container;
		result[i] = y.container[i];
	}
	dt->fit(data, result, X.size);
	return 1;
}

__AQEXPORT__(ColRef_storage) predict(ColRef<ColRef<double>> X){
        double** data = (double**)malloc(X.size*sizeof(double*));
        int* result = (int*)malloc(X.size*sizeof(int));
        for(long i=0; i<X.size; i++){
                data[i] = X.container[i].container;
        }
	for(long i=0; i<X.size; i++){
		result[i]=dt->Test(data[i], dt->DTree);
	}
	
	return ColRef_storage(new ColRef_storage(result, X.size, 0, "prediction", 0), 1, 0, "prediction", 0);
}


