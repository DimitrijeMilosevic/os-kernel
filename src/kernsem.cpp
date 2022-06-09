#include "kernsem.h"
#include "pcb.h"
#include "schedule.h"

extern volatile unsigned int lockFlag;
extern volatile unsigned int context_switch_on_demand;

List<KernelSem*>* KernelSem::allSemaphores = new List<KernelSem*>;

KernelSem::KernelSem(Semaphore* mySem, int init) : mySemaphore(mySem), semaphoreValue(init) {
	blockedThreads = new List<PCB*>();
	sleptThreads = new List<PCB*>();
	KernelSem::allSemaphores->add(this);
}

KernelSem::~KernelSem() {
	// If, at the time the destructor is being called, there are some threads blocked on this semaphore, they need to be deblocked;
	// otherwise, they'd get blocked forever
	if (semaphoreValue < 0) {
		for (int i = 0; i < blockedThreads->numberOfElements(); i++)
			deblock();
		for (int j = 0; j < sleptThreads->numberOfElements(); j++)
			wakeUp();
	}
	delete blockedThreads;
	delete sleptThreads;
	KernelSem::allSemaphores->remove(this);
}

void KernelSem::block() {
	PCB::running->threadState = PCB::THREAD_BLOCKED;
	blockedThreads->add((PCB*)PCB::running);
	context_switch_on_demand = 1;
}

void KernelSem::deblock() {
	PCB* deblockedThread = blockedThreads->removeFirst();
	deblockedThread->threadState = PCB::THREAD_READY;
	Scheduler::put(deblockedThread);
}

void KernelSem::sleep() {
	PCB::running->threadState = PCB::THREAD_BLOCKED;
	sleptThreads->add((PCB*)PCB::running);
	context_switch_on_demand = 1;
}

void KernelSem::wakeUp() {
	PCB* wokenUpThread = sleptThreads->removeFirst();
	wokenUpThread->threadState = PCB::THREAD_READY;
	// If the thread got woken up, that means that it DID NOT got deblocked by its maxTimeToWait running out, so its maxTimeToWait field needs to be updated to -1
	wokenUpThread->maxTimeToWait = -1;
	Scheduler::put(wokenUpThread);
}

int KernelSem::wait(Time maxTimeToWait) {
	THREADSAFE_LOCK
	semaphoreValue--;
	if (semaphoreValue < 0) {
		if (maxTimeToWait != 0) {
			PCB::running->maxTimeToWait = maxTimeToWait;
			sleep();
		}
		else
			block();
	}
	THREADSAFE_UNLOCK
	/*
	When thread's run method gets to this point it means that the thread has been deblocked. Depending on the value of the maxTimeToWait field of that thread is the return value of the
	wait method:
	- maxTimeToWait = -1: Thread got deblocked by some other thread signalling on this semaphore, so the return value is 1
	- maxTimeToWait = 0: Thread got deblocked by its maxTimeToWait running out, so the return value is 0
	Note: in the second case, maxTimeToWait field needs to be updated to -1
	*/
	if (PCB::running->maxTimeToWait == -1) return 1;
	if (PCB::running->maxTimeToWait == 0) { PCB::running->maxTimeToWait = -1; return 0; }
	// For warning purposes
	return 0;
}

int KernelSem::signal(int n) {
	if (n < 0) return n;
	if (n == 0) {
	if (semaphoreValue < 0) {
		if (blockedThreads->numberOfElements() > 0) deblock();
		else wakeUp();
	}
		semaphoreValue++;
		return 0;
	}
	if (n > 0) {
		int numberOfDeblockedThreads = 0;
		for (int i = 0; i < n; i++) {
			if (semaphoreValue < 0) {
				if (blockedThreads->numberOfElements() > 0) deblock();
				else wakeUp();
				numberOfDeblockedThreads++;
			}
			semaphoreValue++;
		}
		return numberOfDeblockedThreads;
	}
	// For warning purposes
	return 0;
}

int KernelSem::val() const { return semaphoreValue; }

void KernelSem::updateTimeBlocked() {
	List<KernelSem*>::Node* currentSemaphore = KernelSem::allSemaphores->first;
		while (currentSemaphore != 0) {
			List<PCB*>::Node *currentPCB = currentSemaphore->data->sleptThreads->first, *previousPCB = 0;
			while (currentPCB != 0) {
				currentPCB->data->maxTimeToWait--;
				if (currentPCB->data->maxTimeToWait == 0) {
					List<PCB*>::Node* oldPCB = currentPCB;
					if (previousPCB == 0) {
						currentSemaphore->data->sleptThreads->first = currentSemaphore->data->sleptThreads->first->next;
						if (currentSemaphore->data->sleptThreads->first == 0)
							currentSemaphore->data->sleptThreads->last = 0;
					}
					else {
						previousPCB->next = currentPCB->next;
						if (currentPCB->next == 0)
							currentSemaphore->data->sleptThreads->last = previousPCB;
					}
					currentPCB = currentPCB->next;
					oldPCB->data->threadState = PCB::THREAD_READY;
					Scheduler::put(oldPCB->data);
					delete oldPCB;
					currentSemaphore->data->semaphoreValue++;
				}
				else {
					previousPCB = currentPCB;
					currentPCB = currentPCB->next;
				}
			}
		currentSemaphore = currentSemaphore->next;
	}
}
