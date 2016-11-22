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
bool useLock = true;
int lockType = 0;

//-----------------------------------------------------------------------
//   Get user input for matrix dimension or printing option
//-----------------------------------------------------------------------
static void
usage(char *prog_name, char *msg)
{
	if (msg != NULL)
		fputs(msg, stderr);

	fprintf(stderr, "Usage: %s [options]\n", prog_name);
	fprintf(stderr, "Options are:\n");
	fpe("-n<size> Set matrix size\n");
	fpe("-p<prio> Set scheduling priority in\n");
	fpe("                 thread attributes object\n");
	fpe("-m<prio> Set scheduling policy and priority on\n");
	fpe("                 main thread before pthread_create() call\n");
	fpe("-S				Don't use threads, perform sequentially \n");
	fpe("-l<lock> Set locking mechanism used during matrix\n");
	fpe("                 updates. Options are:\n");
	fpe("										1 : Mutex\n");
	fpe("										2 : Semaphore\n");
	fpe("										3 : Spinlock\n");
	fpe("										4 : No lock\n");
	exit(EXIT_FAILURE);
}

bool GetUserInput(int argc, char *argv[])
{

	while ((opt = getopt(argc, argv, "n:p:Sl:m:")) != -1) {
		switch (opt) {
			case 'p': attr_sched_str = optarg;      break;
			case 'm': main_sched_str = optarg;      break;
			case 'S': useLock = false;						  break;
			case 'l': lockType = atoi(optarg);		  break;
			case 'n': n = atoi(optarg);						  break;
			default:  usage(argv[0], "Unrecognized option\n");
		}
	}
	if (n<=0) 
	{
		cout << "Matrix size must be larger than 0" <<endl;
		exit(EXIT_FAILURE);
	}
	if (lockType <= 0 || lockType > 4) {
		cout << "Invalid lock type" <<endl;
		exit(EXIT_FAILURE);
	}
	return true;
}

//-----------------------------------------------------------------------
//Initialize the value of matrix x[n x n]
//-----------------------------------------------------------------------
void InitializeMatrix(float** &x,float value)
{
	x = new float*[n];
	x[0] = new float[n*n];

	for (int i = 1; i < n; i++)	x[i] = x[i-1] + n;

	for (int i = 0 ; i < n ; i++)
	{
		for (int j = 0 ; j < n ; j++)
		{
			x[i][j] = value;
		}
	}

	cmutexes = new pthread_mutex_t*[n];
	cmutexes[0] = new pthread_mutex_t[n*n];
	for (int i = 1; i < n; i++)	cmutexes[i] = cmutexes[i-1] + n;

	// init locks
	for (int i = 0 ; i < n; ++i) {
		for (int j = 0 ; j < n; ++j) {
			pthread_mutex_init(&(cmutexes[i][j]), NULL);
		}
	}

	// create helpers for threads
	threads = new helper*[n*n*n];
	for (int i = 0 ; i < n*n*n; ++i) {
		threads[i] = new helper();
		threads[i]->idx = i;
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
//------------------------------------------------------------------
//Do Matrix Multiplication 
//------------------------------------------------------------------

// individual result matrix cell thread callback
void* row_col_sum(void* idp) {
	int id = *(int*)idp;
	int k = id % n; 
	id = id/n;
	int j = id % n;
	id = id/n;
	int i = id % n;
	if (locked == 1) {
		pthread_mutex_lock (&(cmutexes[i][j]));
		c[i][j] += a[i][k]*b[k][j];
		pthread_mutex_unlock (&(cmutexes[i][j]));
	}
	else {
		c[i][j] += a[i][k]*b[k][j];
	}
	pthread_exit(NULL);
}

void MultiplyMatrix()
{
	int rc;
	for (int i = 0 ; i < n * n * n; i++) {
		rc = pthread_create(&(threads[i]->t), NULL, row_col_sum, (void *) &(threads[i]->idx));
		if (rc != 0) {
			cout << "ERROR; return code from pthread_create() is " << rc << endl;
			exit(-1);
		}
	}
}

//------------------------------------------------------------------
// Main Program
//------------------------------------------------------------------
int main(int argc, char *argv[])
{
	int	isPrint;
	float runtime;

	if (GetUserInput(argc,argv,isPrint)==false) return 1;

	//Initialize the value of matrix a, b, c
	InitializeMatrix(a,9.0);
	InitializeMatrix(b,9.0);
	InitializeMatrix(c,0.0);

	runtime = clock()/(float)CLOCKS_PER_SEC;

	MultiplyMatrix();

	void *status;

	for(int t=0; t<n*n*n; t++) {
		int rc = pthread_join(threads[t]->t, &status);
	}

	runtime = clock()/(float)CLOCKS_PER_SEC - runtime;

	// check for errors in result
	bool ok = true;
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < n; ++j) {
			if (c[i][j] != c[0][0]) ok = false;
		}
	}
	if (ok) 
		cout << "ok\t";
	else
		cout << "wrong\t";

	DeleteMatrix(a);	
	DeleteMatrix(b);	
	DeleteMatrix(c);	
	pthread_exit(NULL);
	return 0;
}
