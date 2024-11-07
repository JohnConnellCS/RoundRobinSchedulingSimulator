#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef uint32_t u32;
typedef int32_t i32;

struct process
{
  u32 pid;
  u32 arrival_time;
  u32 burst_time;

  TAILQ_ENTRY(process) pointers;

  /* Additional fields here */
  u32 response_time;
  u32 wait_time;
  u32 gotResponse;
  u32 execution_time;
  /* End of "Additional fields here" */
};

TAILQ_HEAD(process_list, process);

u32 next_int(const char **data, const char *data_end)
{
  u32 current = 0;
  bool started = false;
  while (*data != data_end)
  {
    char c = **data;

    if (c < 0x30 || c > 0x39)
    {
      if (started)
      {
        return current;
      }
    }
    else
    {
      if (!started)
      {
        current = (c - 0x30);
        started = true;
      }
      else
      {
        current *= 10;
        current += (c - 0x30);
      }
    }

    ++(*data);
  }

  printf("Reached end of file while looking for another integer\n");
  exit(EINVAL);
}

u32 next_int_from_c_str(const char *data)
{
  char c;
  u32 i = 0;
  u32 current = 0;
  bool started = false;
  while ((c = data[i++]))
  {
    if (c < 0x30 || c > 0x39)
    {
      exit(EINVAL);
    }
    if (!started)
    {
      current = (c - 0x30);
      started = true;
    }
    else
    {
      current *= 10;
      current += (c - 0x30);
    }
  }
  return current;
}

void init_processes(const char *path,
                    struct process **process_data,
                    u32 *process_size)
{
  int fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    int err = errno;
    perror("open");
    exit(err);
  }

  struct stat st;
  if (fstat(fd, &st) == -1)
  {
    int err = errno;
    perror("stat");
    exit(err);
  }

  u32 size = st.st_size;
  const char *data_start = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data_start == MAP_FAILED)
  {
    int err = errno;
    perror("mmap");
    exit(err);
  }

  const char *data_end = data_start + size;
  const char *data = data_start;

  *process_size = next_int(&data, data_end);

  *process_data = calloc(sizeof(struct process), *process_size);
  if (*process_data == NULL)
  {
    int err = errno;
    perror("calloc");
    exit(err);
  }

  for (u32 i = 0; i < *process_size; ++i)
  {
    (*process_data)[i].pid = next_int(&data, data_end);
    (*process_data)[i].arrival_time = next_int(&data, data_end);
    (*process_data)[i].burst_time = next_int(&data, data_end);
  }

  munmap((void *)data, size);
  close(fd);
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    return EINVAL;
  }
  struct process *data;
  u32 size;
  init_processes(argv[1], &data, &size);

  u32 quantum_length = next_int_from_c_str(argv[2]);

  struct process_list list;
  TAILQ_INIT(&list);
  //your code here
  u32 total_waiting_time = 0;
  u32 total_response_time = 0;
  u32 currTime = 0;
  u32 procsComplete = 0;
  u32 currProcs = 0;

  struct process *procToAdd;
  while(procsComplete < size){
    //increment time
    //check if new processes have arrived
    for (u32 i = size-1; i >= currProcs && i < size; i--) {
      if (data[i].arrival_time == currTime) {
        TAILQ_INSERT_TAIL(&list, &data[i], pointers);
        currProcs++;
      }
    }

    if(procToAdd != NULL){
      TAILQ_INSERT_TAIL(&list, procToAdd, pointers);
      procToAdd = NULL;
    }

    if(!TAILQ_EMPTY(&list)){
      struct process *currProc = TAILQ_FIRST(&list);
      //printf("CURR: %u with burst remaining %u \n", currProc->pid, currProc->burst_time - currProc->execution_time);

      //set response time if not yet set
      if(currProc->gotResponse == 0){
        currProc->response_time = currTime - currProc->arrival_time;
        total_response_time += currProc->response_time;
        currProc->gotResponse = 1;
      }

      currProc->execution_time += 1;

      if(currProc->execution_time == currProc->burst_time){
        currProc->wait_time = currTime + 1 - currProc->arrival_time - currProc->burst_time;
        total_waiting_time += currProc->wait_time;
        procsComplete++;
        //printf("KICKED: Process waited a total of %u seconds \n", currProc->wait_time);
        TAILQ_REMOVE(&list, currProc, pointers);
      }
      else if(currProc->execution_time % quantum_length == 0){
        //printf("DONE: Process kicked out with %u remaining \n", currProc->burst_time - currProc->execution_time);
        TAILQ_REMOVE(&list, currProc, pointers);
        procToAdd = currProc;
      }else{
        //printf("tick %u \n", currProc->pid);
      }
    }
    //printf("%u", procsComplete);
    //printf("%u", size);
    currTime++;
  }
  //each while is 1 second, update current slice and check if we're done, if we're not done and time slice is up, add to uqueue, if done free memory
  //update everything else in the queue and update wait and response time if we haven't run
  //everytime you hit something you want to add, malloc the size of struct process
  //go through every process, decrement burst time
  //keep track of current time in each time slice

  /* End of "Your code here" */

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}

