#include "elevator.h"

/*Description: Creates a passenger of type passenger_type at start_floor that wishes to go to destination_floor.*/
/*This function returns 1 if the request is not valid (one of the variables is out of range), and 0 otherwise.*/

//A passenger type can be translated to an int as follows:
//ADULT 	1
//CHILD 	2
//RSRVICE 	3
//BELLHOP 	4

/*struct Passenger
{
	struct Load load;
	int start
	int dest;
};*/

const int TEST_PASS = 1;
const int TEST_START = 200;
const int TEST_DEST = 2;

#define INVALID 1
#define VALID 0

int issue_request(int passenger_type, int start_floor, int destination_floor)
{
	struct Passenger passenger;

	switch(passenger_type) 
		{
		   case ADULT:
		   {
		      passenger = init_pass(ADULT_LOAD,start_floor,destination_floor);
		      break;
		   }

		   case CHILD:
		   {
		      passenger = init_pass(CHILD_LOAD,start_floor,destination_floor);
		      break;
		   }

		   case RSRVICE:
		   {
		      passenger = init_pass(RSERVICE_LOAD,start_floor,destination_floor);
		      break;
		   }

		  	case BELLHOP:
		  	{
		      passenger = init_pass(BELLHOP_LOAD,start_floor,destination_floor);
		      break;
		    }

		   default :
		   		return INVALID;
		}

	if (start_floor < MIN_FLOOR || destination_floor > MAX_FLOOR)
		return INVALID;
	else
		//add passenger to queue waiting for elevator

	return VALID;
}

int main()
{
	printf("Test %d \n" , issue_request(TEST_PASS,TEST_START,TEST_DEST));
}

struct Passenger init_pass(struct Load load, int start, int dest)
{
	struct Passenger passenger = {load,start,dest};
	return passenger;
}


