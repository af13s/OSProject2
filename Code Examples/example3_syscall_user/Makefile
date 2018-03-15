.PHONY: compile run clean

compile: main.c
	gcc -o main.x main.c

run1: compile
	./main.x 1
run2: compile
	./main.x 2
run3: compile
	./main.x 3

clean:
	rm *.x
