#!/bin/bash

./producer.x
./consumer.x --start

for number in {1..10}
do
	cat /proc/elevator
	sleep 6
done

cat /proc/elevator

./consumer.x --stop

cat /proc/elevator

exit 0
