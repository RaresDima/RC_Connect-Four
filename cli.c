#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>



/* CODUL DE EROARE */
extern int errno;
/*=================*/



/* PORT-UL FOLOSIT */
int port;
/*================*/


/* TABELA DE JOC */
char table[6][7];
/*===============*/



/* FUNCTIILE CE VOR FI FOLOSITE */
void login(int sd);
void register_user(int sd);
void game(int sd);
void initialize();
void display();
int play_again();
/*==============================*/



int main (int argc, char *argv[])
{
do {
	int sd;				// SOCKET-UL
	struct sockaddr_in server;	// STRUCTURA ADRESEI
	char buff[20];			// BUFFER FOLOSIT PENTRU COMUNICARE GENERALA CU SERVER-UL

	bzero(buff, sizeof(buff));



  	/* SE VALIDEAZA ARGUMENTELE */
  	if (argc != 3)
    	{
      		printf ("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
      		return -1;
    	}
	/*==========================*/



	/* SE SETEAZA PORT-UL */
  	port = atoi (argv[2]);
	/*====================*/



  	/* SE CREEAZA SOCKET-UL */
  	if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    	{
      		perror ("Eroare la socket().\n");
      		return errno;
    	}
	/*======================*/



  	/* SE INITIALIZEAZA STRUCTURA FOLOSITA PENTRU CONECTAREA LA SERVER */
  	server.sin_family = AF_INET;
  	server.sin_addr.s_addr = inet_addr(argv[1]);
  	server.sin_port = htons (port);
	/*=================================================================*/



  	/* SE REALIZEAZA CONEXIUNEA LA SERVER */
  	if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    	{
      		perror ("Eroare la connect().\n");
      		return errno;
    	}
	/*====================================*/



	/* READ initial_command FROM USER */
	do
	{
  		printf ("\r\nCOMMAND: login/register/quit\r\n");
  		fflush (stdout);

		bzero(buff, sizeof(buff));
  		read (0, buff, sizeof(buff));
		fflush(stdin);

  		printf("\r\nINPUT %s\r\n", buff);
		fflush(stdout);
	}
	while(strcmp(buff, "login\n") != 0 && strcmp(buff, "register\n") != 0 && strcmp(buff, "quit\n") != 0);
	/*================================*/



  	/* WRITE initial_command TO SERVER */
  	if ((strcmp(buff, "login\n") == 0 || strcmp(buff, "register\n") == 0) && (write (sd, &buff, sizeof(buff)) <= 0))
    	{
      		perror ("[client]Eroare la write() - login/register.\n");
      		return errno;
    	}


	if(strcmp(buff, "login\n") == 0)
	{
		login(sd); // VA INTRA AUTOMAT IN game()
	}
	else if(strcmp(buff, "register\n") == 0)
	{
		register_user(sd); // VA INTRA AUTOMAT IN login()
	}
	/*=================================*/


  	/* SE INCHIDE CONEXIUNEA */
  	close (sd);
	/*=======================*/
} while (play_again() == 1);
}



// FUNCTIA PENTRU LOGIN
void login(int sd)
{

	char buff[20];	// BUFFER DE UZ GENERAL

        do
        {

                bzero(buff, sizeof(buff));



		printf("\r\nLOGIN\r\n");



		/* READ login_user FROM USER */
                printf("\r\nUSERNAME: ");
                fflush(stdout);

                read (0, buff, sizeof(buff));
                fflush(stdin);
		/*===========================*/



                /* WRITE login_user TO SERVER */
                if (write (sd, &buff, sizeof(buff)) <= 0)
                {
                        perror ("[client]Eroare la login() - username.\n");
                        exit(0);
                }
		/*============================*/



		/* READ login_password FROM USER */
                bzero(buff, sizeof(buff));

                printf("\r\nPASSWORD: ");
                fflush(stdout);

                read (0, buff, sizeof(buff));
                fflush(stdin);
		/*===============================*/



                /* WRITE login_password TO SERVER */
                if (write (sd, &buff, sizeof(buff)) <= 0)
                {
                        perror ("[client]Eroare la login() - password.\n");
                        exit(0);
                }
		/*================================*/



                /* READ result FROM SERVER */
                bzero(buff, sizeof(buff));
                if (read (sd, &buff ,sizeof(buff)) < 0)
                {
                        perror ("[client]Eroare la read() de la server - result.\n");
                        exit(0);
                }
		/*=========================*/



                /* DACA USER-UL ESTE VALID SE VA EFECTUA LOGAREA SI I SE VA CAUTA UN JOC UTILIZATORULUI */
                if(strcmp(buff, "success") == 0)
                {
                        printf("\r\nLOGIN: %s\r\n\r\nWaiting for an opponent to connect.\r\n", buff);
                        fflush(stdout);

                        game(sd);
                        break;
                }
                else
                {
                        printf("\r\nLOGIN: %s\r\nINCORRECT USER/PASSWORD\r\n", buff);
                        fflush(stdout);
                }
		/*======================================================================================*/

        }
        while(1);
}



// FUNCTIA PENTRU INREGISTRAREA UTILIZATORILOR NOI
void register_user(int sd)
{
	char buff[20];	// BUFFER DE UZ GENERAL

	do
	{

		bzero(buff, sizeof(buff));



		printf("\r\nREGISTER\r\n");
		fflush(stdout);



		/* READ register_user FROM USER */
		printf("\r\nUSERNAME: ");
		fflush(stdout);

		read (0, buff, sizeof(buff));
        	fflush(stdin);
		/*==============================*/



        	/* WRITE register_user TO SERVER */
        	if (write (sd, &buff, sizeof(buff)) <= 0)
        	{
                	perror ("[client]Eroare la register_user() - username.\n");
                	exit(0);
        	}
		/*==============================*/



		/* READ register_password FROM USER */
		bzero(buff, sizeof(buff));

        	printf("\r\nPASSWORD: ");
        	fflush(stdout);

        	read (0, buff, sizeof(buff));
        	fflush(stdin);
		/*==================================*/



        	/* WRITE register_password TO SERVER*/
        	if (write (sd, &buff, sizeof(buff)) <= 0)
        	{
                	perror ("[client]Eroare la register_user() - password.\n");
                	exit(0);
        	}
		/*==================================*/



		/* READ result FROM SERVER */
		bzero(buff, sizeof(buff));
		if (read (sd, &buff ,sizeof(buff)) < 0)
        	{
                	perror ("[client]Eroare la read() de la server - result.\n");
                	exit(0);
        	}
		/*=========================*/



		/* DACA USER-UL S-A INREGISTRAT CU SUCCES I SE VA CERE SA INTRE IN CONT */
		if(strcmp(buff, "success") == 0)
		{
			printf("\r\nREGISTER: %s\r\n", buff);
			fflush(stdout);

			login(sd);
			break;
		}
		else
		{
			printf("\r\nREGISTER: %s\r\nINPUT YOUR DATA AGAIN\r\n", buff);
                        fflush(stdout);
		}
		/*======================================================================*/

	}
	while(1);

}



// FUNCTIA PENTRU INITIALIZAREA TABELEI DE JOC CU WHITESPACE
void initialize()
{
	int i, j;
	for (i = 0; i < 6; ++i){
		for (j = 0; j < 7; ++j){
			table[i][j] = ' ';
		}
	}
}



// FUNCTIA PENTRU AFISAREA TABELEI DE JOC
void display()
{
        int i, j;
	printf("\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n");
	printf("\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n");
	printf("\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n");
	printf("\r\n    ---------------\r\n    ");
        for (i = 0; i < 6; ++i)
        {
                for(j = 0; j < 7; ++j)
                {
                        printf("|%c", table[i][j]);
                        fflush(stdout);
                }
                printf("|\r\n    ---------------\r\n    ");
                fflush(stdout);
        }
        printf(" 0 1 2 3 4 5 6\r\n\r\n");
        fflush(stdout);
}



// JOCUL PROPRIU-ZIS
void game(int sd)
{
	char flag[20], flag2[50];	// FLAG-URI PENTRU FAZELE JOCULUI

	initialize();



        if (read (sd, &table ,sizeof(table)) <= 0)
        {
                perror ("[client]Eroare la read() de la server - table.\n");
                exit(0);
        }
	display();



	while(1)
	{
		/* SE TRANSMITE UN BUFFER DE TESTARE */
		if (write (sd, "test", sizeof("test")) <= 0)
                {
                        perror ("Eroare la write() spre server.\n");
                        exit(0);
                }
		/*===================================*/



		/* SE PRIMESTE FLAG-UL DE wait SAU input */
		bzero(flag, sizeof(flag));
		if (read (sd, &flag ,sizeof(flag)) <= 0)
                {
                        perror ("[client]Eroare la read() de la server - flag 1.\n");
                        exit(0);
                }
		/*=======================================*/



		/* DACA A FOST PRIMIT FLAG-UL DE input SE AFISEAZA TABELA DE JOC, SE CERE INPUT, SE VALIDEAZA, SI SE TRIMITE LA SERVER */
		if (strcmp(flag, "input") == 0)
		{
			int  temp_int = 0;
			char temp_char[5];

			do
			{
				printf("\r\nINSERT DESIRED COLUMN: ");
				fflush(stdout);

				bzero(temp_char, sizeof(temp_char));
				read(0, temp_char, sizeof(temp_char));
				temp_int = atoi(temp_char);

				printf("Selected col %d\r\n", temp_int);
			}
			while(temp_int<0 || temp_int>6 || table[0][temp_int] != ' ');

			printf("\r\nsending %d\r\n", temp_int);

			if (write (sd, temp_char, sizeof(temp_char)) <= 0)
   			{
   				perror ("Eroare la write() spre server.\n");
   				exit(0);
			}
		}
		else
		{
			write(0, "\r\nWaiting for opponent.", sizeof("\r\nWaiting for opponent."));
			fflush(stdout);
		}
		/*=====================================================================================================================*/



		/* SE PRIMESTE STAREA ACTUALIZATA A JOCULUI DE LA SERVER SI SE AFISEAZA */
		if (read (sd, &table ,sizeof(table)) <= 0)
                {
                        perror ("[client]Eroare la read() de la server - table.\n");
                        exit(0);
                }
                display();
		/*======================================================================*/



		/* SE PRIMESTE FLAG-UL DE win, lose SAU continue DE LA SERVER */
		bzero(flag2, sizeof(flag2));
		if (read (sd, &flag2, sizeof(flag2)) <= 0)
                {
                        perror ("[client]Eroare la read() de la server - win/lose flag.\n");
                        exit(0);
                }
		/*=============================================================*/



		/* IN CAZUL PRIMIRII UNUI SEMNAL DE win SAU lose, SEMNALUL ESTE TRATAT */

		if (flag2[0] == 'W')
		{
			printf("\r\n\tYOU %s\r\n", flag2);
			break;
		}
		else if(flag2[0] == 'L')
		{
			printf("\r\n\tYOU %s\r\n", flag2);
			break;
		}

		/*=====================================================================*/

		bzero(flag2, sizeof(flag2));
	}
}



// FUNCTIA DE GESTIONARE A SFARSITULUI JOCULUI
int play_again()
{
	char buff[5];



	do
	{
		bzero(buff, sizeof(buff));

		printf("\r\nReturn to main menu? (y/n)\r\n");

		read(0, buff, sizeof(buff));
	}
	while (strcmp(buff, "y\n") != 0 && strcmp(buff, "Y\n") != 0 && strcmp(buff, "n\n") != 0 && strcmp(buff, "N\n") != 0);



	if (buff[0] == 'y' || buff[0] == 'Y')
		return 1;
	else
		return 0;
}
