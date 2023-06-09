#include <cstdlib>
#include <omp.h>
#include <unistd.h>

extern "C" void consume(int *p);

inline void kernel_long_compute(int kernelSize = 4096, int repetitons = 4096) {
  int N = kernelSize;

  int a[N];
  int b[N];

  for (int i = 0; i < N; ++i)
    a[i] = 0;

  for (int i = 0; i < N; ++i)
    b[i] = i;

  #pragma omp target parallel for map(to:a) map(to:b) // nowait
      for (int i = 0; i < repetitons; ++i) {
        for (int j = 0; j < 1024; ++j) {
          for (int k = 0; k < N; ++k) {
            a[k] = b[k];
          }
        }
      }

  consume(a);
}

inline void kernel_short(int i) {
  int l_val = i;
#pragma omp target map(l_val)
  { l_val += 1; }

  if (l_val != i + 1) {
    abort();
  }

  consume(&l_val);
}

int main(int argc, char **argv) {
  const int NumTasksLong = 3;
  const int NumTasksShort = 4;
  const int NumKernelsShortPerTask = 4;

  #pragma omp parallel for default(none)
  for(int i = 0; i < 12; ++i) {
    if(omp_get_thread_num() < 4) {
      kernel_long_compute();
    }

    /* else {
      for (int j = 0; j < 256; ++j) {
        kernel_short(i);
      }
    }
    */
  }

/*
#pragma omp parallel
#pragma omp single
  {
    for (int i = 0; i < NumTasksLong; ++i) {
#pragma omp task
      { kernel_long_compute();}
    }

    // Suspend this thread for ~ 0.5 seconds
    usleep(500000);

    for (int i = 0; i < NumTasksShort; ++i) {
// #pragma omp task
      for (int j = 0; j < NumKernelsShortPerTask; ++j) {
        kernel_short(i);
      }
    }

  }
*/

  return 0;
}