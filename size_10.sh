#!/bin/bash

agents_num=2
size=10
timelimit=2048
while [ $agents_num -lt 32 ]
do
    if [ $agents_num -gt 128 ]
    then
        timelimit=4096
    fi
    agents_num=`expr $agents_num \* 2`
    for iter in {0..99}
    do
        ./cbs -m data/map/${size}.map -a data/scen/${size}/${agents_num}_agents_${size}_size_0_density_id_${iter}.scen -o outputs/stats/${agents_num}_agents_${size}.csv --outputPaths=outputs/paths/${size}/${agents_num}_agents_${size}_size_0_density_id_${iter}.txt --outputSteps=outputs/steps/step_${agents_num}_agents_${size}.csv -t $timelimit
    done
done