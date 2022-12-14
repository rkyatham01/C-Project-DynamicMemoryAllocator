#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {

    sf_malloc(66);
    sf_malloc(24);
    sf_show_heap();
    printf("%f", sf_internal_fragmentation());

    sf_show_heap();

    return EXIT_SUCCESS;
}
