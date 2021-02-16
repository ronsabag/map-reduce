# MapReduce

A multi-node, multi-threaded MapReduce implementation from scratch. Support
arbitrary number of files, map and reduce, and supports encrypted and compressed
inputs/outputs.

For details about the design, see [this](./docs/design.md).

## Table of contents

- [Table of contents](#table-of-contents)
- [Background](#background)
- [Usage](#usage)
- [Example application: word counting](#example-application-word-counting)
- [Attribution](#attribution)

## Background

In 2004, engineers at Google introduced a new paradigm for large-scale parallel
data processing known as MapReduce (see the original paper
[here](https://static.googleusercontent.com/media/research.google.com/en//archive/mapreduce-osdi04.pdf),
and make sure to look in the citations at the end. It's **Professor Remzi**!).
One key aspect of MapReduce is that it makes programming such tasks on
large-scale clusters easy for developers; instead of worrying about how to
manage parallelism, handle machine crashes, and many other complexities common
within clusters of machines, the developer can instead just focus on writing
little bits of code (described below) and the infrastructure handles the rest.

This MapReduce is a simplified version of the original paper. The MapReduce
infrastructure supports the execution of user-defined `Map()` and `Reduce()`
functions.

As from the original paper: "Map(), written by the user, takes an input pair and
produces a set of intermediate key/value pairs. The MapReduce library groups
together all intermediate values associated with the same intermediate key K and
passes them to the Reduce() function."

"The Reduce() function, also written by the user, accepts an intermediate key K
and a set of values for that key. It merges together these values to form a
possibly smaller set of values; typically just zero or one output value is
produced per Reduce() invocation. The intermediate values are supplied to the
user's reduce function via an iterator."

## Usage

Users can write their own implementation for `Map()` and `Reduce()` as per
Google paper. In addition, users can write their own `MR_DefaultHashPartition()`
too to better evenly distribute the <key,value> pair across the partitions.

The entire MapReduce is being run by calling on the method (self explanatory)

`void MR_Run(int argc, char *argv[], Mapper map, int num_mappers, Reducer reduce, int num_reducers, Partitioner partition)`

The program can be compiled by doing

`gcc mapreduce.c -o mapreduce -Wall -Werror -pthread -O`

To run the program, do

`./mapreduce <file_1> <file_2> <file_3> ... `

MapReduce can be used for a wide range of application. An example of its
application is in Word Counting. An example code written to use this
infrastructure is as follow.

## Example application: word counting

```cpp
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mapreduce.h"

void Map(char* file_name) {
  FILE* fp = fopen(file_name, "r");
  assert(fp != NULL);

  char* line = NULL;
  size_t size = 0;
  while (getline(&line, &size, fp) != -1) {
    char *token, *dummy = line;
    while ((token = strsep(&dummy, " \t\n\r")) != NULL) {
      MR_Emit(token, "1");
    }
  }
  free(line);
  fclose(fp);
}

void Reduce(char* key, Getter get_next, int partition_number) {
  int count = 0;
  char* value;
  while ((value = get_next(key, partition_number)) != NULL) count++;
  printf("%s %d\n", key, count);
}

int main(int argc, char* argv[]) {
  MR_Run(argc, argv, Map, 10, Reduce, 10, MR_DefaultHashPartition);
}
```

Try running

`./mapreduce test/words.txt`

to see MapReduce in action `Map()` and `Reduce()` for a single file.

## Attribution

Some code from Charles Foo.
