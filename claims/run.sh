#!/bin/bash

cd ../artifact/SMORE-RIOT/examples/coremark
rm -rf bin
make flash
cd ../../../../claims
