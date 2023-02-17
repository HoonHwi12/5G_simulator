#!/bin/bash

NUM_CELL=3

CAL_VID=0
CAL_CBR=0
CAL_BE=0
CAL_VOIP=0

for SC_TYPE in 7;
do
	NUM_VID=12
	NUM_CBR=0
	NUM_BE=0
	NUM_VOIP=0

	#for loop in 1;
	#do
		#for NUM_VID in 12;
		#do      
			CAL_VID=$(($NUM_VID/$NUM_CELL))
			CAL_CBR=$(($NUM_CBR/$NUM_CELL))
			CAL_BE=$(($NUM_BE/$NUM_CELL))
			CAL_VOIP=$(($NUM_VOIP/$NUM_CELL))

			sleep 7
			echo "./5G-air-simulator MultiCellWithIMixedApps $NUM_CELL 1 600 $CAL_VID $CAL_CBR $CAL_BE $CAL_VOIP 0 1 30 0.1 242 $SEED"
			./5G-air-simulator MultiCellWithIMixedApps $NUM_CELL 1 600 $CAL_VID $CAL_CBR $CAL_BE $CAL_VOIP 0 1 30 0.1 242 $SEED
			sleep 7
		#done
	#done
done

echo "test end"
#done
