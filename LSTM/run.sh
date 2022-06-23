#!/bin/bash
./run_test_30_BE.sh 10 10 10 &
./run_test_30_cbr.sh 10 10 10 &
./run_test_30_vid.sh 10 10 10 &
./run_test_30_voip.sh  10 10 10 &
./run_test_50_BE.sh  10 10 10 &
./run_test_50_cbr.sh  10 10 10 &
./run_test_50_vid.sh  10 10 10 &
./run_test_50_voip.sh  10 10 10 
