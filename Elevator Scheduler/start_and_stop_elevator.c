#include "elevator.h"

/* Activates the elevator for service. */
/* From that point onward, the elevator exists and will begin to service requests. */
/*
This system call will return
1 if the elevator is already active
0 for a successful elevator start,
-ERRORNUM if it could not initialize (e.g. -ENOMEM if it couldn’t allocate memory).
*/

/*
Initialize an elevator as follows:
State: IDLE
Current floor: 1
Current Load: 0 passenger units, 0 weight units
*/


int main()
{
	printf("(start)status: %d \n" , start_elevator());
	printf("(start)status: %d\n" , start_elevator());
	printf("(start)status: %d\n" , start_elevator());
	printf("(stop)status: %d\n" , stop_elevator());
	printf("(stop)status: %d\n" , stop_elevator());
	printf("(start)status: %d\n" , start_elevator());
	printf("(start)status: %d\n" , start_elevator());
}

static int status = 0;

int start_elevator()
{	

	if (status == 0)
	{
		elevator = init_elevator(elevator);
		return status++;
	}
	else
		return status;

	// TESTS //
	//printf("\nLoad before add -  p_units: %d w_units: %f\n", elevator->cur_load.p_units,elevator->cur_load.w_units);
	add_load(&elevator->cur_load, ADULT_LOAD);
	add_load(&elevator->cur_load, CHILD_LOAD);
	add_load(&elevator->cur_load, BELLHOP_LOAD);
	printf("Load after add -  p_units: %d w_units: %f\n", elevator->cur_load.p_units,elevator->cur_load.w_units);
	// TESTS // 
}

//Description: Deactivates the elevator.
//At this point, this elevator will process no more new requests (that is, passengers waiting on floors).
//However, before an elevator completely stops, it must offload all of its current passengers.
//Only after the elevator is empty may it be deactivated (state = OFFLINE).
//This function returns 1 if the elevator is already in the process of deactivating, and 0 otherwise.

int stop_elevator()
{
	if (status == 1)
		//if (deactivating())
			//return status;
		//else
			 --status;

	return status;
}

// Helper Subroutines
struct Elevator * init_elevator(struct Elevator * elevator)
{
	elevator = (struct Elevator*)malloc(sizeof(struct Elevator));
	elevator->state = DEFAULT_STATE; // IDLE
	elevator->cur_floor = DEFAULT_FLOOR;  // 1
	elevator->cur_load.p_units = DEFAULT_P_UNIT; // 0
	elevator->cur_load.w_units = DEFAULT_W_UNIT; // 0
	return elevator;
}








