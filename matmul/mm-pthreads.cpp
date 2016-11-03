//-----------------------------------------------------------------------
// Matrix Multiplication - Sequential version to run on single CPU core only
//-----------------------------------------------------------------------
//  Written by: Gita Alaghband, Lan Vu 
//  Updated in 8/8/2011
//-----------------------------------------------------------------------
#include <iostream>
#include <iomanip>
#include <cmath>
#include <time.h>
#include <cstdlib>
#include <stdio.h>
#include <pthread.h>
using namespace std;

struct helper {
	pthread_t t;
	int idx;
};

int n = 0;
float **a,**b,**c;
pthread_mutex_t** cmutexes;
helper **threads;
int locked = 0;
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
		cout << "Y = 1: print the input/output matrix if X < 10" << endl;
		cout << "Y <> 1 or missing: does not print the input/output matrix" << endl;

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

		//is print the input/output matrix
		/*
		if (argc >=3)
			isPrint = (atoi(argv[2])==1 && n <=9)?1:0;
		else
			isPrint = 0;
		*/
		locked = 0;
		if (argc >=3)
			locked = (atoi(argv[2])==1 && n <=9)?1:0;
		else
			locked = 0;

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

	cmutexes = new pthread_mutex_t*[n];
	cmutexes[0] = new pthread_mutex_t[n*n];
	for (int i = 1; i < n; i++)	cmutexes[i] = cmutexes[i-1] + n;

	// init locks
	for (int i = 0 ; i < n; ++i) {
		for (int j = 0 ; j < n; ++j) {
			pthread_mutex_init(&(cmutexes[i][j]), NULL);
		}
	}
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


void* row_col_sum(void* idp) {
	int id = *(int*)idp;
	int id2 = id;
	int k = id % n; 
	id = id/n;
	int j = id % n;
	id = id/n;
	int i = id % n;
	if (locked == 1) {
		pthread_mutex_lock (&(cmutexes[i][j]));
		///cout << id2 << endl;
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

	//Print the input matrices
	if (isPrint==1)
	{
		cout<< "Matrix a[n][n]:" << endl;
		PrintMatrix(a); 
		cout<< "Matrix b[n][n]:" << endl;
		PrintMatrix(b); 
	}

	runtime = clock()/(float)CLOCKS_PER_SEC;

	MultiplyMatrix();

	void *status;

	for(int t=0; t<n*n*n; t++) {
		int rc = pthread_join(threads[t]->t, &status);
	}

	runtime = clock()/(float)CLOCKS_PER_SEC - runtime;

	//Print the output matrix
	/*
	if (isPrint==1)
	{
		cout<< "Output matrix:" << endl;
		PrintMatrix(c); 
	}
	cout<< "Program runs in " << setiosflags(ios::fixed) << setprecision(2) << runtime << " seconds\n"; 
	*/

	bool ok = true;
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < n; ++j) {
			if (c[i][j] != c[0][0]) ok = false;
		}
	}
	if (ok) 
		cout << "great" << endl;
	else
		cout << "ungreat" << endl;
	
	DeleteMatrix(a);	
	DeleteMatrix(b);	
	DeleteMatrix(c);	
	pthread_exit(NULL);
	return 0;
}
