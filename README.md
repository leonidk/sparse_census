# sparse_census
Just a hack-y sparse variant of Census for some folks to use. 

## directions
* Run Makefile
* run ./test_sparse

## documentation
* sparse_census.h requires nothing but std c++
* test.cpp requires opencv
* settings of value are at the top of sparse_census.h
   * #define BOX_RADIUS (4), the radius of the block matching window (4 means 9x9)
   * #define MAX_DISP (128), the maximum search range in pixels
 * currently points at infinity are ignored
 * currently no "fidelity" matching metric exists (see centest repo for outlier removal methods)
