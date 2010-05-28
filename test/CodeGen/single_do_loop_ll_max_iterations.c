#define N 20
#include "limits.h"

int main () {
  int i;
  int A[N];

  A[0] = 0;

  __sync_synchronize();

  i = 0;

  do {
    A[0] = i;
    ++i;
  } while (i < LLONG_MAX);

  __sync_synchronize();

  if (A[0] == LLONG_MAX - 1)
    return 0;
  else
    return 1;
}
