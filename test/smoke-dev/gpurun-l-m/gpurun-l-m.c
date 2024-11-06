#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main(int argc, char* argv[]) {
    // Check for the correct number of command-line arguments
    if (argc != 2) {
        printf("Usage: %s <number of bytes>\n", argv[0]);
        return -1;
    }

    // Convert the command-line argument to a long integer
    long int numBytes = strtol(argv[1], NULL, 10);
    if (numBytes <= 0) {
        printf("Please enter a positive number of bytes.\n");
        return -1;
    }

    // Calculate the number of doubles that can be allocated
    long int numDoubles = numBytes / sizeof(double);
    if (numDoubles == 0) {
        printf("Not enough bytes to allocate a double array.\n");
        return -1;
    }

    // Allocate memory for the double array
    double* array = (double*)malloc(numDoubles * sizeof(double));
    if (array == NULL) {
        printf("Memory allocation failed.\n");
        return -1; // Malloc failed
    }

    // Fill the array with their associated indexes using OpenMP offloading
    #pragma omp target data map(tofrom: array[0:numDoubles])
    {
        {
            #pragma omp target teams distribute parallel for
            for (long int i = 0; i < numDoubles; i++) {
                array[i] = (double)i; // Assign index values
            }
        }
    }

    // Check the array for correctness using OpenMP offloading
    int isCorrect = 1; // Flag to check correctness
    #pragma omp target data map(to: array[0:numDoubles]) map(from: isCorrect)
    {
        {
            #pragma omp target teams distribute parallel for
            for (long int i = 0; i < numDoubles; i++) {
                if (array[i] != (double)i) {
                    isCorrect = 0; // Data is incorrect
                }
            }
        }
    }

    // Print results
    if (isCorrect) {
        printf("Array successfully allocated, assigned, and checked.\n");
    } else {
        printf("Array check failed: data is incorrect.\n");
    }

    // Free the allocated memory
    free(array);
    return 0; // Success
}

