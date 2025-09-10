#!/bin/bash

bench_dir=`ls`

for i in $bench_dir; do
  echo "current module is ${i}"
  cd ${i}
  echo "Remove bin dir in ${i}"
  rm -rf bin
  sleep 0.1s
  cd ../
  echo "done!"
done
