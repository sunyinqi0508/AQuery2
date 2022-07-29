#include "udf.hpp"

int main(){
    vector_type _a{1,2,3,4};
    vector_type _b{2,3,3,5};
    ColRef<int> a("a");
    ColRef<int> b("b");
    a.initfrom(_a, "a");
    b.initfrom(_b, "b");
    ColRef<decltype(covariances2_gettype(a,b,0))> ret{4};
    covariances2(a,b,2,4,ret);
    
    print(ret);
}
