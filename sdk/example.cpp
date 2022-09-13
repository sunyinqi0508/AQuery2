#include "aquery.h"
// __AQ_NO_SESSION__
#include "../server/table.h"

__AQEXPORT__(ColRef<float>) mulvec(int a, ColRef<float> b){
    return a * b;
}

__AQEXPORT__(double) mydiv(int a, int b){
    return a / (double)b;
}
