# Design

## (A high level) Design Picture

```
    File 1               File 2             File 3           File 4             File 5




             		Mappers compete with each other to grab files to map()





          Mapper 1                          Mapper 2                         Mapper 3




   <key,{value,value,value}>        <key,{value,value,value}>           <key,{value,value}>
        <key,{value}>

             ↓                                  ↓                               ↓



              BLACK BOX: Given a key, return the partition number this key should go to





                Partition 1          Partition 2         Partition 3        Partition 4


                     ↓                    ↓                   ↓                 ↓


                  Reducer 1           Reducer 2            Reducer 3         Reducer 4





                    Reducer threads sort the keys in the partitions and then reduce()

```

## Design Explanation

1. User can specify the number of threads used for mappers and reducers
   respectively (they don't have to be the same). If the number of mappers is
   equals to the number of files, each mappers will map exactly one file.

2. There will be k numbers of partitions, with `k = number of reducer threads`.
   It's imperial to note that each reducer thread processes on only exactly one
   partition.

3. Mappers each put values into the corresponding partitions as indicated by
   `MR_DefaultHashPartition`. A default implementation of this hash function is
   being provided. However, user is free to modify it such that the key value
   pairs can be more evenly distributed among the partitions.

4. After the mappers are finished, `Reduce()` is invoked once per key, and is
   passed the key along with a function that enables iteration over all of the
   values that produced that same key. To iterate, the code just calls
   `get_next()` repeatedly until a NULL value is returned; `get_next` returns a
   pointer to the value passed in by the MR_Emit() function, or NULL when the
   key's values have been processed.

5. For each partition, the keys are being sorted in ascending key order. When
   the reducer thread is in work, the `Reduce()` method is being called on each
   key in the sorted order.

6. You might be curious what exactly is this secretive thing called `partition`.
   I drew up a design of it too!

```
struct Partition
{
	//hashtable stores an array of KeyValueNode
	/**
	* 	Design
	*
	*
	*      KeyValueNode -> KeyValueNode -> KeyValueNode -> ...
	*       ^
	*       |
	*    _____________________
	*    |    |    |    |    | Hash Table
	*    |____|____|____|____|
	*
	*       ^
	*       |
	* _________________________________________________________________________
	* |           |           |           |           |           |           |
	* |           |           |           |           |           |           |
	* |           |           |           |           |           |           |     Partitions
	* |           |           |           |           |           |           |
	* |___________|___________|___________|___________|___________|___________|
	*	     P1          P2          P3          P4          P5          P6
	*/
	struct HashTableBucket **hashTable;
	int numKeys;
	struct KeyValuePair** sortedKeyValuePairArray;
	int currentArrayCounter;
	int valueCounterForCurrentKeyValuePair;
	pthread_mutex_t partitionLock;
}__attribute__((packed));

```

As a short summary, there are `k partitions`, with
`k = number of reducer threads`. When user calls on `Map()`, the key and
corresponding value is being stored in the corresponding partition (as decided
by the hash function `MR_DefaultHashPartition()`). In each partition, there's a
hash table that holds the key value pairs, eg: `<"hello": 1,1,2,1>`. The hash
table implements its own hash function to decide which hash table bucket the key
value pair goes to in the hashtable. However, since this hash table is not
perfect hashing, collision might occurs. So the key value pairs are essentially
a node in the linkedlist in the hashtable bucket.

When user calls on `Reduce()`, the reducer thread goes to its corresponding
partition (recalls that each reducer thread works only on exactly one
partition), sort the key value pairs in the partitions and call `Reduce()` in
the sorted order of the keys.
