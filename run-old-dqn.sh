#!/bin/bash

#dqn
echo "video60_242k"
./main > test_results/output/dqn_video60_242k.log
sleep 5s

echo "video30_cbr30_242k"
./main > test_results/output/dqn_video30_cbr30_242k.log
sleep 5s

echo "video15_cbr15_be15_voip15_242k"
./main > test_results/output/dqn_video15_cbr15_be15_voip15_242k.log
sleep 5s

echo "video400_242k"
./main > test_results/output/dqn_video400_242k.log
sleep 5s

echo "video200_cbr200_242k"
./main > test_results/output/dqn_video200_cbr200_242k.log
sleep 5s

echo "video100_cbr100_be100_voip100_242k"
./main > test_results/output/dqn_video100_cbr100_be100_voip100_242k.log


#pf
echo "video60_242k"
./main 0 > test_results/output/pf_video60_242k.log
sleep 5s

echo "video30_cbr30_242k"
./main 0 > test_results/output/pf_video30_cbr30_242k.log
sleep 5s

echo "video15_cbr15_be15_voip15_242k"
./main 0 > test_results/output/pf_video15_cbr15_be15_voip15_242k.log
sleep 5s

echo "video400_242k"
./main 0 > test_results/output/pf_video400_242k.log
sleep 5s

echo "video200_cbr200_242k"
./main 0 > test_results/output/pf_video200_cbr200_242k.log
sleep 5s

echo "video100_cbr100_be100_voip100_242k"
./main 0 > test_results/output/pf_video100_cbr100_be100_voip100_242k.log


#mlwdf
echo "video60_242k"
./main 1 > test_results/output/mlwdf_video60_242k.log
sleep 5s

echo "video30_cbr30_242k"
./main 1 > test_results/output/mlwdf_video30_cbr30_242k.log
sleep 5s

echo "video15_cbr15_be15_voip15_242k"
./main 1 > test_results/output/mlwdf_video15_cbr15_be15_voip15_242k.log
sleep 5s

echo "video400_242k"
./main 1 > test_results/output/mlwdf_video400_242k.log
sleep 5s

echo "video200_cbr200_242k"
./main 1 > test_results/output/mlwdf_video200_cbr200_242k.log
sleep 5s

echo "video100_cbr100_be100_voip100_242k"
./main 1 > test_results/output/mlwdf_video100_cbr100_be100_voip100_242k.log


#exp
echo "video60_242k"
./main 2 > test_results/output/exp_video60_242k.log
sleep 5s

echo "video30_cbr30_242k"
./main 2 > test_results/output/exp_video30_cbr30_242k.log
sleep 5s

echo "video15_cbr15_be15_voip15_242k"
./main 2 > test_results/output/exp_video15_cbr15_be15_voip15_242k.log
sleep 5s

echo "video400_242k"
./main 2 > test_results/output/exp_video400_242k.log
sleep 5s

echo "video200_cbr200_242k"
./main 2 > test_results/output/exp_video200_cbr200_242k.log
sleep 5s

echo "video100_cbr100_be100_voip100_242k"
./main 2 > test_results/output/exp_video100_cbr100_be100_voip100_242k.log


#fls
echo "video60_242k"
./main 3 > test_results/output/fls_video60_242k.log
sleep 5s

echo "video30_cbr30_242k"
./main 3 > test_results/output/fls_video30_cbr30_242k.log
sleep 5s

echo "video15_cbr15_be15_voip15_242k"
./main 3 > test_results/output/fls_video15_cbr15_be15_voip15_242k.log
sleep 5s

echo "video400_242k"
./main 3 > test_results/output/fls_video400_242k.log
sleep 5s

echo "video200_cbr200_242k"
./main 3 > test_results/output/fls_video200_cbr200_242k.log
sleep 5s

echo "video100_cbr100_be100_voip100_242k"
./main 3 > test_results/output/fls_video100_cbr100_be100_voip100_242k.log


#exp-rule
echo "video60_242k"
./main 4 > test_results/output/exp-rule_video60_242k.log
sleep 5s

echo "video30_cbr30_242k"
./main 4 > test_results/output/exp-rule_video30_cbr30_242k.log
sleep 5s

echo "video15_cbr15_be15_voip15_242k"
./main 4 > test_results/output/exp-rule_video15_cbr15_be15_voip15_242k.log
sleep 5s

echo "video400_242k"
./main 4 > test_results/output/exp-rule_video400_242k.log
sleep 5s

echo "video200_cbr200_242k"
./main 4 > test_results/output/exp-rule_video200_cbr200_242k.log
sleep 5s

echo "video100_cbr100_be100_voip100_242k"
./main 4 > test_results/output/exp-rule_video100_cbr100_be100_voip100_242k.log


#log
echo "video60_242k"
./main 5 > test_results/output/log_video60_242k.log
sleep 5s

echo "video30_cbr30_242k"
./main 5 > test_results/output/log_video30_cbr30_242k.log
sleep 5s

echo "video15_cbr15_be15_voip15_242k"
./main 5 > test_results/output/log_video15_cbr15_be15_voip15_242k.log
sleep 5s

echo "video400_242k"
./main 5 > test_results/output/log_video400_242k.log
sleep 5s

echo "video200_cbr200_242k"
./main 5 > test_results/output/log_video200_cbr200_242k.log
sleep 5s

echo "video100_cbr100_be100_voip100_242k"
./main 5 > test_results/output/log_video100_cbr100_be100_voip100_242k.log


echo "test end"