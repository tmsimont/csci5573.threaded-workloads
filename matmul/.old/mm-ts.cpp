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

#define handle_error_en(en, msg) \
       if(en != 0) do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

int LOCK = 0;
int UNLOCK = 1;

using namespace std;



// globals for p threads
struct helper {
	pthread_t t;
	int idx;
};

int n = 0;
float **a,**b,**c;
int** cmutexes;
helper **threads;
int locked = 0;
int priority = 0;
sched_param pri;

pthread_attr_t attr;
static inline void spinlock(int * a);
static inline void spinunlock(int * b);
//-----------------------------------------------------------------------
//   Get user input for matrix dimension or printing option
//-----------------------------------------------------------------------
bool GetUserInput(int argc, char *argv[],int& isPrint)
{
	bool isOK = true;

	if(argc < 2) 
	{
		cout << "Arguments:<X> [<Y>]" << endl;
		cout << "X : Matrix size [X x X]" << endl;
		cout << "Y = 1: Use mutex lock" << endl;
		cout << "Y <> 1 or missing: does not use mutex lock" << endl;
cout << "Z : integer priority between 1 and 99, default if not specified" << endl;

		isOK = false;
	}
	else 
	{
		//get matrix size
		n = atoi(argv[1]);
		if (n <=0) 
		{
			cout << "Matrix size must be larger than 0" <<endl;
			isOK = false;
		}
		locked = 0;
		if (argc >=3)
			locked = (atoi(argv[2])==1 && n <=9)?1:0;
		else
			locked = 0;

		if(argc >= 4)
		{
			priority = atoi(argv[3]);
			if(priority < 1 || priority > 99)
			{
				cout << "Priority must be between 1 and 99, inclusive" << endl;
				isOK = false;
			}
			else 
			{
				const struct sched_param const_pri = {priority};
				handle_error_en(pthread_attr_init(&attr),"pthread_attr_init");
				handle_error_en(pthread_attr_setschedpolicy(&attr,SCHED_RR), "pthread_attr_setschedpolicy");
				handle_error_en(pthread_attr_setschedparam(&attr,&const_pri), "pthread_attr_setschedparam");
				handle_error_en(pthread_setschedparam(pthread_self(),SCHED_RR , &const_pri), "pthread_setschedparam");
			}
		}
	}
	return isOK;
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

	cmutexes = new int*[n];
	cmutexes[0] = new int[n*n];
	for (int i = 1; i < n; i++)	cmutexes[i] = cmutexes[i-1] + n;

	// init locks
	for (int i = 0 ; i < n; ++i) {
		for (int j = 0 ; j < n; ++j) {
			cmutexes[i][j] = &UNLOCK;
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
		spinlock (&(cmutexes[i][j]));
		c[i][j] += a[i][k]*b[k][j];
		spinunlock (&(cmutexes[i][j]));
	}
	else {
		c[i][j] += a[i][k]*b[k][j];
	}
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
	for (int i = 0 ; i < n * n * n; i++) {
		if(priority != 0 )
		{
			handle_error_en(pthread_create(&(threads[i]->t), &attr, row_col_sum, (void *) &(threads[i]->idx)),"pthread_create1");
		}
		else
		{
			handle_error_en(pthread_create(&(threads[i]->t), NULL, row_col_sum, (void *) &(threads[i]->idx)),"pthread_create2");
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
		if(priority == 1)
			printPriInfo();
		handle_error_en(pthread_join(threads[t]->t, &status), "pthread_join");
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

