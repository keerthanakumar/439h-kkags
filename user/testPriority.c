//Text function 1
#include<stdio.h>
#include<lib.h>

#define NUMB 2
	
void 
testPri(){
	
	int i;
	int j;
	int A[10000][1000];
	for(i = 0; i < 10000; i++){
		for ( j = 0; j < 1000; j++){
			A[i][j] = i+j;
		}
	} 
	for(i = 0; i < 10000; i++){
		for( j = 0; j < 1000; j++){
			A[i][j] += A[333][33];
		}
	}
	printf("I am [%04x] and I am done with my functions\n", sys_getenvid());
}

void
children() {
	printf("I am [%04x] and I am going to fork\n", sys_getenvid());	
	if (fork() == 0) {
		testPri();
		exit();
	}
	testPri();
}

void
main() {
	children();
}
