#!/bin/bash

for number in {1..25}
do
	./producer.x
	sleep 1
done

./consumer.x --start

for number in {1..25}
do
	
	sleep 1
	cat /proc/elevator
	echo
done

./consumer.x --stop

exit 0
