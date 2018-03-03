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


int start_elevator()
{
	struct Elevator elevator;

	init_elevator(&elevator);

	printf("\nLoad before add -  p_units: %d w_units: %f\n", elevator.cur_load.p_units,elevator.cur_load.w_units);
	add_load(&elevator.cur_load, ADULT_LOAD);
	add_load(&elevator.cur_load, CHILD_LOAD);
	add_load(&elevator.cur_load, BELLHOP_LOAD);
	printf("Load after add -  p_units: %d w_units: %f\n", elevator.cur_load.p_units,elevator.cur_load.w_units);

	int status = 0;
	return status;
}

int main()
{
	printf("Test %d", start_elevator());
}



// Helper Subroutines

void init_elevator(struct Elevator * elevator)
{
	elevator->state = DEFAULT_STATE;
	elevator->cur_floor = DEFAULT_FLOOR;
	elevator->cur_load.p_units = DEFAULT_P_UNIT;
	elevator->cur_load.w_units = DEFAULT_W_UNIT;
}

bool too_heavy(struct Load pload, struct Load eload)
{
	return (pload.p_units + eload.p_units > MAX_P_LOAD ||  pload.w_units + eload.w_units > MAX_W_LOAD);
}

void add_load(struct Load * total_load , struct Load add_load) 
{ 
	if (!too_heavy(add_load,*total_load))
    {
  		total_load->p_units += add_load.p_units;
  		total_load->w_units += add_load.w_units;
	}
}

void remove_load(struct Load * total_load , struct Load add_load) 
{ 
  total_load->p_units -= add_load.p_units;
  total_load->w_units -= add_load.w_units;
}








