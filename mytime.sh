#!/bin/bash

repeat_count=100


total_time=0
average_time=0


for ((i=1; i<=repeat_count; i++))
do
    execution_time=$((time ./main >/dev/null) 2>&1 | grep "real" | awk '{print $2}' )
    execution_time="${execution_time:2}"
    execution_time="${execution_time%?}"
    echo "$execution_time"
    total_time=$(bc <<< "$total_time + $execution_time")
done


average_time=$(bc <<< "scale=3; $total_time / $repeat_count")


echo "total:$total_time"
echo "aveage:$average_time"
