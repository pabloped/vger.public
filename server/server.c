#include <sys/time.h>
#include <sys/timeb.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include "base64.c"
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#define LARGOHOST 50
#define BUFFERSOCK 9000



/*parametros*/
char *sNombredelserver;
char *sNombrewww;
int iPuertoserver;
char *sDirectorioprincipal;
char *sDirectoriocgi;
char *sDirectoriorestringido;
char *sDirectoriorestringidocgi;
char *sPaginapordefecto;
char *sDirectoriodelprograma;
char *sPedidoestadisticas;
char *sEncabezadocgi1;
char *sEncabezadocgi2;
char *sPasswordcambio;
char *sRepetidorpassword;
char *sLlamadaconfig;
char *sLlamadaconfig1;
char *sLlamadaconfig2;
char *sLlamadaserver;
char *sQuitarserver;
char *sRepetidor;
char *sRepetidorpuerto;

/*Control sistema*/
int iCantidadmaximadehijos;
int iEjecutandocgi,iEscribiocgi;
int iTiempocgiespera;
int iPidcgiexe;


/*Estadisticas*/
float fTiempoproceso,fTiempolatencia;

struct timeb stTiempoinicio,stTiempofinal;


struct tuEstadistic{
        long int liCantidaddehijos;
        float fTiempolatencia;
        float fTiempoproceso;
        long int aiSalida[30];
        };

struct tuEstadistic tuEstadisticas;

/*Control de http*/
int iTiempoespera;

/* variables globales para el control de signals y manejo de hijos*/
int iSeperdioconexion;
int iLecturanula;
int iCerraratenderestesocket;
int iControlhijos;
int iPidpadre;


/*Semaforos y mmoria compartida*/
int iEstsemaforo; /*control del semaforo*/
int iEstmemoriacomp;  /*memoria compartida*/
void *pvEstmemoriacomp;       /*puntero a memoria compartida para estadisticas*/

/*Variables de configuracion*/
int iControlestadisticas; /*Control de memoria compartida y semaforos de estadisticas*/
                         /*key_t esta definiida en types.h como int por eso no hace falta cambiar el tipo de dato*/




int Definirsemaforo(int iLlave){
   /*Define un semaforo.
   */
   int iQsemaforo;
   if ((iQsemaforo = semget ((key_t)iLlave, 1, 0666|IPC_CREAT)) == -1)
      perror("Definirsemaforo: ");
   return (iQsemaforo);
}

static void Trabajarsemaforo(int iSclave,int  iOp){
   /*Modifica el valor de un semaforo
   */

   struct sembuf sbSemabu;

   sbSemabu.sem_num = 0;
   sbSemabu.sem_op = iOp;
   sbSemabu.sem_flg = 0;
   if (semop (iSclave, &sbSemabu, 1) == -1)
      perror("Trabajarsemaforo: ");
   }

void Wsemaforo(int iSclave){
   /*Disminuye un semaforo
   */

   Trabajarsemaforo (iSclave, -1);
}

void Ssemaforo(int iSclave){
   /*Incrementa un semaforo.
   */

   Trabajarsemaforo (iSclave, 1);
}



void Definirmemoriacompartida(int *ipShmclave, void **vpShmpuntero,int iClave,int iMide){
   /*Crea y/o mapea un pedazo de memoria compratida.
   */


   *ipShmclave = shmget((key_t) iClave, iMide, IPC_CREAT | 0666);
   if (*ipShmclave < 0) {
      perror("Definirmemoriacompartida: ");
      exit(-1);
   }
   *vpShmpuntero = (void *) shmat(*ipShmclave,0,0);
}



void Liberarsemaforo(int iSclave){
   /*Libera la memoria ocupada por el semanforo.
   */

   semctl (iSclave, 0, IPC_RMID, 0);
}

void Liberarmemoriacompartida(int ipShmclave, void *vpShmpuntero){
   /*Libera la memoria compartida.
   */

   shmdt ((void *) vpShmpuntero);
   shmctl (ipShmclave, IPC_RMID, 0);
}


void Liberarcontrol(){
   /*Libera la memoria y el semaforo
   */

   Liberarsemaforo(iEstsemaforo);
   Liberarmemoriacompartida(iEstmemoriacomp,pvEstmemoriacomp);
}


void Escribirlog(void *sEscribirlinea,int iCantidadescrita,char *sComentario,int iPonerhora){
   /*Escrobibe una linea en el log.
   */

   //todo fixit
   return;

   FILE *fArchivologescribir;
   time_t *tiempo;
   tiempo=malloc(sizeof(time_t));
   time(tiempo);

   fArchivologescribir=fopen("registro.log","a");
   if (iPonerhora==1){
      fwrite(ctime(tiempo),strlen(ctime(tiempo)),1,fArchivologescribir);
      fwrite(sComentario,strlen(sComentario),1,fArchivologescribir);
      fwrite("\n",1,1,fArchivologescribir);
   }
   fwrite(sEscribirlinea,iCantidadescrita,1,fArchivologescribir);
   fwrite("\n\n\n",3,1,fArchivologescribir);
   fclose(fArchivologescribir);
}


int CrearSocket(unsigned short iNumeropuerto,char *sUsarestaip){
   /*Crea un socket
   */

   char   myname[LARGOHOST+1];
   int    iSock;
   struct sockaddr_in saElsocket;
   struct hostent *hpElsocket;

   memset(&saElsocket, 0, sizeof(struct sockaddr_in));
   hpElsocket= gethostbyname(sUsarestaip);
   if (hpElsocket == NULL)
      return(-1);
   saElsocket.sin_family= hpElsocket->h_addrtype;
   saElsocket.sin_port= htons(iNumeropuerto);
   if ((iSock= socket(AF_INET, SOCK_STREAM, 0)) < 0)
      return(-1);
   if (bind(iSock,(struct sockaddr *) &saElsocket,sizeof(saElsocket)) < 0) {
      close(iSock);
      return(-1);
   }
   listen(iSock, 300);
   return(iSock);
}



char *Reemplazarcadena(char *sCadenavieja,char *sBuscar,char *sReemplazar){
   /*Busca una ocurrencia en una cadena y la reemplaza con otra
   */

   char *sCadenanueva,*sCadenabusca,*sUncaracterviejo;
   int iCuantofor;

   sCadenanueva=malloc(strlen(sCadenavieja)*5);
   memset(sCadenanueva, 0, strlen(sCadenavieja)*5);
   sUncaracterviejo=malloc(5);


   sCadenabusca=sCadenavieja;


   for (iCuantofor=0;iCuantofor<strlen(sCadenavieja);){
      if (strncmp(sCadenabusca,sBuscar,strlen(sBuscar))==0){
         strcat(sCadenanueva,sReemplazar);

         iCuantofor=iCuantofor+strlen(sBuscar);
         sCadenabusca=sCadenabusca+strlen(sBuscar);
      }
      else{
         memset(sUncaracterviejo, 0, 5);
         strncpy(sUncaracterviejo,sCadenabusca,1);
         strcat(sCadenanueva,sUncaracterviejo);
         iCuantofor++;
         sCadenabusca++;
      }
   }
   return (sCadenanueva);
}




char *Buscarencadena(void *sCadena,char *sInicio,char *sFin){
   /*Busca una ocurrencia en una cadena Y devuelve un puntero a una cadena con el valor recuperado
   */

   char *sDevueltoporestafuncion,*sDesdebuscar,*sDesdebuscarsininicio;
   int iCuantomide,iControlfor,iEncontro;
   int iMidebusqueda;



   if ((sDesdebuscar=strstr(sCadena,sInicio))==NULL){
      return( NULL);
   }
   else if (sFin=="\0"){

      iCuantomide=strlen(sDesdebuscar);

      sDevueltoporestafuncion=malloc(iCuantomide+1);
      memset(sDevueltoporestafuncion, 0, iCuantomide+1);


      sDesdebuscarsininicio=sDesdebuscar+strlen(sInicio);
      strcpy(sDevueltoporestafuncion,sDesdebuscarsininicio);


      return (sDevueltoporestafuncion);

   }
   else
   {
      sDesdebuscarsininicio=sDesdebuscar+strlen(sInicio);
      iCuantomide=0;
      iEncontro=0;

      iMidebusqueda=strlen(sDesdebuscarsininicio);
      for (iControlfor=1;iControlfor<=iMidebusqueda;iControlfor++){
         if (strncmp(sDesdebuscarsininicio,sFin,strlen(sFin))==0){
            iEncontro=1;
         }
         if (iEncontro==0){
            iCuantomide++;
         }
         sDesdebuscarsininicio++;
      }


      sDevueltoporestafuncion=malloc(iCuantomide+1);

      memset(sDevueltoporestafuncion, 0, iCuantomide+1);


      sDesdebuscarsininicio=sDesdebuscar+strlen(sInicio);
      strncpy(sDevueltoporestafuncion,sDesdebuscarsininicio,iCuantomide);
      return (sDevueltoporestafuncion);
   }
}



char *Buscarmayusculas(char *sPonermayuscula,char *sQuebusca){
   /*Busca una ocurrencia en una cadena sin dar importancia a la mayusculas
   */

   int iControlfor,iLongstr,iLetramayus;
   char *sTrabajarstring,*sIniciostr,*iEncontro;


   sTrabajarstring=sPonermayuscula;

   sIniciostr=sTrabajarstring;



   iLongstr=strlen(sTrabajarstring);
   iEncontro=NULL;

   for (iControlfor=1;iLongstr>=iControlfor;iControlfor++){
      if(strncasecmp(sTrabajarstring,sQuebusca,strlen(sQuebusca))==0){
         iEncontro=sTrabajarstring;
      }
      sTrabajarstring++;
   }
   return iEncontro;
}



char *Buscarmayusencadena(void *sCadena,char *sInicio,char *sFin){
   /*Busca una ocurrencia en una cadena sin dar importancia a la mayusculas
   */

   char *sDevueltoporestafuncion,*sDesdebuscar,*sDesdebuscarsininicio;
   int iCuantomide,iControlfor,iEncontro,iMidebusqueda;

   if ((sDesdebuscar=Buscarmayusculas(sCadena,sInicio))==NULL){
      return( NULL);
   }
   else if (sFin=="\0"){
      iCuantomide=strlen(sDesdebuscar);

      sDevueltoporestafuncion=malloc(iCuantomide+1);
      memset(sDevueltoporestafuncion, 0, iCuantomide+1);


      sDesdebuscarsininicio=sDesdebuscar+strlen(sInicio);
      strcpy(sDevueltoporestafuncion,sDesdebuscarsininicio);


      return (sDevueltoporestafuncion);

   }
   else
   {
      sDesdebuscarsininicio=sDesdebuscar+strlen(sInicio);
      iCuantomide=0;
      iEncontro=0;
      iMidebusqueda=strlen(sDesdebuscarsininicio);
      for (iControlfor=1;iControlfor<=iMidebusqueda;iControlfor++){
         if (strncasecmp(sDesdebuscarsininicio,sFin,strlen(sFin))==0){
            iEncontro=1;
         }
         if (iEncontro==0){
            iCuantomide++;
         }
         sDesdebuscarsininicio++;
      }


      sDevueltoporestafuncion=malloc(iCuantomide+1);
      memset(sDevueltoporestafuncion, 0, iCuantomide+1);


      sDesdebuscarsininicio=sDesdebuscar+strlen(sInicio);
      strncpy(sDevueltoporestafuncion,sDesdebuscarsininicio,iCuantomide);
      return (sDevueltoporestafuncion);
   }
}



int Buscarconfiguracionchar(char **sVariableconfiguracion,char *sClavebusqueda){
   /*Busca una variable de configuracion en el archivo y devuelve su valor
   */

   FILE *fArchivoconfig;
   char *sLeerconf,*sVercodigo,*sVervalor,*sDevuelve1,*sDevuelve,*sTemporalconfig;
   int iLecturadocs,iEncontrovalor,iTerminovalor;

   fArchivoconfig=fopen("server.cnf","r");
   if (fArchivoconfig==NULL){
      return 1;
   }

   sTemporalconfig=malloc(1000);
   iTerminovalor=1;
   memset(sTemporalconfig,0,1000);
   iLecturadocs=fscanf(fArchivoconfig,"%s\n",sTemporalconfig);
   for (;((iTerminovalor==1) && (iLecturadocs>0));){
      if (strncmp(sTemporalconfig,sClavebusqueda,strlen(sClavebusqueda))==0){

         strcpy(*sVariableconfiguracion,sTemporalconfig+strlen(sClavebusqueda));

        iTerminovalor=0;
      }
      else{
         memset(sTemporalconfig,0,1000);
         iLecturadocs=fscanf(fArchivoconfig,"%s\n",sTemporalconfig);
      }
   }
   fclose(fArchivoconfig);
   free(sTemporalconfig);

   return iTerminovalor;
}


int Cambiarconfiguracion(char *sClavebusqueda,char *sVariableconfiguracion){
   /*Actualiza una variable de configura
   */

   FILE *fArchivoconfig;
   FILE *fArchivonuevo;
   char *sLeerconf,*sVercodigo,*sVervalor,*sDevuelve1,*sDevuelve,*sTemporalconfig;
   int iLecturadocs,iEncontrovalor,iTerminovalor;

   sLeerconf=malloc(1000);
   fArchivoconfig=fopen("server.cnf","r");
   if (fArchivoconfig==NULL){
      return 1;
   }

   fArchivonuevo=fopen("server.tmp","w");
   if (fArchivoconfig==NULL){
      return 1;
   }
   iTerminovalor=1;

   memset(sLeerconf,0,1000);
   iLecturadocs=fscanf(fArchivoconfig,"%s\n",sLeerconf);
   for (;(iLecturadocs>0);){
      if (strncmp(sLeerconf,sClavebusqueda,strlen(sClavebusqueda))==0){
         iLecturadocs=fprintf(fArchivonuevo,"%s%s\n",sClavebusqueda,sVariableconfiguracion);
         memset(sLeerconf,0,1000);
         iLecturadocs=fscanf(fArchivoconfig,"%s\n",sLeerconf);
         iTerminovalor=0;
      }
      else{
         iLecturadocs=fprintf(fArchivonuevo,"%s\n",sLeerconf);
         memset(sLeerconf,0,1000);
         iLecturadocs=fscanf(fArchivoconfig,"%s\n",sLeerconf);
      }
   }
   fclose(fArchivoconfig);
   fclose(fArchivonuevo);

   fArchivonuevo=fopen("server.tmp","r");
   if (fArchivoconfig==NULL){
      return 1;
   }

   fArchivoconfig=fopen("server.cnf","w");
   if (fArchivoconfig==NULL){
      return 1;
   }

   iTerminovalor=1;

   memset(sLeerconf,0,1000);
   iLecturadocs=fscanf(fArchivonuevo,"%s\n",sLeerconf);
   for (;(iLecturadocs>0);){
      iLecturadocs=fprintf(fArchivoconfig,"%s\n",sLeerconf);
      memset(sLeerconf,0,1000);
      iLecturadocs=fscanf(fArchivonuevo,"%s\n",sLeerconf);
   }
   fclose(fArchivoconfig);
   fclose(fArchivonuevo);

   iTerminovalor=0;

   return iTerminovalor;
}



int Buscarconfiguracionint(int *sVariableconfiguracion,char *sClavebusqueda){
   /*Busca una variable de configuracion en el archivo y devuelve su valor
   */

   FILE *fArchivoconfig;
   char *sLeerconf,*sVercodigo,*sVervalor,*sDevuelve1,*sDevuelve,*sTemporalconfig;
   int iLecturadocs,iEncontrovalor,iTerminovalor;

   fArchivoconfig=fopen("server.cnf","r");
   if (fArchivoconfig==NULL){
      return 1;
   }

   sTemporalconfig=malloc(1000);
   iTerminovalor=1;
   iLecturadocs=fscanf(fArchivoconfig,"%s\n",sTemporalconfig);
   for (;((iTerminovalor==1) && (iLecturadocs>0));){
      if (strncmp(sTemporalconfig,sClavebusqueda,strlen(sClavebusqueda))==0){
         *sVariableconfiguracion=atoi(sTemporalconfig+strlen(sClavebusqueda));
        iTerminovalor=0;
      }
      else{
         iLecturadocs=fscanf(fArchivoconfig,"%s\n",sTemporalconfig);
      }
   }
   fclose(fArchivoconfig);
   free(sTemporalconfig);

   return iTerminovalor;
}






int Buscarusuario(char *sUser,char *sClave){
   /*Valida un usuario
   */

   FILE *fArchivoclaves;
   char *sLeerarchivo,*sVerusuario,*sVerpassword;
   int iLecturadocs,iEncontroclave,iTerminoclave;

   sLeerarchivo=malloc(1000);
   iEncontroclave=0;
   iTerminoclave=0;
   fArchivoclaves=fopen("password.pas","r");
   if (fArchivoclaves==NULL){
      return 1;
   }


   for (;iTerminoclave==0;){

      memset(sLeerarchivo, 0, 1000);
      iLecturadocs=fscanf(fArchivoclaves,"%s\n",sLeerarchivo);
      if (iLecturadocs==EOF){
         iTerminoclave=1;
      }
      else
      {
         sVerusuario=Buscarencadena(sLeerarchivo,"nombre=",",");
         sVerpassword=Buscarencadena(sLeerarchivo,"password=",";");


         if ((strcmp(sUser,sVerusuario)==0) && strcmp(sClave,sVerpassword)==0){
            iTerminoclave=1;
            iEncontroclave=1;
         }
      }



   }

   fclose(fArchivoclaves);

   return (iEncontroclave);
}







int Esunbuenencabezado(char *sBufferencabezado){
   /*Copruieba el encabezado en el buffer
   */

   char *sFinalencabezado,*sBcontentType,*sFinalhead;
   char *sEstoybuscando;



   sFinalencabezado=malloc(10);
   sprintf(sFinalencabezado,"%c%c%c%c",13,10,13,10);
   sFinalhead=strstr(sBufferencabezado,sFinalencabezado);
   if (!(sFinalhead==NULL)){
      sprintf(sFinalencabezado,"%c%c",13,10);

      sBcontentType=Buscarmayusencadena(sBufferencabezado,"content-length: ",sFinalencabezado);
      if (sBcontentType==NULL){
         free(sFinalencabezado);
         return 0;}
      else {
         if (strlen(sFinalhead)-4>=atoi(sBcontentType)){
            free(sFinalencabezado);
            return 0;
         }
         else{
            free(sFinalencabezado);
            return 1;
         }
      }
   }
   else
         free(sFinalencabezado);
      return 1;
}



void Escribeestadisticas(float tTpropocer,float tTprolat,int iTiposalida){
   /*Escribe las estadisticas en la memoria compartida
   */
   char *sLineanueva;

   sLineanueva=malloc(1000);
   Wsemaforo(iEstsemaforo);  /*Espera hasta que el padre lea la memoria*/
   memset(pvEstmemoriacomp, 0, sizeof(char)*5000);
   sprintf(sLineanueva,"Hijo numero : %d;\n\0",getpid());
   strcat(pvEstmemoriacomp,sLineanueva);
   sprintf(sLineanueva,"Tiempo proceso: %f;\n\0",tTpropocer);
   strcat(pvEstmemoriacomp,sLineanueva);
   sprintf(sLineanueva,"Tiempo latencia: %f;\n\0",tTprolat);
   strcat(pvEstmemoriacomp,sLineanueva);
   sprintf(sLineanueva,"Tipo salida: %d\n\0;",iTiposalida);
   strcat(pvEstmemoriacomp,sLineanueva);
   free(sLineanueva);
}



int Escribosock(int iEscriatenderestesocket,char *sQueleescribo){
   /*Escribe en un socket
   */

   return write(iEscriatenderestesocket, sQueleescribo,strlen(sQueleescribo) );

}

void VerSIGPIPE(){
   /*Avise que el pipe se rompio
   */

   if (iSeperdioconexion=0){
      printf("pid : %d ",getpid());
      printf("Se rompio el pipe.\n");
   };
   iSeperdioconexion=1;
}

void VerSIGALRM(){
   /*Valida errores de pipe y conexion
   */
   char *sLeermensaje;
   int iLecturadocs,fsArchivodocs;

   printf("pid : %d ",getpid());
   printf("Alarm\n");
   ftime(&stTiempofinal);

   fTiempoproceso=((stTiempofinal.time-stTiempoinicio.time)*1000)+
                  (stTiempofinal.millitm-stTiempoinicio.millitm);
   fTiempolatencia=stTiempofinal.millitm;

   if (iSeperdioconexion==1){
      printf("Se perdio conexion\n");
      close(iCerraratenderestesocket);
      Escribeestadisticas(fTiempoproceso,fTiempolatencia,2);
      exit(1);
   };
   if (iLecturanula==1){
      printf("Lectura nula.\n");
      close(iCerraratenderestesocket);
      Escribeestadisticas(fTiempoproceso,fTiempolatencia,3);
      exit(1);
   };
   if (!(iPidpadre==getppid())){
      printf("Murio el padre.\n");
      close(iCerraratenderestesocket);
      Escribeestadisticas(fTiempoproceso,fTiempolatencia,4);
      exit(1);
   };
   if (iEjecutandocgi==1){
      printf("Cgi colgado.\n");

      signal(SIGCHLD, SIG_IGN);
      kill(iPidcgiexe,SIGKILL); /*el padre mata al cgi por mal hijo*/
      printf("Cgi colgado.\n");

      if (!(iEscribiocgi==1)){
         write(iCerraratenderestesocket, "HTTP/1.0 500 Internal Server Error\n\n",36);
      }
      Escribosock(iCerraratenderestesocket,"TIMEOUT");

/*      sLeermensaje=malloc(1000);
      memset(sLeermensaje, 0, 1000);

      write(iCerraratenderestesocket, "HTTP/1.0 500 Internal Server Error\n\n",36);
      fsArchivodocs=open("404/500.html",O_RDONLY,00400);
      memset(sLeermensaje, 0, 1000);
      iLecturadocs=read(fsArchivodocs,sLeermensaje,25);
      write(iCerraratenderestesocket,sLeermensaje, iLecturadocs);
      for (;iLecturadocs>0;){
         memset(sLeermensaje, 0, 1000);
         iLecturadocs=read(fsArchivodocs,sLeermensaje,25);
         write(iCerraratenderestesocket,sLeermensaje, iLecturadocs);
      };
      close(fsArchivodocs);*/

      close(iCerraratenderestesocket);
      Escribeestadisticas(fTiempoproceso,fTiempolatencia,5);
      Escribirlog("TIMEOUT",strlen("TIMEOUT"),"CGI",1);
      exit(1);
   };

   alarm(iTiempoespera);

}

void Terminarelhijo(){
   /*Mata al hijo
   */

   if (iEjecutandocgi==1){
      kill(iPidcgiexe,SIGKILL); /*Hijicidio*/
   };

   kill(getpid(),SIGKILL);
}




char *Decodificarpedido(char *sBuscarpedido,char *sBufferparam,char *sBufferaccion,char *sParamcgi[1000]){
   /*Decodifica el encabezado
   */

   char *sCadenaencontrada,*sCadenamascuatro,*sTokendevuelto,*sValorfinal;
   char *sCadenaparam,*sCadenaparamusa,*sCadenacgiparamusa;
   char *sBusparametroscgi,*sBusultimo,*sQueparamesta,*sPrincipiobusquedacgi;
   int iSigueparametros,iQueparametro,iEsprimero;


   sPrincipiobusquedacgi=malloc(10000);
   memset(sPrincipiobusquedacgi, 0, 10000);
   sQueparamesta=malloc(10000);
   memset(sQueparamesta, 0, 10000);
   sCadenaparamusa=malloc(10000);
   memset(sCadenaparamusa, 0, 10000);

   strcpy(sCadenaparamusa,sBuscarpedido);

   sCadenaencontrada=strstr(sBuscarpedido,"GET");
   if (!(sCadenaencontrada== NULL)){
      sTokendevuelto=strtok(sCadenaencontrada+4," ?");

      sValorfinal=malloc(10000);
      memset(sValorfinal, 0, 10000);
      strcpy(sValorfinal,sDirectorioprincipal);

      if ((strncmp(sTokendevuelto,"/",1)==0) && (strlen(sTokendevuelto)==1)){

         sValorfinal=strcat(sValorfinal,"/");
         sValorfinal=strcat(sValorfinal,sPaginapordefecto);
      }
      else{

         sCadenacgiparamusa=Buscarencadena(sCadenaparamusa,"GET "," ");
         sBusparametroscgi=strstr(sCadenacgiparamusa,"?");   /*hay mas parametros*/
         if (!(sBusparametroscgi==NULL)){
            sQueparamesta=Buscarencadena(sBusparametroscgi,"?"," ");
            sParamcgi[1]=malloc(strlen(sQueparamesta)+5);
            memset(sParamcgi[1], 0, strlen(sQueparamesta)+5);
            strcpy(sParamcgi[1],sQueparamesta);
         }

         /* Busca en el resto del buffer por el content a ver si hay parametros nuevos*/
         sCadenacgiparamusa=malloc(10000);
         sprintf(sPrincipiobusquedacgi,"%c%c%c%c",13,10,13,10);

         sCadenacgiparamusa=Buscarencadena(sCadenaparamusa,sPrincipiobusquedacgi,"\0");

         if (!(sCadenacgiparamusa==NULL)){
            sParamcgi[2]=malloc(strlen(sCadenacgiparamusa)+5);
            memset(sParamcgi[2], 0, strlen(sCadenacgiparamusa)+5);
            strcpy(sParamcgi[2],sCadenacgiparamusa);
         }

         /*Fin buscar parametros*/


         sValorfinal=strcat(sValorfinal,sTokendevuelto);
      }
      strcpy(sBufferaccion,"!!!!");
      return(sValorfinal);
   }


   sCadenaencontrada=strstr(sBuscarpedido,"HEAD");
   if (!(sCadenaencontrada== NULL)){
      sTokendevuelto=strtok(sCadenaencontrada+4," ?");

      sValorfinal=malloc(10000);
      memset(sValorfinal, 0, 10000);
      strcpy(sValorfinal,sDirectorioprincipal);

      if ((strncmp(sTokendevuelto,"/",1)==0) && (strlen(sTokendevuelto)==1)){
         sValorfinal=strcat(sValorfinal,"/");
         sValorfinal=strcat(sValorfinal,sPaginapordefecto);
      }
      else{
         sValorfinal=strcat(sValorfinal,sTokendevuelto);
      }
      strcpy(sBufferaccion,"====");
      return(sValorfinal);
   }



   sCadenaencontrada=strstr(sBuscarpedido,"POST");
   if (!(sCadenaencontrada== NULL)){
      sTokendevuelto=strtok(sCadenaencontrada+4," ?");


      sValorfinal=malloc(10000);
      memset(sValorfinal, 0, 10000);
      strcpy(sValorfinal,sDirectorioprincipal);

      sCadenaparam=malloc(10000);

      sCadenaparam=rindex(sCadenaparamusa,13);
      strcpy(sBufferparam,sCadenaparam+2);

      strcpy(sBufferaccion,"))))");

      if ((strncmp(sTokendevuelto,"/",1)==0) && (strlen(sTokendevuelto)==1)){
         sValorfinal=strcat(sValorfinal,"/");
         sValorfinal=strcat(sValorfinal,sPaginapordefecto);
      }
      else{
         /* Busca si hay parametros en la linea POST*/

         iQueparametro=1;
         sCadenacgiparamusa=Buscarencadena(sCadenaparamusa,"POST "," ");

         sBusparametroscgi=strstr(sCadenacgiparamusa,"?");   /*hay mas parametros*/

         if (!(sBusparametroscgi==NULL)){
            sQueparamesta=Buscarencadena(sBusparametroscgi,"?"," ");
            sParamcgi[1]=malloc(strlen(sQueparamesta)+5);
            memset(sParamcgi[1], 0, strlen(sQueparamesta)+5);
            strcpy(sParamcgi[1],sQueparamesta);
         }


         /* Busca en el resto del buffer por el content a ver si hay parametros nuevos*/

         sCadenacgiparamusa=malloc(10000);
         sprintf(sPrincipiobusquedacgi,"%c%c%c%c",13,10,13,10);


         sCadenacgiparamusa=Buscarencadena(sCadenaparamusa,sPrincipiobusquedacgi,"\0");

         if (!(sCadenacgiparamusa==NULL)){
            sParamcgi[2]=malloc(strlen(sCadenacgiparamusa)+5);
            memset(sParamcgi[2], 0, strlen(sCadenacgiparamusa)+5);
            strcpy(sParamcgi[2],sCadenacgiparamusa);
         }

         /*Fin buscar parametros*/


         sValorfinal=strcat(sValorfinal,sTokendevuelto);
      }
      return(sValorfinal);
   }
   else{
      return( NULL);
   }


}


void Muertecgi(){
   /*Mata el cgi
   */
   int iEstadosalidacgi;
   char *sLeerarchivo;
   sLeerarchivo=malloc(1000);


   waitpid (-1,&iEstadosalidacgi,WNOHANG);

   if (WIFSIGNALED (iEstadosalidacgi)){
      memset(sLeerarchivo, 0, 1000);
      sprintf (sLeerarchivo,"Error de cgi numero: %d\n", WTERMSIG (iEstadosalidacgi));
      Escribirlog(sLeerarchivo,strlen(sLeerarchivo),"CGI",1);
      if (!(iEscribiocgi==1)){
         write(iCerraratenderestesocket, "HTTP/1.0 500 Internal Server Error\n\n",36);
      }
      Escribosock(iCerraratenderestesocket,sLeerarchivo);
   }
   iEjecutandocgi=0;
}


void ponerentorno(char **sVariableentorno,char *sContenido){
   /*Pone una variable de entorno del cgi
   */

   if (!(sContenido==NULL)){
      *sVariableentorno=malloc(strlen(sContenido)+20);
      memset(*sVariableentorno,0,strlen(sContenido)+20);
      sprintf(*sVariableentorno,"%s",sContenido);
   }
}






void Cargaconfiguracion(){
   /*Carga la configuracion
   */

   iPuertoserver=7575;
   iTiempoespera=2;
   iTiempocgiespera=10;
   iCantidadmaximadehijos=50;
   sprintf(sNombredelserver,"127.000.000.001");
   sprintf(sNombrewww,"www.adw.com");
   sprintf(sDirectorioprincipal,"doc");
   sprintf(sDirectoriocgi,"doc/cgi/");
   sprintf(sDirectoriorestringido,"doc/usuarios/");
   sprintf(sDirectoriorestringidocgi,"doc/usuarioscgi/");
   sprintf(sPaginapordefecto,"index.html");
   sprintf(sDirectoriodelprograma,"/home/pablo/tpso/server/");
   sprintf(sPedidoestadisticas,"%s/estadistica.adw",sDirectorioprincipal);
   sprintf(sEncabezadocgi1,"Content-Type: text/plain");
   sprintf(sEncabezadocgi2,"");
   sprintf(sPasswordcambio,"cambiar");
   sprintf(sRepetidorpassword,"repetidor");
   sprintf(sLlamadaconfig,"cambiarlaconfiguracion");
   sprintf(sLlamadaconfig1,"postcambioformulario");
   sprintf(sLlamadaconfig2,"ponerconfiguracion.html");
   sprintf(sLlamadaserver,"agregarunservidornuevo");
   sprintf(sQuitarserver,"quitarservidor");
   sprintf(sRepetidor,"127.0.0.1");
   sprintf(sRepetidorpuerto,"9001");


   Buscarconfiguracionint(&iPuertoserver,"Puertoserver=");
   Buscarconfiguracionint(&iTiempoespera,"Tiempoespera=");
   Buscarconfiguracionint(&iTiempocgiespera,"Tiempocgiespera=");
   Buscarconfiguracionint(&iCantidadmaximadehijos,"Cantidadmaximadehijos=");
   Buscarconfiguracionchar(&sNombredelserver,"Nombredelserver=");
   Buscarconfiguracionchar(&sNombrewww,"Nombrewww=");
   Buscarconfiguracionchar(&sDirectorioprincipal,"Directorioprincipal=");
   Buscarconfiguracionchar(&sDirectoriocgi,"Directoriocgi=");
   Buscarconfiguracionchar(&sDirectoriorestringido,"Directoriorestringido=");
   Buscarconfiguracionchar(&sDirectoriorestringidocgi,"Directoriorestringidocgi=");
   Buscarconfiguracionchar(&sPaginapordefecto,"Paginapordefecto=");
   Buscarconfiguracionchar(&sDirectoriodelprograma,"Directoriodelprograma=");
   Buscarconfiguracionchar(&sPedidoestadisticas,"Pedidoestadisticas=");
   Buscarconfiguracionchar(&sEncabezadocgi1,"Encabezadocgi1=");
   Buscarconfiguracionchar(&sEncabezadocgi2,"Encabezadocgi2=");
   Buscarconfiguracionchar(&sPasswordcambio,"Passwordcambio=");
   Buscarconfiguracionchar(&sRepetidorpassword,"Repetidorpassword=");
   Buscarconfiguracionchar(&sLlamadaconfig,"Llamadaconfig=");
   Buscarconfiguracionchar(&sLlamadaconfig1,"Llamadaconfig1=");
   Buscarconfiguracionchar(&sLlamadaconfig2,"Llamadaconfig2=");
   Buscarconfiguracionchar(&sLlamadaserver,"Llamadaserver=");
   Buscarconfiguracionchar(&sQuitarserver,"Quitarserver=");
   Buscarconfiguracionchar(&sRepetidor,"Repetidor=");
   Buscarconfiguracionchar(&sRepetidorpuerto,"Repetidorpuerto=");




}






void Atenderhttp(int iAtenderestesocket){
   /*Recibe la peticion del browser y la procesa
   */

   int iContador,iCuenta;
   int iLecturadocs;
   int iCantidaddesockets;
   fd_set pfsSelecion,pfsSelectornueva;
   char *sNombrearchivo,*sParametroscgi[3],*sParametrosentorno[100],*sNombretemporal,*sNombretemporalacceso,*sNombrereemarchivoacceso,*sBuscaencabezado;
   void *sLeerarchivo,*sBufferdatos,*sBufferparam,*sBufferaccion,*sBufferdatosnuevos,*sBufferdatosacceso,*sNombrearchivoacceso;
   int fsArchivodocs;
   int iPidfork,iCuanlee,iQueejecuto,iDevuelve;
   double iCuentaw;
   int iTemporalcgi,iMaxfnuevo,iNumerofdsnuevo,*iLargobase64;
   struct timeval stTiemponuevoselect;
   char *sUsuario,*sPassword,*sDecodificado,*sBufferverifica,*sPonestadistica;
   int iSigue,iNohayencabezado;
   int iTubodesc[2],iPoneentorno;
   int iTubostin[2];
   char *sPoneentornocgi,*sPoneentornocgi1,*sPoneentornocgi2;
   char *sPoneentornocgi3,*sProcesarsncacgi,*sBuffercgi,*sBuffercgi2,*sBuffercgi3;
   char *sCambiarclave,*sCambiarvalor,*sCambiarvariable,*sPongovalor,*sRevisapass,*cUltimonombre;
   int iDondeestas,iDevuelscan,iNumerodecabeza,iContt,iCambioconfig;
   int iQueentorno;
   int iEsconfig,iValidapass;

   ftime(&stTiempoinicio);


   iCerraratenderestesocket=iAtenderestesocket;

   signal(SIGPIPE,VerSIGPIPE);
   signal(SIGALRM,VerSIGALRM);

   signal(SIGCHLD, SIG_IGN);

   signal(SIGINT,SIG_DFL);
   signal(SIGQUIT,SIG_DFL);
   signal(SIGKILL,SIG_DFL);
   signal(SIGTERM,SIG_DFL);
   signal(SIGUSR1,Terminarelhijo);



   alarm(iTiempoespera);

   iEjecutandocgi=0;
   iEscribiocgi=0;
   iSeperdioconexion=0;
   iLecturanula=0;
   sLeerarchivo=malloc(BUFFERSOCK);
   sBufferdatos=malloc(BUFFERSOCK);
   sBufferdatosacceso=malloc(BUFFERSOCK);
   sBufferparam=malloc(BUFFERSOCK);
   sBufferaccion=malloc(BUFFERSOCK);
   sBufferdatosnuevos=malloc(BUFFERSOCK);
   sBufferverifica=malloc(BUFFERSOCK);
   sBuffercgi=malloc(BUFFERSOCK);
   sBuffercgi2=malloc(BUFFERSOCK);
   sBuffercgi3=malloc(BUFFERSOCK);



   sPonestadistica=malloc(1000);
   sPoneentornocgi=malloc(1000);
   sPoneentornocgi1=malloc(1000);
   sPoneentornocgi2=malloc(1000);
   sPoneentornocgi3=malloc(1000);

   sParametroscgi[0]=NULL;
   sParametroscgi[1]=NULL;
   sParametroscgi[2]=NULL;
   sParametroscgi[3]=NULL;
   for (iPoneentorno=0;iPoneentorno<100;iPoneentorno++){
      sParametrosentorno[iPoneentorno]=NULL;
   }


   Escribirlog(sBufferdatos,strlen(sBufferdatos),"Recibido",1);

   iCuanlee=read(iAtenderestesocket, sBufferdatos, BUFFERSOCK);





/* Revisa el socket hasta que no quede ningun dato*/


   strcpy(sBufferverifica,sBufferdatos);
   iNohayencabezado=Esunbuenencabezado(sBufferdatos);


   for (;iNohayencabezado==1;){
      FD_ZERO(&pfsSelectornueva);
      FD_SET(iAtenderestesocket, &pfsSelectornueva);
      iMaxfnuevo=iAtenderestesocket+1;
      stTiemponuevoselect.tv_sec=iTiempoespera;
      stTiemponuevoselect.tv_usec=0;

      iNumerofdsnuevo  = select(iMaxfnuevo, &pfsSelectornueva, NULL, NULL, &stTiemponuevoselect);



      if (iNumerofdsnuevo == -1){
         printf("Error.\n");
         close(iAtenderestesocket);
         exit(1);
      }



      /*timeuot*/
       if (iNumerofdsnuevo == 0) {
         printf("Timeout.\n");
         close(iAtenderestesocket);
         exit(0);
       }


       if (iNumerofdsnuevo > 0) {
         memset(sBufferdatosnuevos, 0, BUFFERSOCK);
         iCuanlee=read(iAtenderestesocket, sBufferdatosnuevos, BUFFERSOCK);

         if (iCuanlee==0) iLecturanula=1;

         strcat(sBufferdatos,sBufferdatosnuevos);
         strcpy(sBufferverifica,sBufferdatosnuevos);
         iNohayencabezado=Esunbuenencabezado(sBufferdatos);
      }
   }


   strcpy(sBufferdatosacceso,sBufferdatos);
   strcpy(sBuffercgi,sBufferdatos);
   strcpy(sBuffercgi2,sBuffercgi);
   strcpy(sBuffercgi3,sBuffercgi);

   sNombrearchivo=Decodificarpedido(sBufferdatos,sBufferparam,sBufferaccion,sParametroscgi);


   Escribirlog(sNombrearchivo,strlen(sNombrearchivo),"Archivo solicitado",1);

   printf("Solicitado : %s\n",sNombrearchivo);


   if ((!(strstr(sNombrearchivo,sLlamadaconfig)==NULL)) ||
       (!(strstr(sNombrearchivo,sLlamadaconfig1)==NULL)) ||
       (!(strstr(sNombrearchivo,sLlamadaconfig2)==NULL))){
        iEsconfig=1;
       }
   else{
        iEsconfig=0;
   };

   /*Atiende un pedido de GET O HEAD o POST*/
   if ( (strncmp(sBufferaccion,"!!!!",4)==0) ||
        (strncmp(sBufferaccion,"))))",4)==0) ||
        (strncmp(sBufferaccion,"====",4)==0)){



     fsArchivodocs=open(sNombrearchivo,O_RDONLY,00400);

     if (!(fsArchivodocs== -1)){

        iLecturadocs=read(fsArchivodocs,sLeerarchivo,25);
        if (iLecturadocs<0){

           cUltimonombre=sNombrearchivo+strlen(sNombrearchivo)-1;
           if (strncmp(cUltimonombre,"/",1)==0){
              strcat(sNombrearchivo,sPaginapordefecto);
           }
           else{
              strcat(sNombrearchivo,"/");
              strcat(sNombrearchivo,sPaginapordefecto);
           }
        }

        close(fsArchivodocs);
     }

      /*se fija si requiere autorizacion*/
      iSigue=0;
      if ((strncmp(sNombrearchivo,sDirectoriorestringido,strlen(sDirectoriorestringido))==0) ||
          (strncmp(sNombrearchivo,sDirectoriorestringidocgi,strlen(sDirectoriorestringidocgi))==0)){

               sBuscaencabezado=malloc(3);
               sprintf(sBuscaencabezado,"%c%c\0",13,10);
               sNombrearchivoacceso=Buscarencadena(sBufferdatosacceso,"Basic ",sBuscaencabezado);

               if (sNombrearchivoacceso==NULL){
                  Escribosock(iAtenderestesocket, "HTTP/1.0 401 Unauthorized\n");
                  if (iEsconfig==1){
                     Escribosock(iAtenderestesocket, "WWW-Authenticate: Basic realm=\"Configuracion\"\n\n");
                  }
                  else{
                     Escribosock(iAtenderestesocket, "WWW-Authenticate: Basic realm=\"ADW\"\n\n");
                  }


                  fsArchivodocs=open("404/401.html",O_RDONLY,00400);
                  memset(sLeerarchivo, 0, BUFFERSOCK);
                  iLecturadocs=read(fsArchivodocs,sLeerarchivo,25);
                  write(iAtenderestesocket, sLeerarchivo, iLecturadocs);
                  for (;iLecturadocs>0;){
                     memset(sLeerarchivo, 0, BUFFERSOCK);
                     iLecturadocs=read(fsArchivodocs,sLeerarchivo,25);
                     write(iAtenderestesocket, sLeerarchivo, iLecturadocs);
                  };
                  close(fsArchivodocs);


                  Escribirlog("HTTP/1.0 401 Unauthorized\n",33,"",0);
                  Escribirlog(sBuffercgi,33,"",0);


                  iSigue=0;
               }
               else{


                  iLargobase64=malloc(sizeof(int));

                  *iLargobase64=60;
                  sDecodificado=php_base64_decode(sNombrearchivoacceso,1000,iLargobase64);


                  sUsuario=Buscarencadena(sDecodificado,"",":");
                  sPassword=Buscarencadena(sDecodificado,":","\0");

                  if (iEsconfig==1){
                     if (strncmp(sPasswordcambio,sPassword,strlen(sPasswordcambio))==0){
                        iValidapass=1;
                     }
                     else{
                        iValidapass=0;
                     }
                  }
                  else{
                     iValidapass=Buscarusuario(sUsuario,sPassword);
                  }

                  if (iValidapass==0){
                     Escribosock(iAtenderestesocket, "HTTP/1.0 401 Unauthorized\n");
                     if (iEsconfig==1){
                        Escribosock(iAtenderestesocket, "WWW-Authenticate: Basic realm=\"Configuracion\"\n\n");
                     }
                     else{
                        Escribosock(iAtenderestesocket, "WWW-Authenticate: Basic realm=\"ADW\"\n\n");
                     }
                     fsArchivodocs=open("404/401.html",O_RDONLY,00400);
                     memset(sLeerarchivo, 0, BUFFERSOCK);
                     iLecturadocs=read(fsArchivodocs,sLeerarchivo,25);
                     write(iAtenderestesocket, sLeerarchivo, iLecturadocs);
                     for (;iLecturadocs>0;){
                        memset(sLeerarchivo, 0, BUFFERSOCK);
                        iLecturadocs=read(fsArchivodocs,sLeerarchivo,25);
                        write(iAtenderestesocket, sLeerarchivo, iLecturadocs);
                     };
                     close(fsArchivodocs);


                     Escribirlog("HTTP/1.1 401 Unauthorized\n\n",33,"",0);
                     Escribirlog(sBuffercgi,33,"",0);
                     iSigue=0;
                  }
                  else{
                     iSigue=1;
                  }
               }
      }
      else{
         iSigue=1;
      }


     fsArchivodocs=open(sNombrearchivo,O_RDONLY,00400);
     if (!(fsArchivodocs== -1)){
     close(fsArchivodocs);


     if (iSigue==1){
      /*Verifica cgi*/
      if ((strncmp(sNombrearchivo,sDirectoriocgi,strlen(sDirectoriocgi))==0) ||
          (strncmp(sNombrearchivo,sDirectoriorestringidocgi,strlen(sDirectoriorestringidocgi))==0)){


         /*los pipes se ponen aca para poder poner en stinput lo que fuere*/
         pipe(iTubodesc);
         pipe(iTubostin);


         iQueentorno=0;
         sParametroscgi[0]=malloc(strlen(sNombrearchivo)+5);
         strcpy(sParametroscgi[0],sNombrearchivo);

         if (!(sParametroscgi[1]==NULL)){
            sParametrosentorno[iQueentorno]=malloc(strlen(sParametroscgi[1])+20);
            memset(sParametrosentorno[iQueentorno],0,strlen(sParametroscgi[1])+20);
            sprintf(sParametrosentorno[iQueentorno],"QUERY_STRING=%s",sParametroscgi[1]);
            iQueentorno++;
            sParametroscgi[1]=NULL;
         }

         if (!(sParametroscgi[2]==NULL)){
            /*escribe los parametros del content en el pipe de stimput*/
            write(iTubostin[1],sParametroscgi[2],strlen(sParametroscgi[2]));
            sParametroscgi[2]=NULL;
         }

         /*arma las variables de entorno*/

         memset(sPoneentornocgi,0,1000);
         sprintf(sPoneentornocgi,"SERVER_SOFTWARE=adw/1.0");
         ponerentorno(&sParametrosentorno[iQueentorno],sPoneentornocgi);
         iQueentorno++;


         memset(sPoneentornocgi,0,1000);
         sprintf(sPoneentornocgi,"SERVER_NAME=%s",sNombredelserver);
         ponerentorno(&sParametrosentorno[iQueentorno],sPoneentornocgi);
         iQueentorno++;

         memset(sPoneentornocgi,0,1000);
         sprintf(sPoneentornocgi,"GATEWAY_INTERFACE=1.0");
         ponerentorno(&sParametrosentorno[iQueentorno],sPoneentornocgi);
         iQueentorno++;

         memset(sPoneentornocgi,0,1000);
         sprintf(sPoneentornocgi,"SERVER_PROTOCOL=HTTP/1.0");
         ponerentorno(&sParametrosentorno[iQueentorno],sPoneentornocgi);
         iQueentorno++;

         memset(sPoneentornocgi,0,1000);
         sprintf(sPoneentornocgi,"SERVER_PORT=%d",iPuertoserver);
         ponerentorno(&sParametrosentorno[iQueentorno],sPoneentornocgi);
         iQueentorno++;


         if (strncmp(sBufferaccion,"!!!!",4)==0){
         memset(sPoneentornocgi,0,1000);
            sprintf(sPoneentornocgi,"REQUEST_METHOD=GET");
            ponerentorno(&sParametrosentorno[iQueentorno],sPoneentornocgi);
            iQueentorno++;
         }

         if (strncmp(sBufferaccion,"))))",4)==0){
         memset(sPoneentornocgi,0,1000);
            sprintf(sPoneentornocgi,"REQUEST_METHOD=POST");
            ponerentorno(&sParametrosentorno[iQueentorno],sPoneentornocgi);
            iQueentorno++;
      }

         memset(sPoneentornocgi,0,1000);
         sprintf(sPoneentornocgi,"AUTH_TYPE=HTTP/1.0");
         ponerentorno(&sParametrosentorno[iQueentorno],sPoneentornocgi);
         iQueentorno++;


         sBuscaencabezado=malloc(3);
         sprintf(sBuscaencabezado,"%c%c\0",13,10);
         sProcesarsncacgi=malloc(255);
         memset(sProcesarsncacgi,0,255);
         sProcesarsncacgi=Buscarmayusencadena(sBuffercgi,"Content-type: ",sBuscaencabezado);

         if (!(sProcesarsncacgi==NULL)){
            memset(sPoneentornocgi,0,1000);
            sprintf(sPoneentornocgi,"CONTENT_TYPE=%s",sProcesarsncacgi);
            ponerentorno(&sParametrosentorno[iQueentorno],sPoneentornocgi);
            iQueentorno++;
         }


         sBuscaencabezado=malloc(3);
         sprintf(sBuscaencabezado,"%c%c\0",13,10);
         sProcesarsncacgi=malloc(255);
         memset(sProcesarsncacgi,0,255);
         sProcesarsncacgi=Buscarmayusencadena(sBuffercgi2,"Content-length: ",sBuscaencabezado);

         if (!(sProcesarsncacgi==NULL)){
            memset(sPoneentornocgi,0,1000);
            sprintf(sPoneentornocgi,"CONTENT_LENGTH=%s",sProcesarsncacgi);
            ponerentorno(&sParametrosentorno[iQueentorno],sPoneentornocgi);
            iQueentorno++;
         }




         iDondeestas=0;
         iNumerodecabeza=iQueentorno;
         iContt=0;

         memset(sPoneentornocgi1,0,1000);
         memset(sPoneentornocgi2,0,1000);
         memset(sPoneentornocgi3,0,1000);
         memset(sPoneentornocgi,0,1000);
         sPoneentornocgi3=sBuffercgi3;
         sPoneentornocgi1=strstr(sBuffercgi3,"\n");

         for (;((!(sPoneentornocgi3==NULL)) && (iContt==0));){
            if (!(sPoneentornocgi1==NULL)){
                iDondeestas=strlen(sPoneentornocgi3)-strlen(sPoneentornocgi1);
            }
            else{
                iDondeestas=strlen(sPoneentornocgi3);
            }

            memset(sPoneentornocgi2,0,1000);
            if (!(sPoneentornocgi3==NULL)){
                    strncpy(sPoneentornocgi2,sPoneentornocgi3,iDondeestas);
            }
            if (!(sPoneentornocgi2==NULL)){
               if (strlen(sPoneentornocgi2)==1){
                  iContt=1;
               }
               else{
                  memset(sPoneentornocgi,0,1000);
                  sprintf(sPoneentornocgi,"HTTP_%s",sPoneentornocgi2);
                  ponerentorno(&sParametrosentorno[iNumerodecabeza],sPoneentornocgi);
               }
            }

            if (!(sPoneentornocgi3==NULL)){
               sPoneentornocgi3++;
               sPoneentornocgi3=strstr(sPoneentornocgi3,"\n");
            }
            if (!(sPoneentornocgi1==NULL)){
               sPoneentornocgi1++;
               sPoneentornocgi1=strstr(sPoneentornocgi1,"\n");
            }
            if (!(sPoneentornocgi3==NULL)){
               sPoneentornocgi3++;}
            iNumerodecabeza++;
         }



         alarm(iTiempocgiespera);
         signal (SIGCHLD, Muertecgi);
         iEjecutandocgi=1;
         iPidfork=fork();
         if (iPidfork==0){

            close(iAtenderestesocket);
            close(iTubodesc[0]);
            close(iTubostin[1]);

            dup2 (iTubodesc[1], 1);
/*            dup2 (iTubodesc[1], 2);*/

            dup2 (iTubostin[0], 0);
            iQueejecuto=execve(sNombrearchivo, sParametroscgi,sParametrosentorno);

            if (iQueejecuto==-1){
/*               printf("HTTP/1.1 500 Internal Server Error\n\n");
               printf("500 Internal Server Error\n");
               write(iAtenderestesocket, "HTTP/1.0 500 Internal Server Error\n\n",36);
               fsArchivodocs=open("404/500.html",O_RDONLY,00400);
               memset(sLeerarchivo, 0, BUFFERSOCK);
               iLecturadocs=read(fsArchivodocs,sLeerarchivo,25);
               write(iAtenderestesocket, sLeerarchivo, iLecturadocs);
               for (;iLecturadocs>0;){
                  memset(sLeerarchivo, 0, BUFFERSOCK);
                  iLecturadocs=read(fsArchivodocs,sLeerarchivo,25);
                  write(iAtenderestesocket, sLeerarchivo, iLecturadocs);
               };
               close(fsArchivodocs);
               Escribirlog("HTTP/1.0 500 Internal Server Error\n\n",33,"",0);
               Escribirlog(sBuffercgi,33,"",0);*/

               close(iTubodesc[1]);
               close(iTubostin[0]);
               return;
               exit(1);
            }
            close(iTubodesc[1]);
            close(iTubostin[0]);
            exit(0);
         }
         iPidcgiexe=iPidfork;
         close(iTubodesc[1]);
         close(iTubostin[0]);
         close(iTubostin[1]);

         Escribosock(iAtenderestesocket, "HTTP/1.0 200 OK\n");

         if (!(strlen(sEncabezadocgi1)==0)){
            Escribosock(iAtenderestesocket,sEncabezadocgi1);
            Escribosock(iAtenderestesocket,"\n");
         }

         if (!(strlen(sEncabezadocgi2)==0)){
            Escribosock(iAtenderestesocket,sEncabezadocgi2);
            Escribosock(iAtenderestesocket,"\n");
         }

         if ((!(strlen(sEncabezadocgi1)==0)) || (!(strlen(sEncabezadocgi2)==0))){
            Escribosock(iAtenderestesocket,"\n");
         }
         memset(sLeerarchivo, 0, BUFFERSOCK);
         iLecturadocs=read(iTubodesc[0],sLeerarchivo,25);

         for(;(iLecturadocs>0) || (iEjecutandocgi==1);){
            write(iAtenderestesocket, sLeerarchivo, iLecturadocs);
            memset(sLeerarchivo, 0, BUFFERSOCK);
            iLecturadocs=read(iTubodesc[0],sLeerarchivo,25);
            iEscribiocgi=1;
         }

         close(iTubodesc[0]);
      }
      else{
         /*Verifica GET o POST*/
         if ((strncmp(sBufferaccion,"!!!!",4)==0) || (strncmp(sBufferaccion,"))))",4)==0)){
            Escribosock(iAtenderestesocket, "HTTP/1.0 200 OK\n");
            Escribirlog("HTTP/1.0 200 OK \n",17,"",0);


            if ((!(strstr(sNombrearchivo,".jpg")== NULL)) ||
               (!(strstr(sNombrearchivo,".jpeg")== NULL)) ||
               (!(strstr(sNombrearchivo,".JPG")== NULL)) ||
               (!(strstr(sNombrearchivo,".JPEG")== NULL))){
                   Escribosock(iAtenderestesocket, "Content-Type: image/jpeg\n\n");
            }
            else if ((!(strstr(sNombrearchivo,".svg")== NULL)) ||
                    (!(strstr(sNombrearchivo,".SVG")== NULL))){
               Escribosock(iAtenderestesocket, "Content-Type: image/svg+xml\n\n");
            }
            else if ((!(strstr(sNombrearchivo,".gif")== NULL)) ||
                    (!(strstr(sNombrearchivo,".GIF")== NULL))){
               Escribosock(iAtenderestesocket, "Content-Type: image/gif\n\n");
            }            
            else if ((!(strstr(sNombrearchivo,".js")== NULL)) ||
                    (!(strstr(sNombrearchivo,".JS")== NULL))){
               Escribosock(iAtenderestesocket, "content-type: text/javascript\n\n");
            }
            else if ((!(strstr(sNombrearchivo,".css")== NULL)) ||
                    (!(strstr(sNombrearchivo,".CSS")== NULL))){
               Escribosock(iAtenderestesocket, "content-type: text/css\n\n");
            }                          
            else if ((!(strstr(sNombrearchivo,".png")== NULL)) ||
                   (!(strstr(sNombrearchivo,".PNG")== NULL))){
               Escribosock(iAtenderestesocket, "Content-Type: image/png\n\n");
            }
            else if ((!(strstr(sNombrearchivo,".htm")== NULL)) ||
                    (!(strstr(sNombrearchivo,".HTM")== NULL)) ||
                    (!(strstr(sNombrearchivo,".shtm")== NULL)) ||
                    (!(strstr(sNombrearchivo,".SHTM")== NULL)) ||
                    (!(strstr(sNombrearchivo,".dhtm")== NULL)) ||
                    (!(strstr(sNombrearchivo,".DHTM")== NULL)))
            {
               Escribosock(iAtenderestesocket, "Content-Type: text/html\n\n");
            }
            else {
               Escribosock(iAtenderestesocket, "Content-Type: text/plain\n\n");
            }
         }


         fsArchivodocs=open(sNombrearchivo,O_RDONLY,00400);

         memset(sLeerarchivo, 0, BUFFERSOCK);
         iLecturadocs=read(fsArchivodocs,sLeerarchivo,25);
         write(iAtenderestesocket, sLeerarchivo, iLecturadocs);


         for (;iLecturadocs>0;){
            memset(sLeerarchivo, 0, BUFFERSOCK);
            iLecturadocs=read(fsArchivodocs,sLeerarchivo,25);
            write(iAtenderestesocket, sLeerarchivo, iLecturadocs);
         };
         close(fsArchivodocs);
       }
     }
   }
   else{


      if (!(strstr(sNombrearchivo,sPedidoestadisticas)==NULL)){
         Escribirlog("HTTP/1.0 200 OK \n",17,"",0);
         Escribirlog(sBuffercgi,33,"",0);

         Escribosock(iAtenderestesocket, "HTTP/1.0 200 OK \n");
         Escribosock(iAtenderestesocket, "Content-Type: text/html\n\n");


         Escribosock(iAtenderestesocket,"<HTML>\n<HEAD>\n<TITLE>ESTADISTICAS ADW</TITLE>\n</HEAD>\n<BODY>\n");
         Escribosock(iAtenderestesocket,"Estadisticas\n");
         sprintf(sPonestadistica,"Cantidad de hijos %d <BR>\n",tuEstadisticas.liCantidaddehijos-1);
         Escribosock(iAtenderestesocket,sPonestadistica);
         sprintf(sPonestadistica,"Tiempo medio procesos (en milisigundos) %6.0f <BR>\n",tuEstadisticas.fTiempoproceso/(tuEstadisticas.liCantidaddehijos-1));
         Escribosock(iAtenderestesocket,sPonestadistica);
         sprintf(sPonestadistica,"Salidads normales %d <BR>\n",tuEstadisticas.aiSalida[1]);
         Escribosock(iAtenderestesocket,sPonestadistica);
         sprintf(sPonestadistica,"Salidads conexion perdida %d <BR>\n",tuEstadisticas.aiSalida[2]);
         Escribosock(iAtenderestesocket,sPonestadistica);
         sprintf(sPonestadistica,"Salidads lectura nula %d <BR>\n",tuEstadisticas.aiSalida[3]);
         Escribosock(iAtenderestesocket,sPonestadistica);
         Escribosock(iAtenderestesocket,"</BODY>\n</HTML>\n");
      }
      else if (!(strstr(sNombrearchivo,sLlamadaconfig)==NULL)){

         sCambiarclave=NULL;
         sCambiarvalor=NULL;
         sCambiarvariable=NULL;

         iCambioconfig=1;
         if (!(sParametroscgi[1]==NULL)){
            sCambiarclave=Buscarencadena(sParametroscgi[1],"password=","&");
            sCambiarvalor=Buscarencadena(sParametroscgi[1],"valor=","&");
            sCambiarvariable=Buscarencadena(sParametroscgi[1],"variable=","\0");
         }

         if (!((sCambiarclave==NULL) || (sCambiarvalor==NULL) || (sCambiarvariable==NULL))){
            if (strncmp(sCambiarclave,sPasswordcambio,strlen(sPasswordcambio))==0){
               sPongovalor=malloc(1000);
               sprintf(sPongovalor,"%s=",sCambiarvalor);
               iCambioconfig=Cambiarconfiguracion(sPongovalor,sCambiarvariable);
               free(sPongovalor);
            }
         }

         if (iCambioconfig==0){
            Escribosock(iAtenderestesocket, "HTTP/1.0 200 OK \n");
            Escribosock(iAtenderestesocket, "Content-Type: text/html\n\n");
            Escribosock(iAtenderestesocket,"<HTML>\n<HEAD>\n<TITLE>CONFIGURACION ADW</TITLE>\n</HEAD>\n<BODY>\n");
            Escribosock(iAtenderestesocket, "Se realizo la modificacion.\n");
            Escribosock(iAtenderestesocket,"</BODY>\n</HTML>\n");
         }
         else{
            Escribosock(iAtenderestesocket, "HTTP/1.0 200 OK \n");
            Escribosock(iAtenderestesocket, "Content-Type: text/html\n\n");
            Escribosock(iAtenderestesocket,"<HTML>\n<HEAD>\n<TITLE>CONFIGURACION ADW</TITLE>\n</HEAD>\n<BODY>\n");
            Escribosock(iAtenderestesocket, "No se a podido hacer la modificacion.\n");
            Escribosock(iAtenderestesocket,"</BODY>\n</HTML>\n");
         }
      }
      else if ((!(strstr(sNombrearchivo,sLlamadaconfig1)==NULL)) && (strstr(sNombrearchivo,sLlamadaconfig1)==NULL)){
      }
      else if ((!(strstr(sNombrearchivo,sLlamadaconfig2)==NULL)) && (strstr(sNombrearchivo,sLlamadaconfig2)==NULL)){
      }
      else{
         Escribosock(iAtenderestesocket, "HTTP/1.1 404 Object Not Found\n\n");
         fsArchivodocs=open("404/404.html",O_RDONLY,00400);
         memset(sLeerarchivo, 0, BUFFERSOCK);
         iLecturadocs=read(fsArchivodocs,sLeerarchivo,25);
         write(iAtenderestesocket, sLeerarchivo, iLecturadocs);
         for (;iLecturadocs>0;){
            memset(sLeerarchivo, 0, BUFFERSOCK);
            iLecturadocs=read(fsArchivodocs,sLeerarchivo,25);
            write(iAtenderestesocket, sLeerarchivo, iLecturadocs);
         };
         close(fsArchivodocs);
         Escribirlog("HTTP/1.1 404 Object Not Found\n\n",33,"",0);
         Escribirlog(sBuffercgi,33,"",0);
         printf("No encontrado\n");
      }

    }
   }
   close(iAtenderestesocket);

   /*Escribe estadisticas*/
   ftime(&stTiempofinal);

   fTiempoproceso=((stTiempofinal.time-stTiempoinicio.time)*1000)+
                  (stTiempofinal.millitm-stTiempoinicio.millitm);
   fTiempolatencia=stTiempofinal.millitm;

    Escribeestadisticas(fTiempoproceso,fTiempolatencia,1);

   exit(0);
}


void Serverweb(int ipuertoserver,char *sUsarestaip){
   /*Crea al padre y espera por conexiones
   */

   int iSocket;
   fd_set pfsFileselector;
   int iNumerofds;
   int iFdconec;
   int iContador,iCuenta;
   int iMaxfdset;
   int iAccepta,*ipAcceptbusca;
   int iLecturadocs;
   int iCantidaddesockets;
   fd_set pfsSelecion;
   char sBufferdatos[BUFFERSOCK],sLeerarchivo[BUFFERSOCK],*sNombrearchivo,*sExtencionarchivo,*sHeader;
   FILE *fsArchivodocs;
   int iPrueba,iPidfork;
   int iArchivo500,iLecturadocs500,iCuanleepadre;
   char *sLeerarchivo500;


   sLeerarchivo500=malloc(BUFFERSOCK);
   iControlhijos=0;
   iNumerofds=0;
   iFdconec=-1;
   iContador=0;
   iCuenta=0;

   iCantidaddesockets=0;



   memset(&sBufferdatos, 0, BUFFERSOCK);
   FD_ZERO(&pfsFileselector);
   FD_SET(0, &pfsFileselector);




   printf("Inicializando servidor web ...\n");
   iSocket=CrearSocket(ipuertoserver,sUsarestaip);
   if (iSocket<0){
      Escribirlog("Servidor Web : error iniciando socket ",strlen("Servidor Web : error iniciando socket "),"Error",1);
      perror("Servidor Web : error iniciando socket ");
      close(iSocket);
      exit(1);
   }
   printf("Servidor listo 1.\n");


   FD_SET(iSocket, &pfsFileselector);


   iMaxfdset=iSocket+1;


   for (;iCuenta<1;){

      pfsSelecion = pfsFileselector;
      iNumerofds  = select(iMaxfdset, &pfsSelecion, NULL, NULL, NULL);



      /*Si error es igual a 4 es que lleyo una de los signals no bloqueantes*/
      if ((iNumerofds == -1) && (!(errno=4))){
         printf("Error : EBADF = %d\n",EBADF);
         printf("Error : EINTR = %d\n",EINTR);
         printf("Error : EINVAL = %d\n",EINVAL);
         printf("Error : ENOMEM = %d\n",ENOMEM);
         printf("Error padre. Error numero %d\n",errno);
         exit(1);
      }
      if ((iNumerofds == -1) && (errno=4)) continue; /*Leyo un signal bloqueante*/

      if (iNumerofds == 0) continue; /*No hay nada nuevo en el select, TIMEOUT (no se usa)*/

      iFdconec = -1;

      for (iContador = 0; iContador <= iMaxfdset; iContador++) /*Testea el socket que finalizo el select*/
         if (FD_ISSET(iContador, &pfsSelecion)) {
            iFdconec = iContador;
            break;
         }


      if (iFdconec == -1) { /*Problema*/
         perror("Servidor Web : error en el select. ");
         close(iSocket);
         exit(1);
      };

      if (iFdconec == 0) {                                      /*Lee el teclado*/
         read(0, sBufferdatos, sizeof(sBufferdatos));

         if (strncmp(sBufferdatos,"FIN",3)==0){
              printf("GRACIAS POR COMPARTIR... \n");
              printf("FIN DE LA TRANSMISI�N\n");
              close(iSocket);
              Escribirlog("Servidor Web : Salida normal",strlen("Servidor Web : Salida normal"),"Sistema",1);

              iCuenta=2;
              continue;

         };


      }


      if (iFdconec == iSocket) {

         iAccepta = accept(iSocket,NULL,NULL);
         if (iAccepta == -1) { /*Problema*/
            perror("Servidor Web : error en el accept. ");
/*            close(iSocket);
            exit(1);*/
         }
         /*Suma la cantidad de hijos antes que el fork */
         /*porque a veces el hijo termina mas rapido que si lo sumo luego de crear el fork*/

         if (iCantidadmaximadehijos>iControlhijos){
            iControlhijos=iControlhijos+1;
            tuEstadisticas.liCantidaddehijos++;
            iPidfork=fork();
            if (iPidfork==0){                      /*hijo*/
               close(iSocket);
               Atenderhttp(iAccepta);
            }
            /*Error en el fork, salgo del programa*/
            /*porque si sigo se cuelga el sistema*/
            if (iPidfork==-1){
               printf("Error al intentar crear proceso hijo\n");
               iControlhijos=iControlhijos-1;
/*               close(iSocket);
               exit(1);*/
            }
         }
         else{
            iCuanleepadre=read(iAccepta, sLeerarchivo500, BUFFERSOCK); /*lee el buffer para que el browser acepte la respuesta*/
            write(iAccepta, "HTTP/1.0 500 Internal Server Error\n\n",36);
            iArchivo500=open("404/500.html",O_RDONLY,00400);
            memset(sLeerarchivo500, 0, BUFFERSOCK);
            iLecturadocs500=read(iArchivo500,sLeerarchivo500,25);
            write(iAccepta, sLeerarchivo500, iLecturadocs500);
            for (;iLecturadocs500>0;){
               memset(sLeerarchivo500, 0, BUFFERSOCK);
               iLecturadocs500=read(iArchivo500,sLeerarchivo500,25);
               write(iAccepta, sLeerarchivo500, iLecturadocs500);
            };
            close(iArchivo500);
            Escribirlog("HTTP/1.1 500 Internal Server Error\n\n",36,"",0);
         }
         close(iAccepta);
      }
   }
   exit(0);
}




void Terminohijo(){
   /*Procesa la terminacion de un hijo
   */

   int iUltimohijoenterrado;
   char *sBuscavalor;

   iUltimohijoenterrado=1;
   for (;iUltimohijoenterrado> 0;){
      iUltimohijoenterrado=waitpid(-1,NULL,WNOHANG) ;
      if (iUltimohijoenterrado> 0){
        iControlhijos=iControlhijos-1;

        /*Lee estadisticas*/

        sBuscavalor=Buscarencadena(pvEstmemoriacomp,"Tiempo proceso: ",";");
        if (!(sBuscavalor==NULL)){
           tuEstadisticas.fTiempoproceso=tuEstadisticas.fTiempoproceso+(atof(sBuscavalor));
           free(sBuscavalor);
        }
        sBuscavalor=Buscarencadena(pvEstmemoriacomp,"Tipo salida: ",";");
        if (!(sBuscavalor==NULL)){
           if ((atoi(sBuscavalor)>0) || (atoi(sBuscavalor)<31)){
              tuEstadisticas.aiSalida[atoi(sBuscavalor)]=tuEstadisticas.aiSalida[atoi(sBuscavalor)]+1;
           }
           free(sBuscavalor);
        }
        Ssemaforo(iEstsemaforo); /*Libera semaforo*/
      }
   }
}








void Vsigalarmpadre(){
   /*Carga la configuracoin automatica
   */
   Cargaconfiguracion();
   alarm(10); /*para cargar el archivo de configuracion*/
}

int Conectar(char *sNombredelhost, unsigned short iNumeropuerto){
   /*Conectar con un socket
   */
   struct sockaddr_in saElsocket;
   struct hostent     *hpElsocket;
   int iSocket;

   if ((hpElsocket= gethostbyname(sNombredelhost)) == NULL) {
      return(-1);
   }

   memset(&saElsocket,0,sizeof(saElsocket));
   memcpy((char *)&saElsocket.sin_addr,hpElsocket->h_addr,hpElsocket->h_length);
   saElsocket.sin_family= hpElsocket->h_addrtype;
   saElsocket.sin_port= htons((u_short)iNumeropuerto);

   if ((iSocket= socket(hpElsocket->h_addrtype,SOCK_STREAM,0)) < 0)
      return(-1);
   if (connect(iSocket,(struct sockaddr *) &saElsocket,sizeof(saElsocket)) < 0) {
      close(iSocket);
      return(-1);
   }
   return(iSocket);
}



void Avisorepetidor(){
   /*Avisa al repetidor que esta online
   */

   int iSocketrepetidor;
   char *sMensajerepetidor;


   if ((iSocketrepetidor=Conectar(sRepetidor,atoi(sRepetidorpuerto)))<0){
      printf("No hay conexcion con el repetidor\n");
      return; /*no esta*/
   }

   printf("Conectado con %s:%d\n",sRepetidor,atoi(sRepetidorpuerto));

   sMensajerepetidor=malloc(1000);
   sprintf(sMensajerepetidor,"GET %s?password=%s&server=%s&puerto=%d%c%c%c%c",sLlamadaserver,sRepetidorpassword,sNombredelserver,iPuertoserver,13,10,13,10);
   Escribosock(iSocketrepetidor,sMensajerepetidor);

   free(sMensajerepetidor);
   close(iSocketrepetidor);
   return;


}


void Quitorepetidor(){
   /*Avisa al repetidor que esta online
   */

   int iSocketrepetidor;
   char *sMensajerepetidor;


   if ((iSocketrepetidor=Conectar(sRepetidor,atoi(sRepetidorpuerto)))<0){
      printf("No hay conexcion con el repetidor\n");
      return; /*no esta*/
   }

   printf("Quitado de %s:%d\n",sRepetidor,atoi(sRepetidorpuerto));

   sMensajerepetidor=malloc(1000);
   sprintf(sMensajerepetidor,"GET %s?password=%s&server=%s&puerto=%d%c%c%c%c",sQuitarserver,sRepetidorpassword,sNombredelserver,iPuertoserver,13,10,13,10);
   Escribosock(iSocketrepetidor,sMensajerepetidor);

   free(sMensajerepetidor);
   close(iSocketrepetidor);
   return;


}

void Terminoproceso(){
   /*efectua las rutinas de finalizacion
   */
   if (iPidpadre==getpid()){
      Quitorepetidor();
      printf("Me mori1 %d\n",getpid());
      Liberarcontrol();
      kill(0,SIGUSR1);
      exit(1);
   }
}

void Terminoexit(){
   /*efectua las rutinas de finalizacion
   */
   if (iPidpadre==getpid()){

      Quitorepetidor();
      printf("Me mori2 %d\n",getpid());
      Liberarcontrol();
      kill(0,SIGUSR1);
   }
}



int main(int iCantidadar,char *sArgu[]){
FILE *fArchivoconfiguracion;
char *sLinea;
char *sPuerto,*sMensaje;
int iPuertoescucha;




if (iCantidadar==1 || iCantidadar==3){
   sNombredelserver=malloc(1500);
   sNombrewww=malloc(1500);
   sDirectorioprincipal=malloc(1500);
   sDirectoriocgi=malloc(1500);
   sDirectoriorestringido=malloc(1500);
   sDirectoriorestringidocgi=malloc(1500);
   sPaginapordefecto=malloc(1500);
   sDirectoriodelprograma=malloc(1500);
   sPedidoestadisticas=malloc(1500);
   sEncabezadocgi1=malloc(1500);
   sEncabezadocgi2=malloc(1500);
   sPasswordcambio=malloc(1500);
   sRepetidorpassword=malloc(1500);
   sLlamadaconfig=malloc(1500);
   sLlamadaconfig1=malloc(1500);
   sLlamadaconfig2=malloc(1500);
   sLlamadaserver=malloc(1500);
   sQuitarserver=malloc(1500);
   sRepetidor=malloc(1500);
   sRepetidorpuerto=malloc(1500);

   /*carga de configuracion dinamica*/
   Cargaconfiguracion();
   //ev if params was send and overdrives conf
   if (iCantidadar==3){
      iPuertoserver=atoi(sArgu[1]);
      sNombredelserver=sArgu[2];
   }

   //if envvars are set overdrive
   char *envvar_LSTPORT = "LSTPORT";
   char *envvarLSTHOST = "LSTHOST";
   if(getenv(envvar_LSTPORT)){
        iPuertoserver=atoi(getenv(envvar_LSTPORT));
    }
   if(getenv(envvarLSTHOST)){
        sNombredelserver=getenv(envvarLSTHOST);
    }


   /*ip de identificacion del padre*/
   iPidpadre=getpid();

   /*Define las areas de memoria compartida y los semaforos*/

   iControlestadisticas=getpid();
   iEstsemaforo=Definirsemaforo(iControlestadisticas);
   Definirmemoriacompartida(&iEstmemoriacomp,&pvEstmemoriacomp,
                             iControlestadisticas,sizeof(char)*5000);

   memset(pvEstmemoriacomp, 0, sizeof(char)*5000);

   Ssemaforo(iEstsemaforo); /*Libera semaforo para el primer hijo*/

   signal(SIGPIPE,SIG_IGN);
   signal(SIGCHLD,Terminohijo);
   signal(SIGINT,Terminoproceso);
   signal(SIGQUIT,Terminoproceso);
   signal(SIGKILL,Terminoproceso);
   signal(SIGTERM,Terminoproceso);
   signal(SIGUSR1,SIG_IGN);
   signal(SIGALRM,Vsigalarmpadre);
   atexit(Terminoexit);





   iPuertoescucha=iPuertoserver;
   printf("Escuchando en puerto : %d\n",iPuertoescucha);
   printf("test");
   printf("Proceso numero : %d ",getpid());
  
   sMensaje=malloc(255);
   sprintf(sMensaje,"Escuchando en puerto : %d\n",iPuertoescucha);


   //get a segmentation fault, todo: fixit
  // Escribirlog(sMensaje,strlen(sMensaje),"Inicio servidor",1);

   alarm(10); /*para cargar el archivo de configuracion cada 30 segundos*/

   Avisorepetidor();


   Serverweb(iPuertoescucha,sNombredelserver);
   }
else{
    printf("Cantidad de parametros erronea.");
    exit(1);
    }
}
