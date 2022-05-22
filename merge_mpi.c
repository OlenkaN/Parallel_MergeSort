#include <mpi.h>

#include <stdlib.h>

#include <stdio.h>

#include <string.h>

#include <math.h>

#include <time.h>

// Arrays size <= SMALL switches to insertion sort
#define SMALL 32
void merge(int a[], int size, int temp[]);
void insertion_sort(int a[], int size);
void mergesort_serial(int a[], int size, int temp[]);
void mergesort_parallel_mpi(int a[], int size, int temp[], int level, int my_rank, int max_rank, int tag, MPI_Comm comm);
int my_topmost_level_mpi(int my_rank);
void run_root_mpi(int a[], int size, int temp[], int max_rank, int tag,
  MPI_Comm comm);
void run_helper_mpi(int my_rank, int max_rank, int tag, MPI_Comm comm);
void randFill(int * a, int n);
void readFromFile(int * a, int n);

int main(int argc, char * argv[]) {
  // All processes
  MPI_Init(NULL, NULL);
  // Check processes and their ranks
  // number of processes == communicator size
  int comm_size;
  MPI_Comm_size(MPI_COMM_WORLD, & comm_size);
  int my_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, & my_rank);

  int max_rank = comm_size - 1;
  int tag = 123;
  // Set test data
  if (my_rank == 0) { 
    puts("-MPI Recursive Mergesort-\t");
    int size = atoi (argv[1]);
    printf("Array size = %d\nProcesses = %d\n", size, comm_size);
    // Array allocation

    int * a = malloc(sizeof(int) * size);
    int * temp = malloc(sizeof(int) * size);

    if (a == NULL || temp == NULL) {
      printf("Error: Could not allocate array of size %d\n", size);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    randFill(a, size);
    //readFromFile(a,size);
    // Sort with root process

    time_t st;
    time_t fn;
    st = clock();
    run_root_mpi(a, size, temp, max_rank, tag, MPI_COMM_WORLD);
    fn = clock();

    double duration = (double)(fn - st) / CLOCKS_PER_SEC;
    printf("Time of parallel merge sort =  %f \n", duration);
    // Result check
    for (int i = 1; i < size; i++) {
      if (!(a[i - 1] <= a[i])) {
        printf("Implementation error: a[%d]=%d > a[%d]=%d\n", i - 1,
          a[i - 1], i, a[i]);
        MPI_Abort(MPI_COMM_WORLD, 1);
      }
    }
  } // Root process end
  else { // Helper processes  
    run_helper_mpi(my_rank, max_rank, tag, MPI_COMM_WORLD);
  }
  fflush(stdout);
  MPI_Finalize();
  return 0;
}

void run_root_mpi(int a[], int size, int temp[], int max_rank, int tag,
  MPI_Comm comm) {
  int my_rank;
  MPI_Comm_rank(comm, & my_rank);
  if (my_rank != 0) {
    printf("Error: run_root_mpi called from process %d; must be called from process 0 only\n", my_rank);
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
  mergesort_parallel_mpi(a, size, temp, 0, my_rank, max_rank, tag, comm);
  /* level=0; my_rank=root_rank=0; */
  return;
}

// Helper process code
void run_helper_mpi(int my_rank, int max_rank, int tag, MPI_Comm comm) {
  int level = my_topmost_level_mpi(my_rank);
  // probe for a message and determine its size and sender
  MPI_Status status;
  int size;
  MPI_Probe(MPI_ANY_SOURCE, tag, comm, & status);
  MPI_Get_count( & status, MPI_INT, & size);
  int parent_rank = status.MPI_SOURCE;
  // allocate int a[size], temp[size] 
  int * a = malloc(sizeof(int) * size);
  int * temp = malloc(sizeof(int) * size);
  MPI_Recv(a, size, MPI_INT, parent_rank, tag, comm, & status);
  mergesort_parallel_mpi(a, size, temp, level, my_rank, max_rank, tag, comm);
  // Send sorted array to parent process
  MPI_Send(a, size, MPI_INT, parent_rank, tag, comm);
  return;
}

// Given a process rank, calculate the top level of the process tree in which the process participates
// Root assumed to always have rank 0 and to participate at level 0 of the process tree
int my_topmost_level_mpi(int my_rank) {
  int level = 0;
  while (pow(2, level) <= my_rank)
    level++;
  return level;
}

void mergesort_parallel_mpi(int a[], int size, int temp[], int level, int my_rank, int max_rank, int tag, MPI_Comm comm) {
  int helper_rank = my_rank + pow(2, level);
  if (helper_rank > max_rank) {
    mergesort_serial(a, size, temp);
  } else {
    MPI_Request request;
    MPI_Status status;
    MPI_Isend(a + size / 2, size - size / 2, MPI_INT, helper_rank, tag, comm, & request);

    mergesort_parallel_mpi(a, size / 2, temp, level + 1, my_rank, max_rank,
      tag, comm);
    // Free the async request (matching receive will complete the transfer).
    MPI_Request_free( & request);
    // Receive second half sorted
    MPI_Recv(a + size / 2, size - size / 2, MPI_INT, helper_rank, tag,
      comm, & status);
    // Merge the two sorted sub-arrays through temp
    merge(a, size, temp);
  }
  return;
}

void mergesort_serial(int a[], int size, int temp[]) {
  if (size <= SMALL) {
    insertion_sort(a, size);
    return;
  }
  mergesort_serial(a, size / 2, temp);
  mergesort_serial(a + size / 2, size - size / 2, temp);
  merge(a, size, temp);
}

void merge(int a[], int size, int temp[]) {
  int i1 = 0;
  int i2 = size / 2;
  int tempi = 0;
  while (i1 < size / 2 && i2 < size) {
    if (a[i1] < a[i2]) {
      temp[tempi] = a[i1];
      i1++;
    } else {
      temp[tempi] = a[i2];
      i2++;
    }
    tempi++;
  }
  while (i1 < size / 2) {
    temp[tempi] = a[i1];
    i1++;
    tempi++;
  }
  while (i2 < size) {
    temp[tempi] = a[i2];
    i2++;
    tempi++;
  }
  // Copy sorted temp array into main array, a
  memcpy(a, temp, size * sizeof(int));
}

void insertion_sort(int a[], int size) {
  int i;
  for (i = 0; i < size; i++) {
    int j, v = a[i];
    for (j = i - 1; j >= 0; j--) {
      if (a[j] <= v)
        break;
      a[j + 1] = a[j];
    }
    a[j + 1] = v;
  }
}

void randFill(int * a, int n) {
  srand(time(0));
  for (int i = 0; i < n; i++) {
    a[i] = rand() ;
  }

}
void writeToFile(int * a, int n) {
  FILE * fp;

  if ((fp = fopen("data", "wb")) == NULL) {
    printf("Cannot open file.\n");
  }

  if (fwrite(a, sizeof(int), n, fp) != n)
    printf("File read error.");
  fclose(fp);
}
void readFromFile(int * a, int n) {
  FILE * fp;

  if ((fp = fopen("data", "rb")) == NULL) {
    printf("Cannot open file.\n");
  }

  if (fread(a, sizeof(int), n, fp) != n) {
    if (feof(fp))
      printf("Premature end of file.");
    else
      printf("File read error.");
  }
  fclose(fp);
}