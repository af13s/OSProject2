#include <linux/unistd.h>
#include <stdio.h>
#include <time.h>
#include<fcntl.h>

int main(){

struct timespec time;

clock_gettime(CLOCK_MONOTONIC, &time);
printf("Hello World");
open("dummy_file" , O_WRONLY|O_CREAT|O_TRUNC);
remove("dummy_file");
open("dummy_file" , O_WRONLY|O_CREAT|O_TRUNC);
remove("dummy_file");


return 0;
}
