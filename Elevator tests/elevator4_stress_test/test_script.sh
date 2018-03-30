#!/bin/bash

./producer.x
./consumer.x --start

for number in {1..10}
do
	cat /proc/elevator >> lastrun.txt
	cat /proc/elevator
	sleep 6
done

./consumer.x --stop
cat /proc/elevator


exit 0
