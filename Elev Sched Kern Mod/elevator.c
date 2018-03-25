
// http://en.cppreference.com/w/c/header
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h> 
#include <linux/list.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Elevator module for processing requests");

#define ENTRY_NAME "elevator"
#define ENTRY_SIZE 10000
#define PERMS 0644
#define PARENT NULL

// ELEVATOR CONSTRAINTS
#define MAX_P_LOAD 10 //passenger units
#define MAX_W_LOAD 15 //weight units
#define FLOOR_WAIT 2 // transition time between floors in seconds
#define LOAD_WAIT  1 // load wait time in seconds
#define UNLOAD_WAIT 1 // unload wait time in seconds
#define MAX_FLOOR 10 // Max floor is 10th floor
#define MIN_FLOOR 1  // Min Floor is 1st floor

// Elevator defaults
#define DEFAULT_STATE  0
#define DEFAULT_FLOOR  1
#define DEFAULT_P_UNIT 0
#define DEFAULT_W_UNIT 0

// An adult counts as 1 passenger unit and 1 weight unit
#define ADULT_P_UNIT 1
#define ADULT_W_UNIT 1

// A child counts as 1 passenger unit and ½ weight unit
#define CHILD_P_UNIT  1
#define CHILD_W_UNIT  1

// Room service counts as 2 passenger units and 2 weight units
#define RSERVICE_P_UNIT 2
#define RSERVICE_W_UNIT 2

// A bellhop counts as 2 passengers unit and 3 weight unit
#define BELLHOP_P_UNIT 2
#define BELLHOP_W_UNIT 3


// Request
#define INVALID 1
#define VALID 0

// PASSENGER TYPES
#define ADULT 	1
#define	CHILD 	2
#define RSERVICE 3
#define BELLHOP 4

enum State {IDLE = 0, LOADING = 1,OFFLINE = 2 , UP = 3, DOWN = 4};
static char * state_arr[5] = {"IDLE", "LOADING", "OFFLINE", "UP", "DOWN"};

struct Passenger
{
	struct list_head pnode;
	struct Load load;
	int start;
	int dest;
};

struct Load
{
	int p_units;
	int w_units;
};

struct Elevator
{
	enum State state;
	int cur_floor;
	struct Load cur_load;
	struct list_head plist;
};

static struct Load ADULT_LOAD = {ADULT_P_UNIT, ADULT_W_UNIT};
static struct Load CHILD_LOAD = {CHILD_P_UNIT, CHILD_W_UNIT};
static struct Load RSERVICE_LOAD = {RSERVICE_P_UNIT, RSERVICE_W_UNIT};
static struct Load BELLHOP_LOAD = {BELLHOP_P_UNIT, BELLHOP_W_UNIT};
static struct Elevator * elevator;
static struct list_head queue;

static int requested =0;
static int status = 0;

struct Elevator * init_elevator(struct Elevator * elevator,int * status)
{
	//elevator = (struct Elevator*)malloc(sizeof(struct Elevator));
	elevator = kmalloc(sizeof(struct Elevator),GFP_KERNEL);

	INIT_LIST_HEAD(&elevator->plist);

	if (elevator == NULL)
	{
		*status = -errno;
		return NULL;
	}

	elevator->state = DEFAULT_STATE; // IDLE
	elevator->cur_floor = DEFAULT_FLOOR;  // 1
	elevator->cur_load.p_units = DEFAULT_P_UNIT; // 0
	elevator->cur_load.w_units = DEFAULT_W_UNIT; // 0
	return elevator;
}

// Start Elevator

int start_elevator()
{	

	if (status == 0)
	{
		elevator = init_elevator(elevator,&status);

		if (status == 0)
			return status++;
		else
		{
			int ret = status;
			status = 0;
			return ret;
		}
	}
	else
		return status;
}

module_init(start_elevator);

// Stop elevator

int stop_elevator()
{
	if (status == 1)
	{
		while(!list_empty($elevator->plist))
			//continue processing requests
		elevator->state = OFFLINE;
		--status;
	}

	return status;
}
module_exit(stop_elevator)


//Issue requests
int issue_request(int passenger_type, int start_floor, int destination_floor)
{  
	if (!requested)
	{
		INIT_LIST_HEAD(&queue);
		request = 1;
	}

	if (status == 0)
		return INVALID;

	if (start_floor < MIN_FLOOR || start_floor > MAX_FLOOR || destination_floor > MAX_FLOOR || destination_floor < MIN_FLOOR)
		return INVALID;

	if (passenger_type < ADULT || passenger_type > BELLHOP)
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

	addRequest(passenger);

	return VALID;
}


struct Passenger * init_pass(struct Load load, int start, int dest)
{
	//struct Passenger * passenger = (struct Passenger*)malloc(sizeof(struct Passenger));
	struct Passenger * passenger = kmalloc(sizeof(struct Passenger),GFP_KERNEL);
	INIT_LIST_HEAD(&passenger->pnode);
	passenger->load = load;
	passenger->start = start;
	passenger->dest = dest;
	return passenger;
}

int too_heavy(struct Load pload, struct Load eload)
{
	return (pload.p_units + eload.p_units > MAX_P_LOAD ||  pload.w_units + eload.w_units > MAX_W_LOAD);
}

int addRequest(struct Passenger * passenger)
{
	list_add_tail(&passenger->pnode, &queue);
}

int removeRequest(struct Passenger * passenger)
{
	struct list_head *temp;
	struct list_head *dummy;
	struct Passenger * passger;

	list_for_each_safe(temp, dummy, &queue) 
	{
		passger = list_entry(temp, struct Passenger, pnode); // returns item at that 

		if (passger == passenger):
		{
			remove_load(&elevator->cur_load,passenger->load)
			list_del(temp); /* init ver also reinits list */
			kfree(passger); /* remember to free alloced data */
		}
	}
}

int addPassenger(struct Passenger * passenger)
{
	if (add_load(&elevator->cur_load,passenger->load) && status == 1)
		list_add_tail(&passenger->pnode, &elevator->plist);
}

void removePassenger(struct Passenger * passenger)
{
	struct list_head *temp;
	struct list_head *dummy;
	struct Passenger * passger;

	list_for_each_safe(temp, dummy, &elevator->plist) 
	{
		passger = list_entry(temp, struct Passenger, pnode); // returns item at that 

		if (passger == passenger):
		{
			remove_load(&elevator->cur_load,passenger->load)
			list_del(temp); /* init ver also reinits list */
			kfree(passger); /* remember to free alloced data */
		}
	}
}

int add_load(struct Load * total_load , struct Load add_load) 
{ 
	if (!too_heavy(add_load,*total_load))
    {
  		total_load->p_units += add_load.p_units;
  		total_load->w_units += add_load.w_units;
  		return 1;
	}

	return 0;
}

void remove_load(struct Load * total_load , struct Load add_load) 
{ 
  total_load->p_units -= add_load.p_units;
  total_load->w_units -= add_load.w_units;
}

