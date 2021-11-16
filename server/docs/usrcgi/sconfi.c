#include <stdio.h>
#include <errno.h>
#include <stdlib.h>



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
int iEjecutandocgi;
int iTiempocgiespera;
int iPidcgiexe;


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


int main(int iCantidadar,char *sArgu[]){
char *sPongovalor;

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

   Cargaconfiguracion();

   sPongovalor=malloc(1000);
   printf( "Content-Type: text/html\n\n");
   printf("<html>\n<head>\n<title>Formulario de configuracion</title>\n</head>\n");
   printf("<body bgcolor=\"#000000\" leftmargin=\"0\" topmargin=\"5\" marginwidth=\"0\" marginheight=\"5\">\n");
   printf("<table width=\"600\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n<tr><center>\n<td>\n");
   printf("<b><font face=\"Verdana, Arial, Helvetica, sans-serif\" size=\"10\" color=\"#FFFF00\">\n");
   printf("Formulario de configuracion.</font></b>\n</td></center>\n</tr>\n</table>\n");
   printf("<form method=\"POST\" action=\"");
   printf(sLlamadaconfig1);
   printf("\">");
   printf("<table width=\"300\" border=\"1\" cellspacing=\"2\" cellpadding=\"4\" align=\"center\">");

   /*iPuertoserver*/
   printf("<tr>\n<td bgcolor=\"#FFCC99\" width=\"150\">\n<div align=\"right\"><b><font size=\"2\"\n face=\"Verdana, Arial, Helvetica, sans-serif\">");
   printf("Puertoserver</font></b></div>\n</td>\n<td bgcolor=\"#FFFFCC\" width=\"150\">");
   printf("<input type=\"text\" name=\"");
   printf("iPuertoserver");
   printf("\" size=\"100\" maxlength=\"200\"");
   printf(" value=\"");
   memset(sPongovalor,0,1000);
   sprintf(sPongovalor,"%d",iPuertoserver);
   printf(sPongovalor);
   printf("\">\n</td>\n</tr>");
   /*iTiempoespera*/
   printf("<tr>\n<td bgcolor=\"#FFCC99\" width=\"150\">\n<div align=\"right\"><b><font size=\"2\"\n face=\"Verdana, Arial, Helvetica, sans-serif\">");
   printf("Tiempoespera</font></b></div>\n</td>\n<td bgcolor=\"#FFFFCC\" width=\"150\">");
   printf("<input type=\"text\" name=\"");
   printf("iTiempoespera");
   printf("\" size=\"100\" maxlength=\"200\"");
   printf(" value=\"");
   memset(sPongovalor,0,1000);
   sprintf(sPongovalor,"%d",iTiempoespera);
   printf(sPongovalor);
   printf("\">\n</td>\n</tr>");
   /*iTiempocgiespera*/
   printf("<tr>\n<td bgcolor=\"#FFCC99\" width=\"150\">\n<div align=\"right\"><b><font size=\"2\"\n face=\"Verdana, Arial, Helvetica, sans-serif\">");
   printf("Tiempocgiespera</font></b></div>\n</td>\n<td bgcolor=\"#FFFFCC\" width=\"150\">");
   printf("<input type=\"text\" name=\"");
   printf("iTiempocgiespera");
   printf("\" size=\"100\" maxlength=\"200\"");
   printf(" value=\"");
   memset(sPongovalor,0,1000);
   sprintf(sPongovalor,"%d",iTiempocgiespera);
   printf(sPongovalor);
   printf("\">\n</td>\n</tr>");
   /*iCantidadmaximadehijos*/
   printf("<tr>\n<td bgcolor=\"#FFCC99\" width=\"150\">\n<div align=\"right\"><b><font size=\"2\"\n face=\"Verdana, Arial, Helvetica, sans-serif\">");
   printf("Cantidadmaximadehijos</font></b></div>\n</td>\n<td bgcolor=\"#FFFFCC\" width=\"150\">");
   printf("<input type=\"text\" name=\"");
   printf("iCantidadmaximadehijos");
   printf("\" size=\"100\" maxlength=\"200\"");
   printf(" value=\"");
   memset(sPongovalor,0,1000);
   sprintf(sPongovalor,"%d",iCantidadmaximadehijos);
   printf(sPongovalor);
   printf("\">\n</td>\n</tr>");


   /*sNombredelserver*/
   printf("<tr>\n<td bgcolor=\"#FFCC99\" width=\"150\">\n<div align=\"right\"><b><font size=\"2\"\n face=\"Verdana, Arial, Helvetica, sans-serif\">");
   printf("Nombredelserver</font></b></div>\n</td>\n<td bgcolor=\"#FFFFCC\" width=\"150\">");
   printf("<input type=\"text\" name=\"");
   printf("sNombredelserver");
   printf("\" size=\"100\" maxlength=\"200\"");
   printf(" value=\"");
   printf(sNombredelserver);
   printf("\">\n</td>\n</tr>");

   /*sNombrewww*/
   printf("<tr>\n<td bgcolor=\"#FFCC99\" width=\"150\">\n<div align=\"right\"><b><font size=\"2\"\n face=\"Verdana, Arial, Helvetica, sans-serif\">");
   printf("Nombrewww</font></b></div>\n</td>\n<td bgcolor=\"#FFFFCC\" width=\"150\">");
   printf("<input type=\"text\" name=\"");
   printf("sNombrewww");
   printf("\" size=\"100\" maxlength=\"200\"");
   printf(" value=\"");
   printf(sNombrewww);
   printf("\">\n</td>\n</tr>");


   /*sDirectorioprincipal*/
   printf("<tr>\n<td bgcolor=\"#FFCC99\" width=\"150\">\n<div align=\"right\"><b><font size=\"2\"\n face=\"Verdana, Arial, Helvetica, sans-serif\">");
   printf("Directorioprincipal</font></b></div>\n</td>\n<td bgcolor=\"#FFFFCC\" width=\"150\">");
   printf("<input type=\"text\" name=\"");
   printf("sDirectorioprincipal");
   printf("\" size=\"100\" maxlength=\"200\"");
   printf(" value=\"");
   printf(sDirectorioprincipal);
   printf("\">\n</td>\n</tr>");
   /*sDirectoriocgi*/
   printf("<tr>\n<td bgcolor=\"#FFCC99\" width=\"150\">\n<div align=\"right\"><b><font size=\"2\"\n face=\"Verdana, Arial, Helvetica, sans-serif\">");
   printf("Directoriocgi</font></b></div>\n</td>\n<td bgcolor=\"#FFFFCC\" width=\"150\">");
   printf("<input type=\"text\" name=\"");
   printf("sDirectoriocgi");
   printf("\" size=\"100\" maxlength=\"200\"");
   printf(" value=\"");
   printf(sDirectoriocgi);
   printf("\">\n</td>\n</tr>");
   /*sDirectoriorestringido*/
   printf("<tr>\n<td bgcolor=\"#FFCC99\" width=\"150\">\n<div align=\"right\"><b><font size=\"2\"\n face=\"Verdana, Arial, Helvetica, sans-serif\">");
   printf("Directoriorestringido</font></b></div>\n</td>\n<td bgcolor=\"#FFFFCC\" width=\"150\">");
   printf("<input type=\"text\" name=\"");
   printf("sDirectoriorestringido");
   printf("\" size=\"100\" maxlength=\"200\"");
   printf(" value=\"");
   printf(sDirectoriorestringido);
   printf("\">\n</td>\n</tr>");
   /*sDirectoriorestringidocgi*/
   printf("<tr>\n<td bgcolor=\"#FFCC99\" width=\"150\">\n<div align=\"right\"><b><font size=\"2\"\n face=\"Verdana, Arial, Helvetica, sans-serif\">");
   printf("Directoriorestringidocgi</font></b></div>\n</td>\n<td bgcolor=\"#FFFFCC\" width=\"150\">");
   printf("<input type=\"text\" name=\"");
   printf("sDirectoriorestringidocgi");
   printf("\" size=\"100\" maxlength=\"200\"");
   printf(" value=\"");
   printf(sDirectoriorestringidocgi);
   printf("\">\n</td>\n</tr>");
   /*sPaginapordefecto*/
   printf("<tr>\n<td bgcolor=\"#FFCC99\" width=\"150\">\n<div align=\"right\"><b><font size=\"2\"\n face=\"Verdana, Arial, Helvetica, sans-serif\">");
   printf("Paginapordefecto</font></b></div>\n</td>\n<td bgcolor=\"#FFFFCC\" width=\"150\">");
   printf("<input type=\"text\" name=\"");
   printf("sPaginapordefecto");
   printf("\" size=\"100\" maxlength=\"200\"");
   printf(" value=\"");
   printf(sPaginapordefecto);
   printf("\">\n</td>\n</tr>");
   /*sDirectoriodelprograma*/
   printf("<tr>\n<td bgcolor=\"#FFCC99\" width=\"150\">\n<div align=\"right\"><b><font size=\"2\"\n face=\"Verdana, Arial, Helvetica, sans-serif\">");
   printf("Directoriodelprograma</font></b></div>\n</td>\n<td bgcolor=\"#FFFFCC\" width=\"150\">");
   printf("<input type=\"text\" name=\"");
   printf("sDirectoriodelprograma");
   printf("\" size=\"100\" maxlength=\"200\"");
   printf(" value=\"");
   printf(sDirectoriodelprograma);
   printf("\">\n</td>\n</tr>");
   /*sPedidoestadisticas*/
   printf("<tr>\n<td bgcolor=\"#FFCC99\" width=\"150\">\n<div align=\"right\"><b><font size=\"2\"\n face=\"Verdana, Arial, Helvetica, sans-serif\">");
   printf("Pedidoestadisticas</font></b></div>\n</td>\n<td bgcolor=\"#FFFFCC\" width=\"150\">");
   printf("<input type=\"text\" name=\"");
   printf("sPedidoestadisticas");
   printf("\" size=\"100\" maxlength=\"200\"");
   printf(" value=\"");
   printf(sPedidoestadisticas);
   printf("\">\n</td>\n</tr>");
   /*sEncabezadocgi1*/
   printf("<tr>\n<td bgcolor=\"#FFCC99\" width=\"150\">\n<div align=\"right\"><b><font size=\"2\"\n face=\"Verdana, Arial, Helvetica, sans-serif\">");
   printf("Encabezadocgi1</font></b></div>\n</td>\n<td bgcolor=\"#FFFFCC\" width=\"150\">");
   printf("<input type=\"text\" name=\"");
   printf("sEncabezadocgi1");
   printf("\" size=\"100\" maxlength=\"200\"");
   printf(" value=\"");
   printf(sEncabezadocgi1);
   printf("\">\n</td>\n</tr>");
   /*sEncabezadocgi2*/
   printf("<tr>\n<td bgcolor=\"#FFCC99\" width=\"150\">\n<div align=\"right\"><b><font size=\"2\"\n face=\"Verdana, Arial, Helvetica, sans-serif\">");
   printf("Encabezadocgi2</font></b></div>\n</td>\n<td bgcolor=\"#FFFFCC\" width=\"150\">");
   printf("<input type=\"text\" name=\"");
   printf("sEncabezadocgi2");
   printf("\" size=\"100\" maxlength=\"200\"");
   printf(" value=\"");
   printf(sEncabezadocgi2);
   printf("\">\n</td>\n</tr>");
   /*sPasswordcambio*/
   printf("<tr>\n<td bgcolor=\"#FFCC99\" width=\"150\">\n<div align=\"right\"><b><font size=\"2\"\n face=\"Verdana, Arial, Helvetica, sans-serif\">");
   printf("Passwordcambio</font></b></div>\n</td>\n<td bgcolor=\"#FFFFCC\" width=\"150\">");
   printf("<input type=\"text\" name=\"");
   printf("sPasswordcambio");
   printf("\" size=\"100\" maxlength=\"200\"");
   printf(" value=\"");
   printf(sPasswordcambio);
   printf("\">\n</td>\n</tr>");
   /*sRepetidorpassword*/
   printf("<tr>\n<td bgcolor=\"#FFCC99\" width=\"150\">\n<div align=\"right\"><b><font size=\"2\"\n face=\"Verdana, Arial, Helvetica, sans-serif\">");
   printf("Repetidorpassword</font></b></div>\n</td>\n<td bgcolor=\"#FFFFCC\" width=\"150\">");
   printf("<input type=\"text\" name=\"");
   printf("sRepetidorpassword");
   printf("\" size=\"100\" maxlength=\"200\"");
   printf(" value=\"");
   printf(sRepetidorpassword);
   printf("\">\n</td>\n</tr>");
   /*Llamadaconfig*/
   printf("<tr>\n<td bgcolor=\"#FFCC99\" width=\"150\">\n<div align=\"right\"><b><font size=\"2\"\n face=\"Verdana, Arial, Helvetica, sans-serif\">");
   printf("Llamadaconfig</font></b></div>\n</td>\n<td bgcolor=\"#FFFFCC\" width=\"150\">");
   printf("<input type=\"text\" name=\"");
   printf("sLlamadaconfig");
   printf("\" size=\"100\" maxlength=\"200\"");
   printf(" value=\"");
   printf(sLlamadaconfig);
   printf("\">\n</td>\n</tr>");
   /*Llamadaconfig1*/
   printf("<tr>\n<td bgcolor=\"#FFCC99\" width=\"150\">\n<div align=\"right\"><b><font size=\"2\"\n face=\"Verdana, Arial, Helvetica, sans-serif\">");
   printf("Llamadaconfig1 (POST)</font></b></div>\n</td>\n<td bgcolor=\"#FFFFCC\" width=\"150\">");
   printf("<input type=\"text\" name=\"");
   printf("sLlamadaconfig1");
   printf("\" size=\"100\" maxlength=\"200\"");
   printf(" value=\"");
   printf(sLlamadaconfig1);
   printf("\">\n</td>\n</tr>");
   /*Llamadaconfig2*/
   printf("<tr>\n<td bgcolor=\"#FFCC99\" width=\"150\">\n<div align=\"right\"><b><font size=\"2\"\n face=\"Verdana, Arial, Helvetica, sans-serif\">");
   printf("Llamadaconfig2 (Pagina de configuracion)</font></b></div>\n</td>\n<td bgcolor=\"#FFFFCC\" width=\"150\">");
   printf("<input type=\"text\" name=\"");
   printf("sLlamadaconfig2");
   printf("\" size=\"100\" maxlength=\"200\"");
   printf(" value=\"");
   printf(sLlamadaconfig2);
   printf("\">\n</td>\n</tr>");
   /*sLlamadaserver*/
   printf("<tr>\n<td bgcolor=\"#FFCC99\" width=\"150\">\n<div align=\"right\"><b><font size=\"2\"\n face=\"Verdana, Arial, Helvetica, sans-serif\">");
   printf("Llamadaserver</font></b></div>\n</td>\n<td bgcolor=\"#FFFFCC\" width=\"150\">");
   printf("<input type=\"text\" name=\"");
   printf("sLlamadaserver");
   printf("\" size=\"100\" maxlength=\"200\"");
   printf(" value=\"");
   printf(sLlamadaserver);
   printf("\">\n</td>\n</tr>");
   /*sQuitarserver*/
   printf("<tr>\n<td bgcolor=\"#FFCC99\" width=\"150\">\n<div align=\"right\"><b><font size=\"2\"\n face=\"Verdana, Arial, Helvetica, sans-serif\">");
   printf("Quitarserver</font></b></div>\n</td>\n<td bgcolor=\"#FFFFCC\" width=\"150\">");
   printf("<input type=\"text\" name=\"");
   printf("sQuitarserver");
   printf("\" size=\"100\" maxlength=\"200\"");
   printf(" value=\"");
   printf(sQuitarserver);
   printf("\">\n</td>\n</tr>");
   /*sRepetidor*/
   printf("<tr>\n<td bgcolor=\"#FFCC99\" width=\"150\">\n<div align=\"right\"><b><font size=\"2\"\n face=\"Verdana, Arial, Helvetica, sans-serif\">");
   printf("Repetidor</font></b></div>\n</td>\n<td bgcolor=\"#FFFFCC\" width=\"150\">");
   printf("<input type=\"text\" name=\"");
   printf("sRepetidor");
   printf("\" size=\"100\" maxlength=\"200\"");
   printf(" value=\"");
   printf(sRepetidor);
   printf("\">\n</td>\n</tr>");
   /*sRepetidorpuerto*/
   printf("<tr>\n<td bgcolor=\"#FFCC99\" width=\"150\">\n<div align=\"right\"><b><font size=\"2\"\n face=\"Verdana, Arial, Helvetica, sans-serif\">");
   printf("Repetidorpuerto</font></b></div>\n</td>\n<td bgcolor=\"#FFFFCC\" width=\"150\">");
   printf("<input type=\"text\" name=\"");
   printf("sRepetidorpuerto");
   printf("\" size=\"100\" maxlength=\"200\"");
   printf(" value=\"");
   printf(sRepetidorpuerto);
   printf("\">\n</td>\n</tr>");


   printf("<tr>\n<td colspan=2 bgcolor=\"#FFCC99\" >\n<center><input type=\"submit\" name=\"Submit\" value=\"Enviar\">\n<input type=\"reset\" name=\"Submit2\" value=\"Borrar\">\n</center></td>\n</tr>\n");
   printf("</table>\n</form>\n<table width=\"610\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n<tr bgcolor=\"#000000\">");
   printf("<td height=\"1\"></td>\n</tr>\n<tr>\n<td height=\"5\"></td>\n</tr><tr bgcolor=\"#FFFF00\"><td height=\"1\">\n</td>\n</tr>\n</table>\n</body>\n</html>\n");
   exit(0);

}
