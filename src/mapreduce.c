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

////////////////////////////////////////////////////////////////////////////////////

/*
 * This hash function is called sdbm. Reference http://www.cse.yorku.ca/~oz/hash.html
 *
 **/
unsigned long hash_function(char* key, int sizeOfHashtable) {
  unsigned long hash = 0;
  int c;

  while ((c = *key++) != '\0') hash = c + (hash << 6) + (hash << 16) - hash;

  return (hash % sizeOfHashtable);
}

char* get_next(char* key, int partition_number) {
  Partition* targetPartition = partitionArray[partition_number];
  KeyValuePair** sortedKeyValuePairArray = targetPartition->sortedKeyValuePairArray;
  KeyValuePair* currentKeyValuePair = sortedKeyValuePairArray[targetPartition->currentArrayCounter];

  assert(strcmp(currentKeyValuePair->key, key) == 0);

  char** valuesArray = currentKeyValuePair->values;
  int valuesArrayCounter = targetPartition->valueCounterForCurrentKeyValuePair;
  int sizeOfValuesArray = currentKeyValuePair->numOfElementsInValuesArray;

  if (valuesArrayCounter >= sizeOfValuesArray) {
    return NULL;
  }

  if (valuesArrayCounter > 0) {
    free(currentKeyValuePair->values[valuesArrayCounter - 1]);
  }

  char* currentValue = valuesArray[valuesArrayCounter];
  targetPartition->valueCounterForCurrentKeyValuePair =
      targetPartition->valueCounterForCurrentKeyValuePair + 1;
  return currentValue;
}

void* GrabFileAndMap() {
  int i;
  char* filename;

grabNewFile:

  filename = NULL;

  // Note: mapper threads should repetitively come here until there's no file left to map.
  // We first grab filetable lock. Check if there's still any file to Map().
  // if there is, grab the file from files[i], set files[i]=NULL, map() the file
  // else, just exit the thread

  Pthread_mutex_lock(&filetableLock);

  // if numOfFiles left is zero, no need to go into loop anymore
  if (numOfFilesLeft == 0) {
    Pthread_mutex_unlock(&filetableLock);
    pthread_exit(0);
  }

  for (i = 0; i < numFiles; i++) {
    if (files[i] != NULL) {
      filename = strdup(files[i]);
      free(files[i]);
      files[i] = NULL;
      numOfFilesLeft--;
      break;
    }
  }

  Pthread_mutex_unlock(&filetableLock);

  // if the mapper thread is unable to grab a file, exit it
  assert(filename != NULL);
  // if(filename==NULL)
  // {
  // 	printf("Something funny happened here.\n");
  // }

  //////////////////////////////////

  _Map(filename);

  free(filename);

  if (numOfFilesLeft == 0)
    pthread_exit(0);
  else
    goto grabNewFile;
}
