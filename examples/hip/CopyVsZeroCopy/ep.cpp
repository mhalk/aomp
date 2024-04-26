// MIT License
//
// Copyright (c) 2017 Advanced Micro Devices, Inc. All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// This program replicates OpenMP behavior for two (extremely reduced)
// kernels of the benchmark SPECaccel 2023 452.ep, emulating OpenMP's
// copy and zero-copy runtime behaviors.


#include <cstdlib>
#include <cstdio>
#include <sys/time.h>
#include <iostream>
#include <cmath>

#include "hip/hip_runtime.h"

__global__ void init_xx(double *xx, int length) {
  int i = threadIdx.x + blockIdx.x*blockDim.x;
  if (i > length) return;
  xx[i] = 1.0;
}

__global__ void inc_xx(double *xx, int blksize, int nk) {
  int k = threadIdx.x + blockIdx.x*blockDim.x;
  if (k >= blksize) {
    return;
  }
  for(int i=0; i<2*nk; i++) {
    xx[k*2*nk + i] += 1.0;
  }
  return;
}

int main() {
  int blksize = 15000;
  int nk = 65536;
  double *xx = (double *)malloc(blksize*2*nk*sizeof(double));
  int m = 40;
  int mk = 16;
  int mm = m - mk;
  int np = (1 << mm);
  int numblks = ceil( (double)np / (double) blksize);
  hipError_t err;
  int Iterations = 10;

#ifdef CHECK_RESULTS
  printf("numblks = %d\n", numblks);
  printf(" > Mode: Correctness Checking\n");
  for (int t = 0; t < blksize * 2 * nk; t++)
    xx[t] = 0.0;
#endif

  char *HSA_XNACK_Env = getenv("HSA_XNACK");
  bool isXnackEnabled = false;
  if (HSA_XNACK_Env) {
    int HSA_XNACK_Val = atoi(HSA_XNACK_Env);
    isXnackEnabled = (HSA_XNACK_Val > 0) ? true : false;
  }

  double *d_xx = nullptr;
  // #pragma omp target enter data map(alloc : xx[0:blksize*2*nk])
  if (!isXnackEnabled) { // Copy
#ifdef CHECK_RESULTS
    printf("OpenMP Copy configuration\n");
#endif
    err = hipMalloc(&d_xx, blksize*2*nk*sizeof(double));
    if (err != HIP_SUCCESS) {
      // printf("Cannot allocate device memory\n");
      return 0;
    }
    //hipMemcpy(d_xx, xx, blksize*2*nk*sizeof(double), hipMemcpyHostToDevice);
  } else {
#ifdef CHECK_RESULTS
    printf("OpenMP Zero-Copy configuration\n");
#endif
    d_xx = xx; // zero-copy
  }

  for (int blk=0; blk < Iterations; ++blk) {
#ifdef CHECK_RESULTS
    printf("blk=%d\n", blk);
#endif
    // #pragma omp target teams loop collapse(2)
    // for(int k=0; k<blksize; k++)
    //   for(int i=0; i<2*nk; i++)
    // 	xx[k*2*nk + i] = 1.0;
    init_xx<<<7680000, 256, 0>>>(d_xx, blksize*2*nk);
    hipDeviceSynchronize();
    // #pragma omp target teams loop
    // for (int k = 0; k < blksize; k++)
    //   for(int i=0; i<2*nk; i++)
    // 	xx[k*2*nk + i] += 1.0;
    inc_xx<<<938, 16, 0>>>(d_xx, blksize, nk);
    hipDeviceSynchronize();
  }

  // #pragma omp target exit data map(from : xx[0:blksize*2*nk])
  if (!isXnackEnabled) { // Copy
    err = hipMemcpy(xx, d_xx, blksize*2*nk*sizeof(double), hipMemcpyDeviceToHost);
    if (err != HIP_SUCCESS) {
      // printf("Cannot copy device to host memory\n");
      return 0;
    }
#ifdef CHECK_RESULTS
    printf(" > Copy -- Device->Host -- DONE\n");
#endif
  }

#ifdef CHECK_RESULTS
  // Keep in mind: this will create a file of size: sizeof(double) * NumElements
  size_t NumElements = 1024 * 1024 * 4;
  FILE *FilePtr;
  FilePtr = fopen("hip.bin", "wb");
  size_t WrittenCount = fwrite(xx, sizeof(double), NumElements, FilePtr);
  fclose(FilePtr);
  printf(" > Done -- HIP results (%lu / %lu elements written)\n",
         WrittenCount, NumElements);

  // std::ofstream FileStream;
  // FileStream.open("hip.txt");
  // for (size_t i = 0; i < NumElements; ++i)
  //  FileStream << xx[i] << '\n';
  // FileStream.close();

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

  printf(" > Done -- HIP results (full check) -- Mismatched elements: %lu\n",
         MaxNumElements - Matches);

#endif

  return 0;
}
