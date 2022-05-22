#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>
#define SMALL 32


void serialMergeSort(int arr[], int l, int r);
void insertion_sort(int a[], int size);

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

// Merges two subarrays of arr[].
// First subarray is arr[l..m]
// Second subarray is arr[m+1..r]

void merge(int arr[], int l, int m, int r) {
  int i, j, k;
  int n1 = m - l + 1;
  int n2 = r - m;

  int * L=malloc(sizeof(int) * n1);
  int *R=malloc(sizeof(int) * n2);
  

  for (i = 0; i < n1; i++)
    L[i] = arr[l + i];
  for (j = 0; j < n2; j++)
    R[j] = arr[m + 1 + j];

  i = 0; // Initial index of first subarray
  j = 0; // Initial index of second subarray
  k = l; // Initial index of merged subarray
  while (i < n1 && j < n2) {
    if (L[i] <= R[j]) {
      arr[k] = L[i];
      i++;
    } else {
      arr[k] = R[j];
      j++;
    }
    k++;
  }

  while (i < n1) {
    arr[k] = L[i];
    i++;
    k++;
  }

  while (j < n2) {
    arr[k] = R[j];
    j++;
    k++;
  }
}

void mergeSortParallel(int arr[], int l, int r) {

  if (l < r) {
    int m = l + (r - l) / 2;
    if (abs(m - l) <= 100 || abs(r - m) <= 100) {
            mergeSortParallel(arr, l, m);
            mergeSortParallel(arr, m + 1, r);
        
        } else {
      #pragma omp parallel sections num_threads(8)
      {
        #pragma omp section 
        {
          mergeSortParallel(arr, l, m);
        }

        #pragma omp section 
        {
          mergeSortParallel(arr, m + 1, r);
        }
      }
      merge(arr, l, m, r);
    }
  }
}


void serialMergeSort(int arr[], int l, int r) {
  if (l < r) {

    int m = l + (r - l) / 2;
    serialMergeSort(arr, l, m);
    serialMergeSort(arr, m + 1, r);

    merge(arr, l, m, r);
  }
}


void printArray(int A[], int size) {
  int i;
  for (i = 0; i < 30; i++)
    printf("%d ", A[i]);
  printf("\n");
}

int main(int argc, char * argv[]) {
  double start;
  double finish;
  double duration;
  int n = atoi (argv[1]);

    int * arr = malloc(sizeof(int) * n);
    int * b = malloc(sizeof(int) * n);
printf("Array size = %d\n", n);

  randFill(arr, n);
  //writeToFile(arr, n);
  //readFromFile(arr, n);

  for (int i = 0; i < n; i++) {
    b[i] = arr[i];
  }

  //printf("Given array is \n");
  //printArray(arr, n);

  start = omp_get_wtime();
  mergeSortParallel(arr, 0, n - 1);
  finish = omp_get_wtime();

  duration = finish - start;

  printf("Time of parallel merge sort = %f \n", duration);

  time_t st;
  time_t fn;

  st = clock();
  serialMergeSort(b, 0, n - 1);
  fn = clock();

  duration = (double)(fn - st) / CLOCKS_PER_SEC;
  printf("Time of serial merge sort =  %f \n", duration);

  printf("\nSorted array is \n");
  printArray(arr, n);

  return 0;
}