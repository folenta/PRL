#include <mpi.h>
#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

#define TAG 0

// Function finds if processor can accept number into his queue
bool fillQueues(int cycle, int processorRank, int numberOfProcessors)
{
    int firstFillingCycle = (1 << (processorRank - 1)) + processorRank - 2;
    int lastFillingCycle;

    if (processorRank == 1)
        lastFillingCycle = 1 << (numberOfProcessors - 1);

    else
        lastFillingCycle = (1 << (numberOfProcessors - 1)) - 1 +
                           (1 << (processorRank - 1)) + processorRank - 1;

    if (cycle >= firstFillingCycle && cycle < lastFillingCycle)
        return true;
    else
        return false;
}

// Function finds if processor can compare numbers and send number to next processor
bool processNumbers(int cycle, int processorRank, int numberOfProcessors)
{
    int firstProcessingCycle = (1 << processorRank) + processorRank - 1;
    int lastProcessingCycle = (1 << (numberOfProcessors - 1)) - 1 + (1 << processorRank) + processorRank;

    if (cycle >= firstProcessingCycle && cycle < lastProcessingCycle)
        return true;
    else
        return false;
}

// Function gets first number in vector ale erase that number from vector
tuple<int, vector<int>> getNumber(vector<int> queue)
{
    int number = queue.front();
    queue.erase(queue.begin());

    return make_tuple(number, queue);
}

int main(int argc, char *argv[])
{
    int numberOfProcessors;
    int processorRank;
    int neighborNumber;
    int mynumber;
    MPI_Status stat;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numberOfProcessors);
    MPI_Comm_rank(MPI_COMM_WORLD, &processorRank);

    vector<int> numbers;
    vector<int> queue1;
    vector<int> queue2;

    bool fillFirstQueue = true;
    int leftToProcessInQ1 = 1 << (processorRank - 1);
    int leftToProcessInQ2 = 1 << (processorRank - 1);

    int changeRate = 1 << processorRank;
    int fillingCycles = 0;
    int numberToPrint;

    int totalCycles = 2 * (1 << (numberOfProcessors - 1)) + numberOfProcessors - 1 - 1;

    if (processorRank == 0)
    {
        char input[] = "numbers";
        int number;
        bool first = true;
        fstream fin;
        fin.open(input, ios::in);

        while (fin.good()) {
            number = fin.get();

            if (!fin.good())
                break;

            numbers.push_back(number);

            if (first){
                cout << number;
                first = false;
            }

            else
                cout << " " << number;
        }

        cout << endl;
        fin.close();
    }

    for (int cycle = 0; cycle <= totalCycles; cycle++) {
        
        if (processorRank == 0) {
            
            if (!numbers.empty()) {
                tie(mynumber, numbers) = getNumber(numbers);
                MPI_Send(&mynumber, 1, MPI_INT, processorRank + 1, TAG, MPI_COMM_WORLD);
            }
        }

        else
        {
            if (fillQueues(cycle, processorRank, numberOfProcessors)) {
                MPI_Recv(&neighborNumber, 1, MPI_INT, processorRank - 1, TAG, MPI_COMM_WORLD, &stat);

                if (fillingCycles % changeRate < (1 << (processorRank - 1)))
                    fillFirstQueue = true;

                else
                    fillFirstQueue = false;

                fillingCycles++;

                if (fillFirstQueue)
                    queue1.push_back(neighborNumber);
                
                else
                    queue2.push_back(neighborNumber);   
            }
            
            if (processNumbers(cycle, processorRank, numberOfProcessors)) {
                
                if (processorRank == (numberOfProcessors - 1)) {
                    
                    if (!queue1.empty() && !queue2.empty()) {
                        
                        if (queue1.front() <= queue2.front())
                            tie(numberToPrint, queue1) = getNumber(queue1);

                        else
                            tie(numberToPrint, queue2) = getNumber(queue2);
                    }

                    else if (queue1.empty())
                        tie(numberToPrint, queue2) = getNumber(queue2);

                    else
                        tie(numberToPrint, queue1) = getNumber(queue1);

                    cout << numberToPrint << endl;
                }
                
                else
                {
                    if ((leftToProcessInQ1 == 0)) {
                        tie(mynumber, queue2) = getNumber(queue2);
                        leftToProcessInQ2--;
                    }
                        
                    else if (leftToProcessInQ2 == 0) {
                        tie(mynumber, queue1) = getNumber(queue1);
                        leftToProcessInQ1--;
                    }
                    
                    else {
                        
                        if (queue1.front() <= queue2.front()) {
                            tie(mynumber, queue1) = getNumber(queue1);
                            leftToProcessInQ1--;
                        }

                        else {
                            tie(mynumber, queue2) = getNumber(queue2);
                            leftToProcessInQ2--;
                        }
                    }

                    MPI_Send(&mynumber, 1, MPI_INT, processorRank + 1, TAG, MPI_COMM_WORLD);

                    if (leftToProcessInQ1 == 0 && leftToProcessInQ2 == 0) {
                        leftToProcessInQ1 = 1 << (processorRank - 1);
                        leftToProcessInQ2 = 1 << (processorRank - 1);
                    }
                }
            }
        }
    }

    MPI_Finalize();
    return 0;
}