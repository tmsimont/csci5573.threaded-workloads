//-----------------------------------------------------------------------
//  Pthreads matrix multiply
//  This creates a stupid amount of threads to inefficiently 
//  perform matrix multiplication. The point of this is to 
//  create a workload with a large number of threads competing over
//  a shared resource
//-----------------------------------------------------------------------
//  Original code by: Gita Alaghband, Lan Vu 
//  Modified by Trevor Simonton, Anthony Pfaff, Cory Linfield, Alex Klein
//-----------------------------------------------------------------------
#include <iostream>
#include <iomanip>
#include <cmath>
#include <time.h>
#include <cstdlib>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>
#include <string>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define handle_error_en(en, msg) \
        do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define fpe(msg) fprintf(stderr, "\t%s", msg);

using namespace std;


// helper for wrapping threads
struct helper {
	pthread_t t;
	int idx;
};
helper **threads;

// matrix 
int n = 0;
float **a,**b,**c;

// locks/mutexes/semaphores
pthread_mutex_t** cmutexes;
sem_t** sems;
bool sequential = false;
int lockType = 0;
int*** spinlocks;
static inline void spinlock(int * a);
static inline void spinunlock(int * b);
int LOCK = 0;
int UNLOCK = 1;

// priority stuff
bool prioritySet = false;
sched_param pri;
int priority = 0;
int mpriority = 0;
pthread_attr_t attr;

// nice stuff
int niceness = 0;
int niceSet = false;
// policy
int policy = 2;

// debugging arg
bool debug = false;

// Timing stuff
timespec **times;
timespec **acquireTimes;
int timerType = 3;
bool enableTimer = true;

//-----------------------------------------------------------------------
//   Get user input for matrix dimension or printing option
//-----------------------------------------------------------------------
static void
usage(char *prog_name, string msg)
{
	if (msg.size() > 0)
		fputs(msg.c_str(), stderr);

	fprintf(stderr, "Usage: %s [options]\n", prog_name);
	fprintf(stderr, "Options are:\n");
	fpe("-n<size> Set matrix size\n");
	fpe("-p<prio> Set scheduling priority in\n");
	fpe("         thread attributes object\n");
	fpe("-i<nice> Set nice value\n");
	fpe("-m<prio> Set scheduling priority on\n");
	fpe("         main thread before pthread_create() call\n");
	fpe("-o<idx> Set scheduling policy:\n");
	fpe("           0 : SCHED_RR\n");
	fpe("           1 : SCHED_FIFO\n");
	fpe("           2 : SCHED_OTHER\n");
	fpe("-S       Don't use threads, perform sequentially \n");
	fpe("-D       Print debug info \n");
	fpe("-l<lock> Set locking mechanism used during matrix\n");
	fpe("         updates. Options are:\n");
	fpe("           0 : No lock\n");
	fpe("           1 : Mutex\n");
	fpe("           2 : Semaphore\n");
	fpe("           3 : Spinlock\n");
	fpe("-t<tmr>  Set the thread lock acquisition timer type\n");
	fpe("           0 : CLOCK_REALTIME \n");
	fpe("           1 : CLOCK_MONOTONIC \n");
	fpe("           2 : CLOCK_MONOTONIC_RAW \n");
	fpe("           3 : (DEFAULT) CLOCK_THREAD_CPUTIME_ID \n");
	fpe("-T       Disable timing output\n");
	exit(EXIT_FAILURE);
}

bool GetUserInput(int argc, char *argv[])
{
	int opt;
	// read program args
	while ((opt = getopt(argc, argv, "hDSn:i:l:p:m:t:To:")) != -1) {
		switch (opt) {
			case 'p': priority = atoi(optarg);
								prioritySet = true;						break;
		  case 'i': niceness = atoi(optarg);					
								niceSet = true;								break;
			case 'm': mpriority = atoi(optarg);     break;
			case 'o': policy = atoi(optarg);        break;
			case 'D': debug = true;								  break;
			case 'S': sequential = true;					  break;
			case 'l': lockType = atoi(optarg);		  break;
			case 'n': n = atoi(optarg);						  break;
			case 'h': usage(argv[0], "");					  break;
			case 't': timerType = atoi(optarg);			break;
			case 'T': enableTimer = false;					break;
			default:  usage(argv[0], "Unrecognized option\n");
		}
	}

	if (sequential && debug) {
		cout << "running sequentially" << endl;
	}

	// check matrix size
	if (n<=0) 
	{
		cout << "Matrix size must be larger than 0" <<endl;
		exit(EXIT_FAILURE);
	}

	// check policy
	if (policy > 2) {
		cout << "Schedule policy must be less than 3" <<endl;
		exit(EXIT_FAILURE);
	}

	if (prioritySet && mpriority < priority) {
		cout << "TODO: Is it OK to run main thread with lower priority than children???" << endl;
		exit(EXIT_FAILURE);
	}

	// check lock type
	if (lockType < 0 || lockType > 3) {
		cout << "Invalid lock type" <<endl;
		exit(EXIT_FAILURE);
	}

	// set priority
	if(prioritySet) {
		int s;
		struct sched_param const_pri;
		const_pri.sched_priority = priority;
		struct sched_param mconst_pri;
		mconst_pri.sched_priority	= mpriority;
		int lpolicy = SCHED_OTHER;
		switch(policy) {
			case 0:
				lpolicy = SCHED_RR;
				break;
			case 1:
				lpolicy = SCHED_FIFO;
				break;
			case 2:
				lpolicy = SCHED_OTHER;
				break;
		}

		// init attr
		s = pthread_attr_init(&attr);
		if (s != 0)
			handle_error_en(s,"pthread_attr_init");

		// set policy on attr (child threads)
		s = pthread_attr_setschedpolicy(&attr,lpolicy);
		if (s != 0)
			handle_error_en(s, "pthread_attr_setschedpolicy");

		// set priority on attr (child threads)
		s = pthread_attr_setschedparam(&attr, &const_pri);
		if (s != 0)
			handle_error_en(s, "pthread_attr_setschedparam");

		// set priority and policy on parent thread
		s = pthread_setschedparam(pthread_self(), lpolicy, &mconst_pri);
		if (s != 0)
			handle_error_en(s, "pthread_setschedparam");
	}
	return true;
}

/**
 * Difference between two timespec structs.
 *
 * Source: https://gist.github.com/diabloneo/9619917
 *
 * @param timespec* start
 * @param timespec* stop
 * @param timespec* result
 */
void timespec_diff(struct timespec *start, struct timespec *stop,
				   struct timespec *result)
{
	if ((stop->tv_nsec - start->tv_nsec) < 0) {
		result->tv_sec = stop->tv_sec - start->tv_sec - 1;
		result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
	} else {
		result->tv_sec = stop->tv_sec - start->tv_sec;
		result->tv_nsec = stop->tv_nsec - start->tv_nsec;
	}

	return;
}

void timerOutput(timespec *tspec)
{
		printf("%d.%09ld\t", tspec->tv_sec, tspec->tv_nsec);
}

void timerOutput(timespec **tspecs)
{
	long long msa = 0;
	long long sa = 0;
	long long maxt = 0;  
	int maxs = 0;
	int maxn = 0; 

	int N = n*n*n;
	for (int i = 0; i < N; i++) {
		time_t s;
		long ms;

		s = tspecs[i]->tv_sec;

		sa += tspecs[i]->tv_sec / N;
		
		// milliseconds
		// ms = round(times[i]->tv_nsec / 1.0e6);
		// printf("Elapsed: %d.%03ld seconds in #%ld.\n", s, ms, i);

		// microseconds
		// ms = round(times[i]->tv_nsec / 1.0e3);
		// printf("Elapsed: %d.%06ld seconds in #%ld.\n", s, ms, i);

		// nanoseconds
		ms = tspecs[i]->tv_nsec;

		long long t = s + ms * 1.0e9;
		if (t > maxt) {
			maxt = t;
			maxs = s;
			maxn = ms;
		}

		// running avg of nanoseconds
		msa += tspecs[i]->tv_nsec / N;

		//printf("Elapsed: %d.%09ld seconds in #%ld.", s, ms, i);
	}

	int sas = sa;
	float sasr = sa - sas;
	msa = msa + (sasr * 1.0e9);

	// avg
	printf("%d.%09ld\t", sas, msa);


	// max
	printf("%d.%09ld\t", maxs, maxn);
}

//-----------------------------------------------------------------------
//Initialize the value of matrix x[n x n]
//-----------------------------------------------------------------------
void InitializeMatrix(float** &x, float value)
{
	// allocate matrix
	x = new float*[n];
	x[0] = new float[n*n];
	for (int i = 1; i < n; i++)	x[i] = x[i-1] + n;

	// init matrix
	for (int i = 0 ; i < n ; i++)
		for (int j = 0 ; j < n ; j++)
			x[i][j] = value;

	// allocate spin locks
	spinlocks = new int**[n];
	spinlocks[0] = new int*[n*n];
	for (int i = 1; i < n; i++)	spinlocks[i] = spinlocks[i-1] + n;

	// init spin locks
	for (int i = 0 ; i < n; ++i)
		for (int j = 0 ; j < n; ++j)
			spinlocks[i][j] = &UNLOCK;

	// allocate mutex locks
	cmutexes = new pthread_mutex_t*[n];
	cmutexes[0] = new pthread_mutex_t[n*n];
	for (int i = 1; i < n; i++)	cmutexes[i] = cmutexes[i-1] + n;

	// init mutex locks
	for (int i = 0 ; i < n; ++i) {
		for (int j = 0 ; j < n; ++j) {
			pthread_mutex_init(&(cmutexes[i][j]), NULL);
		}
	}

	// allocate semaphores
	sems = new sem_t*[n];
	sems[0] = new sem_t[n*n];
	for (int i = 1; i < n; i++)	sems[i] = sems[i-1] + n;

	// init semaphores
	for (int i = 0 ; i < n; ++i) {
		for (int j = 0 ; j < n; ++j) {
			sem_init(&(sems[i][j]), 0, 1);
		}
	}

	// create helpers for threads
	threads = new helper*[n*n*n];
	for (int i = 0 ; i < n*n*n; ++i) {
		threads[i] = new helper();
		threads[i]->idx = i;
	}

	if (enableTimer) {
		// initialize timing structs
		times = new timespec *[n * n * n];
		for (int i = 0; i < n * n * n; i++) {
			times[i] = new timespec();
		}
		// initialize lock acquisition timing structs
		acquireTimes = new timespec *[n * n * n];
		for (int i = 0; i < n * n * n; i++) {
			acquireTimes[i] = new timespec();
		}
	}
}

//------------------------------------------------------------------
//Delete matrix x[n x n]
//------------------------------------------------------------------
void DeleteMatrix(float **x)
{
	delete[] x[0];
	delete[] x; 
}

//------------------------------------------------------------------
//Print matrix	
//------------------------------------------------------------------
void PrintMatrix(float **x) 
{
	for (int i = 0 ; i < n ; i++)
	{
		cout<< "Row " << (i+1) << ":\t" ;
		for (int j = 0 ; j < n ; j++)
		{
			printf("%.2f\t", x[i][j]);
		}
		cout<<endl ;
	}
}


void recordAcquireTime(timespec *start, int timesIndex) {
	if (enableTimer) {
		timespec acquisition;
		switch (timerType) {
			case 0:
				clock_gettime(CLOCK_REALTIME, &acquisition);
				break;
			case 1:
				clock_gettime(CLOCK_MONOTONIC, &acquisition);
				break;
			case 2:
				clock_gettime(CLOCK_MONOTONIC_RAW, &acquisition);
				break;
			case 3:
				clock_gettime(CLOCK_THREAD_CPUTIME_ID, &acquisition);
				break;
		}
		timespec_diff(start, &acquisition, acquireTimes[timesIndex]);
	}
}

//------------------------------------------------------------------
//Do Matrix Multiplication 
//------------------------------------------------------------------

// individual result matrix cell thread callback
void* row_col_sum(void* idp) {
	timespec start, end;

	int id = *(int*)idp;
	int timesIndex = id;
	int k = id % n;
	id = id/n;
	int j = id % n;
	id = id/n;
	int i = id % n;

	if (enableTimer) {
		switch (timerType) {
			case 0:
				clock_gettime(CLOCK_REALTIME, &start);
				break;
			case 1:
				clock_gettime(CLOCK_MONOTONIC, &start);
				break;
			case 2:
				clock_gettime(CLOCK_MONOTONIC_RAW, &start);
				break;
			case 3:
				clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
				break;
		}
	}

	switch (lockType) {
		// mutex
		case 1:
			pthread_mutex_lock (&(cmutexes[i][j]));
			recordAcquireTime(&start, timesIndex);
			c[i][j] += a[i][k]*b[k][j];
			pthread_mutex_unlock (&(cmutexes[i][j]));
			break;

		// semaphore
		case 2:
			sem_wait (&(sems[i][j]));
			recordAcquireTime(&start, timesIndex);
			c[i][j] += a[i][k]*b[k][j];
			sem_post (&(sems[i][j]));
			break;

		// spin lock
		case 3:
			spinlock (spinlocks[i][j]);
			recordAcquireTime(&start, timesIndex);
			c[i][j] += a[i][k]*b[k][j];
			spinunlock (spinlocks[i][j]);
			break;

		// no lock
		case 4:
		default:
			recordAcquireTime(&start, timesIndex);
			c[i][j] += a[i][k]*b[k][j];
			break;
	}

	if (enableTimer) {
		switch (timerType) {
			case 0:
				clock_gettime(CLOCK_REALTIME, &end);
				break;
			case 1:
				clock_gettime(CLOCK_MONOTONIC, &end);
				break;
			case 2:
				clock_gettime(CLOCK_MONOTONIC_RAW, &end);
				break;
			case 3:
				clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
				break;
		}

		timespec_diff(&start, &end, times[timesIndex]);
	}

	if (!sequential)
		pthread_exit(NULL);
}

void printPriInfo()
{
	int pol;
	cout << "sched: " <<  pthread_getschedparam(pthread_self(), &pol, &pri) << endl;
	cout << "Attr pri: " << pri.sched_priority << endl;
	cout << "Attr pol: " << pol << ((pol==SCHED_RR)?"SCHED_RR":"") << endl;
	switch(pol)
	{
		case SCHED_RR:
			cout << "RR" << endl;
			break;
		case SCHED_OTHER:
			cout << "OTHER" << endl;
			break;
		case SCHED_FIFO:
			cout << "FIFO" << endl;
			break;
		case SCHED_IDLE:
			cout << "IDLE" << endl;
			break;
		case SCHED_BATCH:
			cout << "BATCH" << endl;
			break;
	}
}

static inline void spinlock(int * lock)
{
	__asm__ __volatile__(
	"1: \n\t"
	"PUSH {r0-r2}\n\t"
	"LDR r0, %[lock]\n\t"
	"LDR r2, =0x12345678\n\t"
	"SWP r1, r2, [r0]\n\t"
	"CMP r1,r2\n\t"
	"BEQ 1b\n\t"
	"POP {r0-r2}\n\t"
	//"BX lr\n\t"
	//"ENDP\n\t"
	:
	: [lock] "m" (lock)
	: );
}

static inline void spinunlock(int * lock)
{
	__asm__ __volatile__(
  "PUSH {r0-r1}\n\t"	
	"LDR r1, =0x87654321\n\t"
  "LDR r0, %[lock]\n\t"
	"STR r1, [r0]\n\t"
	"POP {r0-r1}\n\t"
	//"BX lr\n\t"
	//"ENDP\n\t"
	:
	: [lock] "m" (lock)
	:	);
}

void MultiplyMatrix()
{
	int s;
	for (int i = 0 ; i < n * n * n; i++) {
		if (sequential) {
			row_col_sum(&(threads[i]->idx));
		}
		else {
		}
			if(prioritySet) {
				s = pthread_create(&(threads[i]->t), &attr, row_col_sum, (void *) &(threads[i]->idx));
				if (s != 0)
					handle_error_en(s,"pthread_create1");
			}
			else {
				if(niceSet)
				{
					nice(niceness);
					if (debug)
						cout <<"Nice"<<getpriority(PRIO_PROCESS,0)<<endl;
				}
				s = pthread_create(&(threads[i]->t), NULL, row_col_sum, (void *) &(threads[i]->idx));
				if (s != 0)
					handle_error_en(s,"pthread_create2");
		}
	}
}

//------------------------------------------------------------------
// Main Program
//------------------------------------------------------------------
int main(int argc, char *argv[])
{
	timespec start, end, runtime;

	if (GetUserInput(argc,argv)==false) return 1;

	//Initialize the value of matrix a, b, c
	InitializeMatrix(a,9.0);
	InitializeMatrix(b,9.0);
	InitializeMatrix(c,0.0);

	clock_gettime(CLOCK_MONOTONIC, &start);
	MultiplyMatrix();
	void *status;


	if (!sequential)
		for(int t=0; t<n*n*n; t++) {
			if(debug && prioritySet)
				printPriInfo();

			int s = pthread_join(threads[t]->t, &status);
			if (s != 0)
				handle_error_en(s, "pthread_join");
		}

	clock_gettime(CLOCK_MONOTONIC, &end);
	timespec_diff(&start, &end, &runtime);

	// check for errors in result
	bool ok = true;
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < n; ++j) {
			if (c[i][j] != c[0][0]) ok = false;
		}
	}

	/*
	if (ok) 
		cout << "ok\t";
	else
		cout << "wrong\t";
	*/

	cout << n << "\t";

	if (prioritySet) {
		cout << priority << "\t";
		cout << mpriority << "\t";
	}
	else {
		cout << "-\t-\t";
	}

	if (niceSet) {
		cout << niceness << "\t";
	}
	else {
		cout << "-\t";
	}

	switch(policy) {
		case 0:
			cout << "SCHED_RR\t";
			break;
		case 1:
			cout << "SCHED_FIFO\t";
			break;
		case 2:
			cout << "SCHED_OTHER\t";
			break;
	}

	if (sequential)
		cout << "1\t";
	else
		cout << "0\t";


	switch (lockType) {
		// mutex
		case 1:
			cout << "MUTEX\t";
			break;

		// semaphore
		case 2:
			cout << "SEMAPHORE\t";
			break;

		// spin lock
		case 3:
			cout << "SPIN\t";
			break;

		// no lock
		case 4:
		default:
			cout << "NONE\t";
			break;
	}


	

	if (enableTimer) {
		timerOutput(&runtime);
		// timerOutput(times);
		timerOutput(acquireTimes);
	}

	cout << endl;

	DeleteMatrix(a);	
	DeleteMatrix(b);	
	DeleteMatrix(c);	
	pthread_exit(NULL);
	return 0;
}
