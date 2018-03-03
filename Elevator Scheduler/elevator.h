//http://en.cppreference.com/w/c/header

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h> 

//ELEVATOR CONSTRAINTS
#define MAX_P_LOAD 10 //passenger units
#define MAX_W_LOAD 15 //weight units
#define FLOOR_WAIT 2 // transition time between floors in seconds
#define LOAD_WAIT  1 // load wait time in seconds
#define UNLOAD_WAIT 1 // unload wait time in seconds
#define MAX_FLOOR 10 // Max floor is 10th floor
#define MIN_FLOOR 1  // Min Floor is 1st floor

//Elevator defaults
#define DEFAULT_STATE  0
#define DEFAULT_FLOOR  1
#define DEFAULT_P_UNIT 0
#define DEFAULT_W_UNIT 0


//An adult counts as 1 passenger unit and 1 weight unit
#define ADULT_P_UNIT 1
#define ADULT_W_UNIT 1

//A child counts as 1 passenger unit and ½ weight unit
#define CHILD_P_UNIT  1
#define CHILD_W_UNIT .5

//Room service counts as 2 passenger units and 2 weight units
#define RSERVICE_P_UNIT 2
#define RSERVICE_W_UNIT 2

//A bellhop counts as 2 passengers unit and 3 weight unit
#define BELLHOP_P_UNIT 2
#define BELLHOP_W_UNIT 2

//PASSENGER TYPES
#define ADULT 	1
#define	CHILD 	2
#define RSRVICE 3
#define BELLHOP 4

enum State {IDLE = 0, LOADING = 1,OFFLINE = -1 , UP = 2, DOWN = 3};

struct Load
{
	int p_units;
	double w_units;
};

struct Elevator
{
	enum State state;
	int cur_floor;
	struct Load cur_load;
};

struct Passenger
{
	struct Load load;
	int start;
	int dest;
};

const struct Load ADULT_LOAD = {ADULT_P_UNIT, ADULT_W_UNIT};
const struct Load CHILD_LOAD = {CHILD_P_UNIT, CHILD_W_UNIT};
const struct Load RSERVICE_LOAD = {RSERVICE_P_UNIT, RSERVICE_W_UNIT};
const struct Load BELLHOP_LOAD = {BELLHOP_P_UNIT, BELLHOP_W_UNIT};

int start_elevator();
void init_elevator(struct Elevator * elevator);
struct Passenger init_pass(struct Load load, int start, int dest);
struct Load* load_constructor(int passenger_unit, double weight_unit);
void add_load(struct Load * total_load , struct Load add_load);
void remove_load(struct Load * total_load , struct Load add_load);
bool too_heavy(struct Load pload, struct Load eload);

