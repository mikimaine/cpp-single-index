#include <fstream>
#include <vector>
#include <iostream>
#include <cstring>

// using namespace std;

struct IndexEntry {
    std::string key;
    std::streamoff offset;
};

std::vector<IndexEntry> indexEntries;

// Function prototypes
void createIndex(const std::string& dataFilename, const std::string& indexFilename, size_t keyLength);
void listRecords(const std::string& dataFilename, const std::string& indexFilename, size_t keyLength);
void searchForKey(const std::string& dataFilename, const std::string& indexFilename, const std::string& key, size_t keyLength);
void checkOrCreateIndexFile(const std::string& indexFilename);
void createIndex_InMemorySort(const std::string& dataFilename, const std::string& indexFilename, size_t keyLength);

int main(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0] << " -c|-l|-s datafile indexfile keylength [key]" << std::endl;
        return 1;
    }

    std::string mode = argv[1];
    std::string dataFilename = argv[2];
    std::string indexFilename = argv[3];
    size_t keyLength = static_cast<size_t>(std::atoi(argv[4]));

    // Attempt to open the index file
    checkOrCreateIndexFile(indexFilename);


    if (mode == "-c") {
        createIndex(dataFilename, indexFilename, keyLength);
    } else if (mode == "-l") {
        listRecords(dataFilename, indexFilename, keyLength);
    } else if (mode == "-s") {
        if (argc != 6) {
            std::cerr << "Usage: " << argv[0] << " -s datafile indexfile keylength key" << std::endl;
            return 1;
        }
        std::string key = argv[5];
        searchForKey(dataFilename, indexFilename, key, keyLength);
    } else {
        std::cerr << "Invalid mode. Use -c to create index, -l to list records, or -s to search for a key." << std::endl;
        return 1;
    }

    return 0;
}

bool compareIndexEntries(const IndexEntry& a, const IndexEntry& b) {
    return a.key < b.key;
}

// Attempt to open the index file
void checkOrCreateIndexFile(const std::string& indexFilename) {
    std::ifstream ifile(indexFilename.c_str(), std::ios::binary);
    bool indexExists = ifile.good();
    ifile.close();
}

/**
 * Create an index file for the data file.
 * The index file contains entries with fixed-length keys and 8-byte offsets to the corresponding records in the data file.
 * The keys are extracted from the beginning of each record in the data file.
 * 
 * Sorting is implemented on the temporary index file to create the final index.
 * 
*/
void createIndex(const std::string& dataFilename, const std::string& indexFilename, size_t keyLength) {
    std::ifstream dataFile(dataFilename, std::ifstream::binary);
    if (!dataFile) {
        std::cerr << "Error opening data file for reading." << std::endl;
        return;
    }

    // Temporary index file to store unsorted index entries
    std::string tempIndexFilename = indexFilename + ".tmp";

    checkOrCreateIndexFile(tempIndexFilename);
    std::ofstream tempIndexFile(tempIndexFilename, std::ofstream::binary);
    if (!tempIndexFile) {
        std::cerr << "Error opening temporary index file for writing." << std::endl;
        return;
    }

    std::string line;
    std::streamoff offset = 0;
    while (getline(dataFile, line)) {
        if (line.length() >= keyLength) {
            std::string key = line.substr(0, keyLength);
            tempIndexFile.write(key.c_str(), keyLength);
            tempIndexFile.write(reinterpret_cast<const char*>(&offset), sizeof(offset));
        }
        // offset += line.length() + 1; // +1 for the newline character
        offset = dataFile.tellg();
    }

    // Close both files
    dataFile.close();
    tempIndexFile.close();

    // Now sort the temporary index file to create the final index file
    // Load the temporary index file into memory for sorting
    std::ifstream tempIndexFileIn(tempIndexFilename, std::ifstream::binary);
    std::vector<IndexEntry> indexEntries;

    while (tempIndexFileIn.good()) {
        IndexEntry entry;
        entry.key.resize(keyLength);
        tempIndexFileIn.read(&entry.key[0], keyLength);
        tempIndexFileIn.read(reinterpret_cast<char*>(&entry.offset), sizeof(entry.offset));
        if (tempIndexFileIn.gcount() == 0) {
            break;  // End of file or error
        }
        indexEntries.push_back(entry);
    }
    tempIndexFileIn.close();

    // Sort the index entries
    std::sort(indexEntries.begin(), indexEntries.end(), compareIndexEntries);

    // Write the sorted entries to the final index file
    std::ofstream indexFile(indexFilename, std::ofstream::binary);
    for (const auto& entry : indexEntries) {
        indexFile.write(entry.key.c_str(), keyLength);
        indexFile.write(reinterpret_cast<const char*>(&entry.offset), sizeof(entry.offset));
    }
    indexFile.close();

    // Optionally, delete the temporary index file
    std::remove(tempIndexFilename.c_str());
}

/**
 * Create an index file for the data file.
 * The index file contains entries with fixed-length keys and 8-byte offsets to the corresponding records in the data file.
 * The keys are extracted from the beginning of each record in the data file.
 * 
 * Sorting approach: In-memory sort using a vector of IndexEntry objects.
*/
void createIndexInMemorySort(const std::string& dataFilename, const std::string& indexFilename, size_t keyLength) {
    std::ifstream dataFile(dataFilename, std::ifstream::binary);
    if (!dataFile) {
        std::cerr << "Error opening data file for reading." << std::endl;
        return;
    }

    std::vector<IndexEntry> indexEntries;
    std::string line;
    std::streamoff offset = 0;

    while (getline(dataFile, line)) {
        if (line.length() >= keyLength) {
            // Extract key of specified length
            std::string key = line.substr(0, keyLength);
            // Store key and offset
            indexEntries.push_back(IndexEntry{key, offset});
        }
        // Update offset with the length of the line plus newline character
        offset += line.length() + 1;
    }

    // Close data file
    dataFile.close();

    // Sort indexEntries by key
    std::sort(indexEntries.begin(), indexEntries.end(), compareIndexEntries);

    // Open index file for writing in binary mode
    std::ofstream indexFile(indexFilename, std::ofstream::binary);
    if (!indexFile) {
        std::cerr << "Error opening index file for writing." << std::endl;
        return;
    }

    // Write each IndexEntry to the index file
    for (const auto& entry : indexEntries) {
        indexFile.write(entry.key.c_str(), keyLength);
        indexFile.write(reinterpret_cast<const char*>(&entry.offset), sizeof(entry.offset));
    }

    // Close index file
    indexFile.close();
}

/**
 * List records from the data file using the index file.
 * The index file contains entries with fixed-length keys and 8-byte offsets to the corresponding records in the data file.
 * The keys are extracted from the beginning of each record in the data file.
 * 
*/
void listRecords(const std::string& dataFilename, const std::string& indexFilename, size_t keyLength) {
    // Open index file for reading in binary mode
    std::ifstream indexFile(indexFilename, std::ifstream::binary);
    if (!indexFile) {
        std::cerr << "Error opening index file for reading." << std::endl;
        return;
    }

    // Open data file for reading
    std::ifstream dataFile(dataFilename);
    if (!dataFile) {
        std::cerr << "Error opening data file for reading." << std::endl;
        return;
    }

    while (indexFile.good()) {
        char* buffer = new char[keyLength];
        indexFile.read(buffer, keyLength);
        if (indexFile.gcount() < static_cast<std::streamsize>(keyLength)) {
            delete[] buffer;
            break;  // End of file or error
        }

        std::streamoff offset;
        indexFile.read(reinterpret_cast<char*>(&offset), sizeof(offset));
        if (!indexFile.good()) {
            delete[] buffer;
            break;  // End of file or error
        }

        // Seek in the data file and read the record
        dataFile.seekg(offset);
        std::string record;
        getline(dataFile, record);

        // Print the record
        std::cout << record << std::endl;

        delete[] buffer;
    }

    // Close the files
    indexFile.close();
    dataFile.close();
}

/**
 * Search for a record by key in the index file.
 * The index file contains entries with fixed-length keys and 8-byte offsets to the corresponding records in the data file.
 * The keys are extracted from the beginning of each record in the data file.
 * 
*/
void searchForKey(const std::string& dataFilename, const std::string& indexFilename, const std::string& key, size_t keyLength) {
    // Open index file for reading in binary mode
    std::ifstream indexFile(indexFilename, std::ifstream::binary);
    if (!indexFile) {
        std::cerr << "Error opening index file for reading." << std::endl;
        return;
    }

    // Open data file for reading
    std::ifstream dataFile(dataFilename);
    if (!dataFile) {
        std::cerr << "Error opening data file for reading." << std::endl;
        return;
    }

    // Get the size of the index file
    indexFile.seekg(0, std::ios::end);

    std::streamoff fileSize = indexFile.tellg();

    indexFile.seekg(0, std::ios::beg);

    /**
     * Calculate the number of records in the index file
     * 
     * sizeof(std::streamoff) is 4 bytes (32 bits) or 8 bytes (64 bits) depending on the system architecture.
     * 
     * For example, if the file size is 100 bytes,
     * the key length is 10 bytes, and the size of a std::streamoff object is 8 bytes,
     * then the number of records in the file would be calculated as follows:
     * numRecords = 100 / (10 + 8) = 100 / 18 = 5 records in the index file
    */
    size_t numRecords = fileSize / (keyLength + sizeof(std::streamoff));
    
    // Perform a binary search for the key
    size_t low = 0;
    size_t high = numRecords;
    bool found = false;
    std::streamoff recordOffset = 0;
    
    while (low < high) {  // Loop until the search range is narrowed down
        size_t mid = low + (high - low) / 2;  // Calculate the middle index to avoid overflow

        /**
        * Seek to the position of the key(mid) in the index file
        * On most 32-bit systems, sizeof(std::streamoff) is 4 bytes (32 bits). 
        * On 64-bit systems, it is 8 bytes (64 bits).
        * This size allows std::streamoff to represent large file offsets,
        * necessary for working with very large files.
        * 
        * For example, if the index file contains 10 records,
        * the key length is 10 bytes, and the size of a std::streamoff object is 8 bytes,
        * then the offset to the middle record would be calculated as follows:
        * 
        * - mid * (keyLength + sizeof(std::streamoff)) = 5 * (10 + 8) = 90 bytes
        * 
        * The seekg() function then seeks to the specified offset in the index file.
        * This allows the program to read the middle record from the file.
        */
        indexFile.seekg(mid * (keyLength + sizeof(std::streamoff)));

        // Allocate memory for reading the key from the file
        char* buffer = new char[keyLength];
        // Read the key from the file into the buffer
        indexFile.read(buffer, keyLength);
        // Create a string from the buffer to compare with the search key
        std::string currentKey(buffer, keyLength);
        // Deallocate the buffer memory
        delete[] buffer;

        // Compare the current key with the search key to determine the next step
        if (currentKey < key) {
            // If currentKey is less than the search key, search in the upper half
            low = mid + 1;
        } else if (currentKey > key) {
            // If currentKey is greater than the search key, search in the lower half
            high = mid;
        } else {
            // If currentKey equals the search key, the record is found
            found = true;
            // Read the associated record offset following the key
            /**
             * Read the associated record offset following the key
             * The sizeof() operator is used to determine the size of the variable recordOffset. 
             * This is necessary because the read() function needs to know how many bytes to read from the file.
             * 
             * read the value of the variable recordOffset from the file indexFile and store it 
             * in the variable recordOffset. This will be used for reading data from a binary file
            */
            indexFile.read(reinterpret_cast<char*>(&recordOffset), sizeof(recordOffset));
            break;  // Exit the loop as the target record is found
        }
    }


    // If found, seek in the data file and read the record
    if (found) {
        dataFile.seekg(recordOffset, std::ios::beg);
        std::string record;
        std::getline(dataFile, record);
        std::cout << record << std::endl;
    } else {
        std::cout << "Record not found" << std::endl;
    }

    // Close the files
    indexFile.close();
    dataFile.close();
}

