#!/bin/sh

#1
cd ./Common
make clean && make
#2
cd ../
cd ./DetectControl
make clean && make
#3
cd ../
cd ./TimingControl
make clean && make
#4
cd ../
cd ./SameControl
make clean && make
#5
cd ../
cd ./MajorHalfDetect
make clean && make
#6
cd ../
cd ./PersonPassStreet
make clean && make
#7
cd ../
cd ./MinorHalfDetect
make clean && make
#8
cd ../
cd ./BusPriority
make clean && make
#9
cd ../
cd ./SelfAdaptControl
make clean && make
#10
cd ../
cd ./GreenTrigger
make clean && make
#11
cd ../
cd ./YellowFlashTrigger
make clean && make
#12
cd ../
cd ./SysOptimizeAdapt
make clean && make

cd ../
cd ./SysOptimizeCoordinate
make clean && make

cd ../
cd ./SysOptimizeTiming
make clean && make

cd ../
cd ./CoordingDetect
make clean && make

cd ../
cd ./RailCrossControl
make clean && make

cd ../
cd ./OverLoadControl
make clean && make

cd ../
cd ./BrainSelfAdapt
make clean && make

cd ../
cd ./MajorSalveControl
make clean && make

cd ../
cd ./CycleRoadControl
make clean && make

cd ../
cd ./InterGratedControl
make clean && make

cd ../
cd ./MainCont
make clean && make

cd ../
