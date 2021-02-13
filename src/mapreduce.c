#include "mapreduce.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define Pthread_create(thread, attr, start_routine, arg) \
  assert(pthread_create(thread, attr, start_routine, arg) == 0);
#define Pthread_join(thread, value_ptr) assert(pthread_join(thread, value_ptr) == 0);
#define Pthread_mutex_lock(m) assert(pthread_mutex_lock(m) == 0);
#define Pthread_mutex_unlock(m) assert(pthread_mutex_unlock(m) == 0);

#define SIZE_HASHTABLE (67723)
#define SIZE_VALUESARRAY (5000)

#define true 1
#define false 0

struct KeyValuePair {
  char* key;
  char** values;
  int numOfElementsInValuesArray;
} __attribute__((packed));

struct KeyValueNode {
  char* key;
  char** values;
  int numOfElementsInValuesArray;
  int sizeOfValuesArray;
  struct KeyValueNode* next;
} __attribute__((packed));

struct HashTableBucket {
  struct KeyValueNode* head;
  // number of keys in this HashTableBucket (number of nodes in the linkedlist)
  pthread_mutex_t bucketLock;
} __attribute__((packed));

struct Partition {
  // hashtable stores an array of KeyValueNode
  /**
   * 	Design
   *
   *
   *     KeyValueNode -> KeyValueNode -> KeyValueNode -> ...
   *        ^
   *        |
   *     ___________________
   *     |    |    |    |   |  Hash Table
   *     |____|____|____|___|
   *
   *        ^
   *        |
   *   _________________________________________________________________________
   *   |           |           |           |           |           |           |
   *   |           |           |           |           |           |           |
   *   |           |           |           |           |           |           |  Partition
   *   |           |           |           |           |           |           |
   *   |___________|___________|___________|___________|___________|___________|
   *        P1           P2         P3          P4          P5          P6
   */
  struct HashTableBucket** hashTable;
  int numKeys;
  struct KeyValuePair** sortedKeyValuePairArray;
  int currentArrayCounter;
  int valueCounterForCurrentKeyValuePair;
  pthread_mutex_t partitionLock;
} __attribute__((packed));

typedef struct KeyValueNode KeyValueNode;
typedef struct HashTableBucket HashTableBucket;
typedef struct Partition Partition;
typedef struct KeyValuePair KeyValuePair;

// variables for grabbing files
char** files;
pthread_mutex_t filetableLock;
int numFiles;
int numOfFilesLeft;

// Variables for partition
Partition** partitionArray;
int numOfPartition;
// Variables for mappers and reducers
Mapper _Map;
Reducer _Reduce;
Partitioner _Partition;
