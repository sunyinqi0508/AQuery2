// __AQ_NO_SESSION__
#include "../server/table.h"
#include "aquery.h"

__AQEXPORT__(ColRef_storage) mulvec(int a, ColRef<float> b){
    return a * b;
}

__AQEXPORT__(double) mydiv(int a, int b){
    	printf("%d, %d\n", a, b);
	double d = a/(double)b;
	printf("%lf", d);
	return d;
}
