#include <stdio.h>		// for printf
#include <stdlib.h>		// for malloc
#include <string.h>		// for memcpy
#include <iostream>		// for CPP
#include <queue>		// for priority queue
#include <sys/time.h>	// for gettimeofday
#include "mpi.h"        // for MPI routines, definitions, etc 
using namespace std;

#include "game.h"

#define INF 1000000				/* Proxy for inifinite */
#define COMM_INTERVAL 200		/* Time between communication steps */
#define MASTER 0				/* ID of the master node */
#define TERMINATION 1			/* Tags termination messages */
#define Token 2					/* Tags token message */
#define UNEXAMINED_SUBPROBLEM 3 /* Tags message containing unexamined subproblem */

enum Color{ WHITE, BLACK };

struct TOKEN
{
	int c;
	enum Color color;
	int count;
	struct state s;
};

int main(int argc, char *argv[])
{
	int myRank, numProcs;				// MPI related variables

	// initialize MPI
	MPI_Init(NULL, NULL);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	MPI_Comm_size(MPI_COMM_WORLD, &numProcs);

	printf("node %d here 1 \n", myRank);

    // initialize variables
	enum Color color;					// process color (for termination detection)
	int global_c, local_c;				// cost of global and local best solution so far
	struct timeval lastComm;			// time of last communication
	int msgCount;						// messages sent minus messages received
	priority_queue<state> queue;		// priority queue, each process has its
                                        // own queue

	MPI_Status status;

	// token passed around ring for termination detection
	struct TOKEN *token	= (TOKEN*)malloc(sizeof(struct TOKEN));	

	int n = 3; //board size is 3x3

	// different states
	struct state *local_bestState = (state*)malloc(sizeof(struct state));
	struct state *currentState  = (state*)malloc(sizeof(struct state));
	struct state *nextState = (state*)malloc(sizeof(struct state));

    //initialize local_bestState
    int *local_board1;
    local_board1 = (int*)malloc(n*n * sizeof(int));
	setState(local_bestState, local_board1, n, 0);
	local_bestState->lowerBound = INF;

    //initialize currentState
    int *local_board2;
    local_board2 = (int*)malloc(n*n * sizeof(int));
	setState(currentState, local_board2, n, 0);
	currentState->lowerBound = INF;

    //initialize nextState
    int *local_board3;
    local_board3 = (int*)malloc(n*n * sizeof(int));
	setState(nextState, local_board3, n, 0);
	nextState->lowerBound = INF;
	
	if(myRank == MASTER) //master initialize board
	{
		int *board;
		board = (int*)malloc(n*n * sizeof(int));

		/*// fill board randomly
		srand(time(NULL));
		fillBoard(board, n);
		shuffleBoard(board, n*n);*/

		// fill specific board 
		board[0] = 8;
		board[1] = 7;
		board[2] = 0;
		board[3] = 2;
		board[4] = 3;
		board[5] = 6;
		board[6] = 4;
		board[7] = 5;
		board[8] = 1;

		printf("node %d here 2 \n", myRank);

		// check solvability
		while(!isSolvable(board, n)){
			shuffleBoard(board, n*n);
		}

		// construct initial state
		struct state *initial = (state*)malloc(sizeof(struct state));

		setState(initial, board, n, 0);

		queue.push(*initial);
		token->c = INF;
		token->color = WHITE;
		token->count = 0;
		// send token to next process (myRank = 1)
		MPI_Send(token, sizeof(struct TOKEN), MPI_BYTE, 1, Token, 
				MPI_COMM_WORLD);

		freeState(initial);
	}


	local_c = INF;
	gettimeofday(&lastComm, NULL);
	msgCount = 0;
	color = WHITE;

	printf("node %d here 3 \n", myRank);

    int i = 0;
    for (int i=0; i<1; i++)
    {
        if(queue.empty() || timeDiff(&lastComm) > COMM_INTERVAL)
        {
            
            /****************************** BandB_Communication() ******************************/
            int terminationFlag = 0, tokenFlag = 0, unexaminedSubFlag = 0;
            // get the neighbors in the ring (receive from left, send to right)
            int leftNeighbor = (myRank + numProcs - 1) / numProcs;
            int rightNeighbor = (myRank + 1) / numProcs;

            printf("node %d here 4 \n", myRank);

            // check pending message with TERMINATION tag
            MPI_Iprobe(MASTER, TERMINATION, MPI_COMM_WORLD, &terminationFlag, &status);
            if(terminationFlag)
            {
                // ????? do we need MPI_Recv here?
                MPI_Recv(0, 0, MPI_INT, MASTER, TERMINATION, MPI_COMM_WORLD, &status);
                return 0;
            }

            printf("node %d here 5 \n", myRank);

            // check pending message with Token tag
            MPI_Iprobe(leftNeighbor, Token, MPI_COMM_WORLD, &tokenFlag, &status);
            if(tokenFlag)
            {
                // need to think about how to pass token
                // http://stackoverflow.com/questions/5972018/
                MPI_Recv(token, sizeof(struct TOKEN), MPI_BYTE, leftNeighbor, Token, 
                    MPI_COMM_WORLD, &status);

                // update token cost if local cost is smaller
                if(local_c < token->c)
                {
                    token->c = local_c;
                    token->s = *local_bestState;
                }

                // clear queue if best solution in queue is worse than token cost
                struct state *tempState  = (state*)malloc(sizeof(struct state));
                *tempState = queue.top();
                if(token->c <= tempState->lowerBound)
                {
                    queue = priority_queue<state>(); 
                }
                freeState(tempState);

                // set global cost to token cost
                global_c = token->c;

                // HEAD node check termination
                if(myRank == MASTER)
                {
                    if( color == WHITE 
                        && token->color == WHITE 
                        && token->count + msgCount == 0)
                    {
                        int rank;
                        for (rank = 1; rank < numProcs; rank++) {
                            MPI_Send(0, 0, MPI_INT, rank, TERMINATION, MPI_COMM_WORLD);
                        }
                        return 0;
                    }
                    else
                    {
                        token->color = WHITE;
                        token->count = 0;
                    }
                }
                // SLAVE node update message count
                else
                {
                    if(color == BLACK)
                        token->color = BLACK;
                    token->count += msgCount;
                }
                MPI_Send(token, sizeof(struct TOKEN), MPI_BYTE, rightNeighbor, Token, 
                    MPI_COMM_WORLD);
                color = WHITE;
            }

            printf("node %d here 6 \n", myRank);
            
            // ????? suppose to be while, how to handle?
            // check pending message with Unexamined subproblem tag
            MPI_Iprobe(leftNeighbor, UNEXAMINED_SUBPROBLEM, MPI_COMM_WORLD, &unexaminedSubFlag, &status);
            while(unexaminedSubFlag)
            {
                struct state *tempRecvState;
                /*MPI_Recv(&tempRecvState, 1, dataState, leftNeighbor, UNEXAMINED_SUBPROBLEM, 
                    MPI_COMM_WORLD, &status);*/
                MPI_Recv(&tempRecvState, sizeof(struct state), MPI_BYTE, leftNeighbor, 
                    UNEXAMINED_SUBPROBLEM, MPI_COMM_WORLD, &status);
                msgCount = msgCount - 1;
                color = BLACK;
                if(tempRecvState->lowerBound < global_c)
                {
                    queue.push(*tempRecvState);
                }
                freeState(tempRecvState);
                unexaminedSubFlag = 0;
                MPI_Iprobe(leftNeighbor, UNEXAMINED_SUBPROBLEM, MPI_COMM_WORLD, &unexaminedSubFlag, &status);
            }

            printf("node %d here 7 \n", myRank);

            // if more than one unexamined subproblem in queue, then
            if(queue.size() > 1)
            {
                struct state *tempSendState = (state*)malloc(sizeof(struct state));
                *tempSendState = queue.top();
                queue.pop();
                /*MPI_Send(&tempSendState, 1, dataState, rightNeighbor, UNEXAMINED_SUBPROBLEM,
                    MPI_COMM_WORLD);*/
                MPI_Send(&tempSendState, sizeof(struct state), MPI_BYTE, rightNeighbor, 
                    UNEXAMINED_SUBPROBLEM, MPI_COMM_WORLD);
                msgCount = msgCount + 1;
                color = BLACK;
            }

            printf("node %d here 8 \n", myRank);

            /***********************************************************************************/

            gettimeofday(&lastComm, NULL);
        }
        else if(!queue.empty())
        {
            *currentState = queue.top();
            queue.pop();

            printf("node %d here 9 \n", myRank);

            // ????? what is best_c, is it local_c or global_c
            if(currentState->lowerBound < local_c )
            {
                color = BLACK;
                if(checkResult(currentState->board, currentState->dim))
                {
                    if(currentState->lowerBound < global_c)
                    {
                        // ????? not sure which one will work, memcpy or dereference
                        // memcpy(local_bestState, currentState, sizeof(state));
                        *local_bestState = *currentState;
                        local_c = currentState->lowerBound;
                    }
                }
                else
                {
                    int mininumBound = 10000, i, k, chosenDirection;
                    int holeCol, holeRow;

                    // Find out the location of the hole
                    for(i=0; i<currentState->dim*currentState->dim; i++){
                        if(currentState->board[i]==0){
                            holeRow = i / currentState->dim;
                            holeCol = i % currentState->dim;
                        }
                    }

                    // Find directions of the states based on the location of the hole
                    int numDirections = 4; //at most 4 directions
                    int *directions = (int*)malloc(numDirections * sizeof(int));
                    numDirections = setHoleDirection(directions, holeRow, holeCol, currentState->dim);

                    // For each direction, find the nextState
                    for(i=0; i<numDirections; i++)
                    {
                        k = directions[i];
                        nextState = makeAState(k, currentState);
                        if(nextState->lowerBound < global_c)
                        {
                            queue.push(*nextState);
                        }
                    }

                    printf("node %d here 10 \n", myRank);

                    free(directions);
                }
            }
        }
        printf("node %d here 11 \n", myRank);
    }
	freeState(local_bestState);
	freeState(currentState);
	printf("node %d here 12 \n", myRank);
	//freeState(nextState);
	/*if(myRank == MASTER)
	{
		freeState(initial);
	}*/
	printf("node %d here 13 \n", myRank);
	free(token);

	MPI_Finalize();

	return 0;
}

/*	JUNK

	gettimeofday(&currTime, NULL);
	printf("time diff is %d \n", timeDiff(&lastComm, &currTime));

*/
