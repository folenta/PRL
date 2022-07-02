/*
 *  PRL - Mesh Multiplication
 *  Author: Jan Folenta (xfolen00)
 *  Date: 3.5.2021
 */


#include <iostream>
#include <mpi.h>
#include <vector>
#include <string>
#include <fstream>
#include <time.h>
#include <sstream>

#define TAG_DIM 1
#define TAG_ROW 2
#define TAG_COL 3
#define TAG_END 4

using namespace std;

// getMatrix parse matrices from input files
vector<int> getMatrix(string inputFile, int *rowsOrCols)
{
    vector<int> matrix;
    string line;
    int inputNumber;
    ifstream matrixFile (inputFile.c_str());

    getline (matrixFile, line);
    *rowsOrCols = stoi(line);

    while (getline(matrixFile, line)) {

        istringstream lineStream(line);
        
        while (lineStream >> inputNumber)
            matrix.push_back(inputNumber);
        
    }

    matrixFile.close();
    return matrix;
}

// checkDimensions checks dimensions of input matrices
int checkDimensions(vector<int> matrix1, int rows, vector<int> matrix2, int cols) 
{
    int colsMat1 = matrix1.size() / rows;
    int rowsMat2 = matrix2.size() / cols;

    if (colsMat1 != rowsMat2) {
        cerr << "Invalid size" << endl;
		MPI_Abort(MPI_COMM_WORLD, 1);
    }

    return colsMat1;   
}

// checkLastCol checks if processor is in the first row of matrix
bool checkFirstRow(int processorRank, int cols)
{
    if (processorRank / cols == 0)
        return true;
    
    else
        return false;
}

// checkLastCol checks if processor is in the first column of matrix
bool checkFirstCol(int processorRank, int cols)
{
    if (processorRank % cols == 0)
        return true;
    
    else
        return false;
}

// checkLastCol checks if processor is in the last row of matrix
bool checkLastRow(int rowIndex, int rows)
{
    if (rowIndex < rows - 1)
        return true;
    else
        return false;
}

// checkLastCol checks if processor is in the last column of matrix
bool checkLastCol(int colIndex, int cols)
{
    if (colIndex < cols - 1)
        return true;
    else
        return false;
}

// printMatrix prints result matrix in required format
void printMatrix(vector<int> matrix, int rows, int cols)
{
    cout << rows << ":" << cols << endl;

    for (int i = 0; i < matrix.size(); i++) {

        cout << matrix[i];
        
        if (((i + 1) % cols) == 0) 
            cout << endl;
        
        else
            cout << " ";
    }
}


int main(int argc, char *argv[])
{
    int numberOfProcessors;
    int processorRank;
    MPI_Status stat;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD,&numberOfProcessors);
    MPI_Comm_rank(MPI_COMM_WORLD,&processorRank);

    vector<int> mat1;
    vector<int> mat2;
    vector<int> resultMatrix;
    vector<int> row;
    vector<int> col;

    int rows;
    int cols;
    int colsMat1_rowsMat2;

    int receivedNumber;
    int sendTo;
    int matrixIndex;
    int receiveFrom;
    int number;

    // If processor rank is 0, processor handles input data and sends information about 
    // dimensions of input matrixes to other processors
    if(processorRank == 0) {
        
        mat1 = getMatrix("mat1", &rows);
        mat2 = getMatrix("mat2", &cols);

        colsMat1_rowsMat2 = checkDimensions(mat1, rows, mat2, cols);

        for(int i = 1; i < numberOfProcessors; i++) {
            
            MPI_Send(&rows, 1, MPI_INT, i, TAG_DIM, MPI_COMM_WORLD);
            MPI_Send(&cols, 1, MPI_INT, i, TAG_DIM, MPI_COMM_WORLD);
            MPI_Send(&colsMat1_rowsMat2, 1, MPI_INT, i, TAG_DIM, MPI_COMM_WORLD);
        }
    }
    
    // Other processors receive information about dimensions of input matrixes
    else {
        
        MPI_Recv(&rows, 1, MPI_INT, 0, TAG_DIM, MPI_COMM_WORLD, &stat);
        MPI_Recv(&cols, 1, MPI_INT, 0, TAG_DIM, MPI_COMM_WORLD, &stat);
        MPI_Recv(&colsMat1_rowsMat2, 1, MPI_INT, 0, TAG_DIM, MPI_COMM_WORLD, &stat);
    }

    // If processor rank is 0, processor sends data to processors in first row and first column
    if(processorRank == 0) {
        
        // Data are sent to processor in first column
        for(int i = 0; i < rows; i++) {
            
            for(int j = 0; j < colsMat1_rowsMat2; j++) {

                matrixIndex = (i * colsMat1_rowsMat2) + j;
                sendTo = i * cols;

                MPI_Send(&mat1[matrixIndex], 1, MPI_INT, sendTo, TAG_ROW, MPI_COMM_WORLD);
            }
        }

        // Data are sent to processors in first row
        for(int j = 0; j < cols; j++) {

            for(int i = 0; i < colsMat1_rowsMat2; i++) {

                matrixIndex = (i * cols) + j;
                sendTo = j;

                MPI_Send(&mat2[matrixIndex], 1, MPI_INT, sendTo, TAG_COL, MPI_COMM_WORLD);
            }
        }
    }

    bool firstRow = checkFirstRow(processorRank, cols);
    bool firstCol = checkFirstCol(processorRank, cols);

    // Processors in first row receive data
    if(firstRow) {

        for(int i = 0; i < colsMat1_rowsMat2; i++) {

            MPI_Recv(&receivedNumber, 1, MPI_INT, 0, TAG_COL, MPI_COMM_WORLD, &stat);

            col.push_back(receivedNumber);
        }
    }

    // Processors in first column receive data
    if(firstCol) {

        for(int j = 0; j < colsMat1_rowsMat2; j++) {

            MPI_Recv(&receivedNumber, 1, MPI_INT, 0, TAG_ROW, MPI_COMM_WORLD, &stat);

            row.push_back(receivedNumber);
        }
    }

    int topValue = 0;
    int leftValue = 0;
    int newValue = 0;

    int rowIndex = processorRank / cols;
    int colIndex = processorRank % cols;
    bool notLastRow = checkLastRow(rowIndex, rows);
    bool notLastCol = checkLastCol(colIndex, cols);

    // New values are computed
    for(int i = 0; i < colsMat1_rowsMat2; i++) {

        if(firstRow) {
            topValue = col[0];
            col.erase(col.begin());
        }

        else {
            receiveFrom = ((rowIndex - 1) * cols) + colIndex;
            
            MPI_Recv(&topValue, 1, MPI_INT, receiveFrom, TAG_COL, MPI_COMM_WORLD, &stat);
        }

        if(firstCol) {
            leftValue = row[0];
            row.erase(row.begin());
        }

        else {
            receiveFrom = (rowIndex * cols) + colIndex - 1;

            MPI_Recv(&leftValue, 1, MPI_INT, receiveFrom, TAG_ROW, MPI_COMM_WORLD, &stat);
        }

        newValue = newValue + (topValue * leftValue); 

        if(notLastRow) {
            sendTo = ((rowIndex + 1) * cols) + colIndex;

            MPI_Send(&topValue, 1, MPI_INT, sendTo, TAG_COL, MPI_COMM_WORLD);
        }

        if(notLastCol) {
            sendTo = (rowIndex * cols) + colIndex + 1;

            MPI_Send(&leftValue, 1, MPI_INT, sendTo, TAG_ROW, MPI_COMM_WORLD);
        }
    }

    // All processors apart of processor with rank 0 send data to processor 0
    if(processorRank != 0)
        MPI_Send(&newValue, 1, MPI_INT, 0, TAG_END, MPI_COMM_WORLD);

    // If processor rank is 0, processor receive data from other processors and prints result matrix
    else {

        resultMatrix.push_back(newValue);

        for(int i = 1; i < numberOfProcessors; i++) {
            MPI_Recv(&receivedNumber, 1, MPI_INT, i, TAG_END, MPI_COMM_WORLD, &stat);
            
            resultMatrix.push_back(receivedNumber);
        }

        printMatrix(resultMatrix, rows, cols);
    }

    MPI_Finalize();
    return 0;
}