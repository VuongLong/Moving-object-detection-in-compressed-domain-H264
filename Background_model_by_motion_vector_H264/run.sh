#!/bin/bash


./main_C1R input/CLH11_102_IOS.mov input/CLH11_102MV.txt input/CLH11_102BitSize.txt 

./main_C1R input/GVO1_3_IOS.mov input/GVO1_3MV.txt input/GVO1_3BitSize.txt

./main_C1R input/GVO1_222_IOS.mov input/GVO1_222MV.txt input/GVO1_222BitSize.txt 

./main_C1R input/ND.mov input/NgoaiDuongMV.txt input/NgoaiDuongBitSize.txt

./main_C1R input/CLH12_180_IOS.mov input/CLH12_180MV.txt input/CLH12_180BitSize.txt 62


./main_C1R input/GVO1_10_IOS.mov input/GVO1_10MV.txt input/GVO1_10BitSize.txt 69

dựa vào giá trị độ dài motion và bitsize quyết định update factor, và fade out factor trên pixel domain
 update factor riêng cho từng block

updatefactor là dự dơán vào tốc độ biến dđổi của background


fade out factor là sự biến mất của object sau khi suất hiện ở 1 block
object di nhanh thì sẽ biến mất nhanh (ôto)

binary pattern

Why MV?
- noise doesn't cause in MVF as bitsize

thanh hà hh1a 16 30

https://drive.google.com/drive/folders/1E-EgmWqOdLlAIBym5ed5oaT7MHZISyN-

ú

futari monologue
hmi307

PBAS: update learning rate by motion vector, if object appear and stop, PBAS may absort but check MV can slow leanring rate and make it still esixt