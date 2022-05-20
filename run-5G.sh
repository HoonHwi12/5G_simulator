#!/bin/bash
echo "video60_242k"
./5G-air-simulator SingleCellWithIMixedApps 5 1 0 60 0 0 0 1 1 30 0.1 242 1  > result/video60_242k.log
sleep 10s

echo "video30_cbr30_242k"
./5G-air-simulator SingleCellWithIMixedApps 5 1 0 30 30 0 0 1 1 30 0.1 242 1 > result/video30_cbr30_242k.log
sleep 10s

echo "video15_cbr15_be15_voip15_242k"
./5G-air-simulator SingleCellWithIMixedApps 5 1 0 15 15 15 15 1 1 30 0.1 242 1 > result/video15_cbr15_be15_voip15_242k.log
sleep 10s

echo "video400_242k"
./5G-air-simulator SingleCellWithIMixedApps 5 1 0 400 0 0 0 1 1 30 0.1 242 1 > result/video400_242k.log
sleep 10s

echo "video200_cbr200_242k"
./5G-air-simulator SingleCellWithIMixedApps 5 1 0 200 200 0 0 1 1 30 0.1 242 1 > result/video200_cbr200_242k.log
sleep 10s

echo "video100_cbr100_be100_voip100_242k"
./5G-air-simulator SingleCellWithIMixedApps 5 1 0 100 100 100 100 1 1 30 0.1 242 1 > result/video100_cbr100_be100_voip100_242k.log

echo "test end"