#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/errno.h>

extern long (*STUB_start_elevator)(void);
extern long (*STUB_issue_request)(int,int,int);
extern long (*STUB_stop_elevator)(void);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Elevator module for processing requests");

#define ENTRY_NAME "elevator"
#define ENTRY_SIZE 3000
#define PERMS 0644
#define PARENT NULL

/**************************** ELEVATOR VARS & CONSTANTS *******************************/

// ELEVATOR CONSTRAINTS
#define MAX_P_LOAD 10 //passenger units
#define MAX_W_LOAD 15 //weight units
#define FLOOR_WAIT 2000 // transition time between floors in seconds
#define LOAD_WAIT  1000 // load wait time in seconds
#define MAX_FLOOR 10 // Max floor is 10th floor
#define MIN_FLOOR 1  // Min Floor is 1st floor
// Elevator defaults
#define DEFAULT_STATE  2
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

struct Load
{
	int p_units;
	int w_units;
};

struct Passenger
{
	int passtype;
	struct list_head pnode;
	struct Load load;
	int start;
	int dest;
};

struct Elevator
{
	enum State state;
	int cur_floor;
	int floorrequest;
	struct Load cur_load;
	struct Passenger plist;
};

static struct Load ADULT_LOAD;
static struct Load CHILD_LOAD;
static struct Load RSERVICE_LOAD;
static struct Load BELLHOP_LOAD;



static struct Elevator elevator;

static struct Passenger queue;
static struct Load waiting;
static int waitingkids;
static struct Load floor[10];

static int gits[10];
static int elv_gits;

static int status;
static int serviced;
static int servedfloor[10];


struct thread_parameter 
{
	struct task_struct *kthread;
	struct mutex mutex;
};

static struct thread_parameter thread1;

static struct mutex start_stop;

/**************************** PROC VARS *******************************/

static struct file_operations fops;
static char *message;
static int read_p;


/**************************** PROC FUNCTIONS *******************************/

int weight(int * w_units, int kids)
{
	*w_units -= kids;
	*w_units += kids/2;
	return(kids%2 == 0);
}

void floor_stats(char * message)
{	
	int i =0;
	int weights = 0;

	sprintf(message + strlen(message),"Floor |  Passenger Units | Weight Units \n");
	for (i =0; i < 10; i++)
	{
		weights = floor[i].w_units;
		if (weight(&weights,gits[i]))
			sprintf(message + strlen(message)," %d\t  %d\t\t   %d\t \n", i+1, floor[i].p_units,weights);
		else
			sprintf(message + strlen(message)," %d\t  %d\t\t   %d.5\t \n", i+1, floor[i].p_units,weights);
	}

	sprintf(message + strlen(message),"Floor | Serviced \n");
	for (i =0; i < 10; i++)
		sprintf(message+ strlen(message),"  %d \t %d \n", i+1, servedfloor[i]);
}

int proc_open(struct inode *sp_inode, struct file *sp_file) {

	int weights = 0;
	read_p = 1;

	message = kmalloc(sizeof(char) * ENTRY_SIZE, __GFP_RECLAIM | __GFP_IO | __GFP_FS);

	if (message == NULL) {
		return -ENOMEM;
	}

	mutex_lock(&thread1.mutex);
	sprintf(message,"Movement State: %s\n",state_arr[elevator.state]);
	sprintf(message + strlen(message),"Current Floor: %d\n", elevator.cur_floor);
	sprintf(message + strlen(message),"Next Floor: %d\n", elevator.floorrequest);

	weights = elevator.cur_load.w_units;
	if (weight(&weights,elv_gits))
		sprintf(message + strlen(message),"Elevator Passenger Units: %d\t Weight Units: %d\n", elevator.cur_load.p_units,weights);
	else
		sprintf(message + strlen(message),"Elevator Passenger Units: %d\t Weight Units: %d.5\n", elevator.cur_load.p_units,weights);


	weights = waiting.w_units;
	if (weight(&weights,waitingkids))
		sprintf(message + strlen(message),"Waiting Passenger Units: %d\t Weight Units: %d\n", waiting.p_units,weights);
	else
		sprintf(message + strlen(message),"Waiting Passenger Units: %d\t Weight Units: %d.5\n", waiting.p_units,weights);

	sprintf(message+ strlen(message),"Passengers Serviced: %d\n",serviced);
	floor_stats(message);
	mutex_unlock(&thread1.mutex);

	return 0;
}

ssize_t proc_read(struct file *sp_file, char __user *buf, size_t size, loff_t *offset) {
	int len = strlen(message);
	
	read_p = !read_p;
	if (read_p)
		return 0;
		
	printk(KERN_INFO "proc called read\n");
	copy_to_user(buf, message, len);
	return len;
}

int proc_release(struct inode *sp_inode, struct file *sp_file) {
	printk(KERN_NOTICE "proc called release\n");
	kfree(message);
	return 0;
}
/***************************** PROC FUNCTINS END ***********************************************/

/**************************** ELEVATOR SYSCALL HELPER FUNCTIONS *******************************/

//ISSUE REQUEST HELPERS /////////////////////////////////////////////

struct Passenger * init_pass(struct Load load, int start, int dest, int pass)
{
	struct Passenger * passenger = kmalloc(sizeof(struct Passenger)*1,__GFP_RECLAIM);
	INIT_LIST_HEAD(&passenger->pnode);
	passenger->load = load;
	passenger->start = start;
	passenger->dest = dest;
	passenger->passtype = pass;
	return passenger;
}


void addRequest(struct Passenger * passenger)
{
	list_add_tail(&passenger->pnode, &queue.pnode);
	waiting.p_units += passenger->load.p_units;
	waiting.w_units += passenger->load.w_units;
	if (passenger->passtype == CHILD)
		waitingkids++;
	floor[passenger->start-1].p_units += passenger->load.p_units;
	floor[passenger->start-1].w_units += passenger->load.w_units;
	if (passenger->passtype ==CHILD) 
		gits[passenger->start-1]++;

}

void removeRequest(struct Passenger * passenger)
{
	struct list_head *temp;
	struct list_head *dummy;
	struct Passenger * passger;

	list_for_each_safe(temp, dummy, &queue.pnode) 
	{
		passger = list_entry(temp, struct Passenger, pnode); // returns item at that 

		if (passger == passenger)
		{
			if (passger->passtype ==CHILD) 
				gits[passger->start-1]--;
			if (passger->passtype == CHILD)
				waitingkids--;
			waiting.p_units -= passger->load.p_units;
			waiting.w_units -= passger->load.w_units;
			floor[passenger->start-1].p_units -= passenger->load.p_units;
			floor[passenger->start-1].w_units -= passenger->load.w_units;
			list_del(temp); // init ver also reinits list //
			kfree(passger); // remember to free alloced data
		}
	}
}

//////////////////////////////////////////////////////////////////////////

// PROCESS REQUEST HELPERS /////////////////////////////////////////////

int too_heavy(struct Load pload)
{
	return (pload.p_units + elevator.cur_load.p_units > MAX_P_LOAD ||  pload.w_units + elevator.cur_load.w_units > MAX_W_LOAD);
}

int add_load(struct Load load) 
{ 
	if (!too_heavy(load))
    	{
  		elevator.cur_load.p_units += load.p_units;
  		elevator.cur_load.w_units += load.w_units;
  		return 1;
	}

	return 0;
}

void remove_load(struct Load load) 
{ 
  	elevator.cur_load.p_units -= load.p_units;
  	elevator.cur_load.w_units -= load.w_units;
}

int addPassengers(void)
{
	struct list_head *temp;
	struct list_head *dummy;
	struct Passenger * passenger;
	struct Passenger * passengercopy;
	int added =0;

	if (waiting.p_units == 0)
		return 0;

	list_for_each_safe(temp, dummy, &queue.pnode) 
	{
		passenger = list_entry(temp, struct Passenger, pnode);

		if ((passenger->start == elevator.cur_floor && ((passenger->dest < elevator.cur_floor && elevator.state == DOWN) || (passenger->dest > elevator.cur_floor && elevator.state == UP))) || (passenger->start == elevator.cur_floor && elevator.cur_load.p_units == 0))
		{	
			if (add_load(passenger->load))
			{
				if (passenger->passtype == CHILD)
					elv_gits++;
				servedfloor[passenger->start-1]++;
				passengercopy = init_pass(passenger->load,passenger->start,passenger->dest,passenger->passtype);
				list_add_tail(&passengercopy->pnode, &elevator.plist.pnode);
				removeRequest(passenger);
				++added;
			}
		}
	}

	return added;
}

int removePassengers(void)
{
	int removed =0;
	struct list_head *temp;
	struct list_head *dummy;
	struct Passenger * passger;

	list_for_each_safe(temp, dummy, &elevator.plist.pnode) 
	{
		passger = list_entry(temp, struct Passenger, pnode); // returns item at that 

		if (passger->dest == elevator.cur_floor)
		{
			if (passger->passtype == CHILD)
					elv_gits--;
			removed =1;
			++serviced;
			remove_load(passger->load);
			list_del(temp); // init ver also reinits list
			kfree(passger); // remember to free alloced data 
		}
	}

	return removed;
}

// START ELEVATOR HELPERS /////////////////////////////////////////////
void init_elevator(void)
{
	INIT_LIST_HEAD(&elevator.plist.pnode);

	if (&elevator.plist.pnode == NULL)
	{
		status = -ENOMEM;
		return;
	}

	elevator.state = DEFAULT_STATE; // IDLE
	elevator.cur_floor = DEFAULT_FLOOR;  // 1
	elevator.floorrequest = DEFAULT_FLOOR;  // 1
	elevator.cur_load.p_units = DEFAULT_P_UNIT; // 0
	elevator.cur_load.w_units = DEFAULT_W_UNIT; // 0
}

static struct Load init_load (int punit, int wunit)
{
     struct Load load = {punit, wunit};
     return load;
}

void init_defaults(void)
{
	int i =0;

	ADULT_LOAD = init_load (ADULT_P_UNIT, ADULT_W_UNIT);
	CHILD_LOAD = init_load (CHILD_P_UNIT, CHILD_W_UNIT);
	RSERVICE_LOAD = init_load (RSERVICE_P_UNIT, RSERVICE_W_UNIT);
	BELLHOP_LOAD = init_load (BELLHOP_P_UNIT, BELLHOP_W_UNIT);
	waiting = init_load(DEFAULT_P_UNIT, DEFAULT_W_UNIT);

	elv_gits = 0;
	for (i =0; i < 10; i++)
		gits[i] = 0;

	for (i =0; i < 10; i++)
		floor[i] = init_load(DEFAULT_P_UNIT, DEFAULT_W_UNIT);

	status = 0;
	serviced = 0;
}

void free_lists(void)
{
	struct Passenger * pass;
	struct list_head *temp, *dummy;

	list_for_each_safe(temp, dummy, &queue.pnode)
	{
		pass = list_entry(temp, struct Passenger, pnode);
		list_del(temp); kfree(pass);
	}

	list_for_each_safe(temp, dummy, &elevator.plist.pnode)
	{
		pass = list_entry(temp, struct Passenger, pnode);
		list_del(temp); kfree(pass);
	}
}

/**************************** ELEVATOR SYSCALL HELPER FUNCTIONS END *******************************/

/**************************** kthread PROCESS REQUESTS  *******************************/

void request(void)
{
	return;
}


int process_requests (void *data)
{
	struct Passenger * passenger;
	struct list_head * temp;
	struct thread_parameter *parm = data;
	int wait = 0;
	int wait2 =0;

	while(!kthread_should_stop())
	{
		mutex_lock(&parm->mutex);

		wait =0;

		if (elevator.cur_load.p_units == 0 && waiting.p_units == 0)
		{
			elevator.state = IDLE;
			mutex_unlock(&parm->mutex);
			schedule();
			mutex_lock(&parm->mutex);

		}

		//get destination of first person on elevator (FIFO)
		if (elevator.cur_floor == elevator.floorrequest)
		{
			if (elevator.cur_load.p_units == 0)
			{
				list_for_each(temp, &queue.pnode)
				{
					passenger = list_entry(temp, struct Passenger, pnode);
					elevator.floorrequest = passenger->start;
					break;
				}
			}
			else
			{
				list_for_each(temp, &elevator.plist.pnode)
				{
					passenger = list_entry(temp, struct Passenger, pnode);
					elevator.floorrequest = passenger->dest;
					break;
				}
			}

		}

		//set next destination (FIFO)
		else if (elevator.cur_floor > elevator.floorrequest)
		{
			elevator.state = DOWN;
			--elevator.cur_floor;
			mutex_unlock(&parm->mutex);
			msleep(FLOOR_WAIT);
			mutex_lock(&parm->mutex);
		}

		else if (elevator.cur_floor < elevator.floorrequest)
		{
			elevator.state = UP;
			++elevator.cur_floor;
			mutex_unlock(&parm->mutex);
			msleep(FLOOR_WAIT);
			mutex_lock(&parm->mutex);
		}

		
		printk(KERN_ALERT "Removing Passengers");
		//drop off passengers if at destination
		
		wait += removePassengers(); 	//offload (remove from plist) if passenger->dest == elevator.cur_floor
		
		printk(KERN_ALERT "Add Passengers");
		// pick up passengers on current floor

		
		wait+=addPassengers();
		
		printk(KERN_ALERT "isSleeping(): %d",wait);
		//sleep if passengers were added
		if (wait > 0)
		{
			elevator.state = LOADING;
			mutex_unlock(&parm->mutex);
			msleep(LOAD_WAIT);
			mutex_lock(&parm->mutex);
		}

		mutex_unlock(&parm->mutex);

		schedule();
	}

	
	
	while(1)
	{
		mutex_lock(&parm->mutex);
		if (elevator.cur_load.p_units==0)
		{
			mutex_unlock(&parm->mutex);
			break;
		}
		if (elevator.cur_floor == elevator.floorrequest)
		{
			list_for_each(temp, &elevator.plist.pnode)
			{
				passenger = list_entry(temp, struct Passenger, pnode);
				elevator.floorrequest = passenger->dest;
				break;
			}
		}

		else if (elevator.cur_floor > elevator.floorrequest)
		{
			elevator.state = DOWN;
			--elevator.cur_floor;
			mutex_unlock(&parm->mutex);
			msleep(FLOOR_WAIT);
			mutex_lock(&parm->mutex);
			
		}

		else if (elevator.cur_floor < elevator.floorrequest)
		{
			elevator.state = UP;
			++elevator.cur_floor;
			mutex_unlock(&parm->mutex);
			msleep(FLOOR_WAIT);
			mutex_lock(&parm->mutex);
		}

		wait2 =0;
		wait2 += removePassengers(); 	//offload (remove from plist) if passenger->dest == elevator.cur_floor

		if (wait2 > 0)
		{
			elevator.state = LOADING;
			mutex_unlock(&parm->mutex);
			msleep(LOAD_WAIT);
			mutex_lock(&parm->mutex);
			
		}
		mutex_unlock(&parm->mutex);

		schedule();
	}

	mutex_lock(&parm->mutex);
	elevator.state = OFFLINE;
	mutex_unlock(&parm->mutex);

	return 0;
}

void thread_init_parameter(struct thread_parameter *parm) 
{
	parm->kthread = kthread_run(process_requests, parm,"Processing request");
}


/**************************** ELEVATOR SYSCALLS *******************************/

long start_elevator(void)
{
    mutex_lock(&start_stop);
    
	if (status == 0)
	{
		init_elevator();
		if (status < 0)
		{
			mutex_unlock(&start_stop);
			return status;
		}

		thread_init_parameter(&thread1);
		printk(KERN_DEBUG "elevator started.\n");
		elevator.state = IDLE;
		status=1;
		mutex_unlock(&start_stop);
		return 0;
	}
	else
	{
		printk(KERN_DEBUG "elevator already started.\n");
		mutex_unlock(&start_stop);
		return 1;
	}
}


long stop_elevator(void)
{
	mutex_lock(&start_stop);
	if(status == 1)
	{
		status = 0;
		kthread_stop(thread1.kthread);
		mutex_unlock(&start_stop);
		return 1;
	}
	else
	{
		mutex_unlock(&start_stop);
		return 0;
	}
}


long issue_request(int passenger_type, int start_floor, int destination_floor)
{
	int ret = 0;

	mutex_lock(&thread1.mutex);

	if (start_floor < MIN_FLOOR || start_floor > MAX_FLOOR || destination_floor > MAX_FLOOR || destination_floor < MIN_FLOOR)
	{
		mutex_unlock(&thread1.mutex);
		return INVALID;
	}

	if (passenger_type < ADULT || passenger_type > BELLHOP)
	{
		mutex_unlock(&thread1.mutex);
		return INVALID;
	}

	switch(passenger_type) 
		{
		   case ADULT:
		   {
		     	addRequest(init_pass(ADULT_LOAD,start_floor,destination_floor,ADULT));
		      break;
		   }

		   case CHILD:
		   {
		      addRequest(init_pass(CHILD_LOAD,start_floor,destination_floor,CHILD));
		      break;
		   }

		   case RSERVICE:
		   {
		      addRequest( init_pass(RSERVICE_LOAD,start_floor,destination_floor,RSERVICE));
		      break;
		   }

		  	case BELLHOP:
		  	{
		      addRequest(init_pass(BELLHOP_LOAD,start_floor,destination_floor,BELLHOP));
		      break;
		    }

		   default :
		   {
		   		ret = INVALID;
		   		mutex_unlock(&thread1.mutex);
		   		return ret;
		   	}
		}

	ret = VALID;

	mutex_unlock(&thread1.mutex);

	return ret;
}


/**************************** MODULE INIT AND EXIT *******************************/

static int elev_mod_init(void) {
	printk(KERN_NOTICE "/proc/%s create\n",ENTRY_NAME);
	fops.open = proc_open;
	fops.read = proc_read;
	fops.release = proc_release;

	STUB_start_elevator = start_elevator;
	STUB_stop_elevator = stop_elevator;
	STUB_issue_request = issue_request;

	// intialize vars
	init_elevator();
	elevator.state = OFFLINE;
	mutex_init(&start_stop);
	mutex_init(&thread1.mutex);
	init_defaults();
	INIT_LIST_HEAD(&(queue.pnode));

	if (IS_ERR(thread1.kthread)) {
		printk(KERN_WARNING "error spawning thread");
		remove_proc_entry(ENTRY_NAME, NULL);
		return PTR_ERR(thread1.kthread);
	}
	
	if (!proc_create(ENTRY_NAME, PERMS, NULL, &fops)) {
		printk(KERN_WARNING "proc create\n");
		remove_proc_entry(ENTRY_NAME, NULL);
		return -ENOMEM;
	}
	
	return 0;
}
module_init(elev_mod_init);




static void elev_mod_exit(void) {

	stop_elevator();
	remove_proc_entry(ENTRY_NAME, NULL);
	free_lists();
	
	STUB_start_elevator = NULL;
	STUB_stop_elevator = NULL;
	STUB_issue_request = NULL;
	mutex_destroy(&start_stop);
	mutex_destroy(&thread1.mutex);
	printk(KERN_NOTICE "Removing /proc/%s\n", ENTRY_NAME);
}
module_exit(elev_mod_exit);
