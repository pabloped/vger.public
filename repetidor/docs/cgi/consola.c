#include <stdio.h>
#include <errno.h>
#include <stdlib.h>



/*parametros*/
char *sNombredelserver;
int iPuertoserver;
char *sDirectorioprincipal;
char *sDirectoriocgi;
char *sDirectoriorestringido;
char *sDirectoriorestringidocgi;
char *sPaginapordefecto;
char *sDirectoriodelprograma;
char *sPedidoestadisticas;
char *sPasswordcambio;
char *sLlamadaconfig;
char *sLlamadaconfig1;
char *sLlamadaconfig2;
char *sLlamadaconfig3;
char *sLlamadaconfig4;
char *sLlamadaconfig5;
char *sLlamadaconfig6;
char *sLlamadaserver;
char *sQuitarserver;


/*Control sistema*/
struct {unsigned short iPuerto;
        char *sIp;} stServerweb[100];


int iCantidadmaximadehijos;
int iEjecutandocgi;
int iTiempocgiespera;
int iPidcgiexe;
int iNumerovector;


/*Estadisticas*/
float fTiempoproceso,fTiempolatencia;



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


int Buscarconfiguracionchar(char **sVariableconfiguracion,char *sClavebusqueda){
   /*Busca una variable de configuracion en el archivo y devuelve su valor
   */

   FILE *fArchivoconfig;
   char *sLeerconf,*sVercodigo,*sVervalor,*sDevuelve1,*sDevuelve,*sTemporalconfig;
   int iLecturadocs,iEncontrovalor,iTerminovalor;

   fArchivoconfig=fopen("repetidor.cnf","r");
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


int Buscarconfiguracionint(int *sVariableconfiguracion,char *sClavebusqueda){
   /*Busca una variable de configuracion en el archivo y devuelve su valor
   */

   FILE *fArchivoconfig;
   char *sLeerconf,*sVercodigo,*sVervalor,*sDevuelve1,*sDevuelve,*sTemporalconfig;
   int iLecturadocs,iEncontrovalor,iTerminovalor;

   fArchivoconfig=fopen("repetidor.cnf","r");
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



void Cargaconfiguracion(){
   /*Carga la configuracion
   */
   FILE *fsArchivod;
   char *sLeerarchivocnf;
   int iLecturadocs,iPruvec,iMaximoservers;

   iPuertoserver=9000;
   iTiempoespera=2;
   iTiempocgiespera=10;
   iCantidadmaximadehijos=50;
   sprintf(sNombredelserver,"127.0.0.1");
   sprintf(sDirectorioprincipal,"doc");
   sprintf(sDirectoriocgi,"doc/cgi/");
   sprintf(sDirectoriorestringido,"doc/usuarios/");
   sprintf(sDirectoriorestringidocgi,"doc/usuarioscgi/");
   sprintf(sPaginapordefecto,"index.html");
   sprintf(sDirectoriodelprograma,"/home/pablo/tpso/server/");
   sprintf(sPedidoestadisticas,"%s/estadistica.adw",sDirectorioprincipal);
   sprintf(sPasswordcambio,"Cambiar");
   sprintf(sLlamadaconfig,"cambiarlaconfiguracion");
   sprintf(sLlamadaconfig1,"postcambioformulario");
   sprintf(sLlamadaconfig2,"configurar.html");
   sprintf(sLlamadaconfig3,"cgi/confser");
   sprintf(sLlamadaconfig4,"usuarioscgi/sconfser");
   sprintf(sLlamadaconfig5,"usuarioscgi/psconfser");
   sprintf(sLlamadaconfig6,"estaserver");
   sprintf(sLlamadaserver,"agregarunservidornuevo");
   sprintf(sQuitarserver,"quitarservidor");

   Buscarconfiguracionint(&iPuertoserver,"Puertoserver=");
   Buscarconfiguracionint(&iTiempoespera,"Tiempoespera=");
   Buscarconfiguracionint(&iTiempocgiespera,"Tiempocgiespera=");
   Buscarconfiguracionint(&iCantidadmaximadehijos,"Cantidadmaximadehijos=");
   Buscarconfiguracionchar(&sNombredelserver,"Nombredelserver=");
   Buscarconfiguracionchar(&sDirectorioprincipal,"Directorioprincipal=");
   Buscarconfiguracionchar(&sDirectoriocgi,"Directoriocgi=");
   Buscarconfiguracionchar(&sDirectoriorestringido,"Directoriorestringido=");
   Buscarconfiguracionchar(&sDirectoriorestringidocgi,"Directoriorestringidocgi=");
   Buscarconfiguracionchar(&sPaginapordefecto,"Paginapordefecto=");
   Buscarconfiguracionchar(&sDirectoriodelprograma,"Directoriodelprograma=");
   Buscarconfiguracionchar(&sPedidoestadisticas,"Pedidoestadisticas=");
   Buscarconfiguracionchar(&sPasswordcambio,"Passwordcambio=");
   Buscarconfiguracionchar(&sLlamadaconfig,"Llamadaconfig=");
   Buscarconfiguracionchar(&sLlamadaconfig1,"Llamadaconfig1=");
   Buscarconfiguracionchar(&sLlamadaconfig2,"Llamadaconfig2=");
   Buscarconfiguracionchar(&sLlamadaconfig3,"Llamadaconfig3=");
   Buscarconfiguracionchar(&sLlamadaconfig4,"Llamadaconfig4=");
   Buscarconfiguracionchar(&sLlamadaconfig5,"Llamadaconfig5=");
   Buscarconfiguracionchar(&sLlamadaconfig6,"Llamadaconfig6=");
   Buscarconfiguracionchar(&sLlamadaserver,"Llamadaserver=");
   Buscarconfiguracionchar(&sQuitarserver,"Quitarserver=");



/* Servers */


   iNumerovector=0;

   fsArchivod=fopen("servers.cnf","r");

   if (fsArchivod==NULL){
      return;
   }

   sLeerarchivocnf=malloc(1000);
   memset(sLeerarchivocnf, 0, 1000);

   iLecturadocs=fscanf(fsArchivod,"%s\n",sLeerarchivocnf);



   iMaximoservers=1;
   for (;(iMaximoservers<99);){
      if (stServerweb[iNumerovector].sIp==NULL){
         stServerweb[iNumerovector].sIp=malloc(17);
      }
      if (iLecturadocs>0){
         memset(stServerweb[iNumerovector].sIp, 0, 17);
         strncpy(stServerweb[iNumerovector].sIp,sLeerarchivocnf,15);
         stServerweb[iNumerovector].iPuerto=atoi(sLeerarchivocnf+16);
      }
      else{
         memset(stServerweb[iNumerovector].sIp, 0, 17);
      }
      iNumerovector++;
      iMaximoservers++;

      memset(sLeerarchivocnf, 0, 21);
      iLecturadocs=fscanf(fsArchivod,"%s\n",sLeerarchivocnf);
   };
   fclose(fsArchivod);


   free(sLeerarchivocnf);


}



int main(int iCantidadar,char *sArgu[]){
char *sPongovalor;
int iQuevector;


   sNombredelserver=malloc(1500);
   sDirectorioprincipal=malloc(1500);
   sDirectoriocgi=malloc(1500);
   sDirectoriorestringido=malloc(1500);
   sDirectoriorestringidocgi=malloc(1500);
   sPaginapordefecto=malloc(1500);
   sDirectoriodelprograma=malloc(1500);
   sPedidoestadisticas=malloc(1500);
   sPasswordcambio=malloc(1500);
   sLlamadaconfig=malloc(1500);
   sLlamadaconfig1=malloc(1500);
   sLlamadaconfig2=malloc(1500);
   sLlamadaconfig3=malloc(1500);
   sLlamadaconfig4=malloc(1500);
   sLlamadaconfig5=malloc(1500);
   sLlamadaconfig6=malloc(1500);
   sLlamadaserver=malloc(1500);
   sQuitarserver=malloc(1500);

   Cargaconfiguracion();

   sPongovalor=malloc(1000);
   printf( "Content-Type: text/html\n\n");
   printf("<html>
<head>

<title>Form de prueba de cgi</title>


</head>


<body>
<table width=\"600\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">
  <tr>
   <td>
      <p align=\"center\"><b><font face=\"Verdana, Arial, Helvetica, sans-serif\" size=\"10\" color=\"#FFFF00\">
      Panel de control.</font></b></p>
   </td>
  </tr>
  <tr bgcolor=\"#FFFF00\">
    <td height=\"1\">
    </td>
  </tr>
  <tr>
   <td>
      <p align=\"left\"><b><font face=\"Verdana, Arial, Helvetica, sans-serif\" size=\"5\" color=\"#FFFF00\">
      Repetidor.</font></b></p>
      <p align=\"left\"><b><a href=\"");
printf("%s",sPedidoestadisticas);
printf("\"><font face=\"Verdana, Arial, Helvetica, sans-serif\" size=\"3\" color=\"#FFFF00\">
      Estadisticas.</font></a></b></p>
      <p align=\"left\"><b><a href=\"");
printf("%s",sLlamadaconfig2);
printf("\"><font face=\"Verdana, Arial, Helvetica, sans-serif\" size=\"3\" color=\"#FFFF00\">
      Configuracion.</font></a></b></p>
    </td>
  </tr>
  <tr>
    <td height=\"10\"></td>
  </tr>
  <tr bgcolor=\"#FFFF00\">
    <td height=\"1\">
    </td>
  </tr>


  <tr>
   <td>
<form method=\"POST\" action=\"");
printf("%s",sLlamadaconfig6);
printf("\">
      <p align=\"left\"><b><font face=\"Verdana, Arial, Helvetica, sans-serif\" size=\"5\" color=\"#FFFF00\">
      Servers.</font></b>      </p>
      <p align=\"left\"><b><font face=\"Verdana, Arial, Helvetica, sans-serif\" size=\"3\" color=\"#FFFF00\">
      Estadisticas.</font></b>
            <SELECT NAME=Nombredelserver SIZE=1>");

      for (iQuevector=0;iQuevector<iNumerovector;iQuevector++){
         if (!((stServerweb[iQuevector].sIp)==NULL)){
            if (!(strlen(stServerweb[iQuevector].sIp)==0)){
               printf("<OPTION VALUE=%s>%s</OPTION>\n",stServerweb[iQuevector].sIp,stServerweb[iQuevector].sIp);
            }
         }
      };


printf("                        </SELECT>
            <SELECT NAME=Puertoserver SIZE=1>");
      for (iQuevector=0;iQuevector<iNumerovector;iQuevector++){
         if (!((stServerweb[iQuevector].sIp)==NULL)){
            if (!(strlen(stServerweb[iQuevector].sIp)==0)){
               printf("<OPTION VALUE=%d>%d</OPTION>\n",stServerweb[iQuevector].iPuerto,stServerweb[iQuevector].iPuerto);
            }
         }
      };



printf("                             </SELECT>
             <input type=\"submit\" name=\"Submit\" value=\"Enviar\">

      </p>
</form>
<form method=\"POST\" action=\"");
printf("%s",sLlamadaconfig4);
printf("\">
      <p align=\"left\"><b><font face=\"Verdana, Arial, Helvetica, sans-serif\" size=\"3\" color=\"#FFFF00\">
      Configuracion.</font></b>
            <SELECT NAME=Nombredelserver SIZE=1>");

      for (iQuevector=0;iQuevector<iNumerovector;iQuevector++){
         if (!((stServerweb[iQuevector].sIp)==NULL)){
            if (!(strlen(stServerweb[iQuevector].sIp)==0)){
               printf("<OPTION VALUE=%s>%s</OPTION>\n",stServerweb[iQuevector].sIp,stServerweb[iQuevector].sIp);
            }
         }
      };



printf("                       </SELECT>
            <SELECT NAME=Puertoserver SIZE=1>");

      for (iQuevector=0;iQuevector<iNumerovector;iQuevector++){
         if (!((stServerweb[iQuevector].sIp)==NULL)){
            if (!(strlen(stServerweb[iQuevector].sIp)==0)){
               printf("<OPTION VALUE=%d>%d</OPTION>\n",stServerweb[iQuevector].iPuerto,stServerweb[iQuevector].iPuerto);
            }
         }
      };

printf("                        </SELECT>
             <input type=\"submit\" name=\"Submit\" value=\"Enviar\">
     </p>
</form>
    </td>
  </tr>
  <tr>
    <td height=\"10\"></td>
  </tr>
  <tr bgcolor=\"#FFFF00\">
    <td height=\"1\">
    </td>
  </tr>
</table>
</body>
</html>\n");


exit(0);

}
