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
extern long (*STUB_stop_elevator)(void);
extern long (*STUB_issue_request)(int,int,int);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Elevator module for processing requests");

#define ENTRY_NAME "elevator"
#define ENTRY_SIZE 2000
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



static struct Elevator * elevator;

static struct Passenger queue;
static struct Load waiting;

static int status;
static int serviced;


struct thread_parameter 
{
	struct task_struct *kthread;
	struct mutex mutex;
};

struct thread_parameter thread1;

/**************************** PROC VARS *******************************/

static struct file_operations fops;
static char *message;
static int read_p;


/**************************** PROC FUNCTIONS *******************************/
int proc_open(struct inode *sp_inode, struct file *sp_file) {

	read_p = 1;

	message = kmalloc(sizeof(char) * ENTRY_SIZE, __GFP_RECLAIM | __GFP_IO | __GFP_FS);

	if (message == NULL) {
		printk(KERN_WARNING "hello_proc_open");
		return -ENOMEM;
	}

	mutex_lock(&thread1.mutex);

	strcat(message,"Movement State: %d\n",state_arr[elevator->state]);
	strcat(message,"Current Floor: %d\n", elevator->cur_floor);
	strcat(message,"Next Floor: %d\n", elevator->floorrequest);
	strcat(message,"Elevator Passenger Units: %d\n Elevator Weight Units: %d\n", elevator->load.p_units,elevator->load.w_units);
	strcat(message,"Waiting Passenger Units: %d\n  Waiting Weight Units: %d\n", waiting.p_units,waiting.w_units);
	strcat(message,"Passengers Serviced: %d\n",serviced);

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

void addRequest(struct Passenger * passenger)
{
	list_add_tail(&passenger->pnode, &queue.pnode);
	waiting.p_units += passenger->load.p_units;
	waiting.p_units += passenger->load.w_units;
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
			waiting.p_units -= passger->load.p_units;
			waiting.p_units -= passger->load.w_units;
			list_del(temp); /* init ver also reinits list */
			kfree(passger); /* remember to free alloced data */
		}
	}
}

//////////////////////////////////////////////////////////////////////////

// PROCESS REQUEST HELPERS /////////////////////////////////////////////

int too_heavy(struct Load pload, struct Load eload)
{
	return (pload.p_units + eload.p_units > MAX_P_LOAD ||  pload.w_units + eload.w_units > MAX_W_LOAD);
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

int addPassengers(int currentfloor)
{
	struct list_head *temp;
	struct list_head *dummy;
	struct Passenger * passenger;
	struct Passenger * passengercopy;
	int added =0;

	if (&queue.load == 0)
		return 0;

	list_for_each_safe(temp, dummy, &queue.pnode) 
	{
		passenger = list_entry(temp, struct Passenger, pnode);

		if (passenger->start == elevator->cur_floor) /* can add condition to make sure elevator going right direction as passenger request*/
		{	
			if (add_load(&elevator->cur_load,passenger->load))
			{
				printk(KERN_ALERT "adding %d %d %d\n",
							passger->load.w_units, passger->start, passger->dest);
				passengercopy = init_pass(passenger->load,passenger->start,passenger->dest)
				list_add_tail(&passengercopy->pnode, &elevator->plist.pnode);
				removeRequest(passenger);
				++added;
			}
		}
	}

	return added;
}

int removePassengers(int currentfloor)
{
	int removed =0;
	struct list_head *temp;
	struct list_head *dummy;
	struct Passenger * passger;

	if (elevator->cur_load.p_units == 0)
		return;

	list_for_each_safe(temp, dummy, &elevator->plist.pnode) 
	{
		passger = list_entry(temp, struct Passenger, pnode); // returns item at that 

		if (passger->dest == elevator->cur_floor)
		{
			removed =1;
			printk(KERN_ALERT "removing %d %d %d\n",
							passger->load.w_units, passger->start, passger->dest);
			++serviced;
			remove_load(&elevator->cur_load,passger->load);
			list_del(temp); /* init ver also reinits list */
			kfree(passger); /* remember to free alloced data */
		}
	}

	return removed;
}


// START ELEVATOR HELPERS /////////////////////////////////////////////
void init_elevator(struct Elevator * elevator)
{
	//elevator = (struct Elevator*)malloc(sizeof(struct Elevator));
	elevator = kmalloc(sizeof(struct Elevator),GFP_KERNEL);

	INIT_LIST_HEAD(&elevator->plist.pnode);

	if (&elevator->plist.pnode == NULL)
	{
		status = -ENOMEM;
		return;
	}

	elevator->state = DEFAULT_STATE; // IDLE
	elevator->cur_floor = DEFAULT_FLOOR;  // 1
	elevator->floorrequest = DEFAULT_FLOOR;  // 1
	elevator->cur_load.p_units = DEFAULT_P_UNIT; // 0
	elevator->cur_load.w_units = DEFAULT_W_UNIT; // 0
}

struct Load init_load (int punit, int wunit)
{
     struct Load load = {punit, wunit};
     return load;
}

void init_defaults(void)
{
	ADULT_LOAD = init_load (ADULT_P_UNIT, ADULT_W_UNIT);
	CHILD_LOAD = init_load (CHILD_P_UNIT, CHILD_W_UNIT);
	RSERVICE_LOAD = init_load (RSERVICE_P_UNIT, RSERVICE_W_UNIT);
	BELLHOP_LOAD = init_load (BELLHOP_P_UNIT, BELLHOP_W_UNIT);
	waiting = init_load(DEFAULT_P_UNIT, DEFAULT_W_UNIT);
	status = 0;
	serviced = 0;
}

void free_queue()
{
	Passenger * pass;
	struct list_head *temp, *dummy;

	list_for_each_safe(temp, dummy, &queue.pnode)
	{
		pass = list_entry(temp, struct Passenger, pnode);
		list_del(temp); kfree(pass);
	}
}

/**************************** ELEVATOR SYSCALL HELPER FUNCTIONS END *******************************/

/**************************** kthread PROCESS REQUESTS  *******************************/

int process_requests (void *data)
{

	struct Passenger * passenger;
	struct list_head * temp;
	struct thread_parameter *parm = data;

	while(!kthread_should_stop())
	{

		if (elevator->load.p_units == 0 && queue.p_units == 0 && status == 1)
		{
			mutex_lock(&parm->mutex);
			elevator->state = IDLE;
			mutex_unlock(&parm->mutex);
			schedule();
		}

		mutex_lock(&parm->mutex);

		int wait = 0;


		//drop off passengers if at destination
		elevator->state = LOADING;
		wait += removePassengers(elevator->cur_floor); 	//offload (remove from plist) if passenger->dest == elevator->cur_floor

		// pick up passengers on current floor
		if (status == 1)
		{wait+=addPassengers(elevator->cur_floor);}

		//sleep if passengers were added
		if (wait > 0)
		{msleep(LOAD_WAIT);}

		//get destination of first person on elevator (FIFO)
		if (elevator->cur_floor == elevator->floorrequest)
		{
			list_for_each(temp, &elevator->plist.pnode)
			{
				passenger = list_entry(temp, struct Passenger, pnode);
				elevator->floorrequest = passenger->dest;
				break;
			}
		}

		//set next destination (FIFO)
		else if (elevator->cur_floor > elevator->floorrequest)
		{
			elevator->state = DOWN;
			msleep(FLOOR_WAIT);
			--elevator->cur_floor;
		}

		else if (elevator->cur_floor < elevator->floorrequest)
		{
			elevator->state = UP;
			msleep(FLOOR_WAIT);
			++elevator->cur_floor;
		}

		mutex_unlock(&parm->mutex);
	}

	mutex_lock(&parm->mutex);

	while(elevator->load.p_units > 0)
	{
		elevator->state = LOADING;
		wait += removePassengers(elevator->cur_floor); 	//offload (remove from plist) if passenger->dest == elevator->cur_floor

		if (wait > 0)
		{msleep(LOAD_WAIT);}

		if (elevator->cur_floor == elevator->floorrequest)
		{
			list_for_each(temp, &elevator->plist.pnode)
			{
				passenger = list_entry(temp, struct Passenger, pnode);
				elevator->floorrequest = passenger->dest;
				break;
			}
		}

		else if (elevator->cur_floor > elevator->floorrequest)
		{
			elevator->state = DOWN;
			msleep(FLOOR_WAIT);
			--elevator->cur_floor;
		}

		else if (elevator->cur_floor < elevator->floorrequest)
		{
			elevator->state = UP;
			msleep(FLOOR_WAIT);
			++elevator->cur_floor;
		}
	}

	mutex_unlock(&parm->mutex);


	return 0;
}

void thread_init_parameter(struct thread_parameter *parm) 
{
	mutex_init(&parm->mutex);
	parm->kthread = kthread_run(process_requests, parm,"Processing request");
}




/**************************** ELEVATOR SYSCALLS *******************************/

long start_elevator(void)
{
    mutex_lock(&thread1.mutex);
	if (status == 0 || status < 0)
	{
		if (status == 0)
		{
			init_elevator(elevator);
			printk(KERN_DEBUG "Shuttle started.\n");
			mutex_unlock(&thread1.mutex);
			return status++;
		}
		else
		{
			int ret = status;
			status = 0;
			printk(KERN_DEBUG "Error Starting shuttle.\n");
			mutex_unlock(&thread1.mutex);
			return ret;
		}
	}
	printk(KERN_DEBUG "elevator already started.\n");
	mutex_unlock(&thread1.mutex);
	return (long)status;
}

long stop_elevator(void)
{
	mutex_lock(&thread1.mutex);
	if(status != 0 && elevator->state != OFFLINE)
	{
		printk(KERN_DEBUG "Elevator deactivating.\n");

		status =0;
		kthread_stop(&thread1.kthread);

		elevator->state = OFFLINE;
		printk(KERN_DEBUG "Elevator deactivated.\n");
		mutex_unlock(&thread1.mutex);
		return 0;
	}
	else
	{
		printk(KERN_DEBUG "Elevator already deactivating.\n");
		mutex_unlock(&thread1.mutex);
		return 1;
	}
}


long issue_request(int passenger_type, int start_floor, int destination_floor)
{

	struct Passenger * passenger;

	if (start_floor < MIN_FLOOR || start_floor > MAX_FLOOR || destination_floor > MAX_FLOOR || destination_floor < MIN_FLOOR)
		return (long)INVALID;

	if (passenger_type < ADULT || passenger_type > BELLHOP)
		return (long)INVALID;

	mutex_lock(&thread1.mutex);

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
		   {
		   		mutex_unlock(&thread1.mutex);  
		   		return (long)INVALID;
		   	}
		}

	addRequest(passenger);

	mutex_unlock(&thread1.mutex);

	return (long)VALID;
}




/**************************** MODULE INIT AND EXIT *******************************/

static int elev_mod_init(void) {
	printk(KERN_NOTICE "/proc/%s create\n",ENTRY_NAME);
	fops.open = hello_proc_open;
	fops.read = hello_proc_read;
	fops.release = hello_proc_release;

	STUB_start_elevator = start_elevator;
	STUB_stop_elevator = stop_elevator;
	STUB_issue_request = issue_request;

	// intialize vars
	init_defaults();
	INIT_LIST_HEAD(&queue.pnode);
	thread_init_parameter(&thread1);

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

	if (status != 0)
		stop_elevator();

	remove_proc_entry(ENTRY_NAME, NULL);
	kthread_stop(thread1.kthread);
	remove_proc_entry(ENTRY_NAME, NULL);
	mutex_destroy(&thread1.mutex);

	kfree(elevator);
	free_queue();

	STUB_start_elevator = NULL;
	STUB_stop_elevator = NULL;
	STUB_issue_request = NULL;

	//release elevator / defaults and queue head
	printk(KERN_NOTICE "Removing /proc/%s\n", ENTRY_NAME);
}
module_exit(elev_mod_exit);
