#!/bin/bash
cd dqn/build
NUM_CELL=3

for SC_TYPE in 0 1 2 4 5 7;
do
	NUM_VID=12
	NUM_CBR=0
	NUM_BE=0
	NUM_VOIP=0

	#for NUM_VID in 12;
	#do
		sleep 5
		echo "./main ${SC_TYPE}  >> test_results/output/SCHED${SC_TYPE}_CELL${NUM_CELL}_VID${NUM_VID}_CBR${NUM_CBR}_BE${NUM_BE}_VOIP${NUM_VOIP}.log"
		./main ${SC_TYPE} >> test_results/output/SCHED${SC_TYPE}_CELL${NUM_CELL}_VID${NUM_VID}_CBR${NUM_CBR}_BE${NUM_BE}_VOIP${NUM_VOIP}.log
		sleep 5
	#done

	'
	for loop in 1;
	do
		for NUM_VID in 12;
		do
			sleep 5
			echo "./main ${SC_TYPE} 0 1 >> test_results/output/SCHED${SC_TYPE}_CELL${NUM_CELL}_VID${NUM_VID}_CBR${NUM_CBR}_BE${NUM_BE}_VOIP${NUM_VOIP}.log"
			./main ${SC_TYPE} 0 1 >> test_results/output/SCHED${SC_TYPE}_CELL${NUM_CELL}_VID${NUM_VID}_CBR${NUM_CBR}_BE${NUM_BE}_VOIP${NUM_VOIP}.log
			sleep 5
		done

	done
	'
done
cd ../../
echo "test end"
