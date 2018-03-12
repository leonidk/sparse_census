# sparse_census
Just a hack-y sparse variant of Census for some folks to use. 

## directions
* Run Makefile
* run ./test_sparse

## documentation
* sparse_census.h requires nothing but std c++
* test.cpp requires opencv
* settings of value are at the top of sparse_census.h
   * BOX_RADIUS the radius of the block matching window (4 means 9x9)
   * MAX_DISP, the maximum search range in pixels
   * MIN_COST, the minimum cost (low values are probably saturated)
   * MAX_COST, the maximum cost (better than random!)
   * SECOND_PEAK, the difference between the best and the second best (good matches are unique)
 * currently points at infinity are ignored
 * currently no "fidelity" matching metric exists (see centest repo for outlier removal methods)
