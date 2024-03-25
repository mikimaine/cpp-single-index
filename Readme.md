# Indexer Program

The Indexer Program is a command-line utility designed to create, manage, and search through an index of a text file. This program allows users to efficiently access records in a large text file by using an index file that stores the offsets of records within the main data file.

## Features

- **Create Index:** Generates an index file from a specified text file, using a defined key length to identify unique records.
- **List Records:** Displays all records in the text file in the order they appear in the index, allowing for sorted output.
- **Search Key:** Quickly retrieves and displays a record from the text file using a key to search the index file.

## Requirements

- C++ compiler (C++11 or later)
- Make sure the compiler supports C++11 standard or later for proper functionality.

## Compilation

To compile the program, use the following command:

```sh
g++ -std=c++11 -o Indexer main.cpp IndexProgram.cpp
```

This command will generate an executable named `Indexer`.

## Usage

The program operates in three modes: create, list, and search.

### Creating an Index

To create an index file, use the `-c` option followed by the data file name, index file name, and key length:

```
./Indexer -c data.txt index.idx 4
```

This will read `data.txt`, create an index based on the first 4 characters of each line, and save it to `index.idx`.

### Listing Records

To list all records sorted according to the index file, use the `-l` option:

```
./Indexer -l data.txt index.idx 4
```

This will display the contents of `data.txt` in the order specified by `index.idx`.

### Searching for a Key

To search for a specific key, use the `-s` option followed by the key you are looking for:

```
./Indexer -s data.txt index.idx 4 ABCD
```

This will search `index.idx` for the key `ABCD` and display the corresponding record from `data.txt`.

## File Format

The data file should be a plain text file with each record on a separate line. The key used for indexing should be at the start of each line.

## Limitations

- The program currently does not support keys containing newline characters.
- Records in the data file must end with a newline character.