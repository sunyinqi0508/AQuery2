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
	long height;
	long* featureId;
	DT* left = nullptr;
	DT* right = nullptr;
	bool created;

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
	srand(seed);
	long i;
	long* ret = (long*) malloc(feature*sizeof(long)); 
	for(i =0; i<feature; i++)ret[i] = i;
	if(maxFeature==feature){
		return ret;
	}
	std::shuffle(ret, ret+feature, g);
	long* ret2 = (long*) malloc(maxFeature*sizeof(long)); 
	for(i=0; i<maxFeature; i++){
		ret2[i] = ret[i];
	}
	free(ret);
	return ret2;
}
double getRand(){
	return (double) rand() / RAND_MAX;
}

void createNode(DT* t, long currentHeight, long f, long classes){
	t->created = true;
	long i;
	t->count = (long***)malloc(f*sizeof(long**));
	for(i=0; i<f; i++)t->count[i]=nullptr;
	t->record = (double**)malloc(f*sizeof(double*));
	for(i=0; i<f; i++)t->record[i]=nullptr;
	t->max = (long*)malloc(f*sizeof(long));
	t->max[0] = -1;
	t->sortedData = (double**) malloc(f*sizeof(double*));
	for(i=0; i<f; i++)t->sortedData[i]=nullptr;
	t->sortedResult = (long**) malloc(f*sizeof(long*));
	for(i=0; i<f; i++)t->sortedResult[i]=nullptr;
	t->dataRecord = nullptr;
	t->resultRecord = nullptr;
	t->height = currentHeight;
	t->feature = -1;
	t->size = 0;

	t->left = (DT*)malloc(sizeof(DT));
	t->right = (DT*)malloc(sizeof(DT));
	t->left->created = false;
	t->right->created = false;
	t->left->height = currentHeight+1;
	t->right->height = currentHeight+1;
}

void stableTree(DT* t, long f){
	long i, j;
	if(not t->created)return;
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
	for(i=0; i<f; i++){
		if(t->sortedData[i]==nullptr)continue;
		free(t->sortedData[i]);
	}
	free(t->sortedData);
	for(i=0; i<f; i++){
		if(t->sortedResult[i]==nullptr)continue;
		free(t->sortedResult[i]);
	}
	free(t->dataRecord);
	free(t->resultRecord);
	free(t->sortedResult);
	if(t->right!=nullptr)stableTree(t->right, f);
	if(t->left!=nullptr)stableTree(t->left, f);
}

void freeTree(DT* t){
	if(t->created){
		freeTree(t->left);
		freeTree(t->right);
	}
	free(t);
}

DecisionTree::DecisionTree(long f, int* sparse, double rate, long maxF, long noClasses, Evaluation e){
	evalue = e;
	long i;
	// Max tree height
	initialIR = rate;
	increaseRate = rate;
	isRF = false;
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
		maxF = f;
	}else if(maxF<=0){
		maxF = (long)round(sqrt(f)); 
	}
	maxFeature = maxF;
	forgetRate = -10.0;
	retain = 0;
	DTree->featureId = Rands(f, maxF);
	DTree->terminate = true;
	DTree->result = 0;
	DTree->size = 0;
	createNode(DTree, 0, f, noClasses);
	// Number of classes of this dataset
	Rebuild = 2147483647;
	roundNo = 64;
	classes = std::max(noClasses, (long)2);
	// last Acc
	lastAll = classes;
	lastT = 1;
}

void DecisionTree::Stablelize(){
	free(Sparse);
	long i, j;
	DT* t = DTree;
	long f = feature;
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
	if(DTree->right!=nullptr)stableTree(t->right, feature);
	if(DTree->left!=nullptr)stableTree(t->left, feature);
}

void DecisionTree::Free(){
	free(DTree->dataRecord);
	free(DTree->resultRecord);
	freeTree(DTree);
}

minEval DecisionTree::incrementalMinGiniSparse(double** dataTotal, long* resultTotal, long sizeTotal, long sizeNew, DT* current, long col, long forgetSize, double** forgottenData, long* forgottenClass){
	long i, j;
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
	long tmp2 = forgetSize;
	long* allForget = (long*)malloc(sizeof(long)*classes);
	for(i=0; i<classes; i++)allForget[i]=0;
	for(i=0; i<sizeTotal; i++){
		bool meet = false;
		if(p1==sizeNew){
			j = oldResult[p2];
			if(allForget[j]!=forgottenClass[j]){
				if(oldData[p2]==forgottenData[j][allForget[j]]){
					allForget[j]++;
					i--;
					meet = true;
				}
			}
			if(not meet){
				newSortedData[i] = oldData[p2];
				newSortedResult[i] = oldResult[p2];
				T[newSortedResult[i]]++;
			}
			p2++;
		}
		else if(p2==sizeTotal-sizeNew+forgetSize){
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
			j = oldResult[p2];
			if(allForget[j]!=forgottenClass[j]){
				if(oldData[p2]==forgottenData[j][allForget[j]]){
					allForget[j]++;
					i--;
					meet = true;
				}
			}
			if(not meet){
				newSortedData[i] = oldData[p2];
				newSortedResult[i] = oldResult[p2];
				T[newSortedResult[i]]++;
			}
			p2++;
		}
	}
	free(allForget);
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
minEval DecisionTree::incrementalMinGiniDense(double** data, long* result, long size, long col, long*** count, double** record, long* max, long newSize, long forgetSize, double** forgottenData, long* forgottenClass){
	// newSize is before forget 
	long low = 0;
	//if(isRoot)
	long i, j, k, tmp;
	long newMax = 0;
	long maxLocal = max[col];
	long **newCount=(long**)malloc(size*sizeof(long*));
	double newRecord[size];
	bool find;
	long tmp3 = newSize-size;
	long tmp4 = forgetSize;
	// find total count for each class
	long T[classes];
	long tmp2=0;
	long* allForget = new long[classes];
	for(i=0;i<classes;i++){
		T[i]=0;
		allForget[i]=0;
	}
	// forget
	for(i=0;i<max[col];i++){
		for(j=0;j<classes;j++){
			tmp = count[col][i][j];
			tmp2+=tmp;
			for(k=0; k<tmp; k++){
				if(allForget[j]==forgottenClass[j])break;
				if(record[col][i]==forgottenData[j][allForget[j]]){
					forgetSize--;
					count[col][i][j]--;
					count[col][i][classes]--;
					allForget[j]++;
				}else{
					break;
				}
			}
			T[j]+=count[col][i][j];
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
			}
		}	
		for(j=0;j<newMax;j++){
			if(data[i][col]==newRecord[j]){
				newCount[j][result[i]]++;
				newCount[j][classes] ++;
				find = true;
			}
		}
		if(not find){
			newCount[newMax] = (long*)malloc((classes+1)*sizeof(long));
			for(j=0;j<= classes;j++)newCount[newMax][j]=0;
			newCount[newMax][result[i]]++;
			newCount[newMax][classes]++;
			newRecord[newMax] = data[i][col];
			newMax++;
		}
	}
	// Updata new count and record 
	long* d;
	if(newMax>0){
		d = (long*)malloc(sizeof(long)*newMax);
		for(i=0;i<newMax;i++)d[i]=i;
		std::sort(d, d+newMax, [&newRecord](long l, long r){return newRecord[l]<newRecord[r];});	
		max[col]+=newMax;
		long** updateCount = (long**)malloc(max[col]*sizeof(long*));
		double* updateRecord = (double*)malloc(max[col]*sizeof(double));
		j = 0;
		k = 0;
		for(i=0; i<max[col]; i++){
			if(k==max[col]-newMax){
				updateCount[i] = newCount[j];
                                updateRecord[i] = newRecord[j];
                                j++;
			}
			else if(j==newMax){
				updateCount[i] = count[col][k];
                                updateRecord[i] = record[col][k];
                                k++;
			}
			else if(newRecord[j]>record[col][k]){
				updateCount[i] = count[col][k];
				updateRecord[i] = record[col][k];
				k++;
			}
			else{
				updateCount[i] = newCount[j];
				updateRecord[i] = newRecord[j];
				j++;
			}
		}
		free(count[col]);
		free(record[col]);
		count[col]=updateCount;
		record[col]=updateRecord;
		free(d);
	}
	free(newCount);

	//calculate gini
	minEval ret;
	if(evalue==Evaluation::gini){
		ret = giniDense(max[col], newSize, classes, count[col], d, record[col], T);
	}else if(evalue==Evaluation::entropy or evalue==Evaluation::logLoss){
		ret = entropyDense(max[col], newSize, classes, count[col], d, record[col], T);
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
	long d[max];
       	for(i=0;i<max;i++){
		d[i] = i;
	}
	std::sort(d, d+max, [&record](long l, long r){return record[l]<record[r];});	
	long** rem = (long**)malloc(max*sizeof(long*));
	double* record2 = (double*)malloc(max*sizeof(double));
	for(i=0;i<max;i++){
		rem[i] = count[d[i]];
		record2[i] = record[d[i]];
	}
	free(count);
	free(record);
       	
	for(i=0;i<max;i++){
		d[i] = i;
	}
	
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

void DecisionTree::fit(double** data, long* result, long size){
        double isUp = -1.0;
	long localT = 0;
        long localAll = 0;
	if(DTree->size==0){
		retain = size;
		maxHeight = (long)log2((double)retain);
		maxHeight = std::max(maxHeight, (long)1);
		Update(data, result, size, DTree);
	}else{
		if(forgetRate<=0){
                	for(long j=0; j<size; j++){
                		if(Test(data[j], DTree)==result[j])localT++;
                        	localAll++;
                	}
			double guessAcc;
			guessAcc = 1.0/classes;
                	if(forgetRate==0.0){
				double lastSm = (double)lastT/lastAll;
			       	double localSm = (double)localT/localAll;
				/*long guesses[classes], i;
			       	for(i=0; i<classes; i++)guesses[i]=0;
				for(i=0; i<DTree->size; i++){
					guesses[DTree->resultRecord[i]]++;
				}
				for(i=0; i<size; i++){
					guessAcc += (double)guesses[result[i]]/DTree->size/size;
				}*/
				if(localSm <= guessAcc){
				//if(localSm <= 1.0/classes){
                			lastT = localT;
               				lastAll = localAll;
					retain = size;
					//increaseRate = 1.0-localSm;
				}
				else if(lastSm <= guessAcc){
				//else if(lastSm <= 1.0/classes){
                			lastT = localT;
               				lastAll = localAll;
					//forgetRate=-5.0;
					retain += size;
					//increaseRate -= localSm;
					//increaseRate = initialIR;
					//increaseRate -= localSm;
					//increaseRate /= (double)localSm-1.0/classes;
				}
				else if(lastSm == localSm){
                			lastT += localT;
               				lastAll += localAll;
					retain+=(long)round(increaseRate*size);
					//increaseRate*=increaseRate;
                			//retain = (long)((double)retain*isUp+0.25*size);
				}
				else{
					/*double lastSd = sqrt(pow((1.0-lastSm),2)*lastT+pow(lastSm,2)*(lastAll-lastT)/(lastAll-1));
					double localSd = sqrt(pow((1.0-localSm),2)*localT+pow(localSm,2)*(localAll-localT)/(localAll-1));
					double v = lastAll+localAll-2;
					double sp = sqrt(((lastAll-1) * lastSd * lastSd + (localAll-1) * localSd * localSd) / v);
					double q;
					//double t=lastSm-localSm;
					if(sp==0)q=1.0;
					else if(lastAll+lastAll<2000){
						q = abs(lastSm-localSm);
					}
					else{
						double t = t/(sp*sqrt(1.0/lastAll+1.0/localAll));
						boost::math::students_t dist(v);
						double c = cdf(dist, t);
						q = cdf(complement(dist, fabs(t)));
					}*/
					isUp = ((double)localSm-guessAcc)/((double)lastSm-guessAcc);
					//isUp = ((double)localSm-1.0/classes)/((double)lastSm-1.0/classes);
					increaseRate = increaseRate/isUp;
					//increaseRate += increaseRate*factor;
					if(isUp>=1.0)isUp=pow(isUp, 2);
					else{
						isUp=pow(isUp, 3-isUp);
					}
					retain = std::min((long)round(retain*isUp+increaseRate*size), retain+size);
					//double factor = ((lastSm-localSm)/localSm)*abs((lastSm-localSm)/localSm)*increaseRate;
					//retain += std::min((long)round(factor*retain+increaseRate*size), size);
                			lastT = localT;
               				lastAll = localAll;
				}
				//printf("   %f, %f, %f\n", increaseRate, localSm, lastSm);
                	}else{
				long i;
				retain = DTree->size+size;
				/*double guessAcc=0.0;
				long guesses[classes];
			       	for(i=0; i<classes; i++)guesses[i]=0;
				for(i=0; i<DTree->size; i++){
					guesses[DTree->resultRecord[i]]++;
				}
				for(i=0; i<size; i++){
					guessAcc += (double)guesses[result[i]]/DTree->size/size;
				}*/
				while(retain>=roundNo){
					if((double)localT/localAll>guessAcc){
						forgetRate+=5.0;
					}
					roundNo*=2;
				}
				if((double)localT/localAll<=guessAcc){
					forgetRate=-10.0;
				}
				if(forgetRate>=0){
					forgetRate=0.0;
				}
                		lastT = localT;
               			lastAll = localAll;
			}
		}
		//if(increaseRate>initialIR)increaseRate=initialIR;
		//printf("%f\n", increaseRate);
		if(retain<size)retain=size;
		maxHeight = (long)log2((double)retain);
		maxHeight = std::max(maxHeight, (long)1);
		IncrementalUpdate(data, result, size, DTree);
	}
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
	long* index;
	bool forgetOld = false;
	index = (long*)malloc(sizeof(long)*current->size);
       	if(current->size+size>retain and current->height==0) {
		forgetSize = std::min(current->size+size - retain, current->size);
	}
	if(forgetSize==current->size){
		Update(data, result, size, current);
		return;
	}
	double** dataNew;
	long* resultNew;
	double*** forgottenData = (double***)malloc(feature*sizeof(double**));
	long* forgottenClass = (long*)malloc(classes*sizeof(long));
	for(i=0;i<classes;i++)forgottenClass[i]=0;
	if(current->height == 0){
		for(i=0; i<feature; i++){
			forgottenData[i] = (double**)malloc(classes*sizeof(double*));
			for(j=0; j<classes; j++){
				forgottenData[i][j] = (double*)malloc(forgetSize*sizeof(double));
			}
		}
		dataNew = (double**)malloc((size+current->size-forgetSize)*sizeof(double*));
		resultNew = (long*)malloc((size+current->size-forgetSize)*sizeof(long));
		for(i=0;i<size;i++){
			dataNew[i] = data[i];
			resultNew[i] = result[i];
		}
		for(i=0; i<current->size; i++){
			index[i] = i;
		}
		if(isRF)std::shuffle(index, index+current->size, g);
		long x = 0;
		for(i=0;i<current->size;i++){
			if(i>=current->size-forgetSize){
				for(j=0; j<feature; j++){
					forgottenData[j][current->resultRecord[index[i]]][forgottenClass[current->resultRecord[index[i]]]]=current->dataRecord[index[i]][j];
				}
				forgottenClass[current->resultRecord[index[i]]]++;
				current->dataRecord[index[i]][feature] = DBL_MAX;
			}else{
				dataNew[i+size] = current->dataRecord[index[i]];
				resultNew[i+size] = current->resultRecord[index[i]];
			}
		}
		for(i=0; i<feature; i++){
			for(j=0; j<classes; j++){
				std::sort(forgottenData[i][j], forgottenData[i][j]+forgottenClass[j]);
			}
		}
	}else{
		forgetSize = 0;
		dataNew = (double**)malloc((size+current->size)*sizeof(double*));
		resultNew = (long*)malloc((size+current->size)*sizeof(long));
		long xxx[current->size];
		for(i=0;i<size;i++){
			dataNew[i] = data[i];
			resultNew[i] = result[i];
		}
		for(i=0;i<current->size;i++){
                        if(current->dataRecord[i][feature] == DBL_MAX){
                        	xxx[forgetSize]=i;
				forgetSize++;
				forgottenClass[current->resultRecord[i]]++;
			}else{
                                dataNew[i+size-forgetSize] = current->dataRecord[i];
                                resultNew[i+size-forgetSize] = current->resultRecord[i];
                        }
                }
		if(forgetSize==current->size){
			free(forgottenData);
			free(forgottenClass);
			if(size!=0){
				free(dataNew);
				free(resultNew);
				Update(data, result, size, current);
			}else{
				// if a node have no new data and forget all old data, just keep old data
				return;
			}
			return;
		}
		for(i=0; i<feature; i++){
			forgottenData[i] = (double**)malloc(classes*sizeof(double*));
			for(j=0; j<classes; j++){
				forgottenData[i][j] = (double*)malloc(std::max(forgottenClass[j], (long)1)*sizeof(double));
			}
		}
		long* k = (long*)malloc(sizeof(long)*classes);
		for(i=0; i<classes; i++)k[i]=0;
		for(i=0;i<forgetSize;i++){
			long tmp = xxx[i];
			for(j=0; j<feature; j++){
				forgottenData[j][current->resultRecord[tmp]][k[current->resultRecord[tmp]]]=current->dataRecord[tmp][j];
			}
			k[current->resultRecord[tmp]]++;
		}
		free(k);
		for(i=0; i<feature; i++){
			for(j=0; j<classes; j++){
				std::sort(forgottenData[i][j], forgottenData[i][j]+forgottenClass[j]);
			}
		}
	}
	free(data);
	free(result);
	current->size -= forgetSize;
	current->size += size;
	// end condition
	if(current->terminate or current->height==maxHeight or current->size==1){
		for(i=0;i<feature;i++){
			for(j=0; j<classes; j++){
				free(forgottenData[i][j]);
			}
			free(forgottenData[i]);
		}
		free(forgottenData);
		free(forgottenClass);
		if(current->height == 0){
			for(i=0; i<forgetSize; i++){
				free(current->dataRecord[index[current->size-size+i]]);
			}
		}
		free(index);
		Update(dataNew, resultNew, current->size, current);
		return;
	}else if(size==0){
		for(i=0;i<feature;i++){
			for(j=0; j<classes; j++){
				free(forgottenData[i][j]);
			}
			free(forgottenData[i]);
		}
		free(forgottenData);
		free(forgottenClass);
		Update(dataNew, resultNew, current->size, current);
		return;
	}
	// find min gini
	minEval c, cMin;
	long cFeature;
	cMin.eval = DBL_MAX;
	cMin.values = nullptr;
	long T[classes];
	double HY=0;
	for(i=0;i<classes;i++){
		T[i] = 0;
	}
	for(i=0;i<size;i++){
		j = resultNew[i];
		T[j]++;
	}
	for(i=0;i<classes;i++){
		if(evalue == Evaluation::entropy){
			if(T[i]!=0)HY -= ((double)T[i]/size)*log2((double)T[i]/size);
		}else{
			HY += pow(((double)T[i]/size), 2);
		}
	}
	for(i=0;i<maxFeature; i++){
		long col = DTree->featureId[i];
		if(Sparse[col]==1){
			c = incrementalMinGiniSparse(dataNew, resultNew, current->size, size, current, col, forgetSize, forgottenData[col], forgottenClass);
		}
		else if(Sparse[col]==0){
			c = incrementalMinGiniDense(dataNew, resultNew, size, col, current->count, current->record, current->max, current->size, forgetSize, forgottenData[col], forgottenClass);
		}else{
			//c = incrementalMinGiniCategorical();
		}
		if(c.eval<cMin.eval){
			cMin.eval = c.eval;
			cMin.value = c.value;
			cMin.values = c.values;
			cFeature = col;
		}
	}
	for(i=0;i<feature;i++){
		for(j=0; j<classes; j++){
			free(forgottenData[i][j]);
		}
		free(forgottenData[i]);
	}
	free(forgottenData);
	free(forgottenClass);
	if(cMin.eval==DBL_MAX){
		current->terminate = true;
		long t[classes];
		for(i=0;i<classes;i++){
			t[i]=0;
		}
		for(i=low;i<low+size;i++){
			t[resultNew[i]]++;
		}
		if(cMin.values!=nullptr)free(cMin.values);
		current->result = std::distance(t, std::max_element(t, t+classes));
		free(index);
		free(current->dataRecord);
		free(current->resultRecord);
		current->dataRecord = dataNew;
		current->resultRecord = resultNew;
		return;
	}
	//diverse data
        long ptL=0, ptR=0;
	double* t;
	long currentSize = current->size;
	// Same diverse point as last time
	if(current->dpoint==cMin.value and current->feature==cFeature){
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
		free(index);
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
	
	// TODO: free memeory
	if(current->height == 0){
		for(i=0; i<forgetSize; i++){
			free(current->dataRecord[index[current->size-size+i]]);
		}
	}
	free(index);
	free(current->dataRecord);
	free(current->resultRecord);
	current->dataRecord = dataNew;
	current->resultRecord = resultNew;
}
void DecisionTree::Update(double** data, long* result, long size, DT* current){
	if(not current->created)createNode(current, current->height, feature, classes);
	long low = 0;
	long i, j;
	double HY = 0;
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
		for(i=0;i<size;i++){
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
		if(evalue == Evaluation::entropy){
			if(T[i]!=0)HY -= ((double)T[i]/size)*log2((double)T[i]/size);
		}else{
			HY += pow(((double)T[i]/size), 2);
		}
	}
	// find min Evaluation
	minEval c, cMin;
	long cFeature, oldMax, col, left=0;
	cMin.eval = DBL_MAX;
	cMin.values = nullptr;
	cFeature = -1;
	//TODO: categorical
	for(i=0;i<maxFeature; i++){
		col = DTree->featureId[i];
		if(Sparse[col]==1){
			c = findMinGiniSparse(data, result, T, size, col, current);
		}
		else if(Sparse[col]==0){
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
			cFeature = col;
			left = c.left;
		}else if(c.values!=nullptr){
			free(c.values);
		}
	}

	if(cMin.eval == DBL_MAX){
		current->terminate = true;
		long max = 0;
		long maxs[classes];
		long count = 0;
		for(i=1;i<classes;i++){
			if(T[max]<T[i]){
				max=i;
			}
		}
		current->result = max;
		return;
	}
	//printf("   %f\n", HY-cMin.eval);
	//diverse data
	current->terminate = false;
	current->feature = cFeature;
	current->dpoint = cMin.value;
        long ptL=0, ptR=0;
	//TODO: categorical
	long* resultL = new long[size];
	long* resultR = new long[size];
	double** dataL = new double*[size];
	double** dataR = new double*[size];
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
	if(root->terminate or root->height == maxHeight)return root->result;
	if(data[root->feature]<=root->dpoint)return Test(data, root->left);
	return Test(data, root->right);
}

void DecisionTree::print(DT* root){
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
