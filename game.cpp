#include <stdio.h>		// for printf
#include <stdlib.h>		// for malloc
#include <string.h>		// for memcpy
#include <iostream>		// for CPP
#include <queue>		// for priority queue
using namespace std;

struct state
{
	//Call(int *board, int dim, int moveSoFar, int lowerBound) :
    //CallBoard(board), CallDim(dim), CallLowerBound(lowerBound) {}

	int *board;
	int dim;
	int moveSoFar;
	int lowerBound;

    //use friend so we can compare the two states
	friend bool operator<(const state& lhs, const state& rhs)
	{
		return lhs.lowerBound > rhs.lowerBound;
	}
};

//struct Comp{
//	friend bool operator<(const state& lhs, const state& rhs)
//	{
//		return lhs.lowerBound > rhs.lowerBound;
//	}
//};


void shuffleBoard(int *array, int n)
{
	if (n > 1){ 
		int i;
		for (i = 0; i < n - 1; i++){
			int j = i + rand() / (RAND_MAX / (n - i) + 1);
			int t = array[j];
			array[j] = array[i];
			array[i] = t;
		}
	}
}

void fillBoard(int *board, int dim)
{
	int i;
	for(i=0; i<(dim*dim); i++){
		board[i] = i;
	} 
}

void printBoard(int *board, int dim)
{
	int i, j;
	for(i=0; i<dim; i++){
		for(j=0; j<dim; j++){
			printf("%d\t", board[dim*i+j]);
		}
		printf("\n");
	}
}

int getManhattan(int i, int dim, int sourceRow, int sourceCol)
{
	int targetRow = (i-1) / dim;
	int targetCol = (i-1) % dim;
	return abs(sourceRow - targetRow)+abs(sourceCol - targetCol);
}

int getBoardManhattan(int *board, int dim)
{
	int i, j, sum=0, temp, item;
	for(i=0; i<dim; i++){
		for(j=0; j<dim; j++){
            item = board[dim*i+j];
            if(item != 0){
                temp = getManhattan(item, dim, i, j);
			    sum = sum + temp;
            }
		}
	}
	return sum;
}

bool checkResult(int *board, int dim)
{
	int i, j;
	bool correct = true;
	for(i=0; i<dim; i++){
		for(j=0; j<dim; j++){
			if(i != dim-1 && j != dim-1){
				if(board[dim*i+j] != dim*i+j+1){
					correct = false;
				}
			}
		}
	}
	return correct;
}

void setState(state *newMove, int *board, int dim, int moveSoFar)
{
	newMove->board = board;
	newMove->dim = dim;
	newMove->moveSoFar = moveSoFar;
	newMove->lowerBound = moveSoFar + getBoardManhattan(board, dim);
}

void freeMove(state *Move)
{
	free(Move->board);
	free(Move);
}

int* moveHole(int direction, int *board, int dim)
{
	int holeRow, holeCol, temp, i;
	// create a new board same as the old one
	int *newBoard;
	size_t size = dim * dim * sizeof(int);
	newBoard = (int*)malloc(size);
	memcpy(newBoard, board, size);
	// Find out the location of the hole
	for(i=0; i<dim*dim; i++){
		if(newBoard[i]==0){
			holeRow = i / dim;
			holeCol = i % dim;
		}
	}
	// make the move
	if(direction == 0)
	{
		// 0 is move up
		temp = newBoard[(holeRow-1)*dim+holeCol];
		newBoard[(holeRow-1)*dim+holeCol] = 0;
		newBoard[holeRow*dim+holeCol] = temp;
	}
	else if(direction == 1)
	{
		// 1 is move down
		temp = newBoard[(holeRow+1)*dim+holeCol];
		newBoard[(holeRow+1)*dim+holeCol] = 0;
		newBoard[holeRow*dim+holeCol] = temp;
	}
	else if(direction == 2)
	{
		// 2 is move left
		temp = newBoard[holeRow*dim+(holeCol-1)];
		newBoard[holeRow*dim+(holeCol-1)] = 0;
		newBoard[holeRow*dim+holeCol] = temp;
	}
	else if(direction == 3)
	{
		// 3 is move right
		temp = newBoard[holeRow*dim+(holeCol+1)];
		newBoard[holeRow*dim+(holeCol+1)] = 0;
		newBoard[holeRow*dim+holeCol] = temp;
	}
	else
	{
		printf("Error moving at direction %d \n", direction);
	}
	return newBoard;
}

state* makeAMove(int direction, state *currentMove)
{
	struct state *nextMove = (state*)malloc(sizeof(struct state));
	setState(nextMove, moveHole(direction, currentMove->board, currentMove->dim), 
		currentMove->dim, currentMove->moveSoFar + 1);
	return nextMove;
}

int setHoleDirection(int *directions, int holeRow, int holeCol, int n)
{
    int numDirections;

    if(holeRow == 0){
        if(holeCol == 0){
            numDirections = 2;
            directions = (int*)malloc(numDirections * sizeof(int));
            directions[0] = 1;
            directions[1] = 3;
        }else if(holeCol == n-1){
            numDirections = 2;
            directions = (int*)malloc(numDirections * sizeof(int));
            directions[0] = 1;
            directions[1] = 2;
        }else{
            numDirections = 3;
            directions = (int*)malloc(numDirections * sizeof(int));
            directions[0] = 1;
            directions[1] = 2;
            directions[2] = 3;
        }
    }else if(holeRow == n-1){
        if(holeCol == 0){
            numDirections = 2;
            directions = (int*)malloc(numDirections * sizeof(int));
            directions[0] = 0;
            directions[1] = 3;
        }else if(holeCol == n-1){
            numDirections = 2;
            directions = (int*)malloc(numDirections * sizeof(int));
            directions[0] = 0;
            directions[1] = 2;
        }else{
            numDirections = 3;
            directions = (int*)malloc(numDirections * sizeof(int));
            directions[0] = 0;
            directions[1] = 2;
            directions[2] = 3;
        }
    }else{
        if(holeCol == 0){
            numDirections = 3;
            directions = (int*)malloc(numDirections * sizeof(int));
            directions[0] = 0;
            directions[1] = 1;
            directions[2] = 3;
        }else if(holeCol == n-1){
            numDirections = 3;
            directions = (int*)malloc(numDirections * sizeof(int));
            directions[0] = 0;
            directions[1] = 1;
            directions[2] = 2;
        }else{
            numDirections = 4;
            directions = (int*)malloc(numDirections * sizeof(int));
            directions[0] = 0;
            directions[1] = 1;
            directions[2] = 2;
            directions[3] = 3;
        }
    }
    return numDirections;
}

int main(int argc, char *argv[])
{
	int n = 3;
	int *board;
	int *bestSolution;
	struct state *currentMove  = (state*)malloc(sizeof(struct state));

	//priority_queue<state, vector<state>, Comp> queue;
	priority_queue<state> queue;

    //building board
	srand(time(NULL));
	board = (int*)malloc(n*n * sizeof(int));
	fillBoard(board, n);
	shuffleBoard(board, n*n);
	printBoard(board, n);

	// DEBUG
	// printf("Manhattan Distance is %d \n", getBoardManhattan(board, n));

	struct state *initial = (state*)malloc(sizeof(struct state));
	setState(initial, board, n, 0);

	// DEBUG
	//printBoard(initial->board, n);
	printf("lowerBound is %d \n", initial->lowerBound);

	queue.push(*initial);

	////while (!queue.empty())

	int j;
	for(j=0; j<5; j++)
	{
		printf("iteration %d \n", j);
		*currentMove = queue.top();
		struct state *nextMove;
		queue.pop();
		if(checkResult(currentMove->board, n))
		{
			printBoard(currentMove->board, n);
			exit(0);
		}
		else
		{
			int mininumBound = 10000, i, k, chosenDirection;
			int holeCol, holeRow;

			// Find out the location of the hole
			for(i=0; i<n*n; i++){
				if(currentMove->board[i]==0){
					holeRow = i / n;
					holeCol = i % n;
				}
			}

            // Find directions of the moves based on the location of the hole
            int numDirections = 4; //at most 4 directions
            int *directions = (int*)malloc(numDirections * sizeof(int));
            numDirections = setHoleDirection(directions, holeRow, holeCol, n);

            // For each direction, find the nextMove
			for(i=0; i<numDirections; i++)
			{
				k = directions[i];
				struct state *nextMove;
				nextMove = makeAMove(k, currentMove);

				// DEBUG
                printf("Info on next possible move \n");
				printBoard(nextMove->board, n);
				printf("move direction %d, lowerboud %d \n", k, nextMove->lowerBound);
				printf("move direction %d, Manhattan total %d \n", k, getBoardManhattan(nextMove->board, n));

				queue.push(*nextMove);
			}
		}
		freeMove(currentMove);
	}

	//system("pause");

	return 0;
}

// junk

//std::cout << queue.top() << std::endl;


				
			//	if(possibleNextMove->lowerBound < mininumBound){
			//		nextMove = possibleNextMove;
			//		mininumBound = possibleNextMove->lowerBound;
			//		chosenDirection = k;
			//		//printf("Manhattan Distance is %d \n", getBoardManhattan(nextMove->board, n));
			//	}else{
			//		freeMove(possibleNextMove);
			//	}

			//	printf("lowerBound Finalized is %d \n", nextMove->lowerBound);
			//	printf("chosenDirection is %d \n", chosenDirection);
			//	printBoard(nextMove->board, n);			
