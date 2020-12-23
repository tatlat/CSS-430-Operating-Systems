#include <stdio.h>        // printf
#include <stdlib.h>       // exit, atoi
#include <pthread.h>      // pthread*
#include <semaphore.h>
#include <unistd.h>

#define BUFSIZE 1000
#define gMax    20

int NUMPROD = 1;    // number of producers
int NUMCONS = 1;    // number of consumers

int gBuf[BUFSIZE];  // global buffer
int gNum = 0;       // global counter

int gIn  = 0;       // input  cursor in gBuf
int gOut = 0;       // output cursor in gBuf

pthread_mutex_t gNumMut = PTHREAD_MUTEX_INITIALIZER; // this mutex guards gNum
pthread_mutex_t gBufMut = PTHREAD_MUTEX_INITIALIZER; // this mutex guards gBuf
pthread_cond_t gBufCond = PTHREAD_COND_INITIALIZER; // this condition checks if producer has inserted

pthread_mutex_t gProdMut = PTHREAD_MUTEX_INITIALIZER; // this mutex guards producer's say
pthread_mutex_t gConsMut = PTHREAD_MUTEX_INITIALIZER; // this mutex guards consumer's say
pthread_cond_t gProdCond = PTHREAD_COND_INITIALIZER; // this condition checks for producer's say order
pthread_cond_t gConsCond = PTHREAD_COND_INITIALIZER; // this condition checks for consumer's say order 

pthread_mutex_t gSayMut = PTHREAD_MUTEX_INITIALIZER; // this mutex guards the say function

int prev = 0; // previously extracted number (consumer)

// say prints the thread id (me), string messsage, and value (x) produced/consumed
void say(int me, const char* msg, int x) {
    //pthread_mutex_lock(&gSayMut); // lock
    printf("%u ", me);
    printf("%s", msg);
    printf("%d \n", x);
    //pthread_mutex_unlock(&gSayMut); // unlock
}

// insert inserts x into the buffer
// assumes caller holds gBufMut mutex
void insert(int x) { 
  pthread_mutex_lock(&gBufMut);
  gBuf[gIn] = x;
  ++gIn;
  //pthread_cond_broadcast(&gBufCond);
  pthread_mutex_unlock(&gBufMut);
}

// retrieves value at gOut index from buffer
// assumes caller holds gBufMut mutex
int extract() {
  //usleep(5);
  pthread_mutex_lock(&gBufMut);
  int x = gBuf[gOut];
  /*while (x == 0) {
    //pthread_cond_wait(&gBufCond, &gBufMut);
    x = gBuf[gOut];
  }*/
  ++gOut;
  pthread_mutex_unlock(&gBufMut);
  return x;
}

// increments gNum and returns its int value
// assumes caller holds gNumMut mutex
int incgNum() {
  pthread_mutex_lock(&gNumMut);
  int num = ++gNum;
  pthread_mutex_unlock(&gNumMut);
  return num;
}

int getgNum() {
  int num = gNum;
  return num;
}

// producer adds 1-20 to the buffer
// called from thread(s) and synchronized
/*void* producer(void* arg) {
  int me = pthread_self();
  int num = 0;
  while (num < gMax) {
    num = incgNum();
    if (num > gMax) { // stop at gMax
      break;
    }

    pthread_mutex_lock(&gProdMut); // lock
    while (num - 1 > gIn) { // let next in order number say first
      pthread_cond_wait(&gProdCond, &gProdMut); // wait for in order number to say
    }
    //pthread_mutex_lock(&gBufMut); // lock gBuf
    say(me, "Produced: ", num); // say in order
    insert(num); // insert in order
    //pthread_cond_broadcast(&gBufCond); // let waiting threads know you finished insert
    //pthread_mutex_unlock(&gBufMut); // unlock gBuf
    pthread_cond_broadcast(&gProdCond); // let waiting threads knoww
    pthread_mutex_unlock(&gProdMut); // unlock
    //int x = (rand() % 2) * 42000;
    //usleep(x);
  }
  return NULL;
}*/

void* producer(void* arg) {
  int me = pthread_self();
  int num = getgNum();
  while (num < gMax) {
    num = incgNum();
    say(me, "Produced: ", num);
    insert(gNum);
  }
  return NULL;
}

// consumer reads each number from gBuf after they're added
/*void* consumer(void* arg) {
  int me = pthread_self(); // get thread id
  while (1) {
    int empty = 0;
    //pthread_mutex_lock(&gBufMut); // lock buffer
    //while(gBuf[gOut] == 0 && gOut < gMax) { 
      //pthread_cond_wait(&gBufCond, &gBufMut); // wait until next number inserted into buffer
    //}
    int num = extract(); // extract next number
    if (num > gMax || num == 0) { // stop at gMax
      //pthread_mutex_unlock(&gBufMut); // lock buffer
      break; // stop
    }
    
    pthread_mutex_lock(&gConsMut); // lock 
    while (num - 1 > prev) { // let in order number say first
      pthread_cond_wait(&gConsCond, &gConsMut);
    }
    say(me, "Consumed: ", num);
    prev++; // update prev
    pthread_cond_broadcast(&gConsCond); // let waiting threads know this thread is finished
    pthread_mutex_unlock(&gConsMut); // unlock critical section

    //pthread_cond_broadcast(&gBufCond);
    //pthread_mutex_unlock(&gBufMut); // unlock buffer
    //int x = (rand() % 3) * 42450;
    //if (s == 0) usleep(x);
  }
  return NULL;
}*/

void* consumer(void* arg) {
  int me = pthread_self();
  while (1) {
    int num = extract();
    say(me, "Consumed: ", num);
    if (gOut == gMax) { 
      break;
    }
  }
  return NULL;
}

void checkInput(int argc, char* argv[]) {
  if (argc == 1) {
    NUMPROD = 1;
    NUMCONS = 1;
    return;
  }

  if (argc != 3) {
    printf("Specify <producers>  <consumer> \n");
    printf("Eg:  2  3 \n");
    exit(0);
  }

  NUMPROD = atoi(argv[1]);
  if (NUMPROD < 1 || NUMPROD > 10) {
    printf("Number of producers must lie in the range 1..10 \n");
    exit(0);
  }

  NUMCONS = atoi(argv[2]);
  if (NUMCONS < 1 || NUMCONS > 10) {
    printf("Number of consumers must lie in the range 1..10 \n");
    exit(0);
  }
}

int main(int argc, char* argv[]) {

  checkInput(argc, argv);

  pthread_t prod[NUMPROD];
  pthread_t cons[NUMCONS];

  for (int i = 0; i < NUMPROD; ++i) pthread_create(&prod[i], 0, producer, NULL);
  for (int i = 0; i < NUMCONS; ++i) pthread_create(&cons[i], 0, consumer, NULL);

  for (int i = 0; i < NUMPROD; ++i) pthread_join(prod[i], NULL);
  for (int i = 0; i < NUMCONS; ++i) pthread_join(cons[i], NULL);

  printf("All done! Hit any key to finish \n");
  getchar();
  
  return 0;
}
