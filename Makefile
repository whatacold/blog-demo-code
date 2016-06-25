all:
	gcc -Wall -Werror -lpthread queue_with_condvar.c -o /dev/shm/queue_with_condvar -g
