#!/bin/bash
./main 7 > test_results/output/video60_242k.log
sleep 10s
./main 7 > test_results/output/video30_cbr30_242k.log
sleep 10s
./main 7 > test_results/output/video15_cbr15_be15_voip15_242k.log
sleep 10s
./main 7 > test_results/output/video400_242k.log
sleep 10s
./main 7 > test_results/output/video200_cbr200_242k.log
sleep 10s
./main 7 > test_results/output/video100_cbr100_be100_voip100_242k.log
sleep 10s