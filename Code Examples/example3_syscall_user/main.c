#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#define __NR_TEST_CALL 333

int test_call(int test) {
	return syscall(__NR_TEST_CALL, test);
}

int main(int argc, char **argv) {
	if (argc != 2) {
		printf("wrong number of args\n");
		return -1;
	}
	
	int test = atoi(argv[1]);
	long ret = test_call(test);

	if (ret < 0)
		perror("system call error");
	else
		printf("Function successful. passed in: %d, returned %ld\n", test, ret);
	
	return 0;
}
