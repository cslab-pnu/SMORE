#!/bin/bash

function exprReplace(){
ed --s ${1} << de
%s/${2}/${3}
w
de
}

dir=`ls`

cur="./"
prev="./"
for i in $dir
do
  cur="${i}"
  echo "cur ${cur}, prev ${prev}"
  exprReplace "./${cur}/main.c" "aha-compress" ${cur}
  echo "replace done!"
  prev="${i}"
done

