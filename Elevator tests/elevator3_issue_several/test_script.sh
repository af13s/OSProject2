#!/bin/bash

./producer.x

./consumer.x --start

for number in {1..10}
do
	cat /proc/elevator
	sleep 2 
done

./consumer.x --stop

cat /proc/elevator

./producer.x
./consumer.x --start

cat /proc/elevator



exit 0
