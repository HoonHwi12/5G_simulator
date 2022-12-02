#!/bin/bash
cd dqn/build
NUM_CELL=1

for SC_TYPE in 0 1 2 4 5 7;
do
	NUM_VID=0
	NUM_CBR=0
	NUM_BE=0
	NUM_VOIP=0
'
	for NUM_VOIP in 1200;
	do
		sleep 5
		echo "./main ${SC_TYPE} > test_results/output/SCHED${SC_TYPE}_CELL${NUM_CELL}_VID${NUM_VID}_CBR${NUM_CBR}_BE${NUM_BE}_VOIP${NUM_VOIP}.log"
		./main ${SC_TYPE} > test_results/output/SCHED${SC_TYPE}_CELL${NUM_CELL}_VID${NUM_VID}_CBR${NUM_CBR}_BE${NUM_BE}_VOIP${NUM_VOIP}.log
		sleep 5
	done

	NUM_VID=0
	NUM_CBR=0
	NUM_BE=0
	NUM_VOIP=0
'	
# single cell
	for NUM_VID in 120 240 360 480 600 720 840;
    do
		sleep 5
		echo "./main ${SC_TYPE} > test_results/output/SCHED${SC_TYPE}_CELL${NUM_CELL}_VID${NUM_VID}_CBR${NUM_CBR}_BE${NUM_BE}_VOIP${NUM_VOIP}.log"
		./main ${SC_TYPE} > test_results/output/SCHED${SC_TYPE}_CELL${NUM_CELL}_VID${NUM_VID}_CBR${NUM_CBR}_BE${NUM_BE}_VOIP${NUM_VOIP}.log
		sleep 5
	done

done


'
for IS_VID in 1;
do
	for IS_CBR in {0..1}
	do
		for IS_BE in {0..1}
		do
			for IS_VOIP in {0..1}
			do
				for NUM_CELL in 5;
				do
					for SC_TYPE in 1 2 3 4 5;
					do
						TOT_APP=$(($IS_VID + $IS_CBR + $IS_BE + $IS_VOIP))
						if [ ${TOT_APP} -ne 0 ];then
							NUM_APP=$((600 / $NUM_CELL / $TOT_APP))
							
							NUM_VID=$(($NUM_APP * $IS_VID))
							NUM_CBR=$(($NUM_APP * $IS_CBR))
							NUM_BE=$(($NUM_APP * $IS_BE))
							NUM_VOIP=$(($NUM_APP * $IS_VOIP))
							echo "./main ${SC_TYPE} > test_results/output/600UE_CELL${NUM_CELL}_VID${NUM_VID}_CBR${NUM_CBR}_BE${NUM_BE}_VOIP${NUM_VOIP}.log"
							./main ${SC_TYPE} > test_results/output/SCHED${SC_TYPE}_600UE_CELL${NUM_CELL}_VID${NUM_VID}_CBR${NUM_CBR}_BE${NUM_BE}_VOIP${NUM_VOIP}.log
							sleep 3
						fi
					done
				done
			done
		done
	done
done
'
cd ../../
echo "test end"
