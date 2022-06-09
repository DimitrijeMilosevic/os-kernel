#ifndef _kernsem_h_
#define _kernsem_h_

#include "list.h"
#include "semaphor.h"

class KernelSem {
public:

	KernelSem(Semaphore* mySem, int init);
	~KernelSem();
	/*
	wait method takes an unsigned variable as a parameter and returns int value:
	* maxTimeToWait parameter means the maximum amount of time a thread can be blocked on this semaphore. That means the thread can either get deblocked by some other thread doing signal on this
	semaphore, or get automatically deblocked once its time runs out (value 0 means unlimited time for the thread to be blocked)
	* return value will indicate how did the thread get deblocked (if it did get blocked):
	  - if the thread got deblocked by its time running out - return value is 0
	  - return value is 1 in any other case
	*/
	int wait(Time maxTimeToWait);
	/*
	signal method takes an int variable as parameter and also returns int value:
	* n parameter:
	  - <0: invalid input - return n
	  - =0: signal method acts 'normally'
	  - >0: signal will deblock up to n threads; if there are not that much threads to be deblocked, it will deblock all of the blocked threads
	  Note: semaphoreValue will be added by n no matter what
	* return value can be:
	  - <0: if the n parameter was <0
	  - the amount of threads deblocked on this semaphore by the signal method, in any other case
	*/
	int signal(int n);

	int val() const;

	// updateTimeBlocked static method will be used inside of the timer interrupt routine in order to update the time for all of the threads 'slept' on the semaphores
	/*
	This static method goes through allSemaphores list, then through each individual blockedThreads list and decrements maxTimeToWait fields of all of the 'slept' threads;
	if the maxTimeToWait field reaches 0, this method also deblocked that thread
	*/
	static void updateTimeBlocked();

	friend void initialize();
	friend void restore();
protected:

	void block();
	void deblock();

	void sleep();
	void wakeUp();

private:
	// allSemaphores list will store all of the Semaphore (KernelSem) objects ever created, and it will be used for the purpose of static method updateTimeBlocked
	static List<KernelSem*>* allSemaphores;
	// Each KernelSem object is realted to one Semaphore object, and no two KernelSem objects can be related to the same Semaphore object; same goes vice-versa
	Semaphore* mySemaphore;
	// blockedThreads list will store all of the Threads blocked (who called wait(0)) on this semaphore
	List<PCB*>* blockedThreads;
	// sleptThreads list will store all of the Threads blocked (who called wait(!=0)) on this semaphore
	List<PCB*>* sleptThreads;
	/* semaphoreValue field has different meanings, depending on its value:
	   	   >0: there can be semaphoreValue threads who call wait on this semaphore and NOT get blocked on it
	   	   =0: there are no blocked threads on this semaphore, but the next thread who calls wait on this semaphore WILL get blocked
	   	   <0: there are |semaphoreValue| threads currently blocked on this semaphore; as long as the semaphoreValue field stays negative, every other thread who tries
	   	   calling wait on this semaphore will also get blocked
	*/
	int semaphoreValue;

};

#endif
