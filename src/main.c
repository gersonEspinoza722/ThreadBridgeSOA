/*
Name: Amith Ananthram
This program simulates a 1-directional bridge, and uses a combination
of locks and conditional variables to synchronize the traffic across
this bridge.
Running this code:
./bridge [NUMBER_OF_CARS_TO_SIMULATE] [MAX_LOAD]
Note that the upper bound for this is MAX_CARS (10000).
Each car sends 1 second on the bridge (a sleep built in).p
It include the additional features poutlined in the project description:
	- cars don't pass each other once on the bridge
		(the print out does not reveal their positions on the bridge, but 
		 is rather numerically order -- this feature can be observed as cars
		 leave, however -- they leave the bridge in the order they arrived)
	- no direction is starved
		(once 2 * max_load cars have gone across the bridge in any one direction,
		 it switches traffic to the other direction)
Designs:
	2 conditional variables:
		toward_west
		toward_east
	They basically functioned as green lights for each direction of traffic.
	More exlanation in the code.
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define MAX_CARS 1000
#define TO_EAST 0
#define TO_WEST 1

// taken from lecture notes
#  define PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP \
  { { 0, 0, 0, PTHREAD_MUTEX_ERRORCHECK_NP, 0, { 0 } } }

// variables globales
// Los siguientes arrays representan el puente y las listas de espera de cada lado.
int bridge[MAX_CARS+1];
int east_waiting[MAX_CARS+1];
int west_waiting[MAX_CARS+1];

int amount_of_traffic; // total # of cars to cross (argv[1])
int max_load;

int east_count; // # of norwich cars on the bridge now
int west_count; // # on hanover cars on the bridge now

int east_wait; // # of norwich cars waiting for the bridge
int west_wait; // # of hanover cars waiting for the bridge

int east_lim; // # of norwich cars that have crossed in a row
int west_lim; // # of hanover cars that have crossed in a row

int firstDirectionAmount = 0;
int secondDirectionAmount = 0;

pthread_mutex_t lock =  PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;


/*
	toward_west -- un semaforo para el trafico desde el oeste (recibe una senial cuando 
		el puente esta libre de trafico desde el este).
	toward_east -- un semaforo para el trafico desde el este (recibe una senial cuando 
		el puente esta libre de trafico desde el oeste).
*/
pthread_cond_t toward_west = PTHREAD_COND_INITIALIZER; 
pthread_cond_t toward_east = PTHREAD_COND_INITIALIZER;


/*
 Genera un numero random desde 0 hasta max.
*/
int generateRandom(int max){
	return rand() % (max + 1);
}

/*
 Imprime el estado del puente y las listas de espera de cada lado.
*/
void printStatus(int direction){
	//Los carros se imprimen en orden numerico. Si el carro en i no esta en el puente, se usa -1.
	//-----------------------------------------------------------
	if(direction == TO_EAST)
		printf("\tPUENTE (HACIA EL ESTE):\t{");
	else
		printf("\tPUENTE (HACIA EL OESTE):\t{");

	for(int i = 0; i < amount_of_traffic; i++)
		if(bridge[i] != -1)
			printf(" %d ", bridge[i]);
			
	//-----------------------------------------------------------
	printf("}\n\tESPERANDO EN EL ESTE:\t{");

	for(int i = 0; i < amount_of_traffic; i++)
		if(east_waiting[i] != -1)
			printf(" %d ", east_waiting[i]);
			
	//-----------------------------------------------------------
	printf("}\n\tESPERANDO EN EL OESTE:\t{");

	for(int i = 0; i < amount_of_traffic; i++)
		if(west_waiting[i] != -1)
			printf(" %d ", west_waiting[i]);	

	printf("}\n\n");

	fflush(NULL);
}

/*
	The main function called by each thread.  Manage entering, crossing, and
	exiting the bridge for each vehicle.
	Includes the locks, conditional variables, etc.  Exlained in more detail
	within.
*/
void *oneVehicle(void *vargp)
{
	int rc;
	
	int direction;

	int car_number = (int) vargp;
	
	if (firstDirectionAmount > 0) { 
		direction = 0;
		firstDirectionAmount--;
	} else if (secondDirectionAmount > 0) { 
		direction = 1;
		secondDirectionAmount--;
	} 


// ArriveBridge
// Here, based on the direction, the thread gets a lock.  If there are no
// cars on the bridge going the opposite direction AND there's still room
// on the bridge for another car, it waits for its conditional variable
// (basically a green light).

	rc = pthread_mutex_lock(&lock);

	if (rc)
		{
			printf("Lock failed!\n");
			exit(-1);
		}
	
	if(direction == TO_EAST)
	{
// Until the signal has been acquired, this vehicle is waiting.  It's saved
// in the waiting array and counted among the waiting cars.
		east_wait++;		
		east_waiting[car_number] = car_number;

// If there's no traffic going the other way on the bridge, and still room
// (under the max_load), then wait for the green light and acquire the lock.
		while(west_count > 0 || east_count >= max_load)
		{
		pthread_cond_wait(&toward_east, &lock);
		}

// Now that it's received the green light, it's no longer waiting.
		east_waiting[car_number] = -1;	
		west_waiting[car_number] = -1;
		east_wait--;

// It's on the bridge, so now it's counted in that array.
		east_count++;	
		bridge[car_number] = car_number;			

// And it's consecutive count is maintained to switch directions later.
		east_lim++;

// The state of the bridge has changed, so print its new state.
		printf("%d is on the bridge:\n", car_number);
		printStatus(TO_EAST);

// If there are no more norwich-bound cars OR max_load*2 norwich-bound cars
// have already crossed, stop traffic and signal traffic for the other direction.
// Otherwise, keep them coming this way.
		if((east_count == 0 || east_lim >= (max_load*2)) && west_wait > 0)
			pthread_cond_signal(&toward_west);
		else
			pthread_cond_signal(&toward_east);
	}
	else
	{
// Until the signal has been acquired, this vehicle is waiting.  It's saved
// in the waiting array and counted among the waiting cars.
		west_wait++;	
		west_waiting[car_number] = car_number;	

// If there's no traffic going the other way on the bridge, and still room
// (under the max_load), then wait for the green light and acquire the lock.
		while(east_count > 0 || west_count >= max_load)
		{
		pthread_cond_wait(&toward_west, &lock);
		}

// Now that it's received the green light, it's no longer waiting.
		east_waiting[car_number] = -1;	
		west_waiting[car_number] = -1;	
		west_wait--;

// It's on the bridge, so now it's counted in that array.
		west_count++;
		bridge[car_number] = car_number;

// And it's consecutive count is maintained to switch directions later.
		west_lim++;

// The state of the bridge has changed, so print its new state.
		printf("%d is on the bridge:\n", car_number);
		printStatus(TO_WEST);

// If there are no more hanover-bound cars OR max_load*2 hanover-bound cars
// have already crossed, stop traffic and signal traffic for the other direction.
// Otherwise, keep them coming this way.
		if((west_count == 0 || west_lim >= (max_load*2)) && east_wait > 0)
			pthread_cond_signal(&toward_east);
		else
			pthread_cond_signal(&toward_west);
	}

	rc = pthread_mutex_unlock(&lock);

	if (rc)
		{
			printf("Unlock failed!\n");
			exit(-1);
		}

// OnBridge
// Simply a sleep, to make the simulation more meaningful.	
	sleep(1);	// takes 1 seconds to get off the bridge

// ExitBridge
// Here, the threads acquire locks and simulate exiting the bridge.
// If there's still traffic going in the same direction AND it hasn't
// reached max_load*2 cars consecutively, it signals for another car
// of the same direction on.  Else, it singals for a car from the different
// direction to come on.
	rc = pthread_mutex_lock(&lock);

	if (rc)
		{
			printf("Lock failed!\n");
			exit(-1);
		}
	
	if(direction == TO_EAST)
	{	
// Car has left the bridge (record it in arrays)
		east_count--;
		bridge[car_number] = -1;	

// The opposite direction's consecutive travel meter can be reset
		west_lim = 0;

// If there are no more norwich-bound cars OR max_load*2 norwich-bound cars
// have already crossed, stop traffic and signal traffic for the other direction.
// Otherwise, keep them coming this way.
		if((east_count == 0 || east_lim >= (max_load*2)) && west_wait > 0)
			pthread_cond_signal(&toward_west);
		else
			pthread_cond_signal(&toward_east);

// The status of the bridge has changed, so print it again.
		printf("%d is off the bridge:\n", car_number);
		printStatus(TO_EAST);
	}
	else
	{
// Car has left the bridge (record it in arrays)
		west_count--;
		bridge[car_number] = -1;		

// The opposite direction's consecutive travel meter can be reset
		east_lim = 0;

// If there are no more hanover-bound cars OR max_load*2 hanover-bound cars
// have already crossed, stop traffic and signal traffic for the other direction.
// Otherwise, keep them coming this way.
		if((west_count == 0 || west_lim >= (max_load*2)) && east_wait > 0)
			pthread_cond_signal(&toward_east);
		else
			pthread_cond_signal(&toward_west);

// The status of the bridge has changed, so print it again.
		printf("%d is off the bridge:\n", car_number);
		printStatus(TO_WEST);
	}

	rc = pthread_mutex_unlock(&lock);

	if (rc)
		{
			printf("Unlock failed!\n");
			exit(-1);
		}

	return NULL;
}

int main(int argc, char *argv[])
{
	firstDirectionAmount = atoi(argv[1]);
	secondDirectionAmount = atoi(argv[2]);
	amount_of_traffic = firstDirectionAmount + secondDirectionAmount;
	max_load = atoi(argv[3]);

	pthread_t cars[amount_of_traffic];		// contains all the pthreads

	int i, rc;

	srand(time(NULL));

// the following lines initialize all the threads variables to zero and the
// values in the arrays to -1

	east_count = 0;
	east_wait = 0;
	east_lim = 0;

	west_count = 0;
	west_wait = 0;
	west_lim = 0;

	for(i = 0; i < amount_of_traffic; i++)
	{
		west_waiting[i] = -1;
		east_waiting[i] = -1;
		bridge[i] = -1;
	}

// this for loop creates all the threads, simply assing them the index of
// each vehicle as an argument
	for(i = 0; i < amount_of_traffic; i++)
	{
		rc = pthread_create(&cars[i],
							NULL,
							oneVehicle,
							(void *) i);

		if (rc)
		{
			printf("Creation of new thread failed!\n");
			exit(-1);
		}
	}

// waits for the threads to close!
	for(i = 0; i < amount_of_traffic; i++)
	{
		rc = pthread_join(cars[i],
						  NULL);

		if (rc)
		{
			printf("Closing a thread failed!\n");
			exit(-1);
		}
	}



	return 0;
}
