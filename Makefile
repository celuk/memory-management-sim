sim:
	rm -f scheduler
	rm -f user_process
	gcc scheduler.c -o scheduler -lpthread
	gcc user_process.c -o user_process