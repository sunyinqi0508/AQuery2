#include <algorithm>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include "DecisionTree.h"
#include "Evaluation.h"
#include <fstream>
#include <float.h>
#include <ctime>
#include <random>

std::random_device rd;
std::mt19937 g(rd());
struct minEval{
	double value;
	int* values;

	double eval;
	long left; // how many on its left
	double* record;
	long max;
	long** count;
};

struct DT{
	int height;
	long* featureId;
	DT* left = nullptr;
	DT* right = nullptr;

	// split info
	bool terminate;
	double dpoint;
	long feature;
	long result;
	
	// Sparse data record
	double** sortedData; // for each feature, sorted data
	long** sortedResult;
	
	// Dense data record
	long*** count = nullptr;// for each feature, number of data belongs to each class and dense value 
	double** record = nullptr;// for each feature, record each dense data
	long* max = nullptr;// number of dense value of each feature
	
	//long* T; // number of data in each class in this node
	double** dataRecord = nullptr;// Record the data
	long* resultRecord = nullptr;// Record the result
	long size = 0;// Size of the dataset
};
long seed = (long)clock();
long* Rands(long feature, long maxFeature){
	//srand(seed++);
	long i;
	long* ret = (long*) malloc(feature*sizeof(long)); 
	for(i =0; i<feature; i++)ret[i] = i;
	if(maxFeature==feature){
		return ret;
	}
	std::shuffle(ret, &ret[feature], g);
	long* ret2 = (long*) malloc(maxFeature*sizeof(long)); 
	for(i=0; i<maxFeature; i++)ret2[i] = ret[i];
	free(ret);
	return ret2;
}
double getRand(){
	return (double) rand() / RAND_MAX;
}


void createTree(DT* t, long currentHeight, long height, long f, long maxF, long classes){
	srand(seed);
	long i;
	t->count = (long***)malloc(f*sizeof(long**));
	for(i=0; i<f; i++)t->count[i]=nullptr;
	t->record = (double**)malloc(f*sizeof(double*));
	for(i=0; i<f; i++)t->record[i]=nullptr;
	t->max = (long*)malloc(f*sizeof(long));
	t->max[0] = -1;
	t->featureId = Rands(f, maxF);
	//t->T = (long*)malloc(classes*sizeof(long));
	t->sortedData = (double**) malloc(f*sizeof(double*));
	for(i=0; i<f; i++)t->sortedData[i]=nullptr;
	t->sortedResult = (long**) malloc(f*sizeof(long*));
	for(i=0; i<f; i++)t->sortedResult[i]=nullptr;
	t->dataRecord = nullptr;
	t->resultRecord = nullptr;
	t->height = currentHeight;
	t->feature = -1;
	t->size = 0;
	if(currentHeight>height){
		t->right = nullptr;
		t->left = nullptr;
		return;
	}

	t->left = (DT*)malloc(sizeof(DT));
	t->right = (DT*)malloc(sizeof(DT));
	createTree(t->left, currentHeight+1, height, f, maxF, classes);
	createTree(t->right, currentHeight+1, height, f, maxF, classes);
}

void stableTree(DT* t, long f){
	long i, j;
	for(i=0; i<f; i++){
		if(t->count[i]==nullptr)continue;
		for(j=0; j<t->max[i]; j++){
			free(t->count[i][j]);
		}
		free(t->count[i]);
	}
	free(t->count);
	for(i=0; i<f; i++){
		if(t->record[i]==nullptr)continue;
		free(t->record[i]);
	}
	free(t->record);
	free(t->max);
	free(t->featureId);
	for(i=0; i<f; i++){
		if(t->sortedData[i]==nullptr)continue;
		free(t->sortedData[i]);
	}
	free(t->sortedData);
	for(i=0; i<f; i++){
		if(t->sortedResult[i]==nullptr)continue;
		free(t->sortedResult[i]);
	}
	free(t->sortedResult);
	free(t->dataRecord);
	free(t->resultRecord);
	if(t->right!=nullptr)stableTree(t->right, f);
	if(t->left!=nullptr)stableTree(t->left, f);
}

void freeTree(DT* t){
	if(t->left != nullptr)freeTree(t->left);
	if(t->right != nullptr)freeTree(t->right);
	free(t);
}

DecisionTree::DecisionTree(int height, long f, int* sparse, double forget=0.1, long maxF=0, long noClasses=2, Evaluation e=Evaluation::gini, long r=-1, long rb=1){
	evalue = e;
	called = 0;	
	long i;
	// Max tree height
	maxHeight = height;
	// Number of features
	feature = f;
	// If each feature is sparse or dense, 0 for dense, 1 for sparse, >2 for number of category
	Sparse = (int*)malloc(f*sizeof(int));
	for(i = 0; i<f; i++){
		Sparse[i] = sparse[i];
	}
	// Create Decision tree
	DTree = (DT*)malloc(sizeof(DT));
        DTree->feature = -1;
	// The number of feature that is considered in each node 
	if(maxF>=f){
		maxFeature = f;
	}else if(maxF<=0){
		maxFeature = (long)round(sqrt(f)); 
	}else{
		maxFeature = maxF;
	}
	forgetRate = std::min(1.0, forget);
	retain = r;
	createTree(DTree, 0, maxHeight, f, maxFeature, noClasses);
	// Randomly generate the features
	//DTree->featureId = Rands();
	//DTree->sorted = (long**) malloc(f*sizeof(long*));
	// Number of classes of this dataset
	Rebuild = rb;
	roundNo = 0;
	classes = std::max(noClasses, (long)2);
	//DTree->T = (long*) malloc(noClasses*sizeof(long));
	/*for(long i = 0; i<noClasses; i++){
		DTree->T[i]=0;
	}*/
}

void DecisionTree::Stablelize(){
	free(Sparse);
	stableTree(DTree, feature);
}

void DecisionTree::Free(){
	freeTree(DTree);
}

minEval DecisionTree::incrementalMinGiniSparse(double** dataTotal, long* resultTotal, long sizeTotal, long sizeNew, DT* current, long col, long forgetSize, bool isRoot){
	long i, j;
	if(isRoot){sizeNew=sizeTotal-forgetSize;}
	long newD[sizeNew];
	for(i=0; i<sizeNew; i++)newD[i]=i;
	long T[classes];
	for(i=0; i<classes; i++)T[i]=0;
	std::sort(newD, newD+sizeNew, [&dataTotal, col](long l, long r){return dataTotal[l][col]<dataTotal[r][col];});
	double* newSortedData = (double*)malloc(sizeTotal*sizeof(double));
	long* newSortedResult = (long*)malloc(sizeTotal*sizeof(long));
	long p1=0, p2=0;
	double* oldData = current->sortedData[col];
	long* oldResult = current->sortedResult[col];
	for(i=0; i<sizeTotal; i++){
		if(p1==sizeNew){
			newSortedData[i] = oldData[p2];
			newSortedResult[i] = oldResult[p2];
			T[newSortedResult[i]]++;
			p2++;
		}
		else if(p2==sizeTotal-sizeNew){
			newSortedData[i] = dataTotal[newD[p1]][col];
			newSortedResult[i] = resultTotal[newD[p1]];
			T[newSortedResult[i]]++;
			p1++;
		}
		else if(dataTotal[newD[p1]][col]<oldData[p2]){
			newSortedData[i] = dataTotal[newD[p1]][col];
			newSortedResult[i] = resultTotal[newD[p1]];
			T[newSortedResult[i]]++;
			p1++;
		}else{
			newSortedData[i] = oldData[p2];
			newSortedResult[i] = oldResult[p2];
			T[newSortedResult[i]]++;
			p2++;
		}
	}
	current->sortedData[col] = newSortedData;
	current->sortedResult[col] = newSortedResult;
	free(oldData);
	free(oldResult);

	minEval ret;
	if(evalue == Evaluation::gini){
		ret = giniSparseIncremental(sizeTotal, classes, newSortedData, newSortedResult, T);
	}else if(evalue == Evaluation::entropy or evalue == Evaluation::logLoss){
		ret = entropySparseIncremental(sizeTotal, classes, newSortedData, newSortedResult, T);	
	}
	ret.values = nullptr;
	return ret;
}
minEval DecisionTree::incrementalMinGiniDense(double** data, long* result, long size, long col, long*** count, double** record, long* max, long newSize, long forgetSize, bool isRoot){
	// newSize is before forget 
	long low = 0;
	if(isRoot)size=newSize-forgetSize;
	long i, j, k;
	long newMax = 0;
	long maxLocal = max[col];
	long **newCount=(long**)malloc(size*sizeof(long*));
	for(i=0;i<size;i++){
		newCount[i] = (long*)malloc((classes+1)*sizeof(long));
		for(j=0;j<= classes;j++)newCount[i][j]=0;
	}
	double newRecord[size];
	bool find;

	// find total count for each class
	long T[classes];
	for(i=0;i<classes;i++)T[i]=0;
	for(i=0;i<max[col];i++){
		for(j=0;j<classes;j++){
			if(isRoot)count[col][i][j]=0;
			else if(T[j]<count[col][i][j])T[j]=count[col][i][j];
		}
	}
	
	// plug in new data
	for(i=0; i<size; i++){
		find = false;
		T[result[i]]++;
		for(j=0;j<max[col];j++){
			if(data[i][col]==record[col][j]){
				count[col][j][result[i]]++;
				count[col][j][classes] ++;	
				find = true;
			}else if(data[i][col]<record[col][j]){
				count[col][j][result[i]]++;
				count[col][j][classes] ++;
			}
		}	
		for(j=0;j<newMax;j++){
			if(data[i][col]==newRecord[j]){
				newCount[j][result[i]]++;
				newCount[j][classes] ++;
				find = true;
	               	} else if(data[i][col]<newRecord[j]){
				newCount[j][result[i]]++;
				newCount[j][classes] ++;
			}
		}
		if(not find){
			newRecord[newMax] = data[i][col];
			double currentMinMax = -1*DBL_MAX;
			for(j=0;j<max[col];j++){
				if(record[col][j]<newRecord[newMax] and record[col][j]>currentMinMax){
					currentMinMax = record[col][j];
					for(k=0;k<=classes;k++)newCount[newMax][k]=count[col][j][k];
				}
			}
			for(j=0;j<newMax;j++){
				if(newRecord[j]<newRecord[newMax] and currentMinMax<newRecord[j]){
					currentMinMax = newRecord[j];
					for(k=0;k<=classes;k++)newCount[newMax][k]=newCount[j][k];
				}
			}
			if(currentMinMax== -1*DBL_MAX){
				for(k=0;k<=classes;k++)newCount[newMax][k]=0;
			}
			newCount[newMax][result[i]]++;
			newCount[newMax][classes]++;
			newMax++;
		}
	}
	// Updata new count and record 
	if(newMax>0){
		max[col]+=newMax;
		long** updateCount = (long**)malloc(max[col]*sizeof(long*));
		double* updateRecord = (double*)malloc(max[col]*sizeof(double));
		for(i=0; i<max[col]; i++){
			if(i>=newMax){
				updateCount[i] = count[col][i-newMax];
				updateRecord[i] = record[col][i-newMax];
			}
			else{
				updateCount[i] = newCount[i];
				updateRecord[i] = newRecord[i];
			}
		}
		free(count[col]);
		free(record[col]);
		count[col]=updateCount;
		record[col]=updateRecord;
	}
	for(i=newMax; i<size; i++){
		free(newCount[i]);
	}
	free(newCount);

	//calculate gini
	minEval ret;
	if(evalue==Evaluation::gini){
		ret = giniDenseIncremental(max[col], record[col], count[col], classes, newSize, T);
	}else if(evalue==Evaluation::entropy or evalue==Evaluation::logLoss){
		ret = entropyDenseIncremental(max[col], record[col], count[col], classes, newSize, T);
	}
	ret.values = nullptr;
	return ret;
}

minEval DecisionTree::findMinGiniSparse(double** data, long* result, long* totalT, long size, long col, DT* current){
	long i, j;
	long* d = (long*)malloc(size*sizeof(long));
	for(i=0; i<size; i++)d[i]=i;
	std::sort(d, d+size, [&data, col](long l, long r){return data[l][col]<data[r][col];});

	minEval ret;
	if(evalue == Evaluation::gini){
		ret = giniSparse(data, result, d, size, col, classes, totalT);
	}else if(evalue == Evaluation::entropy or evalue == Evaluation::logLoss){
		ret = entropySparse(data, result, d, size, col, classes, totalT);
	}	
	if(current->sortedData[col] != nullptr)free(current->sortedData[col]);
	if(current->sortedResult[col] != nullptr)free(current->sortedResult[col]);
	current->sortedData[col] = (double*) malloc(size*sizeof(double));
	current->sortedResult[col] = (long*) malloc(size*sizeof(long));
	for(i=0;i<size; i++){
		current->sortedData[col][i] = data[d[i]][col];
		current->sortedResult[col][i] = result[d[i]];
	}
	free(d);
	ret.values = nullptr;
	return ret;

}

minEval DecisionTree::findMinGiniDense(double** data, long* result, long* totalT, long size, long col){
	long low = 0;
	long i, j, k, max=0;
	long** count = (long**)malloc(size*sizeof(long*));
	// size2 and count2 are after forget
	double* record = (double*)malloc(size*sizeof(double));
	bool find;
	for(i=0;i<size;i++){
		find = false;
		for(j=0; j<max; j++){
			if(record[j]==data[i][col]){
				count[j][result[i]]++;
				count[j][classes]++;
				find = true;
				break;
			}
		}
		if(not find){
			count[max] = (long*)malloc((classes+1)*sizeof(long));
			record[max]=data[i][col];
			for(j=0;j<=classes;j++){
				count[max][j] = 0;
			}
			count[max][result[i]]++;
			count[max][classes]++;
			max++;
		}
	}
	long** rem = (long**)malloc(max*sizeof(long*));
	double* record2 = (double*)malloc(max*sizeof(double));
	for(i=0;i<max;i++){
		rem[i] = count[i];
		record2[i] = record[i];
	}
	free(count);
	free(record);
	
	long d[max];
       	for(i=0;i<max;i++){
		d[i] = i;
	}
	std::sort(d, d+max, [&record2](long l, long r){return record2[l]<record2[r];});	
	minEval ret;
	if(evalue == Evaluation::gini){
		ret = giniDense(max, size, classes, rem, d, record2, totalT);
	}else if(evalue == Evaluation::entropy or evalue == Evaluation::logLoss){
		ret = entropyDense(max, size, classes, rem, d, record2, totalT);
	}
	ret.record = record2;
	ret.max = max;
	ret.count = rem;
	ret.values = nullptr;
	return ret;
}

double xxx;
void DecisionTree::fit(double** data, long* result, long size){
	roundNo++;
	if(DTree->size==0){
		Update(data, result, size, DTree);
	}else{
		IncrementalUpdate(data, result, size, DTree);
	}
	/*
	if(Rebuild and called==10){
		called = 0;
		Rebuild = false;
	}else if(Rebuild){
		called = 11;
	}else{
		called++;
	}*/
}

long* DecisionTree::fitThenPredict(double** trainData, long* trainResult, long trainSize, double** testData, long testSize){
	fit(trainData, trainResult, trainSize);
	long* testResult = (long*)malloc(testSize*sizeof(long));
	for(long i=0; i<testSize; i++){
		testResult[i] = Test(testData[i], DTree);
	}
	return testResult;
}

void DecisionTree::IncrementalUpdate(double** data, long* result, long size, DT* current){
	long i, j;
	long low = 0;
	long forgetSize=0;
       	if(retain>0 and current->size+size>retain) forgetSize = std::min(current->size+size - retain, current->size);
	else if(retain<0) forgetSize = (long)current->size*forgetRate;
	long* index = new long[current->size];
	double** dataNew;
	long* resultNew;
	if(current->height == 0){
		dataNew = (double**)malloc((size+current->size-forgetSize)*sizeof(double*));
		resultNew = (long*)malloc((size+current->size-forgetSize)*sizeof(long));
		for(i=0;i<size;i++){
			dataNew[i] = data[i];
			resultNew[i] = result[i];
		}
		for(i=0; i<current->size; i++){
			index[i] = i;
		}
		std::shuffle(index, index+current->size, g);
		long x = 0;
		for(i=0;i<current->size;i++){
			if(i>=current->size-forgetSize){
				current->dataRecord[index[i]][feature-1] = DBL_MAX;

			}else{
				dataNew[i+size] = current->dataRecord[index[i]];
				resultNew[i+size] = current->resultRecord[index[i]];
			}
		}
	}else{
		forgetSize = 0;
		dataNew = (double**)malloc((size+current->size)*sizeof(double*));
		resultNew = (long*)malloc((size+current->size)*sizeof(long));
		for(i=0;i<size;i++){
			dataNew[i] = data[i];
			resultNew[i] = result[i];
		}
		for(i=0;i<current->size;i++){
                        if(current->dataRecord[i][feature-1] == DBL_MAX){
                        	forgetSize++;
				continue;
			}else{
                                dataNew[i+size-forgetSize] = current->dataRecord[i];
                                resultNew[i+size-forgetSize] = current->resultRecord[i];
                        }
                }
	}
	free(data);
	free(result);
	current->size -= forgetSize;
	current->size += size;
	// end condition
	if(current->terminate or roundNo%Rebuild==0){
		if(current->height == 0){
			for(i=0; i<forgetSize; i++){
				free(current->dataRecord[index[current->size-size+i]]);
			}
		}
		delete(index);
		Update(dataNew, resultNew, current->size, current);
		return;
	}
	// find min gini
	minEval c, cMin;
	long cFeature;
	cMin.eval = DBL_MAX;
	cMin.values = nullptr;
	// TODO
	for(i=0;i<maxFeature; i++){
		if(Sparse[current->featureId[i]]==1){
			c = incrementalMinGiniSparse(dataNew, resultNew, current->size+forgetSize, size, current, current->featureId[i], forgetSize, false);
		}
		else if(Sparse[current->featureId[i]]==0){
			c = incrementalMinGiniDense(dataNew, resultNew, size, current->featureId[i], current->count, current->record, current->max, current->size+forgetSize, forgetSize, false);
		}else{
			//c = incrementalMinGiniCategorical();
		}
		if(c.eval<cMin.eval){
			cMin.eval = c.eval;
			cMin.value = c.value;
			if(cMin.values != nullptr)free(cMin.values);
			cMin.values = c.values;
			cFeature = current->featureId[i];
		}else if(c.values!=nullptr)free(c.values);
	}
	if(cMin.eval==DBL_MAX){
		current->terminate = true;
		long t[classes];
		for(i=0;i<classes;i++){
			t[i]=0;
		}
		for(i=low;i<low+size;i++){
			t[result[i]]++;
		}
		if(cMin.values!=nullptr)free(cMin.values);
		current->result = std::distance(t, std::max_element(t, t+classes));
		return;
	}
	//diverse data
        long ptL=0, ptR=0;
	double* t;
	long currentSize = current->size;
	//TODO:Discrete
	// Same diverse point as last time
	if(current->dpoint==cMin.value and current->feature==cFeature){
		long xxx = current->left->size;
		/*for(i=0; i<size; i++){
			if(dataNew[i][current->feature]<=current->dpoint){
				ptL++;
			}else{
				ptR++;
			}
		}*/
		ptL = size;
		ptR = size;
		long* resultL = (long*)malloc((ptL)*sizeof(long));
		long* resultR = (long*)malloc((ptR)*sizeof(long));
		double** dataL = (double**)malloc((ptL)*sizeof(double*));
		double** dataR = (double**)malloc((ptR)*sizeof(double*));
		ptL = 0;
		ptR = 0;
		for(i=0; i<size; i++){
			if(dataNew[i][current->feature]<=current->dpoint){
				dataL[ptL] = dataNew[i];
				resultL[ptL] = resultNew[i];
				ptL++;
			}else{
				dataR[ptR] = dataNew[i];
				resultR[ptR] = resultNew[i];
				ptR++;
			}
		}
		IncrementalUpdate(dataL, resultL, ptL, current->left);
		IncrementalUpdate(dataR, resultR, ptR, current->right);
		
		if(current->height == 0){
			for(i=0; i<forgetSize; i++){
				free(current->dataRecord[index[current->size-size+i]]);
			}
		}
		delete(index);
		free(current->dataRecord);
		free(current->resultRecord);
		current->dataRecord = dataNew;
		current->resultRecord = resultNew;
		return;
	}
	
	// Different diverse point
	current->feature = cFeature;
	current->dpoint = cMin.value;
	/*for(i=0; i<currentSize; i++){
        	if(dataNew[i][current->feature]<=current->dpoint){
			ptL++;
		}else{
			ptR++;
		}
	}*/
	long* resultL = (long*)malloc(currentSize*sizeof(long));
	long* resultR = (long*)malloc(currentSize*sizeof(long));
	double** dataL = (double**)malloc(currentSize*sizeof(double*));
	double** dataR = (double**)malloc(currentSize*sizeof(double*));
	ptL = 0;
	ptR = 0;
	for(i=0; i<currentSize; i++){
        	if(dataNew[i][current->feature]<=current->dpoint){
			dataL[ptL] = dataNew[i];
			resultL[ptL] = resultNew[i];
			ptL++;
		}else{
                        dataR[ptR] = dataNew[i];
			resultR[ptR] = resultNew[i];
			ptR++;
		}
	}
        Update(dataL, resultL, ptL, current->left);
        Update(dataR, resultR, ptR, current->right);
	
	if(current->height == 0){
		for(i=0; i<forgetSize; i++){
			free(current->dataRecord[index[current->size-size+i]]);
		}
	}
	
	delete(index);
	free(current->dataRecord);
	free(current->resultRecord);
	current->dataRecord = dataNew;
	current->resultRecord = resultNew;
}
void DecisionTree::Update(double** data, long* result, long size, DT* current){
	long low = 0;
	long i, j;
	// end condition
	if(current->dataRecord!=nullptr)free(current->dataRecord);
	current->dataRecord = data;
	if(current->resultRecord!=nullptr)free(current->resultRecord);	
	current->resultRecord = result;
	current->size = size;
	if(current->height == maxHeight){
		current->terminate = true;
		long t[classes];
		for(i=0;i<classes;i++){
			t[i]=0;
		}
		for(i=low;i<low+size;i++){
			t[result[i]]++;
		}
		current->result = std::distance(t, std::max_element(t, t+classes));
		return;
	}
	long T[classes];
	for(i=0;i<classes;i++){
		T[i] = 0;
	}
	for(i=0;i<size;i++){
		j = result[i];
		T[j]++;
	}
	for(i=0;i<classes;i++){
		if(T[i]==size){
			current->terminate = true;
			current->result = i;
			return;
		}
	}
	// find min Evaluation
	minEval c, cMin;
	long cFeature, oldMax, col, left=0;
	cMin.eval = DBL_MAX;
	cMin.values = nullptr;
	//TODO
	for(i=0;i<maxFeature; i++){
		col = current->featureId[i];
		if(Sparse[current->featureId[i]]==1){
			c = findMinGiniSparse(data, result, T, size, col, current);
		}
		else if(Sparse[current->featureId[i]]==0){
			c = findMinGiniDense(data, result, T, size, col);
			if(current->count[col]!=nullptr){
				for(j=0; j<current->max[col]; j++){
					if(current->count[col][j]!=nullptr)free(current->count[col][j]);
				}
				free(current->count[col]);
				free(current->record[col]);
			}
			current->count[col] = c.count;
			current->record[col] = c.record;
			current->max[col] = c.max;
		}else{
		
		}
		if(c.eval<cMin.eval){
			cMin.eval = c.eval;
			if(cMin.values!=nullptr)free(cMin.values);
			cMin.values = c.values;
			cMin.value = c.value;
			cFeature = current->featureId[i];
			left = c.left;
		}else if(c.values!=nullptr){
			free(c.values);
		}
	}
	if(cMin.eval == DBL_MAX){
		current->terminate = true;
		long max = 0;
		for(i=1;i<classes;i++){
			if(T[max]<T[i])max=i;
		}
		if(cMin.values!=nullptr)free(cMin.values);
		current->result = max;
		return;
	}
	//diverse data
	current->terminate = false;
	current->feature = cFeature;
	current->dpoint = cMin.value;
        long ptL=0, ptR=0;
	//TODO:Discrete
	long* resultL = new long[left];
	long* resultR = new long[size-left];
	double** dataL = new double*[left];
	double** dataR = new double*[size-left];
	for(i=low; i<low+size; i++){
        	if(data[i][current->feature]<=current->dpoint){
			dataL[ptL] = data[i];
			resultL[ptL] = result[i];
			ptL++;
		}else{
                        dataR[ptR] = data[i];
			resultR[ptR] = result[i];
			ptR++;
		}
	}
        Update(dataL, resultL, ptL, current->left);
        Update(dataR, resultR, ptR, current->right);
}

long DecisionTree::Test(double* data, DT* root){
	if(root->terminate)return root->result;
	if(data[root->feature]<=root->dpoint)return Test(data, root->left);
	return Test(data, root->right);
}

void DecisionTree::print(DT* root){
	int x;
	//std::cin>>x;
	if(root->terminate){
		printf("%ld", root->result);
		return;
	}
	printf("([%ld, %f]:", root->feature, root->dpoint);
	print(root->left);
	printf(", ");
	print(root->right);
	printf(")");
}
