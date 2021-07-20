char DistribSid[]="%W% %G%";

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifndef linux
#include <_G_config.h>
#else
#define _G_uint32_t __uint32_t
#endif
#include <assert.h>
#if ! defined linux && ! #system(amigaos)
#include <ieeefp.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <strings.h>
#include "comdefs.h"
#ifdef FILES_MMAPPED
#if defined linux || defined __NetBSD__ || #system(amigaos)
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#else
#include <semaphore.h>
#endif
#endif
#include "types.h" 
#include "tcpapps.h"      /*     Tcplib     */
#include "brkdn_dist.h"   /*     Tcplib     */
#include "globals.h"

#ifndef ALEATORIO3
#define	LARGEST_INT	2147483647
#else
#define LARGEST_INT     2147483563
#endif

#define INVERSE_LARGEST_INT	((double)(1.0 / LARGEST_INT))
#define LARGE_N_CUTOFF	30

#ifdef FILES_MMAPPED
#if defined linux || defined __NetBSD__ || #system(amigaos)
struct sembuf opsem[2];
static int sem;
#else
static sem_t *reading, *exec;
#endif
#endif

#ifndef ALEATORIO3
long semilla;

/*-----------------------------------------------------------------------*/
void semilla_aleatorio(long seed)
{
  semilla = seed;
}
/*-----------------------------------------------------------------------*/

long aleatorio()
{
  long hi, lo, test;

#ifndef ALEATORIO2
#define aleatorio_A 48271
#define aleatorio_Q 44488
#define aleatorio_R 3399
#else
#define aleatorio_A 69621
#define aleatorio_Q 30845
#define aleatorio_R 23902
#endif

  hi = semilla / aleatorio_Q;
  lo = semilla % aleatorio_Q;
  test = aleatorio_A * lo - aleatorio_R * hi;
  semilla = (test > 0)? test : test + LARGEST_INT;
  return semilla;
}
#else
long seedOne, seedTwo;

static inline void aleatorio1()
{
  long k = seedOne / 53668;
  
  seedOne = 40014 * (seedOne-k * 53668) - k * 12211;
  if (seedOne < 0) {
    seedOne += 2147483563;
  }
}

static inline void aleatorio2()
{
  long k = seedTwo / 52774;
  seedTwo = 40692 * (seedTwo - k * 52774) - k * 3791;
  if (seedTwo < 0) {
    seedTwo += 2147483399;
  }
}

void semilla_aleatorio(long seed)
{
  unsigned i;
  seedTwo = seed;
  for (i=0; i<10; i++)
    aleatorio2();
  seedOne = seedTwo;
  seedTwo = seed;
}

long aleatorio()
{
  long z;
  aleatorio1();
  aleatorio2();
  z = seedOne - seedTwo;
  if (z < 1) {
    z += 2147483562;
  }
#ifdef INDEP_SEEDS
  if (one_minus_u)
    return LARGEST_INT - z;
  else
#endif
    return z;
}
#endif
/*-----------------------------------------------------------------------*/

static long uniforme_entera(int a, int b)
/* Genera un entero uniformemente distribuido en el intervalo [a,b]      */
{
  return( a + (aleatorio() % (b-a+1)) );
}
/*-----------------------------------------------------------------------*/

/*	returns a random number between 0 and 1	*/

union PrivateSingleType {		   	/* used to access floats as unsigneds */
    float s;
    _G_uint32_t u;
};

union PrivateDoubleType {		   	/* used to access doubles as unsigneds */
    double d;
    _G_uint32_t u[2];
};

static union PrivateSingleType singleMantissa;	/* mantissa bit vector */
static union PrivateDoubleType doubleMantissa;	/* mantissa bit vector */

void initialize(void)
{
  union PrivateDoubleType t;
  union PrivateSingleType s;


	/*	The following is a hack that I attribute to
	Andres Nowatzyk at CMU. The intent of the loop
	is to form the smallest number 0 <= x < 1.0,
	which is then used as a mask for two longwords.
	this gives us a fast way way to produce double
	precision numbers from longwords.

	I know that this works for IEEE and VAX floating
	point representations.

	A further complication is that gnu C will blow
	the following loop, unless compiled with -ffloat-store,
	because it uses extended representations for some of
	of the comparisons. Thus, we have the following hack.
	If we could specify #pragma optimize, we wouldn't need this.
	*/

#if _IEEE == 1
  assert (sizeof(double) == 2 * sizeof(_G_uint32_t)); 
	
  t.d = 1.5;
  if ( t.u[1] == 0 ) {		/* sun word order? */
    t.u[0] = 0x3fffffff;
    t.u[1] = 0xffffffff;
  }
  else {
    t.u[0] = 0xffffffff;	/* encore word order? */
    t.u[1] = 0x3fffffff;
  }

  s.u = 0x3fffffff;
#else
  volatile double x = 1.0; /* volatile needed when fp hardware used,
                              and has greater precision than memory doubles */
  double y = 0.5;

  volatile float xx = 1.0; /* volatile needed when fp hardware used,
                              and has greater precision than memory floats */
  float yy = 0.5;

  assert (sizeof(double) == 2 * sizeof(_G_uint32_t)); 

  do {			    /* find largest fp-number < 2.0 */
    t.d = x;
    x += y;
    y *= 0.5;
  } while (x != t.d && x < 2.0);

  do {			    /* find largest fp-number < 2.0 */
    s.s = xx;
    xx += yy;
    yy *= 0.5;
  } while (xx != s.s && xx < 2.0);
#endif
  /* set doubleMantissa to 1 for each doubleMantissa bit */
  doubleMantissa.d = 1.0;
  doubleMantissa.u[0] ^= t.u[0];
  doubleMantissa.u[1] ^= t.u[1];

  /* set singleMantissa to 1 for each singleMantissa bit */
  singleMantissa.s = 1.0;
  singleMantissa.u ^= s.u;
}

double fract_rand(void)
{
  static unsigned char initiate = 1;
  union PrivateDoubleType result;

  if (initiate)
    {
      initialize();
      initiate = 0;
    }

  result.d = 1.0;
#if defined(_BIG_ENDIAN) || #system(amigaos)
  result.u[1] |= (aleatorio() & doubleMantissa.u[1]);
  result.u[0] |= (aleatorio() & doubleMantissa.u[0]);
#else
  result.u[0] |= (aleatorio() & doubleMantissa.u[0]);
  result.u[1] |= (aleatorio() & doubleMantissa.u[1]);
#endif
  result.d -= 1.0;
  assert( result.d < 1.0 && result.d >= 0);
  return( result.d );
}

/*-----------------------------------------------------------------------*/
double uniform(float a, float b)
{
/*	returns a random number between a and b where a < b	*/
    double x;

    x = a + ((b - a) * fract_rand());
    return (x);
}
/*-----------------------------------------------------------------------*/
double normal(float mean, float std_dev)
{
/*	returns a random number normally distributed	*/

  double x;
  double t1;
  double t2;
  double t3;
  double t4;
  
  do
    {
      t1 = (2 * fract_rand()) - 1;
      t2 = (2 * fract_rand()) - 1;
      t3 = (t1 * t1) + (t2 * t2);
    } 
  while (t3 >= 1);
  t4 = t1 * sqrt((-2.0 * log(t3)) / t3);
  x = mean + (t4 * std_dev);
  return (x);
}
/*-----------------------------------------------------------------------*/

int poisson(float mean)
{
/*	returns a random number poisson distributed	*/

    int x;
    double a, s;

    a = exp(-mean);
    x = 0;
    s = fract_rand();

    while (s >= a)
      {
	x++;
	s = s * fract_rand();
      }
    return (x);
}
/*-----------------------------------------------------------------------*/

double expont(float mean)
{
/*	returns a random number exponentially distributed	*/

    double x;

    x = -mean * log(fract_rand());
    return (x);
}

/*-----------------------------------------------------------------------*/

int bernoulli(float prob1)
/* probability that 1 will be returned	 */
/* -- probability of success */
{

/*	returns 0 or 1 according to probability of returning 1
	which is supplied
*/

    if (aleatorio() <= (prob1 * LARGEST_INT))
	return (1);
    else
	return (0);
}
/*-------------------------------------------------------------------------*/

double Gamma(float shape, float scale, double *pa, double *pb)

/*   Devuelve un numero aleatorio con distribucion Gamma de parametros:
     -Media= shape*escale.
     -Varianza= shape*escale^2.
     La funcion de densidad de probabilidad es:
        f(x)=K * e^{-scale*x} * scale(scale*x)^{shape-1} 

        Si *pa=0 calcula automaticamente el valor de *pb. Si se vuelve a
     llamar a la funcion con los mismos valores de shape y scale, se 
     puede pasar directamente el valor de *pb como parametro (con *pa!=0)
     con lo que se calcula mas rapido el valor de la muestra.
        Si shape=k y scale=mu*k con mu numero real positivo obtenemos una 
     distribucion Erlang-k.

*/
{
    double x;
    double y, gy, x3, alpha, b, a;

    alpha = shape;
    a = *pa;
    b = *pb;
    if (a == 0)
    {
	x3 = 1.0;
	if (shape <= 1.5)
	    b = 0.24797 + (1.34735740 * alpha)
		- pow(1.00004204 * alpha, 2.0)
		+ pow(0.53203176 * alpha, 3.0)
		- pow(0.13671536 * alpha, 4.0)
		+ pow(0.01320864 * alpha, 5.0);
	else if (shape <= 19.0)
	    b = 0.64350 + (0.45839602 * alpha)
		- pow(0.02952801 * alpha, 2.0)
		+ pow(0.00172718 * alpha, 3.0)
		- pow(0.00005810 * alpha, 4.0)
		+ pow(0.00000082 * alpha, 5.0);
	else
	    b = 1.33408 + (0.22499991 * alpha)
		- pow(0.00230695 * alpha, 2.0)
		+ pow(0.00001623 * alpha, 3.0)
		- pow(0.00000006 * alpha, 4.0);
	y = 1.0 + (1.0 / b);
	while (y - 1.0 > 0)
	{
	    y = y - 1.0;
	    x3 = x3 * y;
	}
	if (y - 1.0 < 0)
	{
	    gy = 1.0 + y * (-0.5771017 + y * (0.985854 + y *
					    (-0.8764218 + y * (0.8328212 + y *
					    (-0.5684729 + y * (0.2548205 + y *
							  (-0.05149930)))))));
	    x3 = x3 * gy / y;
	}
	a = pow(x3 / (alpha * scale), b);
	b = 1.0 / b;
	a = 1.0 / a;
	*pa = a;
	*pb = b;
    }
    x = pow(-a * log(fract_rand()), b);
    return (x);
}
/*-----------------------------------------------------------------------*/

static long geometrica(float r)
     
/*      Genera una muestra de una distribucion geometrica de parametro r.
	f(k)=(1-r)pow(r,k-1).                                           */
{
  long p;
  
  p=1;
  do
    {
      if (fract_rand() <= 1-r) return(p);
      else p++;
    }
  while (1);
}
/*---------------------------------------------------------------------*/

double gruber82 (float r, float r1, float c1, float r2, float c2,
		float T, int *silencio)
     /* float r: Parametro distribucion geometrica del talkspurt*/
     /* float r1,c1,r2,c2: Parametros de la duracion del silencio        */
     /* float T: Intervalo interpaquete (Recom. T=22.5 ms)      */
     /* int *silencio: Vale 0 en un talkspurt y 1 en silencio.        */

/* Genera el tiempo entre llegadas en ms para una fuente de voz on-off
   segun el modelo de Gruber:
   -Talkspurt: geometric f(k)=(1-r)pow(r,k-1).
   -Silence:two-weighted geometric 
        f(k)=c1(1-r1)pow(r1,k-1)+c2(1-r2)pow(r2,k-1)
   Parametros recomendados:
        r=0.8674 c1=0.8746 c2=0.1469 r1=0.5372 r2=0.9695 
*/
{
  if (*silencio == 0)
    {
      if (fract_rand() <= 1-r) *silencio = 1;   /*Fin del talkspurt*/
      return(T);
    }
  else
    {
      *silencio = 0;
      if ( fract_rand() <= c1) return(T*geometrica(r1));
      else return(T*geometrica(r2));
    }
}

/*----------------------------------------------------------------------*/

double sriram86(float p , float T , float media_silencio)
     /* float p: Probabilidad de que el siguiente paquete sea talkspurt*/
     /* float T: Intervalo interpaquete*/
     /* float media_silencio: Duracion media del silencio*/

/* Genera el tiempo entre llegadas en ms para una fuente de voz on-off
   segun el modelo de Sriram:
   -El numero de paquetes en el talkspurt distribuido geometricamente.
   -La duracion del silencio distribuida exponencialmente.
   -Parametros recomendados:p=21/22 T=16ms media_silencio=666ms
*/
{
  if (fract_rand() <= p) return(T);
  else return(expont(media_silencio));
}
/*----------------------------------------------------------------------*/

double yegenoglu92 (int *p_estado, double *muestra1, double *muestra2,
		   double *muestra3, double a1, double a2, double a3,
		   double m1, double m2, double m3, double d1,
		   double d2, double d3)
     /* int *p_estado: Estado actual de la cadena de Markov.        */
     /* double *muestra1,*muestra2,*muestra3:
                    Muestras anteriores para cada estado de la cadena. */
     /* double a1,a2,a3: Parametros proceso AR.                       */
     /* double m1,m2,m3: Medias distribucion normal para estados 1,2,3*/
     /* double d1,d2,d3: Desviaciones tipicas.                        */

/* Modelo AR del numero de bits para cada trama de una senhal de video VBR.
   Cadena de Markov de tres estados. Cada uno de los estados es un proceso
   autorregresivo de primer orden.
   Parametros recomendados:
        a1=0.55,     a2=0.79,     a3=0.83
	m1=16936.0,  m2=10476.0,  m3=12386.0
	d1=2008.0,   d2=1518.0,   d3=7466.0

*/
{
  double muestra_siguiente=0;
  /*Matriz de probabilidades de transicion de la cadena de Markov*/
  float p[3][3]={ {0.883721, 0.093023, 0.023256},
		    {0.162963, 0.629620, 0.207407},
		    {0.017708, 0.086458, 0.895833} };
  double u;

  switch (*p_estado)
    {
    case 0: *muestra1 = a1 * (*muestra1) + normal(m1,d1) ;
      muestra_siguiente = *muestra1;
      break;
    case 1: *muestra2 = a2 * (*muestra2) + normal(m2,d2) ;
      muestra_siguiente = *muestra2;
      break;
    case 2: *muestra3 = a3 * (*muestra3) + normal(m3,d3);
      muestra_siguiente = *muestra3;
      break;
    default: fprintf(stderr,"Estado de la cadena de Markov erroneo\n");
      exit(1);
    }
 
  u=fract_rand();                /*Calculo del estado siguiente*/
  if (u<=p[*p_estado][0]) *p_estado=0;
  else  if (u <= (p[*p_estado][0]+p[*p_estado][1]) ) *p_estado=1;
        else *p_estado=2;

  return(muestra_siguiente/1);
}
/*---------------------------------------------------------------------*/

double brady69 (float media_silencio, float media_spurt, float T,
	       double *paquetes, int *silencio, int *primer_paquete)

     /* float media_silencio: Duracion media de un silencio.         */
     /* float media_spurt: Duracion media de un talkspurt.        */
     /* float T: Intervalo interpaquete (Recom. T=5 ms) */
     /* double *paquetes */
     /* int *silencio: Vale 0 en un talkspurt y 1 en sil.     */
     /* int *primer_paquete */

/* Genera el tiempo entre llegadas en ms para una fuente de voz on-off
   segun el modelo de Brady:
   -Talkspurt: exponencial
   -Silence:exponencial 
       
   Parametros recomendados:
   media_silencio=1846ms media_talkspurt=1197ms
   El parametro media_silencio ha de ser mayor de 200 ms.
   En la primera llamada a la funcion *primer_paquete=1.
*/
{
  if (*silencio == 0) /*Talkspurt*/
    {
      if (*primer_paquete == 1)
	{
	  *paquetes = ceil(expont(media_spurt)/5.0 );
	  *primer_paquete = 0;
	}
      if (*paquetes == 1.0) *silencio = 1;   /*Fin del talkspurt*/
      else *paquetes = *paquetes - 1.0;
      return(T);
    }
  else                 /*Silencio */
    {
      *silencio = 0;
      *primer_paquete = 1;
      return( ceil( expont(media_silencio-200.0)/5.0)*5.0 + 200.0 );
    }
}
/*---------------------------------------------------------------------*/

static float m_ant=0, desv_ant=0;
static double m_N=0, d_N=0;

double lognormal(float media, float desv)

     /* float media: Media de la distribucion lognormal*/
     /* float desv: Desviacion tipica*/
{
  double m4, x, m, d;

  if (media != m_ant || desv != desv_ant)
    {
      m_ant = m = media;
      desv_ant = d = desv;
      m *=m;
      m4 = m*m;
      d *= d;
      m_N=log(m4/(m+d))/2;
      d_N=sqrt(log((m+d)/m));
    }
  x=normal(m_N,d_N);
  return(exp(x));
}
/*---------------------------------------------------------------------*/

double krunz95(float mI, float dI, float mP, float dP, float mB,
	      float dB, int *tramas, int *tramasB)
    
     /* float mI,dI: Media y desv. tipica del tamanho de tramas I      */
     /* float mP,dP: Media y desv. tipica del tamanho de tramas P      */
     /* float mB,dB: Media y desv. tipica del tamanho de tramas B      */
     /* int *tramas: Estado que representa el siguiente tipo de trama  */
     /* int *tramasB: que ha de ser generado.                           */   

/*   Genera el numero de bits para cada trama de una senhal de video VBR.
    Se consideran tres tipos de trama: I,P,B (intraframe,predictive,bidirec-
    cional). El numero de bits por trama sigue una distribucion lognormal.
    El esquema de compresion es: I B B P B B P B B P B B P B B.
     Los parametros *tramas y *tramasB representan el estado que determina 
    el siguiente tipo de trama a ser generado. En la primera llamada a la 
    funcion *tramas=0 y *tramasB=0.
*/
{
  double m,d;
 
  if (*tramas == 0)
    {
      /*printf("I\n");*/
      *tramas = 0;
      *tramasB = 0;
      m=mI;
      d=dI;
    }
  else 
    {
      (*tramasB)++;
      if (*tramasB < 3)
	{
	  /*printf("B\n");*/
	  m=mB;
	  d=dB;
	}
      else 
	{
	  *tramasB = 0;
	  /*printf("P\n");*/
	  m=mP;
	  d=dP;
	}
    }
  (*tramas)++;
  if (*tramas == 15) *tramas=0;
  return( rint(lognormal(m,d)) );
}
/*---------------------------------------------------------------------*/

double torbey91(float T, float media_off, float p1, float p2,
	       int uniforme, double *numero, int *on, int *primera_vez)

     /* float T: Tiempo entre paquetes dentro de una imagen*/
     /* float media_off: Media de la distribucion inter-imagenes   */
     /* float p1,p2: Parametros distribucion tamanho imagen    */
     /* int uniforme: Si =1 tam.imagen U(p1,p2) si =0 N(p1,p2)  */
     /* double *numero: Numero de celdas a transmitir en la imagen*/
     /* int *on: Es 1 en el estado ON y 0 en OFF.          */
     /* int *primera_vez */
  

/* Modela la transmision de imagenes fijas mediante un modelo ON-OFF.
   El estado ON representa la transmision de una imagen. El estado OFF 
   representa el periodo entre imagenes.
   Retorna el tiempo entre llegadas de celdas ATM (53 bytes).
   El tamanho de la imagen sin comprimir puede ser modelado:
   1)Mediante una distribucion uniforme.
   2)Mediante una distribucion normal.
   La compresion de la imagen es simulada mediante una distribucion gamma.
   (compresion DPCM jerarquico multires. sin perdidas).
   La duracion del estado OFF es modelada mediante una distribucion exponencial
   La primera vez que se llame a la funcion:
        *on=1   *primera_vez=1
   Los parametros recomendados son: T=50ms
        distr. uniforme U(p1,p2) p1=0.0  P2=0.526e6
	distr. normal N(p1,p2)   p1=100000.0 p2=3162.27766
*/
{
  double tamanho;
  double a = 0.0 , b;

  if (*on == 1)
    {
      if (*primera_vez)
	{
	  *primera_vez = 0;
	  tamanho = (uniforme) ? uniform(p1,p2) 
	                       : normal(p1,p2);
	  tamanho = tamanho * Gamma(3.699061,0.177883,&a,&b);
	  *numero = ceil(tamanho);
	}
      *numero = *numero - 1;
      if (*numero == 0.0) *on=0;
      return(T);
    }
  else 
    {
      *on = 1;
      *primera_vez = 1;
      return(expont(media_off));
    }
}
/*--------------------------------------------------------------------*/

/* static void Parametros_Gamma (float media, float desv, float *p1, float *p2) */
/* Realiza la conversion de parametros para llamar a la funcion Gamma */
/* o a la funcion riyaz95                                             */
/* { */
/*   (*p2) = desv * desv / media; */
/*   (*p1) = media / (*p2); */
/*   return; */
/* } */

/*---------------------------------------------------------------------*/
double riyaz95(float I1, float I2, float P1, float P2, float B1,
	      float B2, float p, double *cambio_escena, double *GOPde10,
	      int *tramas, int *tramasB, int *fin)

     /* float I1,I2: Parametros del tamanho de tramas I.               */
     /* float P1,P2: Parametros del tamanho de tramas P.               */
     /* float B1,B2: Parametros del tamanho de tramas B.               */
     /* float p: Probabilidad de cambio de escena.                 */
     /* double *cambio_escena: Booleana.Valor inicial 0.                */
     /* double *GOPde10: Booleana.Valor inicial 1.                */
     /* int *tramas: Representan el siguiente tipo de trama.           */
     /* int *tramasB: que ha de ser generado.
                   Valores iniciales *tramas=0 *tramasB=0.           */
     /* int *fin:  En la primera llamada fin=10.                    */

/*  Genera el numero de bits para cada trama de una senhal de video VBR 
    de tipo MPEG, expresado en KBITS/trama.
    Un solo nivel de prioridad.
    Se consideran tres tipos de trama: I,P,B .
    El numero de bits por trama sigue una distribucion Gamma.
    El esquema de compresion es: I B B P B B P B B P B B.
    Se produce un cambio de escena con probabilidad p_sc. La localizacion
    del cambio de escena dentro del GOP se determina mediante una variable
    aleatoria uniforme en el intervalo [2,12] o [2,10].
    Tras un cambio de escena el esquema de compresion es: I P B B P B B P B B.
    Parametros recomendados:
      I1=24.7999 I2=8.3333 P1=0.3235 P2=298.8493 B1=0.1899 B2=122.8575
      p=0.2
*/
{
  double a=0.0,b;
  double par1,par2;
   
  if (*tramas == 0)   /*trama I*/
    {
      if ( fract_rand() < p )
	{
	  /*printf("Cambio de escena\n");*/
	  if (*GOPde10==1.0) *fin = uniforme_entera(2,10);
	  else *fin = uniforme_entera(2,10);
	  /*printf("fin=%i\n",*fin);*/
	  *cambio_escena = 1.0;
	}
      /*printf("trama I\n");*/
      *tramas = 0;
      *tramasB = 0;
      par1=I1;
      par2=I2;
    }
  else 
    {
      (*tramasB)++;
      if  ( ((*GOPde10)==1.0)&&(*tramas==1) )  /*trama P*/
	{
	  *tramasB = 0;
	  if (*fin==12) *fin=10;
	  (*GOPde10) = 0.0;
	  /*printf("trama P\n");*/
	  par1=P1;
	  par2=P2;
	}
      else
	{ 
	  if (*tramasB < 3)     /*trama B*/
	    {
	     /* printf("trama B\n");*/
	      par1=B1;
	      par2=B2;
	    }
	  else                  /*trama P*/
   {
	      /* printf("trama P\n");*/
	      *tramasB = 0;
	      par1=P1;
	      par2=P2;
	    }
	}
    }
  (*tramas)++;
  if (*tramas==*fin) 
    {
      *fin=12;
      if ( *cambio_escena==1.0 ) (*GOPde10) = 1.0;
      *cambio_escena=0.0;
      *tramas=0;
    }
  return( ceil( Gamma(par1,par2,&a,&b) ) );
}
/*---------------------------------------------------------------------*/

void prioridades_riyaz95(int PBP, double muestra, double *alta,
			 double *baja)

/* A partir de una longitud de trama obtenida segun el modelo de Riyaz 
   obtiene dos logitudes de trama de prioridades alta y baja en funcion
   del valor del PBP.                                                  */
{
  switch (PBP)
    {
    case 12: *alta = ceil( 0.719 * muestra);
      break;
    case 16: *alta = ceil( 0.805 * muestra);
      break;
    case 20: *alta = ceil( 0.8625 * muestra);
      break;
    case 24: *alta = ceil( 0.899 * muestra);
      break;
    default: printf("Valor de PBP incorrecto.\n");
    }
  *baja = muestra - *alta;

  return;
}

/*---------------------------------------------------------------------------*/
double Pareto (float m, float k)
	  /* float k:  Valor minimo permitido para la distribucion */
	  /* float m:  Pendiente de la cola en la grafica de la funcion
                  de distribucion log-log */
 
/* Genera una muestra de una distribucion Pareto, tambien
  denominada doble-exponencial o hiperbolica. Tiene la
  propiedad de ser heavy-tailed.  La muestra se
  genera mediante el metodo de la transformada inversa.
  La funcion de distribucion es:
            F(x) = 1 - (k/x)^m
  Y la de densidad
            f(x) = m*k^m / x^(m+1)
 
  Si m<=1 la distribucion tiene media infinita 
  Si m>1 media = m*k / m - 1
  Si m<=2 entonces la distribucion tiene varianza infinita.
  Si m>2 varianza = (m*k^2 / m-2) - media^2                  */
 
{
  static float m_ant=0;
  static double inv_m;
  double u;
 
  if (m_ant != m)
    {
      m_ant = m;
      inv_m = -1/m;
    }
  u = fract_rand();
  u = k * pow(u,inv_m);
  return u;
}

/*---------------------------------------------------------------------------*/
double Weillbull (float a , float b)

/*   Genera una muestra de una distribucion Weillbull. La muestra se 
     genera mediante el metodo de la transformada inversa.
     La funcion de densidad de probabilidad es:
                     f(x) = 1 - e^[(-x^a)/b]                                 */
{
  double x;
  double u;

  u=fract_rand();
  x=pow(-b*log(u),1/a);
  return(x);
}
/*-----------------------------------------------------------------------*/

#ifdef TCPLIB
double tam_pkt_smtp()

/* Retorna el tamanho de paquete para trafico SMTP. 
   Llama a la funcion smtp_itemsize() de la libreria Tcplib. 
   El producto por 8 es porque smtp_itemsize retorna bytes*/
{
  return(8.0 * smtp_itemsize());
}
/*-----------------------------------------------------------------------*/

double tiempo_telnet_1 (float media_inicio, double *tiempo_siguiente,
		       double *duracion, int *primer_byte)
     /* float media_inicio: tiempo medio entre comienzo de sesiones TELNET*/
     /* double *tiempo_siguiente: tiempo que falta hasta el comienzo de la 
			       siguiente sesion TELNET*/
     /* double *duracion: tiempo que resta  hasta la finalizacion del estado ON*/
     /* int *primer_byte: variable booleana.Indica el comienzo del estado ON*/
/*
   Retorna el tiempo entre llegadas de paquetes de trafico TELNET. Los 
   paquetes son de tamanho 1  byte. El modelo utilizado es un modelo 
   ON-OFF en el que cada estado ON representa una sesion TELNET. La dura-
   cion de dicho estado ON es obtenida de la funcion telnet_duration() de
   la libreria Tcplib. El tiempo entre llegadas dentro del estado ON se 
   obtiene de la funcion telnet_interarrival() de la libreria tcplib.
   Los instantes de comienzo de los estados ON se modelan utilizando un
   proceso de Poisson.
   Si el comienzo de una sesion TELNET se superpone sobre la anterior
   se espera a que finalice esta ultima.
*/
{
  /*  float telnet_interarrival();  Tcplib */
  /* float telnet_duration();  Tcplib */
  double t;
  
  if (*primer_byte)
    {
      *tiempo_siguiente=expont(media_inicio);
      *primer_byte=0;
      *duracion=telnet_duration();
    }
  t=telnet_interarrival();
  *duracion=*duracion-t;
  if (*duracion<0.0)
    {
      *primer_byte=1;
      if (*tiempo_siguiente<0.0) return(t);
      else return(*tiempo_siguiente);
    }
  *tiempo_siguiente=*tiempo_siguiente-t;

  return(t);

}
/*-----------------------------------------------------------------------*/
double tiempo_telnet_2 (float media_inicio, float m_tam, float d_tam,
		       double *tiempo_siguiente, int *primer_byte,
		       int *tamano)
     /* float media_inicio: tiempo medio entre comienzo de sesiones TELNET  */
     /* float m_tam: Media numero de paquetes enviados en la ses.TELNET*/
     /* float d_tam: Desviacion tipica numero de paquetes sesion TELNET*/
     /* float *tiempo_siguiente: tiempo que falta hasta el comienzo de la 
			       siguiente sesion TELNET*/
     /* int *primer_byte: Variable booleana.Indica el comienzo del estado ON*/
     /* int *tamano: Numero de paquetes que faltan por enviar.         */
/*    Retorna el tiempo entre llegadas de paquetes de trafico TELNET. Los 
   paquetes son de tamanho 1  byte. El modelo utilizado es un modelo 
   ON-OFF en el que cada estado ON representa una sesion TELNET. La dura-
   cion de dicho estado ON en numero de paquetes es obtenida de una distri-
   bucion lognormal de media m_tam y desviacion tipica d_tam.
      El tiempo entre llegadas dentro del estado ON se obtiene de la 
   funcion telnet_interarrival() de la libreria tcplib.
      Los instantes de comienzo de los estados ON se modelan utilizando un
   proceso de Poisson.
   Si el comienzo de una sesion TELNET se superpone sobre la anterior
   se espera a que finalice esta ultima.
*/
{
  /*  float telnet_interarrival(); */
  double t;

  if (*primer_byte==1)
    {
      *tiempo_siguiente=expont(media_inicio);
      *primer_byte=0;
      t=lognormal(m_tam,d_tam);
      *tamano=ceil( t );
    }
  *tamano=*tamano-1;/*Primera vez que se ejecuta: genera un byte de menos*/
  t=telnet_interarrival();
  if (*tamano==0)
    {
      *primer_byte=1;
      if (*tiempo_siguiente<0.0) return(t);
      else return(*tiempo_siguiente);
    }
  *tiempo_siguiente=*tiempo_siguiente-t;

  return(t);
}
/*-----------------------------------------------------------------------*/

double pkt_ftp(float MTU, float cab, double *inicio_tx_item, 
	      double *inicio_ses_ftp, double *flag, int *pkt_final,
	      int *n_pkts, int *items)

     /* float MTU: Tamanho maximo de un pkt en la red*/
     /* float cab: Cabecera a anhadir a cada pkt*/
     /* float *inicio_tx_item: Booleana.Inicio de la tx de un item*/
     /* float *inicio_ses_ftp: Booleana.Inicio de una sesion FTP*/
     /* float *flag: Si =1 estado ON2. Si =2 estado OFF2. Si =3 estado OFF1*/ 
     /* int *pkt_final: Tamanho del ultimo pkt del item*/
     /* int *n_pkts: Numero de pkts que faltan por tx de un item*/
     /* int *items: Numero de items que faltan por tx de una ses. FTP*/
     
{
  long tamanho_item;
  int Mtu,Cab;

  Mtu=floor(MTU);
  Cab=floor(cab);
  
  if (*n_pkts==0)
    {
      *inicio_tx_item=1; /*El siguiente tiempo entre llegadas sera OFF2*/
      if (*items==0)
	{
	  *inicio_ses_ftp=1; /*El siguiente tiempo entre llegadas sera OFF1*/
	  *flag=3.0;
	}
      else *flag=2.0;
    }
  if (*inicio_ses_ftp==1)
    {
      /*printf("INICIO SESION FTP");*/
      *inicio_ses_ftp=0;
      *items=ftp_nitems()+1;
      /*printf("numero items =%i \n",*items);*/
      *inicio_tx_item=1;
    }
  if (*inicio_tx_item==1)
    {
      *items=*items-1;
      tamanho_item = 8 * ftp_itemsize();/*Ya que ftp_itemsize() retorna bytes*/
      /*printf("Tamanho item =%i bytes\n",tamanho_item);*/
      *n_pkts=tamanho_item/(Mtu-Cab) + 1;
      /*printf("numero pkts =%i \n",*n_pkts);*/
      *pkt_final=tamanho_item % (Mtu-Cab);
      *inicio_tx_item=0;
    }

  *n_pkts=*n_pkts-1;

  if (*n_pkts==0) return(*pkt_final);
  else return(MTU);

}
/*-----------------------------------------------------------------------*/

double tam_pkt_nntp()
/* Retorna el tamanho de paquete para trafico NNTP. 
   Llama a la funcion nntp_itemsize() de la libreria Tcplib. 
   El producto por 8 es porque nntp_itemsize retorna bytes*/
{
  return(8.0 * nntp_itemsize());
}
/*-----------------------------------------------------------------------*/


double tiempo_ftp (float media_inicio, float T, float m_off2,
		  float d_off2, double *flag, double *inicio_siguiente)

     /* float media_inicio: Tiempo medio entre comienzo de sesiones FTP*/
     /* float T: Parametro control de flujo                 */
     /* float m_off2: Tiempo medio de duracion del periodo off2  */
     /* float d_off2: Desviacion tipica                          */
     /* float *flag: Si =1 estado ON2. Si =2 estado OFF2. Si =3 estado OFF1*/
     /* float *inicio_siguiente: Tiempo que resta hasta el comienzo de la
			       siguiente sesion FTP*/
{
  double temp;

  if (*flag==1.0)
    {
      *inicio_siguiente=*inicio_siguiente-T;
      return(T);
    }
  if (*flag==2.0)
    {
      *flag=1;
      temp=lognormal(m_off2, d_off2);
      *inicio_siguiente=*inicio_siguiente-temp;
      return(temp);
    }
  /*flag=3*/
  
  *flag=1.0;
  temp=*inicio_siguiente;
  *inicio_siguiente= expont(media_inicio); 
  if (temp>0.0) return(temp);
  else return(T);

}
#endif

/*-----------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/* static int binomial(int n, float prob_success) */
     /* int n: number of trials */
     /* float prob_success: probability of success for any given trial*/
/* { */
/*	returns a random number binomially distributed	*/
/*	if n*p*q >= 10, the normal distribution is used  */
/*	where p=prob of success and q=prob of failure	*/

/*     float var, std_dev, np; */
/*     float prob_fail; */
/*     int x; */
/*     int count; */
/*     int i; */
/*     prob_fail = 1.0 - prob_success; */
/*     var = n * prob_success * prob_fail; */

/*     if (var >= 10) */
/*     { */
/* 	std_dev =  sqrt(var); */
/* 	np = n * prob_success; */
/* 	x = normal(np, std_dev); */
/*     } */
/*     else */
/*     { */
/* 	count = 0; */
/* 	for (i = 0; i < n; i++) */
/* 	    count += bernoulli(prob_success); */
/* 	x = count; */
/*     } */

/*     return (x); */
/* } */

/*--------------------------------------------------------------------------*/
double triangular(float a, float b, float c)
{
    double x, y;			/* a y b son los extremos de la base del
				   triangulo. */
    do
    {
	x = uniform(a, b);
	y = uniform(0.0, 1.0);
    }
    while (((x < c) && (y < (x - a) / (c - a))) || ((x > c) && (y < (x - b) / (c - b))));
    return (x);
}

/*--------------------------------------------------------------------------*/

/* definiciones para mejor comprension del codigo */
#define extremo_izq       dist->value1
#define extremo_drch      dist->value2
#define centro            dist->value3
#define interv            dist->value4
#define tiempo_comienzo   dist->value5
#define tiempo_actual     dist->value6
#define primera           dist->indice

double tiempo_triangular(struct distrib *dist, double tiempo_anterior,
			int longitud)
{
    double total, aux, porcentaje_resto, longitud_resto;
    if (primera)
    {
	primera = 0;
	tiempo_actual = longitud * 8E-3 / triangular(extremo_izq, extremo_drch, centro);
    }
    if ((tiempo_anterior + tiempo_actual) > (tiempo_comienzo + interv))
    {
	tiempo_comienzo += interv;
	total = tiempo_comienzo - tiempo_anterior;
	porcentaje_resto = 1 - total / tiempo_actual;
	longitud_resto = longitud * porcentaje_resto;
	while (TRUE)
	{
	    /*
	       en caso de distribucion triangular de velocidades de recepcion
	       de trama en Mbps
	    */
	    tiempo_actual = longitud * 8E-3 / triangular(extremo_izq, extremo_drch, centro);
	    aux = porcentaje_resto * tiempo_actual;
	    if (aux < interv)
		return (total + aux);
	    total += interv;
	    tiempo_comienzo += interv;
	    longitud_resto -= longitud * interv / tiempo_actual;
	    porcentaje_resto = longitud_resto / longitud;
	}
    }
    return (tiempo_actual);
}
#undef extremo_izq
#undef extremo_drch
#undef centro
#undef interv
#undef tiempo_comienzo
#undef tiempo_actual
#undef primera
/*--------------------------------------------------------------------------*/

#define extremo_izq       dist->value1
#define extremo_drch      dist->value2
#define interv            dist->value4
#define tiempo_comienzo   dist->value5
#define tiempo_actual     dist->value6
#define primera           dist->indice

double velocidad_uniforme(struct distrib *dist, double tiempo_anterior,
			 int longitud)
{
    double total, aux, porcentaje_resto, longitud_resto;
    if (primera)
    {
	primera = 0;
	tiempo_actual = longitud * 8E-3 / uniform(extremo_izq,extremo_drch);
    }
    if ((tiempo_anterior + tiempo_actual) > (tiempo_comienzo + interv))
    {
	tiempo_comienzo += interv;
	total = tiempo_comienzo - tiempo_anterior;
	porcentaje_resto = 1 - total / tiempo_actual;
	longitud_resto = longitud * porcentaje_resto;
	while (TRUE)
	{
	    /*
	       en caso de distribucion uniforme de velocidades de recepcion
	       de trama en Mbps
	    */
	    tiempo_actual = longitud * 8E-3 / uniform(extremo_izq,extremo_drch);
	    aux = porcentaje_resto * tiempo_actual;
	    if (aux < interv)
		return (total + aux);
	    total += interv;
	    tiempo_comienzo += interv;
	    longitud_resto -= longitud * interv / tiempo_actual;
	    porcentaje_resto = longitud_resto / longitud;
	}
    }
    return (tiempo_actual);
}
#undef extremo_izq
#undef extremo_drch
#undef interv
#undef tiempo_comienzo
#undef tiempo_actual
#undef primera
/*--------------------------------------------------------------------------*/

double tabular(struct distrib *dist, double tiempo_anterior)
{
    double t1, t2;
    if ((dist->value1 + dist->tabla->intervalo) < (dist->value2 + tiempo_anterior))
    {
	dist->value1 += dist->tabla->intervalo;
	t1 = dist->value1 - tiempo_anterior;
	t2 = 1 - t1 / dist->value2;
	if (++dist->indice > dist->tabla->limite)
	    dist->indice = 0;
	dist->value2 = dist->tabla->valor[dist->indice];
	t2 *= dist->value2;
	return (t1 + t2);
    }
    return (dist->value2);
}
/*--------------------------------------------------------------------------*/

double file_smpls(struct distrib *p)
{
  struct tipo_tabla *t;

  t = p->tabla;
  if (p->value14)
    {
      if (++p->indice > t->limite)
	p->indice = 0;
      if (p->indice == p->value15)
	sigue_programa = OFF;
    }
  else
    {
      p->value14 = TRUE;
      p->indice = ((t->limite + 1) / t->uses) * t->order++;
      p->value15 = (t->order == t->uses) ? 0 :
	((t->limite + 1) / t->uses) * t->order;
    }
  return t->valor[p->indice];
}

#ifdef FILES_MMAPPED

void binary_control(void)
{
#if defined linux || defined __NetBSD__ || #system(amigaos)
  opsem[0].sem_num = 1;
  opsem[0].sem_op = 1;
  opsem[0].sem_flg = 0;
  semop(sem, opsem, 1);
#else
  sem_post(exec);
#endif
}

#endif

double binary_file(struct distrib *p)
{
  struct tipo_tabla *t = p->tabla;
#ifdef FLOAT
  float
#else
  double
#endif
    valor;
  int bytes;
  int leidos, fd;

#ifdef FILES_MMAPPED
  static int first=1;
  char base_name[200], sem_name[200];
  unsigned c;

  if (first)
    {
      umask(0007);
      first = 0;
#if defined linux || defined __NetBSD__ || #system(amigaos)
#define SEM_FILE "/tmp/sem_fddi"
      if ((fd=open(SEM_FILE, O_CREAT, 00660))== -1 || close(fd)==-1)
        {
          perror("Error abriendo fichero auxiliar para semaforos\n");
          exit(1);
        }
  
      if ((sem=semget(ftok(SEM_FILE, 'a'), 2, 00660 | IPC_CREAT))==-1)
        {
          perror("Error abriendo semaforos\n");
          exit(1);
        }
      opsem[0].sem_num = 1;
      opsem[0].sem_op = -1;
      opsem[0].sem_flg = 0;
      semop(sem, opsem, 1);
#else
      base_name[0] = '\0';
      if (strstr(getenv("HOSTTYPE"), "iris"))
	strcpy(base_name, getenv("HOME"));

      strcpy(sem_name, base_name);
      strcat(sem_name, "/sem_fddi_reading");
      if ((reading = sem_open(sem_name, O_CREAT, 0660 , 1)) == -1)
	{
	  perror("Error abriendo semaforo");
	  exit(1);
	}
      strcpy(sem_name, base_name);
      strcat(sem_name, "/sem_fddi_running");
      if ((exec = sem_open(sem_name, O_CREAT, 0660 , 2)) == -1)
	{
	  perror("Error abriendo semaforo");
	  exit(1);
	}
      sem_wait(exec);
#endif
    }
#endif
  if (t->order == t->limite)
	{
	  t->order = 0;
	  bytes  = t->limite * sizeof(*t->valor);
#ifdef FILES_MMAPPED
#if defined linux || defined __NetBSD__ || #system(amigaos)
      opsem[0].sem_num = 1;
      opsem[0].sem_op = 1;
      opsem[0].sem_flg = 0;
      semop(sem, opsem, 1);
      opsem[0].sem_num = 0;
      opsem[0].sem_op = -1;
      opsem[0].sem_flg = 0;
      semop(sem, opsem, 1);
#else
	  sem_post(exec);
	  sem_wait(reading);
#endif
#endif

	  leidos = read(t->uses, t->valor, bytes);

#ifdef FILES_MMAPPED
#if defined linux || defined __NetBSD__ || #system(amigaos)
      opsem[0].sem_num = 0;
      opsem[0].sem_op = 1;
      opsem[0].sem_flg = 0;
      semop(sem, opsem, 1);
      opsem[0].sem_num = 1;
      opsem[0].sem_op = -1;
      opsem[0].sem_flg = 0;
      semop(sem, opsem, 1);
#else
	  sem_post(reading);
	  sem_wait(exec);
#endif
#endif
	  if (leidos != bytes)
		{
		  if (leidos == 0)
			sigue_programa = OFF;
		  else
			{
			  t->limite = leidos / sizeof(*t->valor);
			  if (leidos % sizeof(*t->valor))
			    {
				fprintf(stderr, "File %s size not float size multiple\n"
						"Read %d bytes\n", t->name, leidos);
				exit (1);
			    }
			}
		}
	}
  valor = t->valor[t->order++];
  if (!finite(valor))
    valor = binary_file(p);
  if (!chd)
    return valor;

  if (p->F == F_nor_0_1 && p->inv == lognor_inv)
    if (one_minus_u)
      valor = lognor_nor_0_1(-valor, p->value3, p->value4);
    else
      valor = lognor_nor_0_1(valor, p->value3, p->value4);
  else
    if ((one_minus_u 
	 && !(p->inv == wei_inv || p->inv == pareto_inv || p->inv == exp_inv2))
	||
	(!one_minus_u 
	 && (p->inv == wei_inv || p->inv == pareto_inv || p->inv == exp_inv2)))
      if (p->F == F_nor_0_1)
	valor = (*p->inv)((*p->F)(-valor, p->value1, p->value2),
			 p->value3, p->value4);
      else
	valor = (*p->inv)(1-(*p->F)(valor, p->value1, p->value2),
			 p->value3, p->value4);
    else
      valor = (*p->inv)((*p->F)(valor, p->value1, p->value2),
		       p->value3, p->value4);
  if (!finite(valor))
    valor = binary_file(p);
  return valor;
}

double sincrono(struct distrib *dist, double ejecucion_anterior)
{
  struct distrib *dist_tiempos;
  double temp;

  dist_tiempos = dist->otro;
  if (dist_tiempos)
    /* devuelve la longitud de control (value1) o la de voz (value2)
       segun cual sea la llegada prevista mas inmediata */
    {
      return (dist_tiempos->value2 < dist_tiempos->value4) ?
        dist->value1 : dist->value2;
    }
  else
    {
      if (dist->value2 < dist->value4)
	/* es mas inmediata la llegada de control:
	   devuelve el delta correspondiente y calcula la siguiente
	   llegada prevista (value2) */
	{
	  temp = dist->value2 - ejecucion_anterior;
	  dist->value2 += dist->value1;
	  return temp;
	}
      /* es mas inmediata la llegada de voz:
	 devuelve el delta correspondiente y calcula el siguiente
	 delta y la siguiente llegada (value4) */
      temp = dist->value4 - ejecucion_anterior;
      dist->value4 += expont(dist->value3);
      return temp;
    }
}
/*--------------------------------------------------------------------------*/

double doble_determinista(float valor1, float valor2, float p)
{
  if (bernoulli(p)) 
	 return(valor1);
  else
	 return(valor2);
}

double lambda(float valor1, float valor2, float valor3, float valor4)
{
  double u;
  double k;

  u = fract_rand();
  k = (pow(u, valor3) - pow(1-u,valor4))/valor2;
  return(valor1 + k);
}

/******************************************************/
/* F_DISTR.C realiza cambio de distribucion CHD local */
/******************************************************/

double MAXIMO=10000;
double MINIMO=-100000;

/*-----------------------------------------------------------------------------
------------ FUNCIONES DE DISTRIBUCION Y SUS INVERSAS -----------------------
---------------------------------------------------------------------------- */

/* En todos los casos, la varibale 'y' representa la probabilidad para
 obtener la funcion de distribucion inversa, es decir, para obtener
 el valor de la v.a. que corresponderia a tal probabilidad. */

/*-----------------------------------------------------------------------------
                       UNIFORME

'a' y 'b' son los extremos del intervalo en que esta definida la v.a.

 Parametros: Media    = (a+b) / 2
             Varianza = (b-a)^2 / 12

 Rango a <= x <= b */

/* Funcion de distribucion inversa. */

double uni_inv(double y, double a, double b)
{  
  return (b-a)*y+a;
}

double uni_inv_diff(double y, double a, double b)
{  
  return b*y+a;
}

/* Funcion de distribucion. */

double F_uni(double x, double a, double b)
{
  if (x<a) return 0;
     else if (x>b) return 1;
          else return (x-a)/(b-a);
}

/* --------------------------------------------------------------------------
            
                         EXPONENCIAL

 'm' es la media de la v.a.
 m > 0

 Parametros: Media    = m
             Varianza = m^2

 Rango 0 <= x <= Infinito */

/* Funcion de distribucion inversa. */

double exp_inv(double y, double m)
{ 
  return -m*log(y);
}

double exp_inv2(double y, double m, double nada)
{
  return exp_inv(y,m);
}

/* Funcion de distribucion. */

double F_exp(double x, double m)
{
  if (x<0) return 0;
     else return 1-exp(-x/m);
}

double F_exp2(double x, double m, double nada)
{
  return F_exp(x,m);
}

/* --------------------------------------------------------------------------

                        NORMAL

 'm' es la media y 'desv' la desviacion tipica de la v.a.
 m, desv > 0

 Parametros: Media    = m
             Varianza = desv^2

 Rango -Infinito <= x <= Infinito */

/* Funcion auxiliar. */

double G_inv(double p)
{   
 #define C0 2.515517  /* estos parametros sirven para construir una aprox. */
 #define C1 0.802853  /* de la funcion de distribucion inversa de una v.a. */
 #define C2 0.010328  /* normal media=0 des.tipica=1, error<4.5E-4 */
 #define D1 1.432788
 #define D2 0.189269
 #define D3 0.001308
 double t=sqrt(-2*log(p));
 return t-(C0+C1*t+C2*t*t)/(1+D1*t+D2*t*t+D3*t*t*t);
}

/* Funcion de distribucion inversa. */

double nor_inv(double y, double m, double desv)
{
 double p=1-y;
 if((0<=p)&(p<=0.5)) 
   return G_inv(p)*desv+m;
 else 
   return -G_inv(1-p)*desv+m;
}

double F_nor_0_1(double x, double nada, double nada2)
{
  static double invsqrt2 = 0;
  static int primera = 1;
  if (primera)
    {
      invsqrt2 = 1/sqrt(2);
      primera = 0;
    }

  if (x<0)
    return 0.5 * erfc(-x*invsqrt2);
  else
    return 0.5 + 0.5 * erf(x*invsqrt2);
}

double F_nor(double x, double m, double desv)
{
  x=(x-m)/desv;
  return F_nor_0_1(x,0,0);
}

/* --------------------------------------------------------------------------

                        LOGNORMAL

 La distribucion lognormal se obtiene directamente mediante una normal de 
 parametros 'm' y 'desv'. Es decir, el logaritmo natural de una
 v.a. lognormal es una v.a. normal.

 Rango 0 <= x <= Infinito
 
 Llamamos var = desv^2

 Parametros: Media    = exp(m+var / 2)
             Varianza = exp(2m+var)*(exp(var)-1)

 Sea F_L la funcion de distribucion de una v.a. Lognormal, X_L = exp(X_N) =
 = exp(m + desv * Z), donde Z es una N(0, 1) (con f.distr. F_Z) y
 'm' y 'desv' la media y desviacion tipica de la normal X_N (de
 f.distr. F_N) de la que deriva X_L. Entonces:

 F_L(x_L) = F_N(x_N) = F_N(Ln X_L) = F_Z (Ln X_L - m / desv)

 La inversa de la funcion de distribucion lognormal se obtiene de
 forma analoga:

 X_L = F_L_inv(y) siendo 0 < y < 1 => F_L_inv(y) = exp(F_N_inv(y) =
 exp(m + desv * F_Z_inv(y))
 donde donde F_N_inv(y) es la inversa de la N(m, desv), y F_Z_inv(y)
 es la inversa de la N(0, 1) */
							 

/* Funcion de distribucion inversa */

static double mm_ant=0, desvv_ant=0, mm_N=0, dd_N=0;

double lognor_inv(double y, double m, double desv)
{
  double m4;

  if (m != mm_ant || desv != desvv_ant)
    {
      mm_ant = m;
      desvv_ant = desv;
      m *=m;
      m4 = m*m;
      desv *= desv;
      mm_N=log(m4/(m+desv))/2;
      dd_N=sqrt(log((m+desv)/m));
    }
  return(exp(nor_inv(y, mm_N, dd_N)));
}

double lognor_nor_0_1(double y, double m, double desv)
{
  double m4;

  if (m != mm_ant || desv != desvv_ant)
    {
      mm_ant = m;
      desvv_ant = desv;
      m *=m;
      m4 = m*m;
      desv *= desv;
      mm_N=log(m4/(m+desv))/2;
      dd_N=sqrt(log((m+desv)/m));
    }
  return(exp(y * dd_N + mm_N));
}


/* Funcion de distribucion Lognormal */

double F_log_nor(double x, double m, double desv)
{
  double m4;

  if (m != mm_ant || desv != desvv_ant)
    {
      mm_ant = m;
      desvv_ant = desv;
      m *=m;
      m4 = m*m;
      desv *= desv;
      mm_N=log(m4/(m+desv))/2;
      dd_N=sqrt(log((m+desv)/m));
    }
  if (x <= 0)
    return(0);
  else
    return(F_nor(log(x), mm_N, dd_N));
}

/* --------------------------------------------------------------------------

                    WEIBULL

 'alfa' y 'beta' son los parametros de escala y forma, respectivamente.
 alfa, beta > 0

 G es la funcion factorial gamma

 Parametros: Media    = a/b * G(1/b)
             Varianza = (a/b)^2 * {2b*G(2/b) - [G(1/b)]^2}

 Rango 0 <= x <= Infinito */

/* Funcion de distribucion inversa. */

double wei_inv(double y, double alfa, double beta)
{
  static double alfa_ant = 0, inv_alfa;
  if (alfa_ant != alfa)
    {
      inv_alfa = 1/alfa;
      alfa_ant = alfa;
    }
  return beta*(pow(-log(y),inv_alfa));
}

/* Funcion de distribucion Weibull. */

double F_wei(double x, double alfa, double beta)
{
 if (x<=0) return 0;
    else return 1-exp(-pow(x/beta,alfa));
}

/* --------------------------------------------------------------------------

                              GAMMA

 'alfa' y 'beta' son los parametros de escala y forma, respectivamente.
 alfa, beta > 0

 Parametros: Media    = alfa * beta
             Varianza = alfa^2 * beta

 Rango 0 <= x <= Infinito */

double gamln(double z)
{
  #define c0 1
  #define c1 76.18009173
  #define c2 -86.50532033
  #define c3 24.01409822
  #define c4 -1.231739516
  #define c5 0.00120858003
  #define c6 -0.00000536382
  #define lr2pi 0.918938533204
  
  double suma;
  
  suma=(z-0.5)*log(z+4.5)-(z+4.5)+lr2pi;
  suma+=log(c0+c1/z+c2/(z+1)+c3/(z+2)+c4/(z+3)+c5/(z+4)+c6/(z+5));
  return suma;
}

double beta(double z, double w)
{
  return exp(gamln(z)+gamln(w)-gamln(z+w));
}

double gauinv(double p)
{
  p=1-p;
  if ( (0<=p) & (p<=0.5) ) return G_inv(p);
  else return -G_inv(1-p);
}

double gamain( double x, double a )
{
  # define a1 0.31938153
  # define a2 -0.356563782
  # define a3 1.781477937
  # define a4 -1.821255978
  # define a5 1.330274429

  int aa0;
  int aa1;
  int aa2;
  int j;
  double sum,fac,F0,F1,F2,F3,A,B,C,val1,val2;
  
  if (x<=0.000002) return 0;
  if (x>MAXIMO) return 1;
  if (a<=1) return 1-exp(-x);
  if (a<=2) return ((1-exp(-x))*(2-a) + (1-exp(-x)*(1+x))*(a-1));
  else {
    aa0 = (int)(a-1.0);
    aa1 = (int)(a);
    aa2 = (int)(a+1);
    sum=1; 
    fac=0;
    j=1;
    if (aa0==0) { F0=0; F1=1-exp(-x); F2=1-exp(-x)*(1+x); F3=1-exp(-x)*(1+x+x*x/2); };
    if (aa0==1) { F0=1-exp(-x); F1=1-exp(-x)*(1+x); F2=1-exp(-x)*(1+x+x*x/2); F3=1-exp(-x)*(1+x+x*x/2+x*x*x/6); };
    if (aa0>1)
      {
        for (j=1; j<=aa0-1; j++)
        {
          fac=fac+log(j);
          sum+=exp(j*log(x)-fac);
        };
        F0=1-exp(-x)*sum; 
        fac=fac+log(j); sum+=exp(j*log(x)-fac);
        F1=1-exp(-x)*sum; 
        j++; fac=fac+log(j); sum+=exp(j*log(x)-fac);
        F2=1-exp(-x)*sum;
        j++; fac=fac+log(j); sum+=exp(j*log(x)-fac);
        F3=1-exp(-x)*sum;
      };
    if (aa1==0) return F2;
    if ((a>aa1-0.001)&(a<aa1+0.001)) return F1;
    if ((a>aa2-0.001)&(a<aa2+0.001)) return F2;
    else 
    {
      A=(F2-F0)/(4*aa1);
      B=F2-F1-(2*aa1+1)*A;
      C=F1-A*aa1*aa1-B*a1;
      val1=A*a*a+B*a+C;
      A=(F3-F1)/(4*aa2);
      B=F3-F2-(2*a2+1)*A;
      C=F2-A*aa2*aa2-B*aa2;
      val2=A*a*a+B*a+C;
      val1=(val1+val2)/2;
      if (val1<0) val1=0;
      else if (val1>1) val1=1;
      return val1;
    };
  };
}

double gaminv(double p, double V)
{
  int itmax=10;
  int it=0;
  double XX,C,CH,A,Q,P1,P2,T,X,G,B,paso;
  double s1,s2,s3,s4,s5,s6;
  #define E 0.0000005
  #define AA 0.6931471805
  if (p<=0.000002) return 0.0;
  else if (p>=0.999998) return MAXIMO;
  else
  {
    int bucle=0;
    V=V*2;
    G=gamln(V/2);
    XX=0.5*V;
    C=XX-1.0;
    if (V>=-1.24*log(p)) bucle=1;
    else {
           CH=pow(p*XX*exp(G+XX*AA),1.0/XX);
           if (CH-E<0) bucle=6;
           else bucle=4;
         };
    do {
       if (bucle==1) {
			if (V>=0.32) bucle=3;
                        else {
				CH=0.4;
				A=log(1.0-p);
		             };
		     };
	if (bucle==2) {
			Q=CH;
			P1=1.0+CH*(4.67+CH);
			P2=CH*(6.73+CH*(6.66+CH));
			T=-0.5+(4.67+2.0*CH)/P1-(6.73+CH*(13.32+3.0*CH))/P2;
			CH=CH-(1.0-exp(A+G+0.5*CH+C*AA)*P2/P1)/T;
			if (fabs(Q/CH-1.0)-0.01>0.0) bucle=2;
			else bucle=4;
		      };
	if (bucle==3) {
			X=gauinv(p);
			P1=0.222222/V;
			/* CH=V*pow((X*sqrt(P1)+1.0-P1),3); */
			CH = X*sqrt(P1)+1.0-P1;
			CH *= CH*CH*V;
			if (CH>2.2*V+6.0) CH=-2.0*(log(1.0-p)-C*log(0.5*CH)+G);
			P1=0.5*CH;
			paso=10;
			while (paso>0.001)
			{
				while(gamain(P1+paso,XX)<p)
				{ P1=P1+paso; CH=2*P1; };
				paso=paso/5;
			};
			bucle=4;
		      };
	if (bucle==4) {
 			Q=CH; 
			P1=0.5*CH;
			P2=p-gamain(P1,XX);
			bucle=5;
			if ((fabs(P2)<E)|(P2<0)) bucle=6;
			if (fabs(CH)<E) bucle=6;
		       };
	if (bucle==5) {
			it++; 
			T=P2*exp(XX*AA+G+P1-C*log(CH));
			B=T/CH;
			A=0.5*T-B*C;
			s1=(210.0+A*(140.0+A*(105.0+A*(84.0+A*(70.0+60.0*A)))))/420.0;
			s2=(420.0+A*(735.0+A*(966+A*(1141.0+1278.0*A))))/2520.0;
			s3=(210.0+A*(462.0+A*(707+932.0*A)))/2520.0;
			s4=(252.0+A*(672.0+1182.0*A)+C*(294+A*(889.0+1740.0*A)))/5040;
			s5=(84.0+264.0*A+C*(175+606*A))/2520.0;
			s6=(120.0+C*(346.0+127.0*C))/5040;
			CH=CH+T*(1+0.5*T*s1-B*C*(s1-B*(s2-B*(s3-B*(s4-B*(s5-B*s6))))));
			if ( fabs(Q/CH-1.0) > E ) bucle=4;
			else bucle=6;
		      };
  	} while ( (bucle!=6) && (it<itmax) );
	return P1;
     }
}

/* Funcion inversa */

double gam_inv(double y, double alfa, double beta)
{
   return gaminv(y,alfa)*beta;
}

/* Funcion de distribucion Gamma  */

double F_gam(double x, double alfa, double beta)
{
  if (x<=0) return(0);
  else
	 return gamain(x/beta,alfa);
}
      
/* --------------------------------------------------------------------------

                           BETA

 'alfa' y 'beta' son parametros de forma. alfa, beta > 0

 Parametros: Media    = alfa / (alfa + beta)
             Varianza = alfa*beta / [(alfa+beta)^2 * (alfa+beta+1)]  */

/* Rango 0 <= x <= 1 */

double betacf(double x, double a, double b)
{
 int itmax=100;
 double eps=0.00003;
 int m=0;
 double tem,qap,qam,qab,em,d,bz,bpp,bp,bm,az,app,am,aold,ap;
 am=1; bm=1; az=1; qab=a+b; qap=a+1;
 qam=a-1; bz=1-qab*x/qap;
 do
 {
  m++;
  em=m; tem=em+em; d=em*(b-m)*x/((qam+tem)*(a+tem));
  ap=az+d*am; bp=bz+d*bm;
  d=-(a+em)*(qab+em)*x/((a+tem)*(qap+tem)); app=ap+d*az;
  bpp=bp+d*bz; aold=az; am=ap/bpp; bm=bp/bpp;
  az=app/bpp; bz=1;
 } while ((m<itmax)&((fabs(az-aold))<(eps*fabs(az))));
 return az;
}

double betai(double x, double a, double b)
{
  double bt;
  if (x<=0) return 0;
  if (x>=1) return 1;
  bt=exp(gamln(a+b)-gamln(a)-gamln(b)+a*log(x)+b*log(1-x));
  if (x<((a+1)/(a+b+2))) return bt*betacf(x,a,b)/a;
  else return 1.0-bt*betacf(1.0-x,b,a)/b;
}

double betinv(double ALPHA, double P, double Q)
{
  double ACU,XINBTA,TX,A,PP,QQ,R,Y,S,T,H,W,YPREV,PREV,SQ,ADJ,G;
  int INDEX;
  int bucle=2;
  double BETA;
  ACU=0.000001;
  XINBTA=ALPHA;
  
  /* Se comprueba la validez de los argumentos */

  if (ALPHA<=0) return 0;
  else if(ALPHA>=1) return 1;
  else {

  BETA=beta(P,Q);

  /* Se cambia de cola si hace falta */

  if (ALPHA<=0.5) { A=ALPHA; PP=P; QQ=Q; INDEX=0; }
  else { A=1.0-ALPHA; PP=Q; QQ=P; INDEX=1; };

  /* Calculo de la aprximacion inicial */

  do {
  if (bucle==2) { R=sqrt(-log(A*A));
		  Y=R-(2.30753+0.27061*R)/(1.0+(0.99229+0.04481*R)*R);
		  if ((PP>1.0)&(QQ>1.0)) bucle=5;
		  else
		    {
		      R=QQ+QQ;
		      /* T=R*pow(1.0-T+Y*sqrt(T),3); */
		      T = 1.0-T+Y*sqrt(T);
		      T *= T*T*R;
		      if (T<=0.0) bucle=3;
		      else {
			T=(4.0*PP+R-2.0)/T;
			if (T<=1.0) bucle=4;
			else {
			  XINBTA=1.0-2.0/(T+1.0);
			  bucle=6;
			};
		      };
		    };
                  };
  if (bucle==3) { XINBTA=1.0-exp((log((1.0-A)*QQ)+BETA)/QQ);
		  bucle=6;
		};
  if (bucle==4) { XINBTA=exp((log(A*PP)+BETA)/PP);
 		  bucle=6;
		};
  if (bucle==5) { R=(Y*Y-3.0)/6.0;
		  S=1.0/(PP+PP-1.0);
                  T=1.0/(QQ+QQ-1.0);
                  H=2.0/(S+T);
		  W=Y*sqrt(H+R)/H-(T-S)*(R+5.0/6.0-2.0/(3.0*H));
		  XINBTA=PP/(PP+QQ*exp(W+W));
		  bucle=6;
		};
}while (bucle!=6);

/* Solucion por el metodo de Newton Rapshon modoficado */

do {
  if (bucle==6) { R=1.0-PP; T=1.0-QQ; YPREV=0.0; SQ=1.0; PREV=1.0;
   		  if (XINBTA<=0.0001) XINBTA=0.0001;
		  if (XINBTA>=0.9999) XINBTA=0.9999;
		  bucle=7;
		};
  if (bucle==7) { Y=betai(XINBTA,PP,QQ);
                  bucle=8;
		};
  if (bucle==8) { Y=(Y-A)*exp(BETA+R*log(XINBTA)+T*log(1.0-XINBTA));
		  if (Y*YPREV<=0.0) PREV=SQ;
		  G=1.0;
		  bucle=9;
		};
  if (bucle==9) { ADJ=G*Y;
 		  SQ=ADJ*ADJ;
  		  if (SQ>=PREV) bucle=10;
                  else { TX=XINBTA-ADJ;
                         if ((TX>=0.0)&(TX<=1.0)) bucle=11;
                         else bucle=10;
                       };
		};
  if (bucle==10) { G=G/3.0;
		   bucle=9;
		 };
  if (bucle==11) { if (PREV<=ACU) bucle=12;
		   else if (Y*Y<=ACU) bucle=12;
			else if ( (TX==0.0) || (TX==1.0) ) bucle=10;
			     else if (TX==XINBTA) bucle=12;
                                  else { XINBTA=TX; YPREV=Y; bucle=7; };
		};
}while (bucle!=12);
if (INDEX==1) XINBTA=1.0-XINBTA;
return XINBTA;
};
} 
  
/*  Funcion de distribucion Beta. */

double F_bet(double x,double alfa,double beta)
{
  if (x<=0)
	 return(0);
  if (x>=1)
	 return(1);
  return betai(x,alfa,beta);
}

/* Beta inversa */

double bet_inv(double y,double alfa,double beta)
{
  return betinv(y,alfa,beta);
}

/* --------------------------------------------------------------------------

                     PARETO

 'alfa' es el parametro de forma. alfa > 0.
 'k' es el valor minimo.

 La funcion de distribucion es:
            F(x) = 1 - (k/x)^alfa
  Y la de densidad
            f(x) = m*k^alfa / x^(alfa+1)
 
  Si alfa<=1 la distribucion tiene media infinita 
  Si alfa>1 media = alfa*k / alfa - 1
  Si alfa<=2 entonces la distribucion tiene varianza infinita.
  Si alfa>2 varianza = (alfa*k^2 / alfa-2) - media^2

 Rango k <= x <= Infinito */

/* Funcion de distribucion Pareto */

double F_pareto(double x, double alfa, double k)
{
  if (x<=1)
	 return(0);
  else
  return(1 - pow(k/x, alfa));
}

/* Pareto inversa */

double pareto_inv(double y, double alfa, double k)
{
  return(k / pow(y, 1/alfa));
}

/* --------------------------------------------------------------------------

	                   POISSON

'tasa' es la tasa media de llegada de clientes al sistema (clientes por u.t.) 

*/

double F_poisson(unsigned x, double tasa)
{
  static double tasa_ant = 0;
  static long double tasaL, expnL, *poissons=NULL, *factors=NULL;
  static unsigned P_POS;
  long double *m, *l, pr;
  int i, j, superior;

  if (tasa_ant != tasa)
    {
      superior = 0;
      tasa_ant = tasa;
      tasaL = tasa;
      P_POS = 250 > tasa*2 ? 250 : tasa*2;
      if (poissons) free(poissons);
      if (factors) free(factors);
      if ((poissons = malloc(sizeof(*poissons)*P_POS))==NULL
	  ||(factors = malloc(sizeof(*factors)*P_POS))==NULL)
	{
	  fprintf(stderr,"Not enough memory due to poisson rate %g\n", tasa);
	  exit(1);
	}
      for (i = 1, pr = factors[0] = 1, m = &factors[1];
	   i<P_POS; i++, m++)
	*m = pr *= tasaL/(long double)i;
      for (i = 0, m = poissons; i<P_POS; i++, m++)
	{
	  for (j = i, pr = 0, l = &factors[i]; j>=0; j--, l--)
	    pr += *l;
	  if (!superior && pr == poissons[i-1])
	    {
	      superior = i - 1;
	      fprintf(stderr,"Tasa %g maximo %d valor %.40Lg\n", tasa, superior,
		      poissons[superior]);
	    }
	  *m=pr;
	}
      expnL = (long double)1/poissons[P_POS-1];
      if (!superior)
	{
	  fprintf(stderr,"Tasa %g sin maximo en %d valores\n", tasa, P_POS);
	  exit(1);
	}
    }

  if (x>=P_POS)
    return 1;
  return poissons[x]*expnL;
}

double F_poisson2(double x, double tasa, double nada)
{
  return F_poisson((unsigned) x, tasa);
}
