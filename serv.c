#include <my_global.h>
#include <mysql.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>


/* PORT-UL FOLOSIT */
#define PORT 1500
/*=================*/



/* CONEXIUNEA CU BAZA DE DATE */
MYSQL *con = NULL;
/*============================*/



/* CONTOR PENTRU NUMARUL DE CLIENTI, SI DE INSTANTE ALE JOCULUI
   (O INSTANTA DE JOC CONTINE CEI DOI CLIENTI CARE JOACA) */
int instance_count = 0,
    client_count   = 0;
/*===========================================================*/



/* SIMBOLURILE ASCII FOLOSITE CA PIESE DE JOC PENTRU CEI DOI JUCATORI */
const char p1 = '1',
           p2 = '2';
/*====================================================================*/



// FUNCTIA PENTRU SQL ERROR HANDLING
void finish_with_error(MYSQL *con)
{
	fprintf(stderr, "\r\nERROR: %s\r\n", mysql_error(con));
	mysql_close(con);
	exit(1);
}



/* CODUL DE DEROARE */
extern int errno;
/*==================*/


/* STRUCTURILE PENTRU CLIENT SI PENTRU O INSTANTA DE JOC */
typedef struct thData{
	int idThread;
	int cl;
}thData;


typedef struct lobby{
	int idThread;
	int cl1, cl2;
	int id1, id2;
}lobby;
/*=======================================================*/



/* INSTANTELE */
struct lobby *instance;
pthread_t instances[100];
/*============*/



// FUNCTIA EXECUTATA DE THREAD-URI
static void *treat(void *);



// FUNCTIILE DE LOGIN SI REGISTER
void login(void *arg);
void register_user(void *arg);



// JOCUL PROPRIU-ZIS SI FUNCTIILE NECESARE: INITIALIZAREA TABELEI DE JOC, AFISAREA ACESTEIA
// ADAUGAREA UNEI PIESE IN TABELA SI VERIFICAREA DACA A FOST EFECTUATA O MUTARE CASTIGATOARE
static void *game(void *arg);

void initialize(char table[6][7]);

void display(char table[6][7]);

void p1_turn(char table[6][7], int input_col);
void p2_turn(char table[6][7], int input_col);

int p1_win(char table[6][7]);
int p2_win(char table[6][7]);



// FUNCTIILE PENTRU INCREMENTAREA SAU AFLAREA SCORULUI
int update_scor(int id);
int    get_scor(int id);



int main ()
{
	struct sockaddr_in server;	// STRUCTURILE FOLOSITE DE SERVER
  	struct sockaddr_in from;
  	int sd;				// DESCRIPTORUL DE SOCKET
  	int pid;
  	pthread_t th[100];    		// IDENTIFICATORII THREAD-URILOR
	int i=0;



	/* TABELA DE JOC SI INITIALIZAREA ACESTEIA */
	char t[6][7];
	initialize(t);
	display(t);
	/*=========================================*/



	/* SE CREEAZA SOCKET-UL */
	if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror ("[server]Eroare la socket().\n");
		return errno;
	}
	/*======================*/



  	/* SETAREA OPTIUNII SO_REUSEADDR */
 	int on=1;
  	setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
	/*===============================*/



  	/* PREGATIREA STRUCTURILOR DE DATE */
  	bzero (&server, sizeof (server));
  	bzero (&from, sizeof (from));
	/*=================================*/



  	/* INITIALIZAREA STRUCTURII DE SERVER */
    	server.sin_family = AF_INET;
    	server.sin_addr.s_addr = htonl (INADDR_ANY);
    	server.sin_port = htons (PORT);
	/*====================================*/



  	/* SE ATASEAZA SOCKET-UL */
  	if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    	{
      		perror ("[server]Eroare la bind().\n");
      		return errno;
    	}
	/*=======================*/



  	/* ASCULTAM PENTRU MAXIM 100 CLIENTI */
  	if (listen (sd, 100) == -1)
    	{
      		perror ("[server]Eroare la listen().\n");
      		return errno;
    	}
	/*===================================*/



	/* CONNECT LA BAZA DE DATE */
	con = mysql_init(NULL);
	if (mysql_real_connect(con, "fenrir.info.uaic.ro", "project1", "KH6irDcJZs", "project1", 0, NULL, 0) == NULL)
	{
		finish_with_error(con);
	}
	/*=========================*/



	while (1)
	{
		int client;
		thData *td;
		int length = sizeof (from);



		printf ("[server]Asteptam la portul %d...\n",PORT);
		fflush (stdout);



      		/* SE ACCEPTA UN CLIENT */
      		if ( (client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
		{
	  		perror ("[server]Eroare la accept().\n");
	  		continue;
		}
		/*======================*/



        	/* S-A REALIZAT CONEXIUNEA, SE CREEAZA NOUL THREAD */
    		int idThread;
		int cl;

		td=(struct thData*)malloc(sizeof(struct thData));
		td->idThread=++i;
		td->cl=client;

		pthread_create(&th[i], NULL, &treat, td);
		/*=================================================*/
	}
};



// FUNCTIA EXECUTATA DE UN THREAD NOU
static void *treat(void * arg)
{
		struct thData td;
		char action[20];	// COMANDA PRIMITA DE LA CLIENT

		bzero(action, sizeof(action));

		td = *((struct thData*)arg);

		printf ("\r\nCLIENT FOUND: login/register\r\n", td.idThread);

		fflush (stdout);

		pthread_detach(pthread_self());



		/* READ initial_command FROM CLIENT */
		if (read (td.cl, &action, sizeof(action)) <= 0)
		{
			printf("\r\n THREAD %d\r\n",td.idThread);
			perror ("\r\nEroare la read() de la client.\r\n");
		}
		/*==================================*/



		/* SE EFECTUEAZA COMANDA DE LA CLIENT */
		if(strcmp(action, "login\n") == 0)
		{
			login(&td); // VA INTRA AUTOMAT IN game() LA succes
		}
		else if(strcmp(action, "register\n") == 0)
		{
			register_user(&td); // VA INTRA AUTOMAT IN login() LA success
		}
		/*====================================*/



		close ((intptr_t)arg);

		return(NULL);
}




// JOCUL PROPRIU-ZIS
static void *game(void *arg)
{
	struct lobby td;

        td = *((struct lobby*)arg);

	printf("NEW GAME LOBBY:\r\n   CLIENT1: %d\r\n   CLIENT2: %d\r\n", td.cl1, td.cl2);

	char table[6][7], input_col[5];	// TABELA DE JOC SI BUFFER-UL PENTRU INPUT DE LA CLIENT

	int current_player = 1, input_col_int; // CONTORUL PENTRU JUCATORUL CURENT
                                               // SI BUFFER-UL DE TIP INT PENTRU INPUT DE LA CLIENT

	initialize(table);



	/* SE TRANSMITE STAREA INITIALA A JOCULUI CATRE CLIENTI */
        if (write (td.cl1, table, sizeof(table)) <= 0)
        {
                perror ("1. Eroare la write() catre client 1.\n");
        }
        else
        {
                printf ("\r\nTabela de joc a fost trasmisa cu succes catre jucatorul 1.\r\n");
        }


        if (write (td.cl2, table, sizeof(table)) <= 0)
        {
                perror ("1. Eroare la write() catre client 2.\n");
        }
        else
        {
                printf ("\r\nTabela de joc a fost trasmisa cu succes catre jucatorul 2.\r\n");
        }
	/*======================================================*/



	while(1)
	{
		printf("\r\n=========================================================\r\n");

		display(table);



		/* INITIAL SE PRIMESTE SEMNALUL DE TEST DE LA CLIENTI */
		char test[10]; bzero(test, sizeof(test));
		if (read (td.cl1, &test ,sizeof(test)) <= 0)
                {
                        perror ("Eroare la read() de la client - test.\n");
                        exit(0);
                }
		if (read (td.cl2, &test ,sizeof(test)) <= 0)
                {
                        perror ("Eroare la read() de la client - test.\n");
                        exit(0);
                }
		/*====================================================*/



		/* SE TRANSMITE FLAG-UL DE wait SAU input CATRE CLIENTI IN FUNCTIE DE CLIENTUL CARE URMEAZA SA EFECTUEZE O MUTARE */
      		printf ("\r\nEste randul jucatorului %d.\r\n", current_player);
      		fflush (stdout);

		if (current_player == 1)
		{
			if (write (td.cl1, "input", sizeof("input")) <= 0)
			{
				perror ("2. Eroare la write() catre client 1.\n");
			}
			else
			{
				printf ("\r\nINPUT cerut cu succes catre jucatorul 1.\r\n");
			}

			if (write (td.cl2, "wait", sizeof("wait")) <= 0)
			{
				perror ("2. Eroare la write() catre client 2.\n");
			}
			else
			{
				printf ("\r\nWAIT cerut cu succes catre jucatorul 2.\r\n");
			}
		}
		else if (current_player == 2)
		{
			if (write (td.cl2, "input", sizeof("input")) <= 0)
			{
				perror ("3. Eroare la write() catre client 2.\n");
			}
			else
			{
				printf ("\r\nINPUT cerut cu succes catre jucatorul 2.\r\n");
			}

			if (write (td.cl1, "wait", sizeof("wait")) <= 0)
			{
				perror ("3. Eroare la write() catre client 1.\n");
			}
			else
			{
				printf ("\r\nWAIT transmis cu succes catre jucatorul 1.\r\n");
			}
		}
		/*=================================================================================================================*/



		/* SE PRIMESTE INPUT-UL DE LA JUCATORUL CURENT */
		bzero(input_col, sizeof(input_col));
		if (current_player == 1)
		{
			if (read (td.cl1, input_col, sizeof(input_col)) <= 0)
			{
				perror ("Eroare la read() de la client 1.\n");
			}
			else
			{
				printf ("\r\nInput <%s\b> primit cu succes catre jucatorul 1.\r\n", input_col);
			}
		}
		else if (current_player == 2)
		{
			if (read (td.cl2, input_col, sizeof(input_col)) <= 0)
			{
				perror ("Eroare la read() de la client 2.\n");
			}
			else
			{
				printf ("\r\nInput <%s> primit cu succes catre jucatorul 2.\r\n", input_col);
			}
		}
		input_col[strlen(input_col) - 1] = '\0';
		input_col_int = atoi(input_col);
		/*=============================================*/



		/* TABELA DE JOC SE ACTUALIZEAZA CONFORM INPUT-ULUI JUCATORULUI */
		if (current_player == 1)
		{
			p1_turn(table, input_col_int);
		}
		else if (current_player == 2)
		{
			p2_turn(table, input_col_int);
		}
		display(table);
		fflush(stdout);
		/*==============================================================*/



		/* TABELA DE JOC ACTUALIZATA ESTE TRANSMISA INAPOI JUCATORILOR */
		if (write (td.cl1, table, sizeof(table)) <= 0)
		{
			perror ("5. Eroare la write() catre client 1.\n");
		}
		else
		{
			printf ("\r\nTabela de joc a fost trasmisa cu succes catre jucatorul 1.\r\n");
		}

		if (write (td.cl2, table, sizeof(table)) <= 0)
		{
			perror ("5. Eroare la write() catre client 2.\n");
		}
		else
		{
			printf ("\r\nTabela de joc a fost trasmisa cu succes catre jucatorul 2.\r\n");
		}
		/*=============================================================*/



		/* SE VERIFICA DACA VREUNUL DIN JUCATORI A EFECTUAT MUTAREA CASTIGATOARE CE INCHEIE JOCUL
		   SI SE TRANSMIT FLAG-URILE DE win, lose SAU continue IN CAZ CA NU A CASTIGAT NICI UN JUCATOR TURA ACEASTA*/
			/* PLAYER 1 CASTIGA */
		if (p1_win(table) == 1)
		{

			char temp[50], aux[10];
			bzero(temp, sizeof(temp));
			bzero(aux, sizeof(aux));

			sprintf(aux, "%d", update_scor(td.id1));

			strcpy(temp, "WIN! \r\n\tYOUR SCORE: ");
			strcat(temp,  aux);

			if (write (td.cl1, temp, sizeof(temp)) <= 0)
			{
				perror ("6. Eroare la write() catre client 1.\n");
			}
			else
			{
				printf ("\r\nWIN transmis cu succes catre jucatorul 1.\r\n");

			}



			bzero(temp, sizeof(temp));
                        bzero(aux, sizeof(aux));

                        sprintf(aux, "%d", get_scor(td.id2));

                        strcpy(temp, "LOSE :(\r\n\tYOUR SCORE: ");
                        strcat(temp,  aux);

			if (write (td.cl2, temp, sizeof(temp)) <= 0)
			{
				perror ("6. Eroare la write() catre client 2.\n");
			}
			else
			{
				printf ("\r\nLOSE transmis cu succes catre jucatorul 2.\r\n");
			}

			break;
		}
			/* PLAYER 2 CASTIGA */
		else if (p2_win(table) == 1)
		{

			char temp[50], aux[10];
                        bzero(temp, sizeof(temp));
                        bzero(aux, sizeof(aux));

                        sprintf(aux, "%d", update_scor(td.id2));

                        strcpy(temp, "WIN! \r\n\tYOUR SCORE: ");
                        strcat(temp,  aux);

			if (write (td.cl2, temp, sizeof(temp)) <= 0)
			{
				perror ("7. Eroare la write() catre client 2.\n");
			}
			else
			{
				printf ("\r\nWIN transmis cu succes catre jucatorul 2.\r\n");
				update_scor(td.id2);
			}



                        bzero(temp, sizeof(temp));
                        bzero(aux, sizeof(aux));

                        sprintf(aux, "%d", get_scor(td.id1));

                        strcpy(temp, "LOSE :(\r\n\tYOUR SCORE: ");
                        strcat(temp,  aux);

			if (write (td.cl1, temp, sizeof(temp)) <= 0)
			{
				perror ("7. Eroare la write() catre client 1.\n");
			}
			else
			{
				printf ("\r\nLOSE transmis cu succes catre jucatorul 1.\r\n");
			}

			break;
		}
			/* JOCUL CONTINUA */
		else
		{
			if (write (td.cl1, "cont", sizeof("cont")) <= 0)
			{
				perror ("8. Eroare la write() catre client 1.\n");
			}
			else
			{
				printf ("\r\nCONTINUE transmis cu succes catre jucatorul 1.\r\n");
			}

			if (write (td.cl2, "cont", sizeof("cont")) <= 0)
			{
				perror ("8. Eroare la write() catre client 2.\n");
			}
			else
			{
				printf ("\r\nCONTINUE transmis cu succes catre jucatorul 2.\r\n");
			}
		}
		/*========================================================================================*/



		/* SE ACTUALIZEAZA JUCATORUL CURENT */
		if (current_player == 1)
		{
			current_player = 2;
		}
		else
		{
			current_player = 1;
		}
		/*==================================*/
	}
}



// FUNCTIA DE LOGIN
void login(void *arg)
{
	struct thData td;
	char user[20], password[20], select[200], result[20];
	int id;
	td = *((struct thData*)arg);

	printf("\r\nLOGIN: CLIENT %d FROM THEREAD %d\r\n", td.cl, td.idThread);
	fflush(stdout);


	do
	{
		bzero(user, sizeof(user));
		bzero(password, sizeof(password));



		/* READ login_user FROM CLIENT */
                if (read (td.cl, &user, sizeof(user)) <= 0)
                {
                        printf("\r\n THREAD %d\r\n",td.idThread);
                        perror ("\r\nEroare la login() - username.\r\n");

                }
                user[strlen(user) - 1] = '\0';

                printf("\r\nusername:\t%s\r\n", user);
                fflush(stdout);
		/*=============================*/



                /* READ login_password FROM CLIENT */
                if (read (td.cl, &password, sizeof(password)) <= 0)
                {
                        printf("\r\n THREAD %d\r\n",td.idThread);
                        perror ("\r\nEroare la login() - password.\r\n");

                }
                password[strlen(password) - 1] = '\0';

                printf("\r\npassword:\t%s\r\n", password);
                fflush(stdout);
		/*=================================*/



                /* VERIFICAM DACA DATELE PRIMITE SUNT VALIDE PRINTR-O INTEROGARE SQL */
                bzero(select, sizeof(select));
                strcpy(select, "SELECT id FROM project1 WHERE user = \"");
                strcat(select, user);
                strcat(select, "\" AND password = \"");
		strcat(select, password);
		strcat(select, "\";");

                if (mysql_query(con, select))
                {
                        finish_with_error(con);
                }

                MYSQL_RES *select_result = mysql_store_result(con);
		MYSQL_ROW row = mysql_fetch_row(select_result);
                int num_rows = mysql_num_rows(select_result);
                mysql_free_result(select_result);
		id = atoi(row[0]);

                printf("\r\nUSER FOUND: %d\r\nID: %d", num_rows, id);
		/*===================================================================*/



		/* CONT VALID GASIT */
		bzero(result, sizeof(result));
		if(num_rows == 1)
		{
			strcpy(result, "success");
		}
		else
		{
			strcpy(result, "fail");
		}
		/*==================*/



		/* WRITE result TO CLIENT */
                if (write (td.cl, &result, sizeof(result)) <= 0)
                {
                	printf("\r\n THREAD %d\r\n",td.idThread);
                        perror ("\r\nEroare la login() - result.\r\n");
                }
		/*========================*/



		/* LOGIN CU SUCCES, INTRAM IN JOC */
		if(strcmp(result, "success") == 0)
		{
			if(client_count % 2 == 0)// NR PAR DE CLIENTI ACTIVI, NOUL CLIENT TREBUIE SA ASTEPTE UN OPONENT
			{
				instance = (struct lobby*)malloc(sizeof(struct lobby));
				instance -> idThread = instance_count++;
				instance -> cl1 = td.cl;
				instance -> id1 = id;
				++client_count;
			}
			else // NR IMPAR DE CLIENTI, NOUL CLIENT VA JUCA CU CEL CARE ERA IN ASTEPTARE
			{
				++client_count;
				instance -> cl2 = td.cl;
				instance -> id2 = id;
				pthread_create(&instances[instance_count], NULL, &game, instance);// SE CREAZA INSTANTA DE JOC
			}
			break;
		}
		/*================================*/
	}
	while(1);
}



// FUNCTIA PENTRU INREGISTRAREA UNUI UTILIZATOR NOU
void register_user(void *arg)
{
	struct thData td;
	char user[20], password[20], result[20], select[200];

	td = *((struct thData*)arg);


	while(1)
	{

		/* SE INITIALIZEAZA BUFFER-URILE */
		bzero(user, sizeof(user));
        	bzero(password, sizeof(password));
        	bzero(result, sizeof(result));
		/*===============================*/



		/* READ register_user FROM CLIENT */
		if (read (td.cl, &user, sizeof(user)) <= 0)
        	{
			printf("\r\n THREAD %d\r\n",td.idThread);
                	perror ("\r\nEroare la register_user() - username.\r\n");

        	}
		user[strlen(user) - 1] = '\0';

		printf("\r\nusername:\t%s\r\n", user);
		fflush(stdout);
		/*================================*/



		/* READ register_password FROM CLIENT */
        	if (read (td.cl, &password, sizeof(password)) <= 0)
        	{
                	printf("\r\n THREAD %d\r\n",td.idThread);
                	perror ("\r\nEroare la register_user() - password.\r\n");

        	}
		password[strlen(password) - 1] = '\0';

        	printf("\r\npassword:\t%s\r\n", password);
		fflush(stdout);
		/*====================================*/



		/* VERIFICAM DACA USER-UL ESTE AVAILABLE CU O INTEROGARE SQL */
		bzero(select, sizeof(select));
		strcpy(select, "SELECT user FROM project1 WHERE user = \"");
		strcat(select, user);
		strcat(select, "\";");

		if (mysql_query(con, select))
		{
			finish_with_error(con);
		}

		MYSQL_RES *select_result = mysql_store_result(con);
		int num_rows = mysql_num_rows(select_result);
		mysql_free_result(select_result);

		printf("\r\nROWS: %d\r\n", num_rows);
		/*===========================================================*/



		/* DACA NU EXISTA USER-UL */
		if(num_rows == 0)
		{
			strcpy(result, "success");


			/* SE CALCULEAZA ID-UL NOULUI USER */
			int id;
			char id_char[10];

			bzero(id_char, sizeof(id_char));



			if (mysql_query(con, "SELECT MAX(id) FROM project1;"))
			{
				finish_with_error(con);
			}



			MYSQL_RES *select_result = mysql_store_result(con);
			MYSQL_ROW row = mysql_fetch_row(select_result);
			mysql_free_result(select_result);



			strcpy(id_char, row[0]);

			id = atoi(id_char);

			++id;

			printf("\r\nID new user: %d\r\n", id); fflush(stdout);


			bzero(id_char, sizeof(id_char));
			sprintf(id_char, "%d", id);
			id_char[strlen(id_char) + 1] = '\0';
			/*=================================*/



			/* SE INSEREAZA NOUL USER IN BAZA DE DATE */
			bzero(select, 100);

			strcpy(select, "INSERT INTO project1(id,user,password) VALUES(");
			strcat(select, id_char);
			strcat(select, ",'");
			strcat(select, user);
			strcat(select, "','");
			strcat(select, password);
			strcat(select, "');");

			if (mysql_query(con, select))
			{
				finish_with_error(con);
			}

			printf("\r\n%s\r\n", select);
			/*========================================*/

		}
		else
		{
			strcpy(result, "username taken");
		}
		/*========================*/



		/* WRITE result TO CLIENT */
		if (write (td.cl, &result, sizeof(result)) <= 0)
		{
			printf("\r\n THREAD %d\r\n",td.idThread);
                	perror ("\r\nEroare la register_user() - result.\r\n");
		}
		/*========================*/



		/* BREAK LA REGISTER TERMINAT CU SUCCES */
		if(strcmp(result, "success") == 0)
		{
			login(&td);
			break;
		}
		/*======================================*/
	}
}



// INITIALIZAREA TABELEI DE JOC CU WHITESPACE
void initialize(char table[6][7]){
	for (int i = 0; i < 6; ++i){
		for (int j = 0; j < 7; ++j){
			table[i][j] = ' ';
		}
	}
}



// AFISAREA TABELEI
void display(char table[6][7])
{
	int i, j;
	printf("\r\n---------------\r\n");
	for (i = 0; i < 6; ++i)
	{
		for(j = 0; j < 7; ++j)
		{
			printf("|%c", table[i][j]);
		}
		printf("|\r\n---------------\r\n");
	}
	printf(" 0 1 2 3 4 5 6\r\n");
}



// FUNCTIILE PENTRU ADAUGAREA PIESELOR DE JOC IN TABELA
void p1_turn(char table[6][7], int input_col){

	int col = input_col, row = 0;

	table[row][col] = p1;

	// PIESA "CADE" IN CELULA DE JOS, CAND TIMP CELULA RESPECTIVA ESTE GOALA
	while(table[row + 1][col] == ' '){
		table[row + 1][col] = p1;
		table[row][col]     = ' ';
		row++;
	}
}



void p2_turn(char table[6][7], int input_col){

	int col = input_col, row = 0;

	table[row][col] = p2;

	//   THE PIECE "FALLS" TO THE CELL BELOW IT AS LONG AS IT IS EMPTY
	while(table[row + 1][col] == ' '){
		table[row + 1][col] = p2;
		table[row][col]     = ' ';
		row++;
	}
}



// FUNCTIILE PENTRU VERIFICAREA EXISTENTEI UNEI MUTARI CASTIGATOARE
int p1_win(char table[6][7]){

	//   HORIZONTAL CHECK
	for(int i = 0; i < 6; ++i){
		for(int j = 0; j < 3; ++j){
			if(table[i][j] == p1 &&
			   table[i][j] == table[i][j + 1] &&
			   table[i][j] == table[i][j + 2] &&
			   table[i][j] == table[i][j + 3]){
				return 1;
			}
		}
	}

	//   VERTICAL CHECK
	for(int i = 0; i < 3; ++i){
		for(int j = 0; j < 7; ++j){
			if(table[i][j] == p1 &&
			   table[i][j] == table[i + 1][j] &&
			   table[i][j] == table[i + 2][j] &&
			   table[i][j] == table[i + 3][j]){
				return 1;
			}
		}
	}

	//   MAIN DIAGONAL CHECK
	for(int i = 0; i < 4; ++i){
		for(int j = 0; j < 4; ++j){
			if(table[i][j] == p1 &&
			   table[i][j] == table[i + 1][j + 1] &&
			   table[i][j] == table[i + 2][j + 2] &&
			   table[i][j] == table[i + 3][j + 3]){
				return 1;
			}
		}
	}

	//   SECONDARY DIAGONAL CHECK
	for(int i = 3; i < 6; ++i){
		for(int j = 0; j < 3; ++j){
			if(table[i][j] == p1 &&
			   table[i][j] == table[i - 1][j + 1] &&
			   table[i][j] == table[i - 2][j + 2] &&
			   table[i][j] == table[i - 3][j + 3]){
				return 1;
			}
		}
	}

	return 0;
}



int p2_win(char table[6][7]){

	//   HORIZONTAL CHECK
	for(int i = 0; i < 6; ++i){
		for(int j = 0; j < 3; ++j){
			if(table[i][j] == p2 &&
			   table[i][j] == table[i][j + 1] &&
			   table[i][j] == table[i][j + 2] &&
			   table[i][j] == table[i][j + 3]){
				return 1;
			}
		}
	}

	//   VERTICAL CHECK
	for(int i = 0; i < 2; ++i){
		for(int j = 0; j < 7; ++j){
			if(table[i][j] == p2 &&
			   table[i][j] == table[i + 1][j] &&
			   table[i][j] == table[i + 2][j] &&
			   table[i][j] == table[i + 3][j]){
				return 1;
			}
		}
	}

	//   MAIN DIAGONAL CHECK
	for(int i = 3; i < 5; ++i){
		for(int j = 3; j < 6; ++j){
			if(table[i][j] == p2 &&
			   table[i][j] == table[i - 1][j - 1] &&
			   table[i][j] == table[i - 2][j - 2] &&
			   table[i][j] == table[i - 3][j - 3]){
				return 1;
			}
		}
	}

	//   SECONDARY DIAGONAL CHECK
	for(int i = 3; i < 5; ++i){
		for(int j = 0; j < 3; ++j){
			if(table[i][j] == p2 &&
			   table[i][j] == table[i - 1][j + 1] &&
			   table[i][j] == table[i - 2][j + 2] &&
			   table[i][j] == table[i - 3][j + 3]){
				return 1;
			}
		}
	}

	return 0;
}



// FUNCTIA PENTRU INCREMENTAREA SCORULUI
int update_scor(int id)
{
 	char buff[200], id_char[10];	// BUFFER PENTRU COMENZI SQL SI PENTRU ID-UL JUCATORULUI



	bzero(buff, sizeof(buff));
	bzero(id_char, sizeof(id_char));



	/* ID-UL ESTE CONVERTIT IN SIR DE CARACTERE */
	sprintf(id_char, "%d", id);
        id_char[strlen(id_char) + 1] = '\0';
	/*==========================================*/



	/* SE COMPUNE COMANDA SQL NECESARA */
        strcpy(buff, "UPDATE project1 SET scor = scor+1 WHERE id = ");
        strcat(buff, id_char);
        strcat(buff, ";");
	/*=================================*/



	/* SE EFECTUEAZA COMANDA SQL */
        if (mysql_query(con, buff))
        {
                finish_with_error(con);
        }
	/*===========================*/



	/* SE RETURNEAZA NOUL SCOR PRINTR-O ALTA COMANDA SQL */
	bzero(buff, sizeof(buff));
	strcpy(buff, "SELECT scor FROM project1 WHERE id = ");
	strcat(buff, id_char);
	strcat(buff, ";");
	if (mysql_query(con, buff))
        {
                finish_with_error(con);
        }

        MYSQL_RES *select_result = mysql_store_result(con);
        MYSQL_ROW row = mysql_fetch_row(select_result);
        mysql_free_result(select_result);
	/*===================================================*/


        return atoi(row[0]);
}



// FUNCTIA PENTRU AFLAREA SCORULUI
int get_scor(int id)
{
	char buff[200], id_char[10];    // BUFFER PENTRU COMENZI SQL SI PENTRU ID-UL JUCATORULUI



        bzero(buff, sizeof(buff));
        bzero(id_char, sizeof(id_char));



        /* ID-UL ESTE CONVERTIT IN SIR DE CARACTERE */
        sprintf(id_char, "%d", id);
        id_char[strlen(id_char) + 1] = '\0';
        /*==========================================*/



        /* SE RETURNEAZA NOUL SCOR PRINTR-O ALTA COMANDA SQL */
        bzero(buff, sizeof(buff));
        strcpy(buff, "SELECT scor FROM project1 WHERE id = ");
        strcat(buff, id_char);
        strcat(buff, ";");

	if (mysql_query(con, buff))
        {
                finish_with_error(con);
        }

        MYSQL_RES *select_result = mysql_store_result(con);
        MYSQL_ROW row = mysql_fetch_row(select_result);
        mysql_free_result(select_result);
        /*===================================================*/


        return atoi(row[0]);
}
