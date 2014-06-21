#include "SignaledEvent.h"
#include "RakAssert.h"
#include "RakSleep.h"

#if defined(__GNUC__)
#include <sys/time.h>
#include <unistd.h>
#endif

SignaledEvent::SignaledEvent()
{
	isSignaled=false;
}
SignaledEvent::~SignaledEvent()
{
	// Intentionally do not close event, so it doesn't close twice on linux
}

void SignaledEvent::InitEvent(void)
{
	pthread_condattr_init( &condAttr );
	pthread_cond_init(&eventList, &condAttr);
	pthread_mutexattr_init( &mutexAttr	);
	pthread_mutex_init(&hMutex, &mutexAttr);
}

void SignaledEvent::CloseEvent(void)
{
	pthread_cond_destroy(&eventList);
	pthread_mutex_destroy(&hMutex);
	pthread_condattr_destroy( &condAttr );
}

void SignaledEvent::SetEvent(void)
{
	// Different from SetEvent which stays signaled.
	// We have to record manually that the event was signaled
	isSignaledMutex.Lock();
	isSignaled=true;
	isSignaledMutex.Unlock();

	// Unblock waiting threads
	pthread_cond_broadcast(&eventList);
}

void SignaledEvent::WaitOnEvent(int timeoutMs)
{
	// If was previously set signaled, just unset and return
	isSignaledMutex.Lock();
	if (isSignaled==true)
	{
		isSignaled=false;
		isSignaledMutex.Unlock();
		return;
	}
	isSignaledMutex.Unlock();


	struct timespec ts;
	struct timeval  tp;
	ts.tv_sec  = tp.tv_sec;
	ts.tv_nsec = tp.tv_usec * 1000;

	while (timeoutMs > 30)
	{
		// Wait 30 milliseconds for the signal, then check again.
		// This is in case we  missed the signal between the top of this function and pthread_cond_timedwait, or after the end of the loop and pthread_cond_timedwait
		ts.tv_nsec += 30*1000000;
		if (ts.tv_nsec >= 1000000000)
		{
		        ts.tv_nsec -= 1000000000;
		        ts.tv_sec++;
		}
		pthread_cond_timedwait(&eventList, &hMutex, &ts);
		timeoutMs-=30;

		isSignaledMutex.Lock();
		if (isSignaled==true)
		{
			isSignaled=false;
			isSignaledMutex.Unlock();
			return;
		}
		isSignaledMutex.Unlock();
	}

	// Wait the remaining time, and turn off the signal in case it was set
	ts.tv_nsec += timeoutMs*1000000;
	if (ts.tv_nsec >= 1000000000)
	{
	        ts.tv_nsec -= 1000000000;
	        ts.tv_sec++;
	}
	pthread_cond_timedwait(&eventList, &hMutex, &ts);

	isSignaledMutex.Lock();
	isSignaled=false;
	isSignaledMutex.Unlock();
}
