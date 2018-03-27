#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/delay.h>

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
	int floorrequest;
	struct Load cur_load;
	struct list_head plist;
};

static struct Load ADULT_LOAD;
static struct Load CHILD_LOAD;
static struct Load RSERVICE_LOAD;
static struct Load BELLHOP_LOAD;
static struct Load waiting;

static struct Elevator * elevator;
static struct list_head queue;
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

//PROC DATA

// movement state
static char * state_arr[5] = {"IDLE", "LOADING", "OFFLINE", "UP", "DOWN"};

//current floor
//nextfloor to service
//current load

//load of people weighting
//number of people serviced


/*************** STUB VARS *******************************************/
extern long (*STUB_start_elevator)();
extern long (*STUB_stop_elevator)();
extern long (*STUB_issue_request)(int,int,int);


/**************************** PROC FUNCTIONS *******************************/
int hello_proc_open(struct inode *sp_inode, struct file *sp_file) {
	int i;
	char *buf;
	read_p = 1;

	buf = kmalloc(sizeof(char) * ENTRY_SIZE, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
	message = kmalloc(sizeof(char) * ENTRY_SIZE, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
	if (message == NULL) {
		printk(KERN_WARNING "hello_proc_open");
		return -ENOMEM;
	}
	
	sprintf(message, "Thread %d has cycled this many times:\n", thread1.id);
	if (mutex_lock_interruptible(&thread1.mutex) == 0) {
		for (i = 0; i < CNT_SIZE; i++) {
			sprintf(buf, "%d\n", thread1.cnt[i]);
			strcat(message, buf);
		}
	}
	else {
		for (i = 0; i < CNT_SIZE; i++) {
			sprintf(buf, "%d\n", -1);
			strcat(message, buf);
		}
	}
	mutex_unlock(&thread1.mutex);
	strcat(message, "\n");

	return 0;
}

ssize_t hello_proc_read(struct file *sp_file, char __user *buf, size_t size, loff_t *offset) {
	int len = strlen(message);
	
	read_p = !read_p;
	if (read_p)
		return 0;
		
	printk(KERN_INFO "proc called read\n");
	copy_to_user(buf, message, len); //edit to diplay necessary data
	return len;
}

int hello_proc_release(struct inode *sp_inode, struct file *sp_file) {
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

int addRequest(struct Passenger * passenger)
{
	list_add_tail(&passenger->pnode, &queue);
	waiting.p_units += passenger->p_units;
	waiting.p_units += passenger->w_units;
}

int removeRequest(struct Passenger * passenger)
{
	struct list_head *temp;
	struct list_head *dummy;
	struct Passenger * passger;

	list_for_each_safe(temp, dummy, &queue) 
	{
		passger = list_entry(temp, struct Passenger, pnode); // returns item at that 

		if (passger == passenger)
		{
			waiting.p_units -= passger->p_units;
			waiting.p_units -= passger->w_units;
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
  	++serviced;
}

int addPassengers(int currentfloor)
{
	struct Passenger * passenger;
	int added =0;

	if (list_empty(&queue))
		return;

	list_for_each_safe(temp, dummy, &queue) 
	{
		passenger = list_entry(temp, struct Passenger, pnode);

		if (add_load(&elevator->cur_load,passenger->load) && status == 1) // can add condition to make sure elevator going right direction as passenger request
		{
			list_add_tail(&passenger->pnode, &elevator->plist);
			removeRequest(passenger);
			++added;
		}
	}

	return added;
}

void removePassengers(int currentfloor)
{
	struct list_head *temp;
	struct list_head *dummy;
	struct Passenger * passger;

	if (list_empty(&elevator->plist))
		return;

	list_for_each_safe(temp, dummy, &elevator->plist) 
	{
		passger = list_entry(temp, struct Passenger, pnode); // returns item at that 

		if (passger->dest == currentfloor)
		{
			remove_load(&elevator->cur_load,passenger->load)
			list_del(temp); /* init ver also reinits list */
			kfree(passger); /* remember to free alloced data */
		}
	}
}


// START ELEVATOR HELPERS /////////////////////////////////////////////
void init_elevator(struct Elevator * elevator)
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
	elevator->floorrequest = DEFAULT_FLOOR;  // 1
	elevator->cur_load.p_units = DEFAULT_P_UNIT; // 0
	elevator->cur_load.w_units = DEFAULT_W_UNIT; // 0
}

struct Load init_load (int punit, int wunit)
{
     struct Load load = {punit, wunit};
     return load;
}

void init_defaults()
{
	ADULT_LOAD = init_load (ADULT_P_UNIT, ADULT_W_UNIT);
	CHILD_LOAD = init_load (CHILD_P_UNIT, CHILD_W_UNIT);
	RSERVICE_LOAD = init_load (RSERVICE_P_UNIT, RSERVICE_W_UNIT);
	BELLHOP_LOAD = init_load (BELLHOP_P_UNIT, BELLHOP_W_UNIT);
	waiting = init_load(DEFAULT_P_UNIT, DEFAULT_W_UNIT);
	status = 0;
	serviced = 0;
}

/**************************** ELEVATOR SYSCALL HELPER FUNCTIONS END *******************************/

/**************************** kthread PROCESS REQUESTS  *******************************/

void thread_init_parameter(struct thread_parameter *parm) 
{
	mutex_init(&parm->mutex);
	parm->kthread = kthread_run(process_request, parm,"Processing request");
}


void process_request (void *data)
{

	struct Passenger * passenger;
	struct list_head * temp;
	struct thread_parameter *parm = data;

	// if (mutex_lock_interruptible(&parm->mutex) == 0) 
	// {
	// 	if (list_empty(&elevator->plist))
	// 	{
	// 		elevator->state = IDLE;
	// 		return
	// 	}
	// }

	// mutex_unlock(&parm->mutex);

	while(1)
	{
		if (mutex_lock_interruptible(&parm->mutex) == 0) 
		{
			if ((list_empty(&queue) || status == 0) && list_empty(elevator->plist))
				break;

			elevator->state = LOADING;
			msleep(LOAD_WAIT);
			removePassengers(elevator->cur_floor); 	//offload (remove from plist) if passenger->dest == elevator->cur_floor
			addPassengers(elevator->cur_floor); 	//onload (add to plist) if status set to one

			if (elevator->cur_floor == elevator->floorrequest)
			{
				list_for_each_safe(temp, &example_list)
				{
					passenger = list_entry(temp, struct Passenger, pnode);
					elevator->floorrequest = passenger->dest;
					break;
				}
			}

			//set elevator state to UP if passenger[0]->dest > elevator->cur_floor
			if (elevator->cur_floor > elevator->floorrequest)
			{
				elevator->state = DOWN;
				msleep(FLOOR_WAIT);
				--elevator->cur_floor;
			}

			//set elevator state to DOWN if passenger[0]->dest < elevator->cur_floor
			if (elevator->cur_floor < elevator->floorrequest)
			{
				elevator->state = UP;
				msleep(FLOOR_WAIT);
				++elevator->cur_floor;
			}

			
		}

		mutex_unlock(&parm->mutex);
	}

	mutex_unlock(&parm->mutex);
	elevator->state = IDLE;

	return;
}

/**************************** kthread PROCESS REQUESTS END *******************************/

/**************************** ELEVATOR SYSCALLS *******************************/

long start_elevator()
{	
	if (status == 0 or status < 0)
	{
		if (status == 0)
		{
			elevator->state = IDLE;
			return status++;
		}
		else
		{
			int ret = status;
			status = 0;
			return ret;
		}
	}

	return (long)status;
}

long stop_elevator()
{
	if (status == 1)
	{	
		status = 0;

		while(!list_empty($elevator->plist)) // or while thread is still running
			// continue executing 
		elevator->state = OFFLINE;
	}

	return (long)status;
}

long issue_request(int passenger_type, int start_floor, int destination_floor)
{  

	if (status == 0)
		return (long)INVALID;

	if (start_floor < MIN_FLOOR || start_floor > MAX_FLOOR || destination_floor > MAX_FLOOR || destination_floor < MIN_FLOOR)
		return (long)INVALID;

	if (passenger_type < ADULT || passenger_type > BELLHOP)
		return (long)INVALID;

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
		   		return (long)INVALID;
		}

	if (mutex_lock_interruptible(thread1.mutex) == 0) 
	{
		addRequest(passenger);
	}

	mutex_unlock(thread1.mutex);

	return (long)VALID;
}

/**************************** ELEVATOR SYSCALLS END *******************************/


/**************************** MODULE INIT *******************************/
static int elev_mod_init(void) {
	printk(KERN_NOTICE "/proc/%s create\n",ENTRY_NAME);
	fops.open = hello_proc_open;
	fops.read = hello_proc_read;
	fops.release = hello_proc_release;

	STUB_start_elevator = start_elevator;
	STUB_stop_elevator = stop_elevator;
	STUB_issue_request = issue_request;

	// intialize vars
	init_elevator(elevator);
	init_defaults();
	INIT_LIST_HEAD(&queue);

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


/**************************** MODULE EXIT *******************************/
static void elev_mod_exit(void) {
	remove_proc_entry(ENTRY_NAME, NULL);
	kthread_stop(thread1.kthread);
	remove_proc_entry(ENTRY_NAME, NULL);
	mutex_destroy(&thread1.mutex);
	//release elevator / defaults and queue head
	printk(KERN_NOTICE "Removing /proc/%s\n", ENTRY_NAME);
}
module_exit(elev_mod_init);
