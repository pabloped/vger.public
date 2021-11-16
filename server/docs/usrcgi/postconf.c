#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#define BUFFERSOCK 9000


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



int main(int iCantidadar,char *sArgu[]){
char *sContenidopost,*sCambiarclave;
int iCambioconfig;


         iCambioconfig=0;
         sContenidopost=malloc(BUFFERSOCK);
         memset(sContenidopost,0,BUFFERSOCK);
         sCambiarclave=NULL;


         read(0,sContenidopost,BUFFERSOCK);
         printf("HTTP/1.0 200 OK \n");
         printf("Content-Type: text/html\n\n");

         sCambiarclave=Buscarencadena(sContenidopost,"iPuertoserver=","&");
         if (!((sCambiarclave==NULL))){
            iCambioconfig=Cambiarconfiguracion("Puertoserver=",sCambiarclave);
         }
         sCambiarclave=Buscarencadena(sContenidopost,"iTiempoespera=","&");
         if (!((sCambiarclave==NULL))){
            iCambioconfig=Cambiarconfiguracion("Tiempoespera=",sCambiarclave);
         }
         sCambiarclave=Buscarencadena(sContenidopost,"iTiempocgiespera=","&");
         if (!((sCambiarclave==NULL))){
            iCambioconfig=Cambiarconfiguracion("Tiempocgiespera=",sCambiarclave);
         }
         sCambiarclave=Buscarencadena(sContenidopost,"iCantidadmaximadehijos=","&");
         if (!((sCambiarclave==NULL))){
            iCambioconfig=Cambiarconfiguracion("Cantidadmaximadehijos=",sCambiarclave);
         }
         sCambiarclave=Buscarencadena(sContenidopost,"sNombredelserver=","&");
         if (!((sCambiarclave==NULL))){
            iCambioconfig=Cambiarconfiguracion("Nombredelserver=",Reemplazarcadena(sCambiarclave,"%2F","/"));
         }
         sCambiarclave=Buscarencadena(sContenidopost,"sNombrewww=","&");
         if (!((sCambiarclave==NULL))){
            iCambioconfig=Cambiarconfiguracion("Nombrewww=",Reemplazarcadena(sCambiarclave,"%2F","/"));
         }

         sCambiarclave=Buscarencadena(sContenidopost,"sDirectorioprincipal=","&");
         if (!((sCambiarclave==NULL))){
            iCambioconfig=Cambiarconfiguracion("Directorioprincipal=",Reemplazarcadena(sCambiarclave,"%2F","/"));
         }
         sCambiarclave=Buscarencadena(sContenidopost,"sDirectoriocgi=","&");
         if (!((sCambiarclave==NULL))){
            iCambioconfig=Cambiarconfiguracion("Directoriocgi=",Reemplazarcadena(sCambiarclave,"%2F","/"));
         }
         sCambiarclave=Buscarencadena(sContenidopost,"sDirectoriorestringido=","&");
         if (!((sCambiarclave==NULL))){
            iCambioconfig=Cambiarconfiguracion("Directoriorestringido=",Reemplazarcadena(sCambiarclave,"%2F","/"));
         }
         sCambiarclave=Buscarencadena(sContenidopost,"sDirectoriorestringidocgi=","&");
         if (!((sCambiarclave==NULL))){
            iCambioconfig=Cambiarconfiguracion("Directoriorestringidocgi=",Reemplazarcadena(sCambiarclave,"%2F","/"));
         }
         sCambiarclave=Buscarencadena(sContenidopost,"sPaginapordefecto=","&");
         if (!((sCambiarclave==NULL))){
            iCambioconfig=Cambiarconfiguracion("Paginapordefecto=",Reemplazarcadena(sCambiarclave,"%2F","/"));
         }
         sCambiarclave=Buscarencadena(sContenidopost,"sDirectoriodelprograma=","&");
         if (!((sCambiarclave==NULL))){
            iCambioconfig=Cambiarconfiguracion("Directoriodelprograma=",Reemplazarcadena(sCambiarclave,"%2F","/"));
         }
         sCambiarclave=Buscarencadena(sContenidopost,"sPedidoestadisticas=","&");
         if (!((sCambiarclave==NULL))){
            iCambioconfig=Cambiarconfiguracion("Pedidoestadisticas=",Reemplazarcadena(sCambiarclave,"%2F","/"));
         }
         sCambiarclave=Buscarencadena(sContenidopost,"sEncabezadocgi1=","&");
         if (!((sCambiarclave==NULL))){
            iCambioconfig=Cambiarconfiguracion("Encabezadocgi1=",Reemplazarcadena(sCambiarclave,"%2F","/"));
         }
         sCambiarclave=Buscarencadena(sContenidopost,"sEncabezadocgi2=","&");
         if (!((sCambiarclave==NULL))){
            iCambioconfig=Cambiarconfiguracion("Encabezadocgi2=",Reemplazarcadena(sCambiarclave,"%2F","/"));
         }


         sCambiarclave=Buscarencadena(sContenidopost,"sPasswordcambio=","&");
         if (!((sCambiarclave==NULL))){
            iCambioconfig=Cambiarconfiguracion("Passwordcambio=",sCambiarclave);
         }
         sCambiarclave=Buscarencadena(sContenidopost,"sRepetidorpassword=","&");
         if (!((sCambiarclave==NULL))){
            iCambioconfig=Cambiarconfiguracion("Repetidorpassword=",sCambiarclave);
         }
         sCambiarclave=Buscarencadena(sContenidopost,"sLlamadaconfig=","&");
         if (!((sCambiarclave==NULL))){
            iCambioconfig=Cambiarconfiguracion("Llamadaconfig=",Reemplazarcadena(sCambiarclave,"%2F","/"));
         }
         sCambiarclave=Buscarencadena(sContenidopost,"sLlamadaconfig1=","&");
         if (!((sCambiarclave==NULL))){
            iCambioconfig=Cambiarconfiguracion("Llamadaconfig1=",Reemplazarcadena(sCambiarclave,"%2F","/"));
         }
         sCambiarclave=Buscarencadena(sContenidopost,"sLlamadaconfig2=","&");
         if (!((sCambiarclave==NULL))){
            iCambioconfig=Cambiarconfiguracion("Llamadaconfig2=",Reemplazarcadena(sCambiarclave,"%2F","/"));
         }



         sCambiarclave=Buscarencadena(sContenidopost,"sLlamadaserver=","&");
         if (!((sCambiarclave==NULL))){
            iCambioconfig=Cambiarconfiguracion("Llamadaserver=",Reemplazarcadena(sCambiarclave,"%2F","/"));
         }
         sCambiarclave=Buscarencadena(sContenidopost,"sQuitarserver=","&");
         if (!((sCambiarclave==NULL))){
            iCambioconfig=Cambiarconfiguracion("Quitarserver=",Reemplazarcadena(sCambiarclave,"%2F","/"));
         }

         sCambiarclave=Buscarencadena(sContenidopost,"sRepetidor=","&");
         if (!((sCambiarclave==NULL))){
            iCambioconfig=Cambiarconfiguracion("Repetidor=",Reemplazarcadena(sCambiarclave,"%2F","/"));
         }

         sCambiarclave=Buscarencadena(sContenidopost,"sRepetidorpuerto=","&");
         if (!((sCambiarclave==NULL))){
            iCambioconfig=Cambiarconfiguracion("Repetidorpuerto=",sCambiarclave);
         }


         printf("<HTML>\n<HEAD>\n<TITLE>ADW</TITLE>\n</HEAD>\n<BODY>\n");
         printf("Se realizo la modificacion.\n");
         printf("</BODY>\n</HTML>\n");

}
