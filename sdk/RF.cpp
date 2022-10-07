#include "RF.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctime>

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

RandomForest::RandomForest(long mTree, long actTree, long rTime, int h, long feature, int* s, double forg, long maxF, long noC, Evaluation eval, long r, long rb){
	srand((long)clock());
	Rebuild = rb;
	if(actTree<1)actTree=1;
	noTree = actTree;
	activeTree = actTree;
	treePointer = 0;
	if(mTree<actTree)mTree=activeTree;
	maxTree = mTree;
	if(rTime<=0)rTime=1;
	rotateTime = rTime;
	timer = 0;
	retain = r;	

	long i;
	height = h;
	f = feature;
	sparse = new int[f];
	for(i=0; i<f; i++)sparse[i]=s[i];
	forget = forg;
	maxFeature = maxF;
	noClasses = noC;
	e = eval;

	DTrees = (DecisionTree**)malloc(mTree*sizeof(DecisionTree*));
	for(i=0; i<mTree; i++){
		if(i<actTree){
			DTrees[i] = new DecisionTree(height, f, sparse, forget, maxFeature, noClasses, e, r, rb);
		}
		else{
			DTrees[i]=nullptr;
		}
	}
}

void RandomForest::fit(double** data, long* result, long size){
	if(timer==rotateTime and maxTree!=activeTree){
		Rotate();
		timer=0;
	}
	long i, j, k;
	double** newData;
	long* newResult;
	for(i=0; i<activeTree; i++){
		newData = new double*[size];
		newResult = new long[size];
		for(j = 0; j<size; j++){
			newData[j] = new double[f];
			for(k=0; k<f; k++){
				newData[j][k] = data[j][k];
			}
			newResult[j] = result[j];
		}
		DTrees[(i+treePointer)%maxTree]->fit(newData, newResult, size);
	}
	timer++;
}

long* RandomForest::fitThenPredict(double** trainData, long* trainResult, long trainSize, double** testData, long testSize){
        fit(trainData, trainResult, trainSize);
        long* testResult = (long*)malloc(testSize*sizeof(long));
        for(long i=0; i<testSize; i++){
                testResult[i] = Test(testData[i]);
        }
        return testResult;
}

void RandomForest::Rotate(){
	if(noTree==maxTree){
		DTrees[(treePointer+activeTree)%maxTree]->Free();
		delete DTrees[(treePointer+activeTree)%maxTree];
	}else{
		noTree++;
	}
	DTrees[(treePointer+activeTree)%maxTree] = new DecisionTree(height, f, sparse, forget, maxFeature, noClasses, e, retain, Rebuild);
	long size = DTrees[(treePointer+activeTree-1)%maxTree]->DTree->size;
	double** newData = new double*[size];
	long* newResult = new long[size];
	for(long j = 0; j<size; j++){
		newData[j] = new double[f];
                for(long k=0; k<f; k++){
                        newData[j][k] = DTrees[(treePointer+activeTree-1)%maxTree]->DTree->dataRecord[j][k];
                }
                newResult[j] = DTrees[(treePointer+activeTree-1)%maxTree]->DTree->resultRecord[j];
        }

	DTrees[(treePointer+activeTree)%maxTree]->fit(newData, newResult, size);
	DTrees[treePointer]->Stablelize();
	if(++treePointer==maxTree)treePointer=0;
}


long RandomForest::Test(double* data){
	long i;
	long predict[noClasses];
	for(i=0; i<noClasses; i++)predict[i]=0;
	for(i=0; i<noTree; i++){
		predict[DTrees[i]->Test(data, DTrees[i]->DTree)]++;
	}
	
	long ret = 0;
	for(i=1; i<noClasses; i++){
		if(predict[i]>predict[ret])ret = i;
	}

	return ret;
}
