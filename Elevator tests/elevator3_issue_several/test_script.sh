#!/bin/bash

for number in {1..2}
do
	./producer.x
done

./consumer.x --start

for number in {1..10000}
do
	cat /proc/elevator >> logfile
	cat /proc/elevator
done
exit 0
