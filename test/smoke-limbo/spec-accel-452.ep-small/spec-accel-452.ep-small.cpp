#include <cmath>
#include <cstdio>
#include <cstdlib>

int main() {

  int blksize = 15000;
  int nk = 65536;
  double *xx = (double *)malloc(blksize * 2 * nk * sizeof(double));
  int m = 40;
  int mk = 16;
  int mm = m - mk;
  int np = (1 << mm);
  int numblks = ceil((double)np / (double)blksize);
  int Iterations = 10;

#ifdef CHECK_RESULTS
  printf("numblks = %d\n", numblks);
  printf(" > Mode: Correctness Checking\n");
  for (int t = 0; t < blksize * 2 * nk; ++t)
    xx[t] = 0.0;
#endif

// Kernel trace =2/3 -- look for API calls of target_alloc?
// ftrace: hsa_ ... _pool_alloc?
#pragma omp target enter data map(alloc : xx[0:blksize * 2 * nk])

  for (int blk = 0; blk < Iterations; ++blk) {
#ifdef CHECK_RESULTS
    printf("blk=%d\n", blk);
#endif
#pragma omp target teams loop collapse(2)
    for (int k = 0; k < blksize; k++)
      for (int i = 0; i < 2 * nk; i++)
        xx[k * 2 * nk + i] = 1.0;

#pragma omp target teams loop
    for (int k = 0; k < blksize; k++)
#pragma omp unroll partial(16)
      for (int i = 0; i < 2 * nk; i++)
        xx[k * 2 * nk + i] += 1.0;
  }

#pragma omp target exit data map(from : xx[0:blksize * 2 * nk])

#ifdef CHECK_RESULTS
  // Keep in mind: this will create a file of size: sizeof(double) * NumElements
  size_t NumElements = 1024 * 1024 * 4;
  FILE *FilePtr;
  FilePtr = fopen("openmp.bin", "wb");
  size_t WrittenCount = fwrite(xx, sizeof(double), NumElements, FilePtr);
  fclose(FilePtr);
  printf(" > Done -- OpenMP results (%lu / %lu elements written)\n",
         WrittenCount, NumElements);

  for (size_t i = 0; i < 20; ++i)
    printf("%f : ", xx[i]);
  printf("\n");

  double Expected = 2.0f;
  size_t Matches = 0;
  size_t MaxNumElements = blksize * 2 * nk;

  for (size_t i = 0; i < MaxNumElements; ++i) {
    if (xx[i] == Expected)
      ++Matches;
  }

  printf(" > Done -- OpenMP results (full check) -- Mismatched elements: %lu\n",
         MaxNumElements - Matches);
#endif

  return 0;
}
