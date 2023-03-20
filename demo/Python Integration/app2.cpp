#include "server/vector_type.hpp"
#include "sdk/aquery.h"
#include "matplot/matplot.h"

__AQEXPORT__(bool)
draw(vector_type<int> x, vector_type<int> y) {
    using namespace matplot;
    auto plt = gca();
    plt->plot(vector_type_std{x}, vector_type_std{y});
    show();
    return true;
}
