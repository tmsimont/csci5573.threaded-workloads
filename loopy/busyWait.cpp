//---------------------------------------------------------------------
// Pthreads busy waiting
// Creates busy waiting by creating an user input number of Pthreads
//  based on command line argument that initiate infinite loops, for 
//  the purpose of learning about the Linux CFS scheduler
//---------------------------------------------------------------------
// Created by Trevor Simonton, Anthony Pfaff, Cory Linfield, Alex Klein
//---------------------------------------------------------------------


#include <iostream>
#include <iomanip>
#include <cmath>
#include <time.h>
#include <cstdlib>
#include <stdio.h>
#include <pthread.h>
using namespace std;


//----------------------------------------------------------------
// Busy Spin
//----------------------------------------------------------------

void *busy_spin(void *idp) {
	int id = (int)idp;

	while(true) {
	
	}

	pthread_exit(NULL);

}


//-------------------------
// Test for Pthreads
//-------------------------

void *print_test(void *threadid) {
	int tid = (int)threadid;
	cout << "This is thread id: " << tid << endl;
	pthread_exit(NULL);
}



//------------------------------------------------------------------
// Main Program
//------------------------------------------------------------------
int main(int argc, char *argv[])
{
	int num_threads;
	int rc;

	if (argc == 1) {
		cout << "Number of Threads not specified, default is 10." << endl;
		num_threads = 10;
	}
	else if (argc > 2 || atoi(argv[1]) < 0 || atoi(argv[1]) > 2147483647) {
		cout << "Command Line arguments invalid." << endl;
		return -1;
	}
	else {
		num_threads = atoi(argv[1]);
	}

	pthread_t threads[num_threads];

	for (int i = 0; i < num_threads; ++i) {
	//	cout << "main() : creating thread, " << i << endl;
		rc = pthread_create(&threads[i], NULL, busy_spin, (void *)i);

		if (rc) {
			cout << "Error:unable to create thread, " << rc << endl;			exit(-1);
		}
	}

	pthread_exit(NULL);

	return 0;
}
