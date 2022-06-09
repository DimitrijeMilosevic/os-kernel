#include "ivtentry.h"

KernelEv* IVTEntry::allEvents[256] = { 0 };
pInterrupt IVTEntry::allRoutines[256] = { 0 };

IVTEntry::IVTEntry(IVTNo ivtEntry, pInterrupt newRoutine) : ivtNo(ivtEntry) {
	IVTEntry::allRoutines[ivtNo] = newRoutine;
}

IVTEntry::~IVTEntry() { IVTEntry::allRoutines[ivtNo] = 0; }

void IVTEntry::callOldRoutine() { IVTEntry::allEvents[ivtNo]->oldRoutine(); }

void IVTEntry::signal() { IVTEntry::allEvents[ivtNo]->signal(); }
