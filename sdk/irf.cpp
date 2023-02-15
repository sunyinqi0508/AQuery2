#include "DecisionTree.h"
#include "RF.h"

// __AQ_NO_SESSION__
#include "../server/table.h"
#include "aquery.h"

DecisionTree *dt = nullptr;
RandomForest *rf = nullptr;

__AQEXPORT__(bool)
newtree(int height, long f, ColRef<int> X, double forget, long maxf, long noclasses, Evaluation e, long r, long rb)
{
	if (X.size != f)
		return false;
	int *X_cpy = (int *)malloc(f * sizeof(int));
	
	memcpy(X_cpy, X.container, f);

	if (maxf < 0)
		maxf = f;
	dt = new DecisionTree(f, X_cpy, forget, maxf, noclasses, e);
	rf = new RandomForest(height, f, X_cpy, forget, noclasses, e)
	return true;
}

// size_t pt = 0;
// __AQEXPORT__(bool) fit(ColRef<ColRef<double>> X, ColRef<int> y){
// 	if(X.size != y.size)return 0;
// 	double** data = (double**)malloc(X.size*sizeof(double*));
// 	long* result = (long*)malloc(y.size*sizeof(long));
// 	for(long i=0; i<X.size; i++){
// 		data[i] = X.container[i].container;
// 		result[i] = y.container[i];
// 	}
// 	data[pt] = (double*)malloc(X.size*sizeof(double));
// 	for(uint32_t j=0; j<X.size; j++){
// 		data[pt][j]=X.container[j];
// 	}
// 	result[pt] = y;
// 	pt ++;
// 	return 1;
// }

__AQEXPORT__(bool)
fit(vector_type<vector_type<double>> v, vector_type<long> res)
{
	double **data = (double **)malloc(v.size * sizeof(double *));
	for (int i = 0; i < v.size; ++i)
		data[i] = v.container[i].container;
	// dt->fit(data, res.container, v.size);
	rf->fit(data, res.container, v.size);
	return true;
}

__AQEXPORT__(vectortype_cstorage)
predict(vector_type<vector_type<double>> v)
{
	int *result = (int *)malloc(v.size * sizeof(int));

	for (long i = 0; i < v.size; i++)
		//result[i] = dt->Test(v.container[i].container, dt->DTree);
		result[i] = rf->Test(v.container, rf->DTrees);
	auto container = (vector_type<int> *)malloc(sizeof(vector_type<int>));
	container->size = v.size;
	container->capacity = 0;
	container->container = result;
	auto ret = vectortype_cstorage{.container = container, .size = 1, .capacity = 0};
	return ret;
}
