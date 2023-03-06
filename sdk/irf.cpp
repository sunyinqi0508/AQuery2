#include "DecisionTree.h"
#include "RF.h"

// __AQ_NO_SESSION__
#include "../server/table.h"

#include "aquery.h"

#include "../server/gc.h"

inline GC* GC::gc_handle = nullptr;
inline ScratchSpace* GC::scratch_space = nullptr;
__AQEXPORT__(void) __AQ_Init_GC__(Context* cxt) {
    GC::gc_handle = static_cast<GC*>(cxt->gc);
    GC::scratch_space = nullptr;
}

DecisionTree *dt = nullptr;
RandomForest *rf = nullptr;

__AQEXPORT__(bool)
newtree(int ntree, long f, ColRef<int> sparse, double forget, long nclasses, Evaluation e)
{
	if (sparse.size != f)
		return false;
	int *X_cpy = (int *)malloc(f * sizeof(int));
	
	memcpy(X_cpy, sparse.container, f);

	// dt = new DecisionTree(f, X_cpy, forget, maxf, noclasses, e);
	rf = new RandomForest(ntree, f, X_cpy, forget, nclasses, e, true);
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
fit_inc(vector_type<vector_type<double>> v, vector_type<long> res)
{
	static uint32_t last_offset = 0;
	if(last_offset > v.size) 
		last_offset = 0;
	const auto curr_size = (v.size - last_offset);
	if(curr_size > 0) {
		double **data = (double **)malloc( curr_size * sizeof(double *));
		for (uint32_t i = last_offset; i < v.size; ++i)
			data[i - last_offset] = v.container[i].container;
		rf->fit(data, res.container + last_offset, curr_size);
		
		last_offset = v.size;
		free(data);
		return true;
	}
	return false;
}


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

	for (uint32_t i = 0; i < v.size; i++)
		//result[i] = dt->Test(v.container[i].container, dt->DTree);
		result[i] = int(rf->Test(v[i].container));
	auto container = (vector_type<int> *)malloc(sizeof(vector_type<int>));
	container->size = v.size;
	container->capacity = 0;
	container->container = result;
	auto ret = vectortype_cstorage{.container = container, .size = 1, .capacity = 0};
	return ret;
}
template <typename T>
constexpr T sq(const T x) {
	return x * x;
}
__AQEXPORT__(double)
test(vector_type<vector_type<double>> x, vector_type<long> y) {
	int result = 0;
	printf("y_hat = (");
	double err = 0.;
	for (uint32_t i = 0; i < x.size; i++) {
		//result[i] = dt->Test(v.container[i].container, dt->DTree);
		result = int(rf->Test(x[i].container));
		err += sq(result - y.container[i]);
		printf("%d ", result);
	}
	puts(")");
	printf("error: %lf\n", err/=double(x.size));
	return err;
}
