#!/bin/bash

for number in {1..30}
do
	./producer.x
	echo
	sleep 1
done
for number in {1..25}
do
	./consumer.x --start
done

cat /proc/elevator
sleep 10
echo sleeping

for number in {1..25}
do
	./consumer.x --stop
done

cat /proc/elevator

exit 0
