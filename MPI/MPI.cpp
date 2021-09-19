#include <iostream>
#include <omp.h>
#include <mpi.h>
#include <assert.h>
#include <iomanip>
using namespace std;
//================
// MPI
//================
int main(int argc, char** argv) {

    int N = 300000000;  // Limit for finding prime numbers
    int sqrtN = 0;  // sqrtN used to eliminate multiples of it
    int c = 0; // used in for loop (sqrtN)
    int m = 0; // used in for loop (mark array)
    char* list1; // list one uses of 2 to sqrtN, used to eliminate multiples
    char* list2; // list two consist of the remaining numbers
    int splitSize = 0; // split size in array list2
    int remainders = 0; // remainders that are going to be assigned to the last processes
    int lowestValue = 0; // lowest value in list2 of different processes
    int highestValue = 0; // highest value in list2 of different processes
    int rank = 0; // processes
    int size = 0; // rank size
    double start_time;
    double run_time;
    int found = 0; // total number of prime number found
    int error;

    //Initialize the MPI Environment
    MPI_Init(&argc, &argv);

    // Determine the rank of the current process and the number of processes 
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);


    // Calculate sqrtN 
    sqrtN = (int)sqrt(N);

    // Calculate splitSize, remainders, lowestValue, and highestValue 
    // Split size for different processes 
    splitSize = (N - (sqrtN + 1)) / size;

    // Remainder size for last processes
    remainders = (N - (sqrtN + 1)) % size;

    // Lowest value in each split
    lowestValue = sqrtN + rank * splitSize + 1;

    // Highest value in each split
    highestValue = lowestValue + splitSize - 1;

    // Finally until the last processes,it will be responsible for the remainders (add into last rank)
    if (rank == size - 1) {
        highestValue += remainders;
    }

    // Allocate memory for lists
    list1 = (char*)malloc((sqrtN + 1) * sizeof(char));
    list2 = (char*)malloc((highestValue - lowestValue + 1) * sizeof(char));

    // Exit if malloc failed 
    if (list1 == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for list1.\n");
        exit(-1);
    }
    if (list2 == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for list2.\n");
        exit(-1);
    }

    start_time = omp_get_wtime();

    //Initialize array with 1 
    //Assume all is one first 
    //list1 is	designated	for	the	numbers 2 through sqrtN
    for (c = 2; c <= sqrtN; c++) {
        list1[c] = 1;
    }

    //list2 contains a section	of	the	remaining	numbers. 
    //Different processes will run the program once, each has different lowestValue and highestValue
    for (c = lowestValue; c <= highestValue; c++) {
        list2[c - lowestValue] = 1;
    }

    // Run through each number in list1 
    for (c = 2; c <= sqrtN; c++) {
        //If the number is marked 
        if (list1[c]) {
            // Run through the multiples
            for (m = c + c; m <= sqrtN; m += c) {
                // If m is a multiple of c, mark as 0
                list1[m] = 0;
            }
            int temp = lowestValue;
            // Run through each number bigger than c in list2 
            // While temporary value is not divisible, increment the value
            // So that it will not eliminate the wrong value in list2
            while (temp % c != 0) {
                temp++;
            }

            // Start eliminate value in list 2
            for (m = temp; m <= highestValue; m += c)
            {
                list2[m - lowestValue] = 0;
            }
        }
    }


    //If Rank 0 is the current process 
    if (rank == 0) {
        // Run through each of the numbers in list1 
        for (c = 2; c <= sqrtN; c++) {
            // If the number is does not mark as not prime 
            if (list1[c] == 1) {
                // The number is prime, print it 
               // printf("%d ", c);
                found++;
            }
        }

        // Run through each of the numbers in list2 
        for (c = lowestValue; c <= highestValue; c++) {
            // If the number is does not mark as not prime 
            if (list2[c - lowestValue] == 1) {
                // The number is prime, print it 
               // printf("%d ", c);
                found++;
            }
        }

        // Run through each of the other processes
        for (rank = 1; rank <= size - 1; rank++) {

            //Calculate lowestValue and highestValue for ranks
            lowestValue = sqrtN + rank * splitSize + 1;
            highestValue = lowestValue + splitSize - 1;

            // Assign remianders to last processes
            if (rank == size - 1) {
                highestValue += remainders;
            }

            // Receive list2 from processes other than 1 
            MPI_Recv(list2, highestValue - lowestValue + 1, MPI_CHAR, rank, 0, MPI_COMM_WORLD,
                MPI_STATUS_IGNORE);

            // Run through the list2 that was just received 
            for (c = lowestValue; c <= highestValue; c++) {

                // If the number is unmarked 
                if (list2[c - lowestValue] == 1) {
                    // The number is prime, print it 
                   // printf("%d ", c);
                    found++;
                }
            }
        }

        // Send the value found to rank one for printing
        MPI_Send(&found, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
        cout << "\n";

        // If the process is not Rank 0 
    }
    else {

        // Send list2 to Rank 0 
        MPI_Send(list2, highestValue - lowestValue + 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    }

    run_time = omp_get_wtime() - start_time;

    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 1) {
        // Receive the value found for printing
        MPI_Recv(&found, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        cout << "\nTotal elapsed time " << fixed << setprecision(6) << run_time << " seconds \n";
        cout << "There are total of " << found << " prime number found within " << N << "\n\n";
    }
    /* Deallocate memory for list */
    if (rank == 0) {
        free(list2);
        free(list1);
    }

    //       MPI_Barrier(MPI_COMM_WORLD);
    /* Finalize the MPI environment */
    error = MPI_Finalize();

    return 0;
}


