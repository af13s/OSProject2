#!/bin/bash

for number in {1..10}
do
	./producer.x
	sleep 1
done
for number in {1..25}
do
	./consumer.x --start
	sleep .5
	cat /proc/elevator
	sleep .5
done
exit 0
