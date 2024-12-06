#!/bin/bash
set -x
export AOMP=${AOMP:-/opt/rocm/llvm}
git clone https://github.com/ROCm/aomp.git /tmp/aomp$$
cd /tmp/aomp$$/test/smoke/helloworld
make run
cd /tmp/aomp$$/test/smoke/veccopy
make run
cd /tmp
rm -rf aomp$$


