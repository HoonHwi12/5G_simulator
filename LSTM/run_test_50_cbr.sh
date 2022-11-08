#!/bin/bash

# DIR=( ue_30_cbr_small ue_30_video_small ue_50_cbr_small ue_50_video_small )
DIR=( new_ue_50_cbr)
BENCHROOT=/home/hoonhwi/net/LTESim/LSTM/test_results


iter_0=0
iter_1=$1

epoch_0=0
epoch_1=$2

echo "-------------------------START-------------------------"
while [ ${epoch_0} -le ${epoch_1} ]
do
    iter_0=0
    for dir in ${DIR[@]}
    do
        for files in $BENCHROOT/$dir/*
        do
            echo $files;
            echo "iter:["$iter_0"] epoch:["$epoch_0"] "
            ./build/main $files $iter_0 $BENCHROOT/"cbr"/"output_2"/train $epoch_0 $iter_1 > $BENCHROOT/"cbr"/"output_2"/"output"$iter_0"_"$epoch_0".txt"

            
            if [ ${iter_0} -eq ${iter_1} ] ; then
                break
            fi

            iter_0=$((iter_0+1))
        done

    done
    epoch_0=$((epoch_0+1))
done

echo "--------------------------END--------------------------"