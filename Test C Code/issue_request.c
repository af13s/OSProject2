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



int issue_request(int passenger_type, int start_floor, int destination_floor)
{  
	if (status == 0)
		return INVALID;

	struct Passenger * passenger;

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

		   case RSERVICE:
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

	if (start_floor < MIN_FLOOR || start_floor > MAX_FLOOR || destination_floor > MAX_FLOOR || destination_floor < MIN_FLOOR)
		return INVALID;
	else
		addPassenger(passenger,elevator);

	return VALID;
}


struct Passenger * init_pass(struct Load load, int start, int dest)
{
	struct Passenger * passenger = (struct Passenger*)malloc(sizeof(struct Passenger));;
	passenger->load = load;
	passenger->start = start;
	passenger->dest = dest;
	return passenger;
}

bool too_heavy(struct Load pload, struct Load eload)
{
	return (pload.p_units + eload.p_units > MAX_P_LOAD ||  pload.w_units + eload.w_units > MAX_W_LOAD);
}

int addPassenger(struct Passenger * passenger, struct Elevator * elevator)
{
	int queue_num = 0;
	if (add_load(&elevator->cur_load,passenger->load))
	{	
		//queue_num = add_to_queue(struct Passenger * passenger);
		return queue_num;
	}
	else
		return -1;
}

int removePassenger(struct Passenger * passenger, struct Elevator * elevator)
{
	int queue_num = 0;
	remove_load(&elevator->cur_load,passenger->load);	
	//queue_num = remove_from_queue(struct Passenger * passenger);
	
	return queue_num;
}

bool add_load(struct Load * total_load , struct Load add_load) 
{ 
	if (!too_heavy(add_load,*total_load))
    {
  		total_load->p_units += add_load.p_units;
  		total_load->w_units += add_load.w_units;
  		return true;
	}

	return false;
}

void remove_load(struct Load * total_load , struct Load add_load) 
{ 
  total_load->p_units -= add_load.p_units;
  total_load->w_units -= add_load.w_units;
}


// Code to edit to use for this program -- adding and removing from queue
/*
int add_child(pid_t * queue, pid_t child)
{
	int i=1;
	for ( i = 1; i < queue[0]; i++)
		if (queue[i] == 0)
		{
			queue[i] = child;
			return i;
		}

	return 0;
}

int remove_child(pid_t * queue, pid_t child)
{
	int i=1;
	for ( i = 1; i < queue[0]; i++)
		if (queue[i] == child)
		{
			queue[i] = 0;
			return i;
		}

	return 0;
}

void addbgcmd(int queue_num,struct PCMD pcmd)
{
	pcmd.originalcmd[strlen(pcmd.originalcmd)-2] = '\0';
	pcmd.bgcmds[queue_num] = (char*) calloc(strlen(pcmd.originalcmd+1), sizeof(char));
	strcpy(pcmd.bgcmds[queue_num], pcmd.originalcmd);
}

void removebgcmd(int queue_num,struct PCMD pcmd)
{
	free(pcmd.bgcmds[queue_num]);
	pcmd.bgcmds[queue_num] = NULL;
}
*/

