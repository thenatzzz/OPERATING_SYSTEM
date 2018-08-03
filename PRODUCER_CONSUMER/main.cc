/******************************************************************
 * The Main program with the two functions. A simple
 * example of creating and using a thread is provided.
 ******************************************************************/
#include "helper.h"

#define SIZE 10   // buffer size
#define TRUE 1
#define FALSE 0

void *producer (void *id);
void *consumer (void *id);

int buffer[SIZE];
int buffer_index = 0;
int num_task = 0;
int queue_size =0;

// an index for the different buffer/sem type
enum{NOT_FULL, NOT_EMPTY, CONSUMERS};

pthread_mutex_t buffer_mutex;

typedef struct job_struct{
  int id;
  int duration;
} JOB_STRUC;

JOB_STRUC *job_array;

void insertToBuffer(int value){
  if (buffer_index < SIZE){
    buffer[buffer_index++] = value;
  }else{
    cout<< "Error: Buffer Overflow"<< endl;
  }
}

int removeFromBuffer(){
  if (buffer_index > 0){
    return buffer[--buffer_index];
  }else{
    cout << "Error: Buffer Underflow"<< endl;
  }
  return 0;
}

int isEmpty(){
  if (buffer_index == 0)
     return TRUE;
  return FALSE;
}

int isFull(){
  if (buffer_index == SIZE)
     return TRUE;
  return FALSE;
}

int main (int argc, char **argv){
  int sem_id;
  queue_size = atoi(argv[1]);
  num_task = atoi(argv[2]);
  int num_producer = atoi(argv[3]);
  int num_consumer = atoi(argv[4]);

  pthread_t producerid[num_producer];
  pthread_t consumerid[num_consumer];

  if(argc < 2){
    cout << "Error: please provide valid number of arguments"<< endl;
    exit(1);
  }

  if((queue_size = atoi(argv[1])) < 1){
    cout << "Error: queue must be at least size of 1"<< endl;
    exit(1);
  }

  job_array = (JOB_STRUC*) malloc(queue_size*sizeof(job_array[0]));
  for(int i = 0; i < queue_size; i++){
    job_array[i].id = 0;
    job_array[i].duration =0;
  }

//-------------------Creating Semaphore-------------------------------
  key_t key = (key_t) getuid();

  if ((sem_id = sem_create(key, 3)) == -1){
    cerr<<"Error: fail to initialize sem_create"<< endl;
    exit(1);
  }
  if (sem_init(sem_id, NOT_FULL, queue_size) == -1){
    cerr<<"Error: fail to initialize sem_init NOT_FULL"<< endl;
    exit(1);
  }
  if (sem_init(sem_id, NOT_EMPTY, 0) == -1){
    cerr<<"Error: fail to initialize sem_init NOT_EMPTY"<< endl;
    exit(1);
  }
  if (sem_init(sem_id, CONSUMERS, 0) == -1){
    cerr<<"Error: fail to initialize sem_init CONSUMERS"<< endl;
    exit(1);
  }

// ---------------Creating Producer & Consumer Thread-----------------------
  int thread_consumer[num_consumer];
  int thread_producer[num_producer];

//  Create Producer thread(s)
	for(int i = 0; i < num_producer; i++){
    thread_producer[i] = i;
		pthread_create(&producerid[i],NULL,producer,(void *) &thread_producer[i]);
	}

//  Create Consumer thread(s)
	for(int i = 0; i < num_consumer; i++){
    thread_consumer[i] =i;
		pthread_create(&consumerid[i],NULL,consumer,(void *) &thread_consumer[i]);
	}
//-------------------------------------------------------------------

  for(int i = 0; i < num_producer; i++){
    pthread_join(producerid[i],NULL);
  }
  for(int i = 0; i < num_consumer; i++){
    pthread_join(consumerid[i],NULL);
  }

  pthread_mutex_destroy(&buffer_mutex);
  sem_close(sem_id);

  return 0;
}

void *producer(void *parameter){

  int sem_id;
  int param = *(int *) parameter;
  int value;
  int i=0;

  while (i++ < num_task){

      value = rand() % 100;
      pthread_mutex_lock(&buffer_mutex);
      auto start = chrono::high_resolution_clock::now();

      /* This conditional loop does unlock/wait and wakeup/lock atomically,
         in order to avoid possible race conditions */
      do{
          pthread_mutex_unlock(&buffer_mutex);
            // cannot go to slepp while holding lock
          sem_wait(sem_id,NOT_EMPTY);
            /* There could be race conditions here; thus, we need to
               check for spurious wakeups ie thread could wake up
               and aqcuire lock and fill up buffer.*/

          pthread_mutex_lock(&buffer_mutex);
      }while(isFull()); // check for spurios wake-ups

      this_thread::sleep_for(chrono::seconds(rand()%5+1));

      insertToBuffer(value);
      job_array[i].id  = i;

      pthread_mutex_unlock(&buffer_mutex);
      sem_signal(sem_id,NOT_FULL);

      auto end =  chrono::high_resolution_clock::now();
      chrono::duration<double, milli> elapsed = end-start;
      job_array[i].duration = elapsed.count();

      cout<<"Producer("<<param<<"): Job id "<<job_array[i].id<< " duration "<<job_array[i].duration/1000<<endl;
    }
  pthread_exit(0);
}

void *consumer (void *id)
{
  int sem_id;
  int thread_num = *(int *)id;
  int value;
  int i=0;

  sem_signal(sem_id, CONSUMERS);

  auto start = chrono::high_resolution_clock::now();

  while (i++ < num_task){
      pthread_mutex_lock(&buffer_mutex);

      do{
          pthread_mutex_unlock(&buffer_mutex);

          sem_wait(sem_id,NOT_FULL);

	        this_thread::sleep_for(chrono::seconds(rand()%5+1));

          pthread_mutex_lock(&buffer_mutex);

      }while(isEmpty()); //check for spurios wakeups

      auto end =  chrono::high_resolution_clock::now();
      chrono::duration<double, milli> elapsed = end-start;
      job_array[i].duration = elapsed.count();

      cout << "Consumer("<<thread_num<<"): job id "<<job_array[i].id<< " executing sleep duration "<<job_array[i].duration/1000<< endl;

      value = removeFromBuffer();

      pthread_mutex_unlock(&buffer_mutex);

      sem_signal(sem_id, NOT_EMPTY);

      cout<<"Consumer("<<thread_num<<"): job id "<<job_array[i].id<< " completed"<< endl;
  }
  pthread_exit(0);
}
