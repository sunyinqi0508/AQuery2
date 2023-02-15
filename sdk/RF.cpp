#include "RF.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctime>
#include <math.h>
#include <algorithm>
#include <boost/math/distributions/students_t.hpp>
#include <random>

long poisson(int Lambda)
{
	int  k = 0;
	long double p = 1.0;
	long double l = exp(-Lambda);
	srand((long)clock());
	
	while(p>=l)
	{
		double u = (double)(rand()%10000)/10000;
		p *= u;
		k++;
	}
	if (k>11)k=11;
	return k-1;
}
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

RandomForest::RandomForest(long mTree, long feature, int* s, double forg, long noC, Evaluation eval, bool b, double t){
	srand((long)clock());
	bagging = b;
	activeTree = mTree;
	maxTree = mTree;
	allT = new long[mTree];

	tThresh=t;
	lastT = -2;
	lastAll = 0;
	long i;
	f = feature;
	sparse = new int[f];
	for(i=0; i<f; i++)sparse[i]=s[i];
	forget = forg;
	noClasses = noC;
	e = eval;
	minF = floor(sqrt((double)f))+2;
	if(minF>f)minF=f;
	DTrees = (DecisionTree**)malloc(mTree*sizeof(DecisionTree*));
	for(i=0; i<maxTree; i++){
		DTrees[i] = new DecisionTree(f, sparse, forget, minF+rand()%(f+1-minF), noClasses, e);
		DTrees[i]->isRF=true;
	}
}

void RandomForest::fit(double** data, long* result, long size){
	long i, j, k, l;
	double** newData;
	long* newResult;
	long localT = 0;
	int stale = 0;
	if(lastT==-2){
		lastT=-1;
	}else{
		for(i=0; i<maxTree; i++)allT[i] = 0;
		for(i=0; i<size; i++){
                	if(Test(data[i], result[i])==result[i])localT++;
        	}
		long localAll = size;
		if(lastT>=0){
			double lastSm = (double)lastT/lastAll;
                        double localSm = (double)localT/localAll;
                        double lastSd = sqrt(pow((1.0-lastSm),2)*lastT+pow(lastSm,2)*(lastAll-lastT)/(lastAll-1));
                        double localSd = sqrt(pow((1.0-localSm),2)*localT+pow(localSm,2)*(localAll-localT)/(localAll-1));
                        double v = lastAll+localAll-2;
                        double sp = sqrt(((lastAll-1) * lastSd * lastSd + (localAll-1) * localSd * localSd) / v);
                        double q;
			double t = lastSm-localSm;
			if(sp==0){q = 1;}
			else{
				t = t/(sp*sqrt(1.0/lastAll+1.0/localAll));
                        	boost::math::students_t dist(v);
                        	double c = cdf(dist, t);
				q = cdf(complement(dist, fabs(t)));
			}
                        if(q<=tThresh){
				lastT += localT;
				lastAll += localAll;
			}else if(t<0){
				lastT = localT;
				lastAll = localAll;
			}else{
				double newAcc = (double)localT/localAll;
				double lastAcc= (double)lastT/lastAll;
				stale = floor((newAcc-lastAcc)/(lastAcc)*maxTree);
				lastT = localT;
				lastAll = localAll;
			}
		}else{
			lastT = localT;
			lastAll = localAll;
		}
	}
	Rotate(stale);
	for(i=0; i<maxTree; i++){
		long times;
	       	if(bagging)times = poisson(6);
		else times=1;
		if(times==0)continue;
		newData = (double**)malloc(sizeof(double*)*size*times);
		newResult = (long*)malloc(sizeof(long)*size*times);
		long c = 0;
		for(j = 0; j<size*times; j++){
			long jj;
		       	if(bagging) jj = rand()%size;
			else jj=j;
			newData[j] = (double*)malloc((f+1)*sizeof(double));
			for(l=0; l<f; l++){
				newData[j][l] = data[jj][l];
			}
			newData[j][f] = 0;
			newResult[j] = result[jj];
		}
		DTrees[i]->fit(newData, newResult, size*times);
	}
	/*for(i=0; i<maxTree; i++){
		//backupTrees[i]->retain = 10*size;
		//if(backupTrees[i]==nullptr) continue;
		long times;
		//times = poisson(posMean);
		//if(times==0)continue;
		times=1;
		newData = (double**)malloc(sizeof(double*)*size*times);
		newResult = (long*)malloc(sizeof(long)*size*times);
		long c = 0;
		for(j = 0; j<size; j++){
			long jj = rand()%size;
			jj=j;
			for(k=0; k<times; k++){
				newData[j*times+k] = (double*)malloc((f+1)*sizeof(double));
				for(l=0; l<f; l++){
					newData[j*times+k][l] = data[jj][l];
				}
				newData[j*times+k][f] = 0;
				newResult[j*times+k] = result[jj];
			}
		}
		backupTrees[i]->fit(newData, newResult, size*times);
	}*/
}

long* RandomForest::fitThenPredict(double** trainData, long* trainResult, long trainSize, double** testData, long testSize){
        fit(trainData, trainResult, trainSize);
        long* testResult = (long*)malloc(testSize*sizeof(long));
        for(long i=0; i<testSize; i++){
                testResult[i] = Test(testData[i]);
        }
        return testResult;
}

void RandomForest::Rotate(long stale){
	long i, j, k;
	long minIndex = -1;
	if(stale>=0)return;
	else{
		stale = std::min(stale, maxTree);
		stale*=-1;
		while(stale>0){
			long currentMin = 2147483647;
			for(i = 0; i<maxTree; i++){
				if(allT[i]<currentMin){
					currentMin=allT[i];
					minIndex=i;
				}
			}
			stale--;
			if(minIndex<0)break;
			allT[minIndex] = 2147483647; 
				double** newData;
				long* newResult;
				long size = 0;
				long lastT2 = 0;
				size = DTrees[minIndex]->DTree->size;
				newData = (double**)malloc(sizeof(double*)*size);
				newResult = (long*)malloc(sizeof(long)*size);
				for(j = 0; j<size; j++){
					newData[j] = (double*)malloc(sizeof(double)*(f+1));
               				for(k=0; k<f; k++){
                       				newData[j][k] = DTrees[minIndex]->DTree->dataRecord[j][k];
                			}
					newData[j][f] = 0;
                			newResult[j] = DTrees[minIndex]->DTree->resultRecord[j];
        			}
				DTrees[minIndex]->Stablelize();
				DTrees[minIndex]->Free();
				delete DTrees[minIndex];
				DTrees[minIndex] = new DecisionTree(f, sparse, forget, minF+rand()%(f+1-minF), noClasses, e);
				DTrees[minIndex]->isRF=true;
				DTrees[minIndex]->fit(newData, newResult, size);
				for(j=0; j<size; j++){
					if(DTrees[minIndex]->Test(newData[j], DTrees[minIndex]->DTree)==newResult[j])lastT2++;
				}
				DTrees[minIndex]->lastAll=size;
				DTrees[minIndex]->lastT=lastT2;
		}	
	}
}


long RandomForest::Test(double* data, long result){
	long i;
	long predict[noClasses];
	for(i=0; i<noClasses; i++){
		predict[i]=0;
	}
	for(i=0; i<maxTree; i++){
		long tmp = DTrees[i]->Test(data, DTrees[i]->DTree);
		predict[tmp]++;
		if(tmp==result)allT[i]++;
	}
	
	long ret = 0;
	for(i=1; i<noClasses; i++){
		if(predict[i]>predict[ret])ret = i;
	}

	return ret;
}

long RandomForest::Test(double* data){
	long i;
	long predict[noClasses];
	for(i=0; i<noClasses; i++)predict[i]=0;
	for(i=0; i<maxTree; i++){
		predict[DTrees[i]->Test(data, DTrees[i]->DTree)]++;
	}
	
	long ret = 0;
	for(i=1; i<noClasses; i++){
		if(predict[i]>predict[ret])ret = i;
	}

	return ret;
}
