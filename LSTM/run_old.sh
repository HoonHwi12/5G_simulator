#!/bin/bash

DIR=( new_ue_30_vid)
BENCHROOT=/home/hoonhwi/net/LSTM

num=$2
iter=$1

while [ ${num} -le ${3} ]
do
   for dir in ${DIR[@]}
   do
      num2=0
      mkdir $BENCHROOT/"input_output_1"/$dir
      for files in $BENCHROOT/$dir/*
      do
         echo $files
         echo "iter:["$iter"] epoch:["$num"]"
         ./build/main  $files $num  $BENCHROOT/"input_output_1"/$dir/ $iter 1 > $BENCHROOT/"input_output_1"/$dir/$iter"_"$num"_"$num2".out"
         num2=$((num2+1))

         if [ ${num2} -eq 3 ] ; then
            break
         fi
      done
      num=$((num+1))
   done
done
