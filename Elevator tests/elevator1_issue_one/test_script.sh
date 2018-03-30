#!/bin/bash

for number in {1..15}
do
	./producer.x
	sleep 1
done

./consumer.x --start

for number in {1..4}
do
	
	sleep .1
	cat /proc/elevator
	echo
done

for number in {1..10}
do
	./consumer.x --stop
	echo STOPPING!
	cat /proc/elevator
	echo
done


exit 0
