#!/bin/bash

dir=`ls`

cur="./"
prev="./"
for i in $dir
do
  cur="./${i}"
  echo "cur ${i}, prev ${i}"
  cp ${prev}/main.c ${cur}/main.c
  echo "copy done!"
  prev="./${i}"
done

