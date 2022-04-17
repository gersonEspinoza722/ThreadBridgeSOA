// *************  Lista de bibliotecas importadas *************
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

// ************* Lista de varibles CONSTANTES definidas *************
#define MAX_CARS 1000
#define TO_EAST 0
#define TO_WEST 1
#define MEDIAN 2


// TODO: revisar para saber si remover o no.
#define PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP            \
	{                                                      \
		{                                                  \
			0, 0, 0, PTHREAD_MUTEX_ERRORCHECK_NP, 0, { 0 } \
		}                                                  \
	}
// ************* Lista de varibles globales *************
// Los siguientes arrays representan el puente y las listas de espera de cada lado.
int bridge[MAX_CARS + 1];
int east_waiting[MAX_CARS + 1];
int west_waiting[MAX_CARS + 1];

int total_cars_simulated; 					// Total de autos.
int max_load;								// Total de autos que soporta el puente.
int east_count; 	// # de carros cruzando el puente puente desde el este
int west_count; 	// # de carros cruzando el puente puente desde el oeste
int east_wait;		// # de carros hacia el este esperando para cruzar el puente
int west_wait;		// # de carros hacia el oeste esperando para cruzar el puente
int to_east_amount = 0;
int to_west_amount = 0;

// ************* Lista de estructuras de datos *************
struct arg_struct
{
	int id;
	int dir;
} * thread_args;

// ************* Lista de variables de configuración para los threads *************
pthread_mutex_t lock = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;
// toward_west -- un semaforo para el trafico desde el oeste (recibe una senial cuando 	el puente esta libre de trafico desde el este).
pthread_cond_t toward_west = PTHREAD_COND_INITIALIZER;
// toward_east -- un semaforo para el trafico desde el este (recibe una senial cuando el puente esta libre de trafico desde el oeste).
pthread_cond_t toward_east = PTHREAD_COND_INITIALIZER;

// ************* Lista de funciones de utilidad *************


// Devuelve el tiempo de espera en segundos según una distribución exponencial.
double get_delay_time (int median) {
    int val = rand();
    while (val == RAND_MAX)
        val = rand();
    double decimal = (double) val / RAND_MAX;
    return log (1 - decimal) * (-median);
}

// Imprime el estado del puente y las listas de espera de cada lado.
void printStatus(int direction)
{
	// Los carros se imprimen en orden numerico. Si el carro en i no esta en el puente, se usa -1.
	//-----------------------------------------------------------
	if (direction == TO_EAST)
		printf("\tPUENTE (HACIA EL ESTE):\t{");
	else
		printf("\tPUENTE (HACIA EL OESTE):\t{");

	for (int i = 0; i < total_cars_simulated; i++)
		if (bridge[i] != -1)
			printf(" %d ", bridge[i]);

	//-----------------------------------------------------------
	printf("}\n\tESPERANDO EN EL ESTE:\t{");

	for (int i = 0; i < total_cars_simulated; i++)
		if (east_waiting[i] != -1)
			printf(" %d ", east_waiting[i]);

	//-----------------------------------------------------------
	printf("}\n\tESPERANDO EN EL OESTE:\t{");

	for (int i = 0; i < total_cars_simulated; i++)
		if (west_waiting[i] != -1)
			printf(" %d ", west_waiting[i]);

	printf("}\n\n");
	fflush(NULL);
}

/*
Funcion principal que usa cada hilo/carro.
*/
void *carMovement(void *vargp)
{
	struct arg_struct *args = vargp;
	int lock_result;
	int car_id = args->id;
	int direction = args->dir;

	if (direction == TO_EAST)
	{
		to_east_amount--;
	}
	else if (direction == TO_WEST)
	{
		to_west_amount--;
	}

	printf("\n-----[Se creó el %d con dirección %d y va en camino al puente]-----\n", car_id, direction);
	sleep(3);
	printf("\n-----[Ha llegado al puente el %d con dirección %d]-----\n", car_id, direction);
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

		//printf("%d esta en el puente:\n", car_id); borrar
		printf("\n-----[%d esta en el puente:]-----\n", car_id);
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

		//printf("%d esta en el puente:\n", car_id); borrar
		printf("\n-----[%d esta en el puente:]-----\n", car_id);
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

		//printf("%d esta fuera del puente:\n", car_id); borrar
		printf("\n-----[%d esta fuera del puente:]-----\n", car_id);
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

		//printf("%d esta fuera del puente:\n", car_id); borrar
		printf("\n-----[%d esta fuera del puente:]-----\n", car_id);
		printStatus(TO_WEST);
	}

	lock_result = pthread_mutex_unlock(&lock);
	if (lock_result)
	{
		printf("Unlock fallo! %d\n", lock_result);
		exit(-1);
	}
	sleep(3);
	printf("\n-----[%d ya termino su trayecto completo con dirección %d]-----\n", car_id, direction);
	return NULL;
}

// Función utilizada para revisar que se ingresaron los parametros correctamente.
int check_paramethers(int argc, char *argv[])
{
	if (argc < 4)
	{
		printf("\n--Warning--\nPara ejecutar escriba:  ./main to_east_amount to_west_amount bridge_max_load\n");
		return 1;
	}
	else
		return 0;
}

// Intercambia los valores de posiciones aleatorias del array.
void swap(int *a, int *b)
{
	int temp = *a;
	*a = *b;
	*b = temp;
}

// Esta función  crea un array con las direcciones y luego lo revuelve de forma aleatoria.
void create_dirs_and_suffle(int *dirs, int to_east_amount, int total_cars_simulated)
{

	for (int index = 0; index < total_cars_simulated; index++)
	{
		if (index < to_east_amount)
		{
			dirs[index] = 0;
		}
		else
		{
			dirs[index] = 1;
		}
	}

	int n = sizeof(dirs) / sizeof(dirs[0]);
	srand(time(NULL));
	for (int i = n - 1; i > 0; i--)
	{
		int j = rand() % (i + 1);
		swap(&dirs[i], &dirs[j]);
	}
}

// Esta función crea todos los vehiculos como hilos con tiempos de espera alternados.
int createCars(int *dirs, pthread_t *cars){
	int i, result;
	// Ciclo que recorre la cantidad total de autos creando de forma aleatoria y con tiempos distribuidos sus respectivos hilos.
	for (i = 0; i < total_cars_simulated; i++)
	{
		// Create the arguments to be send through the thread creator.
		thread_args = malloc(sizeof(struct arg_struct) * 1);
		thread_args->id = i;
		thread_args->dir = dirs[i];

		// Pthread crea un hilo para un auto, se le asigna un id y la dirección. Además, se asocia la función handler del hilo (carMovement).
		result = pthread_create(&cars[i], NULL, carMovement, thread_args);
		int delay_time = get_delay_time(MEDIAN);
		sleep((int)round(delay_time));

		// Si se produce un error al crear el hilo se genera un mensaje.
		if (result)
		{
			printf("Fallo al crear hilo nuevo %d\n", result);
			exit(-1);
		}
	}
	return result;
}

int main(int argc, char *argv[])
{
	// Revisar los parametros antes de ejecutar.
	int error = check_paramethers(argc, argv);

	if (error == 1)
	{
		exit(1);
	}
	// Leer de la consola la cantidad de autos en cada dirección.
	to_east_amount = atoi(argv[1]);
	to_west_amount = atoi(argv[2]);

	// Leer de los parametros la cantidad total de flujo de autos posibles para el puente.
	max_load = atoi(argv[3]);

	// Cantidad total de tráfico para esta ejecución.
	total_cars_simulated = to_east_amount + to_west_amount;

	// Almacenas todos los hilos/autos.
	pthread_t cars[total_cars_simulated];

	int i, result;

	// Se estable que los números aleatorios se generaran usando como semilla la función de hora del procesador.
	srand(time(NULL));

	// Inicializar todo
	east_count = 0;
	east_wait = 0;

	west_count = 0;
	west_wait = 0;

	// Antes de iniciar, se quiere generar los autos de forma aleatoria, así que generamos una lista para las direcciones.
	int dirs[total_cars_simulated];
	create_dirs_and_suffle(dirs, to_east_amount, total_cars_simulated);

	// Crea los estados por defecto para las listas de espera y el puente.
	for (i = 0; i < total_cars_simulated; i++)
	{
		west_waiting[i] = -1;
		east_waiting[i] = -1;
		bridge[i] = -1;
	}

	result = createCars(dirs, cars);

	//Esperar para cierre de los hilos.
	for (i = 0; i < total_cars_simulated; i++)
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
