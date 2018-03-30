#!/bin/bash

for number in {1..30}
do
	./producer.x
	echo
	sleep .5
done
for number in {1..25}
do
	./consumer.x --start
	echo
	sleep .1
	cat /proc/elevator
	echo
	sleep .1
done

for number in {1..25}
do
	./consumer.x --stop
	sleep .1
	echo STOPING!
	cat /proc/elevator
	sleep .1
	echo
done
exit 0
