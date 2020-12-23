#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>

#define THREADS 11

/* structure for passing data to threads */
typedef struct
{
    int row;
    int column;
} parameters;

// newParameter creates a new paramter to pass to threads
parameters* newParameter(int r, int c) {
	parameters *retVal = malloc(sizeof(parameters));
	retVal->row = r;
	retVal->column = c;
	return retVal;
}

// sudoku grid
int sudoku[9][9] = {
	{6, 2, 4, 5, 3, 9, 1, 8, 7},
	{5, 1, 9, 7, 2, 8, 6, 3, 4},
	{8, 3, 7, 6, 1, 4, 2, 9, 5},
	{1, 4, 3, 8, 6, 5, 7, 2, 9},
	{9, 5, 8, 2, 4, 7, 3, 6, 1},
	{7, 6, 2, 3, 9, 1, 4, 5, 8},
	{3, 7, 1, 9, 5, 6, 8, 4, 2},
	{4, 9, 6, 1, 8, 2, 5, 7, 3},
	{2, 8, 5, 4, 7, 3, 9, 1, 6}
};

int results[THREADS] = {0}; // results of validator. 0 means invalid, all 1's means valid.
pthread_t threads[THREADS]; // 11 threads. 1 for rows, 1 for cols, 9 for each subgrid.

// printGrid prints sudoku
void printGrid() {
	for (int row = 0; row < 9; row++) { // loop over each row and col
		for (int col = 0; col < 9; col++) {
			char num[2];
			sprintf(num, "%d", sudoku[row][col]); // convert int to string
			printf("%s%s", num, " ");
		}
		printf("\n"); // new line when done with row
	}
}

// initResults initializes results to 0
void initResults() {
	for (int i = 0; i < THREADS; i++) {
		results[i] = 0;
	}
}

// declaration for 3x3 square function.
void *checkGrid(void *grid);

// declaration for the checkRows function.
void *checkRows(void *row);

// declaration for the checkColumns function.
void *checkColumns(void *col);

// declaration  
int validateSolution();

// main method validates a correct solution and an incorrect solution
int main(void) {
	printf("Validating correct grid: \n");
	printGrid();
	printf("Result: ");
	validateSolution();

	initResults(); // reset results
	sudoku[0][0] = 7; // create invalid solution
	printf("\n\n");

	printf("Validating incorrect grid: \n");
	printGrid();
	printf("Result: ");
	validateSolution();
	exit(EXIT_SUCCESS);
}

// checkGrid validates one of nine subgrids
// sets an index between 0 and 8 of results to 1 if valid
// otherwise returns without changing results
void *checkGrid(void *grid) {
	parameters g = *(parameters*)grid;
	int row = g.row; // get row and column from paramter
	int col = g.column;
	int valid[9] = {0}; // int set for each box in subgrid

	for (int r = row; r < row + 3; r++) { // loop over subgrid rows
		for (int c = col; c < col + 3; c++) { // loop over subgrid columns
			if (valid[sudoku[r][c] - 1] == 1) { // if int set has current, then invalid
				return NULL;
			}
			else {
				valid[sudoku[r][c] - 1] = 1; // add current to int set
			}
		}
	}

	results[row + (col / 3)] = 1; // valid
	return NULL;
}

// checkRows checks each row
// sets index 9 of results to 1 if valid
// otherwise returns without doing anything
void *checkRows(void *row) {
	for (int r = 0; r < 9; r++) { // loop over rows
		int row[9] = {0}; // int set
		for (int i = 0; i < 9; i++) { // looo over each item in row
			if (row[sudoku[r][i] - 1] == 1) { // if int set has current item, invalid
				return NULL;
			}
			else {
				row[sudoku[r][i] - 1] = 1; // add item to int set
			}
		}
	}

	results[9] = 1; // valid
	return NULL;
}

// checkColumns checks each column
// sets index 10 of results to 1 if valid
void *checkColumns(void *col) {
	for (int c = 0; c < 9; c++) { // loop over columns
		int col[9] = {0};
		for (int i = 0; i < 9; i++) { // loop over every item in column
			if (col[sudoku[i][c] - 1] == 1) { // if int set has current item, invalid
				return NULL;
			}
			else {
				col[sudoku[i][c] - 1] = 1; // add item to int set
			}
		}
	}

	results[10] = 1; // valid
	return NULL;
}

// validateSolution validates the sudoku grid
// prints valid if the solution is correct and invalid otherwise
int validateSolution() {
	if (pthread_create(&threads[9], NULL, checkRows, NULL) != 0) { // 10th thread checks each row
		printf("Unable to create thread: rows.");
		exit(EXIT_FAILURE);
	}

	if (pthread_create(&threads[10], NULL, checkColumns, NULL) != 0) { // 11th thread checks each column
		printf("Unable to create thread: columns.");
		exit(EXIT_FAILURE);
	}

	for (int r = 0; r < 7; r += 3) { // loop over every other third row (first, fourth, seventh)
		for (int c = 0; c < 7; c+= 3) { // loop over every other third column
			// threads 0-8 checks each subgrid
			if (pthread_create(&threads[r + (c / 3)], NULL, checkGrid, (void *) newParameter(r, c)) != 0) {
				printf("Unable to create thread: grid.");
				exit(EXIT_FAILURE);
			}
		}
	}

	for (int i = 0; i < THREADS; i++) {
		pthread_join(threads[i], NULL); // wait for each thread to finish
	}


	int success = 1; // valid

	for (int i = 0; i < THREADS; i++) {
		if (results[i] != 1) { // if results has 0
			success = 0; // invalid
			break;
		}
	}


	if (success) {
		printf("Valid\n");
	}
	else {
		printf("Invalid\n");
	}
}