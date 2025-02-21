#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define CAPACITY 5
#define VIPSTR(vip) ((vip) ? "  vip  " : "not vip")

//Defino mutex y variable de condicion
pthread_mutex_t mutex_aforo;
pthread_cond_t cond_aforo;

//Variables de control
int aforo_actual = 0;
int esperando_vip = 0;

typedef struct{
	int id;
	int isvip;

}cliente_t;

void enter_normal_client(int id)
{
 	pthread_mutex_lock(&mutex_aforo);//bloqueo para acceder a variables compartidas
    printf("Cliente normal %d está esperando para entrar. Aforo actual: %d/%d\n", id, aforo_actual, CAPACITY);
    
    while (aforo_actual >= CAPACITY || esperando_vip > 0) {
        pthread_cond_wait(&cond_aforo, &mutex_aforo);
    }
    
    aforo_actual++;
    printf("Cliente normal %d ha entrado. Aforo actual: %d/%d\n", id, aforo_actual, CAPACITY);
    pthread_mutex_unlock(&mutex_aforo);
}

void enter_vip_client(int id)
{
	pthread_mutex_lock(&mutex_aforo); //tengo que bloquear pq accedo a variables compartidas. Por qué?? si dos hilos entran a la vez y ejecutan aforo_actual a la vez solo se sumará 1 cuando debería ser 2.
    esperando_vip++;
    printf("Cliente VIP %d está esperando para entrar. Aforo actual: %d/%d\n", id, aforo_actual, CAPACITY);
    
    while (aforo_actual >= CAPACITY) {
        pthread_cond_wait(&cond_aforo, &mutex_aforo);
    }
    
    esperando_vip--;
    aforo_actual++;
    printf("Cliente VIP %d ha entrado. Aforo actual: %d/%d\n", id, aforo_actual, CAPACITY);
    pthread_mutex_unlock(&mutex_aforo);
}

void dance(int id, int isvip)
{
	printf("Client %2d (%s) dancing in disco\n", id, VIPSTR(isvip));
	sleep((rand() % 3) + 1);
}

//función para manejar la salida de clientes
void disco_exit(int id, int isvip)
{
	pthread_mutex_lock(&mutex_aforo);
	aforo_actual--;
	printf("Cliente %d ha salido. Aforo actual: %d/%d\n", id, aforo_actual,CAPACITY);
	pthread_cond_broadcast(&cond_aforo); //Despierta a todos los hilos esperando para que vean si pueden entrar ya
	pthread_mutex_unlock(&mutex_aforo);
}

//Función ejecutada por cada hilo (cliente)
void *client(void *arg)
{
	cliente_t *cliente = arg;

	//Entrar al aforo según el tipo de cliente
	if (cliente->isvip)
		enter_vip_client(cliente->id);
	else
		enter_normal_client(cliente->id);

	//simular que el cliente está dentro bailando
	dance(cliente->id, cliente->isvip);

	//salir del aforo
	disco_exit(cliente->id, cliente->isvip);

	free(cliente);//libero memoria

}

int main(int argc, char *argv[])
{	
	if(argc < 2){
		fprintf(stderr, "Uso: %s \n", argv[0]);
		exit (EXIT_FAILURE);
	}
	 // Leer aforo máximo
   	// aforo_maximo = atoi(argv[2]);
	
	//Inicializo mutex y variable de condicion
	pthread_mutex_init(&mutex_aforo,NULL);
	pthread_cond_init(&cond_aforo, NULL);

	//Abro fichero de clientes
	FILE *file = fopen(argv[1], "r");
	if(!file){
		perror("Error al abrir el fichero de clientes");
		exit(EXIT_FAILURE);
	}

	int M;
	//leo primer valor entero
	fscanf(file, "%d", &M); //leo el numero de clientes

	pthread_t hilos[M]; //me cre un array de hilos

	for(int i=0; i < M; i++){
		int isvip;
		fscanf(file,"%d", &isvip); //lee si es vip 1 o normal 0

		//Creo estructura de cliente
		cliente_t *cliente = malloc (sizeof(cliente_t)); //tiene que ser puntero porque sino en el bucle la estructura podría ser sobrescrita o desturida cuando el bucle avance al siguiente cliente. Con puntero los hilos no comparten direccion de memoria
		cliente->id = i+1;
		cliente->isvip = isvip;

		//Creo hilo para el cliente
		pthread_create(&hilos[i], NULL, client, (void *)cliente); // (void *) lo pongo para casting pero si no lo pongo compila tmb
	}
	fclose(file);

	//Esperar a que todos los hilos terminen
	for(int i=0; i<M; i++){
		pthread_join(hilos[i], NULL);
	}

	// Destruir mutex y variable de condición
    pthread_mutex_destroy(&mutex_aforo);
    pthread_cond_destroy(&cond_aforo);
    
    printf("Todos los clientes han salido. Programa terminando.\n");

	return 0;
}


/*con dos variables de condicion
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define CAPACITY 5
#define VIPSTR(vip) ((vip) ? "  vip  " : "not vip")

// Defino mutex y variables de condición
pthread_mutex_t mutex_aforo;
pthread_cond_t cond_normal;
pthread_cond_t cond_vip;

// Variables de control
int aforo_actual = 0;
int esperando_vip = 0;
int esperando_normal = 0;

typedef struct {
    int id;
    int isvip;
} cliente_t;

void enter_normal_client(int id) {
    pthread_mutex_lock(&mutex_aforo);
    esperando_normal++;
    printf("Cliente normal %d está esperando para entrar. Aforo actual: %d/%d\n", id, aforo_actual, CAPACITY);

    while (aforo_actual >= CAPACITY || esperando_vip > 0) {
        pthread_cond_wait(&cond_normal, &mutex_aforo);
    }

    esperando_normal--;
    aforo_actual++;
    printf("Cliente normal %d ha entrado. Aforo actual: %d/%d\n", id, aforo_actual, CAPACITY);
    pthread_mutex_unlock(&mutex_aforo);
}

void enter_vip_client(int id) {
    pthread_mutex_lock(&mutex_aforo);
    esperando_vip++;
    printf("Cliente VIP %d está esperando para entrar. Aforo actual: %d/%d\n", id, aforo_actual, CAPACITY);

    while (aforo_actual >= CAPACITY) {
        pthread_cond_wait(&cond_vip, &mutex_aforo);
    }

    esperando_vip--;
    aforo_actual++;
    printf("Cliente VIP %d ha entrado. Aforo actual: %d/%d\n", id, aforo_actual, CAPACITY);
    pthread_mutex_unlock(&mutex_aforo);
}

void dance(int id, int isvip) {
    printf("Client %2d (%s) dancing in disco\n", id, VIPSTR(isvip));
    sleep((rand() % 3) + 1);
}

// Función para manejar la salida de clientes
void disco_exit(int id, int isvip) {
    pthread_mutex_lock(&mutex_aforo);
    aforo_actual--;
    printf("Cliente %d ha salido. Aforo actual: %d/%d\n", id, aforo_actual, CAPACITY);

    // Priorizar a los VIP, pero también despertar a normales si no hay VIP esperando
    if (esperando_vip > 0) {
        pthread_cond_signal(&cond_vip);
    } else {
        pthread_cond_signal(&cond_normal);
    }

    pthread_mutex_unlock(&mutex_aforo);
}

// Función ejecutada por cada hilo (cliente)
void *client(void *arg) {
    cliente_t *cliente = arg;

    // Entrar al aforo según el tipo de cliente
    if (cliente->isvip)
        enter_vip_client(cliente->id);
    else
        enter_normal_client(cliente->id);

    // Simular que el cliente está dentro bailando
    dance(cliente->id, cliente->isvip);

    // Salir del aforo
    disco_exit(cliente->id, cliente->isvip);

    free(cliente); // Libero memoria
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <fichero_clientes>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Inicializo mutex y variables de condición
    pthread_mutex_init(&mutex_aforo, NULL);
    pthread_cond_init(&cond_normal, NULL);
    pthread_cond_init(&cond_vip, NULL);

    // Abro fichero de clientes
    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Error al abrir el fichero de clientes");
        exit(EXIT_FAILURE);
    }

    int M;
    // Leo primer valor entero
    fscanf(file, "%d", &M); // Leo el número de clientes

    pthread_t hilos[M]; // Me creo un array de hilos

    for (int i = 0; i < M; i++) {
        int isvip;
        fscanf(file, "%d", &isvip); // Lee si es VIP (1) o normal (0)

        // Creo estructura de cliente
        cliente_t *cliente = malloc(sizeof(cliente_t));
        cliente->id = i + 1;
        cliente->isvip = isvip;

        // Creo hilo para el cliente
        pthread_create(&hilos[i], NULL, client, (void *)cliente);
    }
    fclose(file);

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < M; i++) {
        pthread_join(hilos[i], NULL);
    }

    // Destruir mutex y variables de condición
    pthread_mutex_destroy(&mutex_aforo);
    pthread_cond_destroy(&cond_normal);
    pthread_cond_destroy(&cond_vip);

    printf("Todos los clientes han salido. Programa terminando.\n");

    return 0;
}

*/