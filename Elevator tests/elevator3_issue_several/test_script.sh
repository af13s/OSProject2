#!/bin/bash

for number in {1..10}
do
	./producer.x
done
for number in {1..1000}
do
	./consumer.x --start
	cat /proc/elevator
done
exit 0
