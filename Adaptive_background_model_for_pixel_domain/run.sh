#!/bin/bash


./main_C1R input/CLH11_56_IOS.mov input/CLH11_56MV.txt input/CLH11_56BitSize.txt
./main_C1R input/CLH11_102_IOS.mov input/CLH11_102MV.txt input/CLH11_102BitSize.txt 

./main_C1R input1/GVO1_3_IOS.mov input1/GVO1_3MV.txt input1/GVO1_3BitSize.txt
./main_C1R input1/CLH11_56_IOS.mov input1/CLH11_56MV.txt input1/CLH11_56BitSize.txt 58


./main_C1R input/GVO2_100_IOS.mov input/GVO2_100MV.txt input/GVO2_100BitSize.txt 57

./demo ../vibe/input/GVO1_10_IOS.mov output/ROI.png output/


./main_C1R input/GVO1_222_IOS.mov input/GVO1_222MV.txt input/GVO1_222BitSize.txt 

./main_C1R input/GVO2_141_IOS.mov input/GVO2_141MV.txt input/GVO2_141BitSize.txt

./main_C1R input/ND.mov input/NgoaiDuongMV.txt input/NgoaiDuongBitSize.txt


./main_C1R input/CLH12_2_IOS.mov input/CLH12_2MV.txt input/CLH12_2BitSize.txt 62


./main_C1R input/GVO1_10_IOS.mov input/GVO1_10MV.txt input/GVO1_10BitSize.txt 69

dựa vào giá trị độ dài motion quyết định update factor, và fade out factor
update factor riêng cho từng block

updatefactor là dự dơán vào tốc độ biến dđổi của background


fade out factor là sự biến mất của object sau khi suất hiện ở 1 block
object di nhanh thì sẽ biến mất nhanh (ôto)

binary pattern

Why MV?
- noise doesn't cause in MVF as bitsize

thanh hà hh1a 16 30