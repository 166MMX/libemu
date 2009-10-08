/* $Id$ */

#include <stdio.h>
#include <stdlib.h>
#if !defined(_MSC_VER)
	#include <sys/time.h>
#endif /* _MSC_VER */
#if defined(WIN32)
	#define _WIN32_WINNT 0x0500
	#include <windows.h>
#else
	#define __USE_POSIX
	#include <signal.h>
#endif /* WIN32 */
#include "types.h"
#include "pic.h"

typedef struct PICNode {
	uint32 usec_left;
	uint32 usec_delay;
	void (*callback)();
} PICNode;

#if defined(WIN32)
static HANDLE _timer_main   = NULL;
static HANDLE _timer_handle = NULL;
static int _timer;
#else
static struct itimerval _timer;
#endif /* WIN32 */
static PICNode *_pic_nodes = NULL;
static int _pic_node_count = 0;
static int _pic_node_size  = 0;
static uint32 _pic_last_sec;
static uint32 _pic_last_usec;
const uint32 _pic_speed    = 20000; /* Our PIC runs at 50Hz */

void _pic_run()
{
	PICNode *node;
	uint32 new_sec, new_usec, usec_delta, delta;
	int i;

	/* Lock the PIC, to avoid double-calls */
	static uint8 pic_running = 0;
	if (pic_running == 1) return;
	pic_running = 1;

	/* Calculate the time between calls */
	new_sec    = pic_get_sec();
	new_usec   = pic_get_usec();
	usec_delta = (new_sec - _pic_last_sec) * 1000000 + (new_usec - _pic_last_usec);
	_pic_last_sec     = new_sec;
	_pic_last_usec    = new_usec;

	/* Walk all our timers, see which (and how often) it should be triggered */
	node = _pic_nodes;
	for (i = 0; i < _pic_node_count; i++, node++) {
		delta = usec_delta;

		/* No delay means: as often as possible, but don't worry about it */
		if (node->usec_delay == 0) {
			node->callback();
			continue;
		}

		while (node->usec_left < delta) {
			delta -= node->usec_left;
			node->usec_left = node->usec_delay;
			node->callback();
		}
		node->usec_left -= delta;
	}

	pic_running = 0;
}

#if defined(WIN32)
void CALLBACK pic_windows_tick(LPVOID arg, BOOLEAN TimerOrWaitFired) {
	SuspendThread(_timer_main);
	_pic_run();
	ResumeThread(_timer_main);
}
#endif /* WIN32 */

void pic_init()
{
	_pic_last_sec  = pic_get_sec();
	_pic_last_usec = pic_get_usec();

#if defined(WIN32)
	_timer = _pic_speed / 1000;
	DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &_timer_main, 0, FALSE, DUPLICATE_SAME_ACCESS);
#else
	_timer.it_value.tv_sec = 0;
	_timer.it_value.tv_usec = _pic_speed;
	_timer.it_interval.tv_sec = 0;
	_timer.it_interval.tv_usec = _pic_speed;

	{
		struct sigaction pic_sa;

		sigemptyset(&pic_sa.sa_mask);
		pic_sa.sa_handler = _pic_run;
		pic_sa.sa_flags   = 0;
		sigaction(SIGALRM, &pic_sa, NULL);
	}
#endif /* WIN32 */
	pic_resume();
}

void pic_uninit()
{
	pic_suspend();
#if defined(WIN32)
	CloseHandle(_timer_main);
#endif
}

void pic_timer_add(void (*callback)(), uint32 usec_delay)
{
	PICNode *node;
	if (_pic_node_count == _pic_node_size) {
		_pic_node_size += 2;
		_pic_nodes = (PICNode *)realloc(_pic_nodes, _pic_node_size * sizeof(PICNode));
	}
	node = &_pic_nodes[_pic_node_count++];

	node->usec_left  = usec_delay;
	node->usec_delay = usec_delay;
	node->callback   = callback;
}

void pic_timer_change(void (*callback)(), uint32 usec_delay)
{
	int i;
	PICNode *node = _pic_nodes;
	for (i = 0; i < _pic_node_count; i++, node++) {
		if (node->callback == callback) {
			node->usec_delay = usec_delay;
			return;
		}
	}
}

void pic_timer_del(void (*callback)())
{
	int i;
	PICNode *node = _pic_nodes;
	for (i = 0; i < _pic_node_count; i++, node++) {
		if (node->callback == callback) {
			*node = _pic_nodes[--_pic_node_count];
			return;
		}
	}
}

void pic_suspend()
{
#if defined(WIN32)
	DeleteTimerQueueTimer(NULL, _timer_handle, NULL);
#else
	setitimer(ITIMER_REAL, NULL, NULL);
#endif /* WIN32 */
}

void pic_resume()
{
#if defined(WIN32)
	CreateTimerQueueTimer(&_timer_handle, NULL, pic_windows_tick, NULL, _timer, _timer, WT_EXECUTEINTIMERTHREAD);
#else
	setitimer(ITIMER_REAL, &_timer, NULL);
#endif /* WIN32 */
}

uint32 pic_get_sec()
{
#if defined(_MSC_VER)
	DWORD t;
	t = timeGetTime();
	return t / 1000;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec;
#endif /* _MSC_VER */
}

uint32 pic_get_usec()
{
#if defined(_MSC_VER)
	DWORD t;
	t = timeGetTime();
	return (t % 1000) * 1000;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_usec;
#endif /* _MSC_VER */
}
