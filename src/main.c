#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define MAX_CARS 1000
#define TO_EAST 0
#define TO_WEST 1

// taken from lecture notes TODO: Quitar esto?
#define PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP            \
	{                                                      \
		{                                                  \
			0, 0, 0, PTHREAD_MUTEX_ERRORCHECK_NP, 0, { 0 } \
		}                                                  \
	}

// variables globales
// Los siguientes arrays representan el puente y las listas de espera de cada lado.
int bridge[MAX_CARS + 1];
int east_waiting[MAX_CARS + 1];
int west_waiting[MAX_CARS + 1];

int amount_of_traffic; // # de carros en total
int max_load;

int east_count; // # de carros cruzando el puente puente desde el este
int west_count; // # de carros cruzando el puente puente desde el oeste
int east_wait;	// # de carros hacia el este esperando para cruzar el puente
int west_wait;	// # de carros hacia el oeste esperando para cruzar el puente

int to_east_amount = 0;
int to_west_amount = 0;

pthread_mutex_t lock = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;

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
int generateRandom(int max)
{
	return rand() % (max + 1);
}

/*
 Imprime el estado del puente y las listas de espera de cada lado.
*/
void printStatus(int direction)
{
	// Los carros se imprimen en orden numerico. Si el carro en i no esta en el puente, se usa -1.
	//-----------------------------------------------------------
	if (direction == TO_EAST)
		printf("\tPUENTE (HACIA EL ESTE):\t{");
	else
		printf("\tPUENTE (HACIA EL OESTE):\t{");

	for (int i = 0; i < amount_of_traffic; i++)
		if (bridge[i] != -1)
			printf(" %d ", bridge[i]);

	//-----------------------------------------------------------
	printf("}\n\tESPERANDO EN EL ESTE:\t{");

	for (int i = 0; i < amount_of_traffic; i++)
		if (east_waiting[i] != -1)
			printf(" %d ", east_waiting[i]);

	//-----------------------------------------------------------
	printf("}\n\tESPERANDO EN EL OESTE:\t{");

	for (int i = 0; i < amount_of_traffic; i++)
		if (west_waiting[i] != -1)
			printf(" %d ", west_waiting[i]);

	printf("}\n\n");
	fflush(NULL);
}

/*
Funcion principal que usa cada hilo/carro.
*/
void *oneVehicle(void *vargp)
{
	int lock_result;
	int direction;
	int car_id = (int)vargp;

	if (to_east_amount > 0)
	{
		direction = TO_EAST;
		to_east_amount--;
	}
	else if (to_west_amount > 0)
	{
		direction = TO_WEST;
		to_west_amount--;
	}

	printf ("Se creó el %d con dirección %d \n", car_id, direction);

	/*
	Llegada al puente: El hilo puede obtener el lock segun su direccion. Si no hay carros en el puente
	en direccion opuesta y aun hay campo, el hilo espera por su luz verde.
	*/
	lock_result = pthread_mutex_lock(&lock);
	if (lock_result)
	{ // Hubo un error
		printf("Lock fallo! %d\n", lock_result);
		exit(-1);
	}

	if (direction == TO_EAST)
	{
		east_wait++; // El carro esta en espera de la senial.
		east_waiting[car_id] = car_id;

		while (west_count > 0 || east_count >= max_load)
		{ // Si no hay trafico en la otra direccion y espacio, espere senial
			pthread_cond_wait(&toward_east, &lock);
		}

		east_waiting[car_id] = -1; // Ya recibio la senial
		west_waiting[car_id] = -1;
		east_wait--;

		east_count++; // Entra al puente
		bridge[car_id] = car_id;

		printf("%d esta en el puente:\n", car_id);
		printStatus(TO_EAST);

		// Si no hay mas carros que van hacia el este, se da la senial para la otra direccion.
		if (east_count == 0 && west_wait > 0)
			pthread_cond_signal(&toward_west);
		else
			pthread_cond_signal(&toward_east);
	}
	else
	{
		west_wait++; // El carro esta en espera de la senial.
		west_waiting[car_id] = car_id;

		while (east_count > 0 || west_count >= max_load)
		{ // Si no hay trafico en la otra direccion y espacio, espere senial
			pthread_cond_wait(&toward_west, &lock);
		}

		east_waiting[car_id] = -1; // Ya recibio la senial
		west_waiting[car_id] = -1;
		west_wait--;

		west_count++; // Entra al puente
		bridge[car_id] = car_id;

		printf("%d esta en el puente:\n", car_id);
		printStatus(TO_WEST);

		// Si no hay mas carros que van hacia el oeste, se da la senial para la otra direccion.
		if (west_count == 0 && east_wait > 0)
			pthread_cond_signal(&toward_east);
		else
			pthread_cond_signal(&toward_west);
	}

	lock_result = pthread_mutex_unlock(&lock);
	if (lock_result)
	{
		printf("Unlock fallo! %d\n", lock_result);
		exit(-1);
	}

	/*
	Dentro del puente: Un pequenio sleep para darle mas sentido a la simulacion. Se dura 3 segundo cruzando el puente.
	*/
	sleep(3);

	/*
	Salida del puente: El hilo obtiene el lock y envia seniales para el siguiente carro o cambio de direccion.
	*/
	lock_result = pthread_mutex_lock(&lock);
	if (lock_result)
	{ // Hubo un error
		printf("Lock fallo! %d\n", lock_result);
		exit(-1);
	}

	if (direction == TO_EAST)
	{
		east_count--; // Salio del puente
		bridge[car_id] = -1;

		// Si no hay mas carros que van hacia el este, se da la senial para la otra direccion.
		if (east_count == 0 && west_wait > 0)
			pthread_cond_signal(&toward_west);
		else
			pthread_cond_signal(&toward_east);

		printf("%d esta fuera del puente:\n", car_id);
		printStatus(TO_EAST);
	}
	else
	{
		west_count--; // Salio del puente
		bridge[car_id] = -1;

		// Si no hay mas carros que van hacia el oeste, se da la senial para la otra direccion.
		if (west_count == 0 && east_wait > 0)
			pthread_cond_signal(&toward_east);
		else
			pthread_cond_signal(&toward_west);

		printf("%d esta fuera del puente:\n", car_id);
		printStatus(TO_WEST);
	}

	lock_result = pthread_mutex_unlock(&lock);
	if (lock_result)
	{
		printf("Unlock fallo! %d\n", lock_result);
		exit(-1);
	}

	return NULL;
}

/**
 * @brief Funcion utilizada para revisar que se ingresaron los parametros correctamente.
 */
void check_paramethers(int argc, char *argv[])
{
	if (argc < 3)
	{
		printf("Para ejecutar escriba: \n ./main to_east_amount to_west_amount bridge_max_load");
		exit(-1);
	}
}

int main(int argc, char *argv[])
{
	// Revisar los parametros antes de ejecutar.
	check_paramethers(argc, argv);

	// Leer de la consola la cantidad de autos en cada dirección.
	to_east_amount = atoi(argv[1]);
	to_west_amount = atoi(argv[2]);

	// Leer de los parametros la cantidad total de flujo de autos posibles para el puente.
	max_load = atoi(argv[3]);

	// Cantidad total de tráfico para esta ejecución.
	amount_of_traffic = to_east_amount + to_west_amount;

	// Almacenas todos los hilos/autos.
	pthread_t cars[amount_of_traffic];

	int i, result;

	// Se estable que los números aleatorios se generaran usando como semilla la función de hora del procesador.
	srand(time(NULL));

	// Inicializar todo
	east_count = 0;
	east_wait = 0;

	west_count = 0;
	west_wait = 0;

	// Crea los estados por defecto para las listas de espera y el puente.
	for (i = 0; i < amount_of_traffic; i++)
	{
		west_waiting[i] = -1;
		east_waiting[i] = -1;
		bridge[i] = -1;
	}

	// Ciclo que recorre la cantidad total de autos creando de forma aleatoria y con tiempos distribuidos sus respectivos hilos.
	for (i = 0; i < amount_of_traffic; i++)
	{
		// Pthread crea un hilo para un auto, se le asigna un id y la dirección. Además, se asocia la función handler del hilo (oneVehicle).
		result = pthread_create(&cars[i], NULL, oneVehicle, (void *)i);

		// Si se produce un error al crear el hilo se genera un mensaje.
		if (result)
		{
			printf("Fallo al crear hilo nuevo %d\n", result);
			exit(-1);
		}
	}

	// Esperar cierre de los hilos.
	for (i = 0; i < amount_of_traffic; i++)
	{
		result = pthread_join(cars[i], NULL);

		if (result)
		{
			printf("Fallo al cerrar un hilo %d\n", result);
			exit(-1);
		}
	}
	return 0;
}
