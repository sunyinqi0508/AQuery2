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
	pt ++;
	return 1;
}
__AQEXPORT__(bool) fit(vector_type<vector_type<double>> v, vector_type<long> res){
	double** data = (double**)malloc(v.size*sizeof(double*));
	for(int i = 0; i < v.size; ++i)
		data[i] = v.container[i].container;
	dt->fit(data, res.container, v.size);
	return true;
}

__AQEXPORT__(vectortype_cstorage) predict(vector_type<vector_type<double>> v){
	int* result = (int*)malloc(v.size*sizeof(int));
	
	for(long i=0; i<v.size; i++){
		result[i]=dt->Test(v.container[i].container, dt->DTree);
		//printf("%d ", result[i]);
	}
	auto container = (vector_type<int>*)malloc(sizeof(vector_type<int>));
	container->size = v.size;
	container->capacity = 0;
	container->container = result;
	// container->out(10);
	// ColRef<vector_type<int>>* col = (ColRef<vector_type<int>>*)malloc(sizeof(ColRef<vector_type<int>>));
	auto ret = vectortype_cstorage{.container = container, .size = 1, .capacity = 0};
	// col->initfrom(ret, "sibal");
	// print(*col);
	return ret;
	//return true;
}


