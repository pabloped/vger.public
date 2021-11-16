#include <stdio.h>
#include <errno.h>
#include <stdlib.h>


int main(int iCantidadar,char *sArgu[]){
char *sPalabra1,*sPalabra2;
double iCoco;

printf("antescgi\n");
for (iCoco=1;iCoco<10000000000;iCoco++){};
printf("despuescgi\n");

sPalabra2=malloc(100);

memset(sPalabra2,0,100);

strcpy(sPalabra2,"coco");
sPalabra1="gogogogo";
strcpy(sPalabra1,sPalabra2);

}
