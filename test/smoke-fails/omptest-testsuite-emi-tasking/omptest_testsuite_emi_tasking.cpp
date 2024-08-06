#include "OmptTester.h"

#include <omp.h>

using namespace omptest;
using namespace internal;

TEST(TaskingSuite, TaskDestruction) {
  const int NumThreads = 4;
  int N = 128;
  int a[N];
  int b[N];

  // Allow set asserter to pass even when more than the expected events occur
  OMPT_ASSERT_SET_MODE_RELAXED()

  // Allow certain host events (since most are disabled by default)
  OMPT_PERMIT_EVENT(EventTy::SyncRegion)
  OMPT_PERMIT_EVENT(EventTy::TaskCreate)
  OMPT_PERMIT_EVENT(EventTy::ImplicitTask)

  for (int i = 0; i < N; i++)
    a[i] = 0;

  for (int i = 0; i < N; i++)
    b[i] = i;

  // Add NumThreads expected events of type SyncRegion-begin
  OMPT_GENERATE_EVENTS(
      NumThreads, OMPT_ASSERT_SET(SyncRegion, SR_BARRIER_IMPL_PARALLEL, BEGIN));
  // There should be a single SyncRegion-end where: parallel_data == nullptr
  OMPT_ASSERT_SET(SyncRegion, SR_BARRIER_IMPL_PARALLEL, END,
                  /*parallel_data=*/nullptr);

#pragma omp parallel for num_threads(NumThreads)
  {
    for (int j = 0; j < N; j++)
      a[j] = b[j];
  }

  // Assert that all expected events have occurred
  OMPT_ASSERT_SYNC_POINT("S1")

  int rc = 0;
  for (int i = 0; i < N; i++)
    if (a[i] != b[i]) {
      rc++;
      printf("Wrong value: a[%d]=%d\n", i, a[i]);
    }

  if (!rc)
    printf("Success\n");
}

// Leave this suite down here so it gets discovered last and executed first.
TEST(InitialSuite, DeviceLoad) {
  // We only want to assert on DeviceLoads: ignore other events
  OMPT_ASSERT_SET_MODE_RELAXED()

  // Supress unwanted events so they do not clutter console output
  OMPT_SUPPRESS_EVENT(EventTy::TargetEmi)
  OMPT_SUPPRESS_EVENT(EventTy::TargetDataOpEmi)
  OMPT_SUPPRESS_EVENT(EventTy::TargetSubmitEmi)
  OMPT_SUPPRESS_EVENT(EventTy::BufferRequest)
  OMPT_SUPPRESS_EVENT(EventTy::BufferComplete)
  OMPT_SUPPRESS_EVENT(EventTy::BufferRecord)
  OMPT_SUPPRESS_EVENT(EventTy::BufferRecordDeallocation)

  for (int i = 0; i < omp_get_num_devices(); ++i) {
    OMPT_ASSERT_SET(DeviceLoad, /*DeviceNum=*/i)
#pragma omp target device(i)
    {
      ;
    }
  }
}

#ifndef LIBOFFLOAD_LIBOMPTEST_USE_GOOGLETEST
int main(int argc, char **argv) {
  Runner R;
  return R.run();
}
#endif
