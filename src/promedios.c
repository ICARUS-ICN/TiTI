/*#define DEBUG*/
#define SEGUIMIENTO

#define FULL_ANALYSIS
#define CONTROL_VARIATES

#ifdef FULL_ANALYSIS
/*#define ADD_FILE_RESULTS*/
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define PROMEDIOS_C
#define FALSE 0
#define TRUE 1
#define INFINITY 1E30

/* variables locales */

static FILE *efp = NULL;

#ifdef FULL_ANALYSIS
char PromediosSid[] = "%W% %G%";

static double *reloj = NULL;
static int file_limit = 10000;
int inter_file = 0;

static int CV_ASINC=0;
static int CV_S=0;

#ifdef ADD_FILE_RESULTS 
static char nombreCV[] = "ResultadosCV";
static char nombreNormal[] = "ResultadoNormal";
static char *nombre;
static FILE *results;
#endif /* ADD_FILE_RESULTS */
#endif /* FULL_ANALYSIS */

static unsigned real_promedios;

/* estructuras para analisis estadistico */

struct bloques_muestras
/* estructura para el calculo del tamanho de bloque
   para el que se puede despreciar la autocorrelacion */
{
  double y[400];
  double time[400];
  unsigned long tamanho;
  unsigned long actual;
  unsigned long muestras;
  double sum;
  float calidad; /* calidad minima exigida */
  float precision; /* factor de calidad minimo exigido */
  float error_margin; /* tolerancia relativa demandada */
  float precision_actual; /* ultimo factor de calidad conseguido */
  double media_actual;  /* ultimo promedio computado por analisis_est*/
  unsigned anticuado; /* indica si se han recibido mas muestras desde el
			 ultimo calculo de analisis_est */
  int autocorrelado; /* indica si los bloques aun estan correlacionados */
  unsigned comprobacion; /* indica el numero de bloques para la siguiente
			    comprobacion de requisitos */
  float r;   /* ultimo coeficiente de autocorrelacion calculado */

#ifdef FULL_ANALYSIS
  double tiempo_fin;		/* tiempo de termino de la simulacion */
#endif
#ifdef CONTROL_VARIATES
  double c[1200];  /* bloques de muestras de la variable de control */

  double sum_control;
  double tv;
  double media_control;
  double promedio_control;
  unsigned bloques_cobertura; /* número de bloques para el cálculo de cobertura */
  unsigned bloques_a; /* número de bloques para el cálculo de a */
  unsigned muestras_promedio; /* número de muestras para el cálculo del promedio */
  unsigned nc;
  double ccruz;

  double a; /* factor de correccion del metodo CV */

  unsigned CV; /* indica si se esta utilizando el metod CV */

  double r_control;       /* autocorrelacion de las muestras de la variable
			     de control, para el caso de variable asincrona
			     */
  int r_autocorrelado;    /* indicador de autocorrelado de las muestras de
			     la variable de control */
#endif
  unsigned orden_promedio; /* numero de orden de la variable promedio actual */
};

#define LONG_NOMBRES 128

struct analisis_estadistico
{
  FILE *fichero;		/* fichero de muestras */
  char nombre_fichero[LONG_NOMBRES];
  /* nombres ficheros */
  int next_results;
  int next_results_pot;

  unsigned long samples;
  int autocorrelado; /* indica si los bloques aun estan correlacionados */
  double sq;      /* accumulative square sum of the considered variable */
  float max_value;		/* maximun value of the considered variable */
  float min_value;		/* minimun value of the considered variable */
  
  int diferencia; /* indica si se va a calcular el error respecto a
		     valor analitico */
  double teorico; /* valor analitico, exacto o aproximado */

  double sum_tot;
  unsigned long num_tot;
#ifdef CONTROL_VARIATES
  double sum_control;
  unsigned long long num_control;  /* lleva la cuenta para actualizar n_c */
  double reloj_control;
#endif

  struct bloques_muestras *mul_1, *mul_2;

  struct analisis_estadistico *next;
};

static struct analisis_estadistico *First=NULL;

typedef struct analisis_estadistico * promedio;

#include "promedios.h"

#if defined FULL_ANALYSIS || defined FULL_PROMEDIOS
#if !(defined FULL_ANALYSIS && defined FULL_PROMEDIOS)
ERROR: solo una definida de FULL_ANALYSIS y FULL_PROMEDIOS
#endif
#endif

#if defined CONTROL_VARIATES || defined CV_PROMEDIOS
#if !(defined CONTROL_VARIATES && defined CV_PROMEDIOS)
ERROR solo una definida de CONTROL_VARIATES y CV_PROMEDIOS
#endif
#endif

/* FUNCIONES */

/* t-Student */

static double t_inv(double p, unsigned df)
{
  double t;
  int positive = p >= 0.5;
  p = (positive)? 1.0 - p : p;
  if (p <= 0.0 || df <= 0)
    t = HUGE_VAL;
  else if (p == 0.5)
    t = 0.0;
  else if (df == 1)
    t = 1.0 / tan((p + p) * 1.57079633);
  else if (df == 2)
    t = sqrt(1.0 / ((p + p) * (1.0 - p)) - 2.0);
  else
  {	
    double ddf = df;
    double a = sqrt(log(1.0 / (p * p)));
    double aa = a * a;
    a = a - ((2.515517 + (0.802853 * a) + (0.010328 * aa)) /
             (1.0 + (1.432788 * a) + (0.189269 * aa) +
              (0.001308 * aa * a)));
    t = ddf - 0.666666667 + 1.0 / (10.0 * ddf);
    t = sqrt(ddf * (exp(a * a * (ddf - 0.833333333) / (t * t)) - 1.0));
  }
  return (positive)? t : -t;
}

static float factor_calidad(float calidad, unsigned gl)
{
  double t;

  calidad = 1 - calidad;
  t = 1 - calidad / 2;

  return (float) t_inv(t,gl);
}

static float calidad_tabla(float x, unsigned n)
{
  double a = 0.000000000000001;
  double b = 0.999999999999999;
  double fa = t_inv(a,n) - x;
  double fb = t_inv(b,n) - x;
  double c, fc;

  if (fa >= 0)
    return 0;
  if (fb <= 0)
    return 1;

  while (a != b)
    {
      c = -fa * (b-a) / (fb-fa) + a;
      if (c == b || c==a)
	break;
      fc = t_inv(c,n) - x;
      if (fc > 0)
	{
	  fb = fc;
	  b = c;
	}
      else
	{
	  fa = fc;
	  a = c;
	}
    }

  c = 2 * c - 1;

  if (c < 2E-7)
    c = 0;

  return c;
}

/* INICIALIZACION */

static void inic_bloques(struct bloques_muestras *p, float calidad,
			 float tolerancia_relativa)
{
  p->actual = 0;
  p->muestras = 0;
  p->sum = 0;
  p->calidad = calidad;
  p->precision = factor_calidad(calidad,39);
  p->precision_actual = 0.0;
  p->error_margin = tolerancia_relativa;
  p->autocorrelado = TRUE;
  p->anticuado = TRUE;
  p->comprobacion = 200;
  p->r = 1.0;

  p->bloques_cobertura = 40;
  p->bloques_a = 0;
  p->muestras_promedio = 0;
  p->ccruz = 0;

#ifdef CONTROL_VARIATES
  p->sum_control = 0.0;
  p->nc = 0;
  p->CV = FALSE;
  p->a = 0;
  p->r_autocorrelado = TRUE;
  p->r_control = 1.0;
#endif
  p->orden_promedio = real_promedios;
  p->tiempo_fin = -1;
}

static void init_medida(promedio p, float calidad,
			float tolerancia_r)
{
  int i;

  /* 400 en hexadecimal? */
  p->next_results = 0x400;
  /* Qué hace esto? */
  for (p->next_results_pot=0, i = p->next_results; i!= 1;
       i>>=1, p->next_results_pot++);

  p->sum_tot = 0;
  p->num_tot = 0;
  p->samples = 0;
  p->sq = 0;
  p->autocorrelado = TRUE;
  
  /* Por defecto no calcula la media de la diferencia respecto a un
     valor te'orico */
  p->teorico = 0.0;
  p->diferencia = FALSE;

  if ((p->mul_1 = (struct bloques_muestras *)
       malloc(sizeof(struct bloques_muestras))) == NULL)
    {
      fprintf(stderr,
	      "promedios: can't allocate memory for bloques_muestras\n");
      exit(1);
    }
  p->mul_1->tamanho = 2;
  inic_bloques(p->mul_1, calidad, tolerancia_r);
  
  if ((p->mul_2 = (struct bloques_muestras *)
       malloc(sizeof(struct bloques_muestras))) == NULL)
    {
      fprintf(stderr,
	      "promedios: can't allocate memory for bloques_muestras\n");
      exit(1);
    }
  p->mul_2->tamanho = 3;
  inic_bloques(p->mul_2, calidad, tolerancia_r);

#ifdef CONTROL_VARIATES
  p->sum_control = 0.0;
  p->num_control = 0;
#endif

#ifdef FULL_ANALYSIS
  p->min_value = INFINITY;
  p->max_value = 0;
  p->fichero = NULL;
  p->nombre_fichero[0] = '\0';
#endif
  real_promedios++;
  p->next = First;
  First = p;
}

promedio inic_promedio(float calidad, float tolerancia_relativa)
{
  promedio temp;
  
  if (!efp)
    efp = stderr;
  if (calidad >= 1.0 || calidad < 0 || tolerancia_relativa <= 0)
    {
      fprintf(efp,
	      "Error en parametros en init_promedio(calidad %f tolerancia %f)\n",
	      calidad, tolerancia_relativa);
      exit(1);
    }
  temp = (promedio) malloc(sizeof(struct analisis_estadistico));
  init_medida(temp, calidad, tolerancia_relativa);
  if(calidad > 0.0)
    num_promedios++;
  else
    temp->mul_1->comprobacion = temp->mul_2->comprobacion = 0;
  return temp;
}

promedio inic_promedio_dif(float calidad, float tolerancia_relativa,
			   double analitico)
{
  promedio temp;

  temp = inic_promedio(calidad, tolerancia_relativa);
  temp->teorico = analitico;
  temp->diferencia = TRUE;
  return temp;
}

#ifdef FULL_ANALYSIS
void inic_parametros(double *clock, int files_limit, int inter_est,
		     FILE *fichero_error)
{
  char inter_file_name[90] = "";
  sprintf(inter_file_name, "/tmp/inter_results%d.log", inter_est);
  reloj = clock;
  file_limit = files_limit;
  if (inter_est)
    if ((inter_file = open(inter_file_name, O_WRONLY|O_CREAT|O_EXCL, 00644)) == -1)
      {
	fprintf(efp, "cannot open file %s for output\n", inter_file_name);
	exit(1);
      }
  efp = fichero_error;
}

static void inic_files(promedio p, const char *nombre_fichero)
{
  if (strlen(nombre_fichero) >= LONG_NOMBRES)
    {
      fprintf(efp, "name %s too long for present array\n", nombre_fichero);
      exit(1);
    }
  strcpy(p->nombre_fichero, nombre_fichero);

  if ((p->fichero = fopen(p->nombre_fichero, "w")) == NULL)
    {
      fprintf(efp, "cannot open file %s for output\n", p->nombre_fichero);
      exit(1);
    }
}
  
promedio inic_promedio_files(float calidad, float tolerancia_relativa,
			     const char *nombre_fichero)
{
  promedio p;

  p = inic_promedio(calidad, tolerancia_relativa);
  
  inic_files(p, nombre_fichero);

  return p;
}
#endif /* FULL_ANALYSIS */

#ifdef CONTROL_VARIATES

promedio inic_promedioCVasinc(float calidad, float tolerancia_relativa,
			      double MediaControl, double Tv)
{
  promedio p;

  CV_ASINC = 1;

  p = inic_promedio(calidad, tolerancia_relativa);
  p->mul_1->media_control = p->mul_2->media_control = MediaControl;
  p->mul_1->tv = Tv; /* por doble de bloques para control */
  p->mul_2->tv = 3 * Tv / 2; /* idem */

  /* inhibe el m'etodo hasta que las muestras de control est'en
     descorrelacionadas */
  p->mul_1->CV = p->mul_2->CV = FALSE;
  return p;
}

promedio inic_promedioCVsinc(float calidad, float tolerancia_relativa,
			      double MediaControl)
{
  promedio p;

  CV_S = 1;

  p = inic_promedio(calidad, tolerancia_relativa);
  p->mul_1->media_control = p->mul_2->media_control = MediaControl;
  p->mul_1->CV = p->mul_2->CV = TRUE;
  p->mul_1->r_autocorrelado=p->mul_2->r_autocorrelado=0;
  return p;
}

#ifdef FULL_ANALYSIS

promedio inic_promedioCVasinc_files(float calidad, float tolerancia_relativa,
				    double MediaControl, double Tv,
				    const char *nombre_fichero)
{
  promedio p;
  
  p = inic_promedioCVasinc(calidad, tolerancia_relativa, MediaControl, Tv);
  inic_files(p, nombre_fichero);

  return p;
}

promedio inic_promedioCVsinc_files(float calidad, float tolerancia_relativa,
				   double MediaControl,
				   const char *nombre_fichero)
{
  promedio p;
  
  p = inic_promedioCVsinc(calidad, tolerancia_relativa, MediaControl);
  inic_files(p, nombre_fichero);

  return p;
}
#endif /* FULL_ANALYSIS */
#endif /* CONTROL_VARIATES */
                       
/* PROCESADO */

static void agrupa(double *origen, double *destino, unsigned grupo,
		   unsigned num_grupos)
{
  register int i, j;
  register double t;
  
  for (i=0;i<num_grupos;i++)
    {
      for (j=1, t=*origen++; j < grupo;
	   j++, t += *origen++);
      *destino++ = t;
    }
}

static double pseudo_r1(double *p, int n,
			double *sum, double *sum_sq, double *sum_sq_d)
{
  int i, iguales = TRUE;
  double primero,anterior=0,s=0,sq=0,sqd=0,media,r1;
  register double actual;
  
  primero = *p;
  for (i=0; i<n; i++)
    {
      actual = *p++;
      s += actual;
      sq += actual * actual;
      sqd += actual * anterior;
      if (iguales && i && anterior != actual)
	iguales = FALSE;
      anterior = actual;
    }
  *sum = s; *sum_sq = sq; *sum_sq_d = sqd;
  if (iguales)
    return 0.0;
  media = s/n;
  r1 = sqd + media * (primero + actual - (n+1) * media);
  r1 /= sq - n * media * media;
  return r1;
}

static float calculo_r1(double p[],int n)
{
  double sum_1,sum_2,s_sq_1,s_sq_2,sq_d_1,sq_d_2;
  double media,r1_1,r1_2,r1;
  
  r1_1 = pseudo_r1(&p[0],n/2,&sum_1,&s_sq_1,&sq_d_1);
  r1_2 = pseudo_r1(&p[n/2],n/2,&sum_2,&s_sq_2,&sq_d_2);
  if (r1_1 == 0.0 && r1_2 == 0.0)
    return 0.0;
  media = (sum_1 + sum_2)/n;
  r1 = sq_d_1 + sq_d_2 + p[n/2-1] * p[n/2] + media * (p[0] + p[n-1] - (n+1) * media);
  r1 /= s_sq_1 + s_sq_2 - n * media * media;
  return (float) (2 * r1 - (r1_1 + r1_2) / 2);
}

static float r1_200(double *p)
{
  double x[200];
  
  agrupa(p,x,2,200);
  return calculo_r1(x,200);
}

#ifdef CONTROL_VARIATES

static int agrupac(struct bloques_muestras *p, double *x, int grupo,
		    unsigned *numbloques)
// Devuelve indice del primer valor devuelto en x[], que tendrá
// numbloques valores de control, ajustado al bloque medio de datos, y
// empezando con el de posición índice
{
  double mintime, reftime = p->time[p->actual / 2];
  int i, j, m, posref, first, last;

  i = p->nc / 2;
  mintime = p->c[i];
  if (mintime < reftime)
    for (i++; p->c[i] < reftime && i<p->nc; i++);
  else
    {
      for (i--; p->c[i] > reftime && i; i--);
      i++;
    }
  if (!i || i == p->nc)
    {
      *numbloques = 0;
      return 0;
    }  
  if (p->c[i] - reftime > reftime - p->c[i-1])
    i--;
  posref = i;
  
  m = (*numbloques * grupo)/2;
  
  first = posref - m;
  while (first < 0)
    first += grupo;
  last = posref + m;
  while (last >= p->nc)
    last -= grupo;
  *numbloques = (last-first)/grupo;

  for (i = 0, j = first; i<*numbloques; i++, j += grupo)
    x[i] = p->c[j+grupo] - p->c[j];

  return (m - (posref - first))/grupo; /* por doble de bloques de control  quizá haya
					que cambiarlo */
}

static double ccruz;
static double accruz;
static unsigned bloques_ccruz, tamanho_ccruz;

static void correlacion_cruzada(struct bloques_muestras *p)
{
  double control[400], *x, *z, vc=0, vx=0, mx=0, mc=0, cruce=0;
  unsigned i;
  int firstidx, l;

  l = 400;
  if (CV_ASINC)
    firstidx = agrupac(p, control, 2, &l);
  else
    if (CV_S)
      {
	firstidx=0;
	for (i=0;i<400;i++)
	  control[i]=p->c[i];
      }
  
  if (l == 0)
    return;

  if (firstidx < 0 || l < 0 || firstidx + l > 400)
    {
      fprintf(stderr, "Algo mal en correlacion cruzada\nfirstidx %d p->nc %d l %d",
	      firstidx, p->nc, l);
      exit(2);
    }

  for (i = 0, x = &p->y[firstidx], z = control; i < l;
       i++, x++, z++)
    {
      mx += *x;
      mc += *z;
      cruce += *x * *z;
      vc += *z * *z;
      vx += *x * *x;
    }
  vc = (vc - mc*mc/l);
  vx = (vx - mx*mx/l);

  p->ccruz = ccruz = (cruce - mx*mc/l)/sqrt(vc*vx);
  accruz = (cruce - mx*mc/l)/vc;
  bloques_ccruz = l;
  tamanho_ccruz = p->tamanho;

#ifdef SEGUIMIENTO
  fprintf(stderr, "%d: %d bloques\tCorrelacion cruzada: %f\tFactor a: %f\n",
	  p->orden_promedio, l, p->ccruz, accruz);
#endif
}
#endif

static double CVa = 0;

static void analisis_est(struct bloques_muestras *p)
{
  double mx, varianza, muestras[40], *x, cruce;
  unsigned i, j;
  int grupo, firstidx, l = 40, n;
  struct analisis_estadistico *aux;

#ifdef CONTROL_VARIATES
  double control[400], *z, a, aux_mc, mc, vx, vc, sx, sc, sxc, s, escala;
#endif /* CONTROL_VARIATES */

  if (!p->anticuado)
    return;
  p->anticuado = FALSE;
  grupo = p->actual / l;
  
  firstidx = p->actual / 2 - grupo * 20;
  agrupa(&p->y[firstidx], muestras, grupo, l);
  firstidx = 0;

#ifdef CONTROL_VARIATES
  /* calculo de las nuevas muestras de bloques seg'un el metodo Control
     Variates */
  if (p->CV)
    {
      l = 40;
      p->bloques_a = 0;
      if (CV_ASINC)
	firstidx = agrupac(p, control, grupo*2, &l);
      else
	if (CV_S)
	  {
	    firstidx = 0;
	    agrupa(&p->c[firstidx], control, grupo, l);
	  }
      
      if (l < 30)
	{
	  p->CV = FALSE;
	  p->anticuado = TRUE;
	  analisis_est(p);
	  p->CV = TRUE;
	  return;
	}
      p->bloques_a = l;
      p->bloques_cobertura = l-1;
      p->precision = factor_calidad(p->calidad, l-2);

      if (firstidx < 0 || l < 0 || firstidx + l > 40)
	{
	  fprintf(stderr, "Algo mal en analisis_est\nfirstidx %d p->nc %d l %d\n",
		  firstidx, p->nc, l);
	  exit(2);
	}

      /* cálculo de "a" */
      escala = grupo * p->tamanho;

      /* estimacion covarianza */
      mx = mc = vc = vx = cruce = 0.0;
      for (i = 0, x = &muestras[firstidx], z = control; i < l;
	   i++, x++, z++)
	{
	  mx += *x;
	  mc += *z;
	  cruce += *x * *z;
	  vc += *z * *z;
	  vx += *x * *x;
	}
      sx = vx-mx*mx/l;
      sx /= l-1;
      sc = vc-mc*mc/l;
      sc /= l-1;
      sxc = cruce-mx*mc/l;
      sxc /= l-1;
      s = sx - sxc*sxc/sc;
      s *= l-1;
      s /= l-2;
      aux_mc = p->media_control * escala;
      varianza = mc/l - aux_mc;
      varianza *= varianza;
      varianza /= sc * (l-1);
      varianza += 1.0/l;
      varianza *= s;

      p->a = sxc/sc;

      if (!p->autocorrelado)
	if (varianza <= 0.0)
	  p->precision_actual = INFINITY / 2;
	else
	  p->precision_actual = p->error_margin * fabs(mx) / (l * sqrt(varianza));

      /* Cálculo de a medio, aunque sólo uso el local */
      a = 0;
      i = 0;
      for (aux = First; aux; aux = aux->next)
	if (aux->mul_1->a != 0)
	  {
	    a += aux->mul_1->a;
	    i++;
	  }
      CVa = a/i;

      a = p->a;

      /* Tengo en cuenta todas la muestras posibles para el promedio */
      if (CV_ASINC)
	{
	  l = p->actual;
	  firstidx = agrupac(p, control, 2, &l);
	  if (l == 0)
	    {
	      fprintf(stderr, "Error inesperado en analisis_est\n");
	      exit (2);
	    }
	  p->muestras_promedio = l * p->tamanho;
	  
	  if (firstidx < 0 || l < 0 || firstidx + l > p->actual)
	    {
	      fprintf(stderr, "Algo mal en analisis_est b\nfirstidx %d p->nc %d l %d",
		      firstidx, p->nc, l);
	      exit(2);
	    }
	  mx = 0;
	  mc = 0;
	  for (i = 0, x = &p->y[firstidx], z = control; i < l;
	       i++, x++, z++)
	    {
	      mx += *x;
	      mc += *z;
	    }

	  mx = mx - a * (mc - p->media_control * p->tamanho * l);
	  mx /= l * p->tamanho;

	  p->promedio_control = mc / (p->tamanho * l);
	}
      else
	if (CV_S)
	  {
	    l = p->actual;
	    p->muestras_promedio = l * p->tamanho;
	    mx = 0;
	    mc = 0;
	    for (i = 0, x = &p->y[firstidx], z = &p->c[firstidx]; i < l;
		 i++, x++, z++)
	      {
		mx += *x;
		mc += *z;
	      }

	    mx = mx - a * (mc - p->media_control * p->tamanho * l);
	    mx /= l * p->tamanho;

	    p->promedio_control = mc / (p->tamanho * l);
	  }
    }
  else
#endif /* CONTROL_VARIATES */
    {
      mx = varianza = 0.0;
      for (i = 0, x = &muestras[firstidx]; i < l; i++, x++)
	{
	  mx += *x;
	  varianza += *x * *x;
	}
      varianza -= mx * mx / l;
      varianza /= l-1;
      if (!p->autocorrelado)
	if (varianza <= 0.0)
	  p->precision_actual = INFINITY / 2;
	else
	  p->precision_actual = p->error_margin * fabs(mx) / sqrt(varianza*l);

      /* computa la media teniendo en cuenta posibles muestras sueltas */
      firstidx = p->actual / 2 - 20 * grupo;
      n = p->actual % 40;
      cruce = 0.0;
      for (i = n, j = 0, x = &p->y[0]; i && j < firstidx; j++, i--, x++)
	cruce += *x;
      for (x = &p->y[40 * grupo+firstidx]; i; i--, x++)
	cruce += *x;
      cruce += p->sum;
  
      mx += cruce;
      mx /= p->actual * p->tamanho + p->muestras;
    }

  p->media_actual = mx;
}

static void diezma(double *x, unsigned num, unsigned factor)
{
  unsigned i, j;
  for(i=1, j=factor; j<num; i++, j+=factor)
    x[i] = x[j];
}

static int bloques(struct bloques_muestras *p, double x, double time_arrive)
{
  register float r1;
  unsigned comp, t;

#ifdef CONTROL_VARIATES
  double *control_t, r_ctrl;  /* para el c'alculo de la autocorrelaci'on de las
				 muestras de control */
  unsigned i;
#endif
  
  p->sum += x;
  if (CV_S)
    {
      double S = time_arrive;
      p->sum_control += S;
    }
  else
    if (!p->muestras)
      p->time[p->actual] = time_arrive;
  p->anticuado = TRUE;
    
  if (++(p->muestras) == p->tamanho)
    {
      i=p->actual++;
      p->y[i] = p->sum;
      p->sum = 0;
      p->muestras = 0;
      if (CV_S)
	{
	  p->c[i]=p->sum_control;
	  p->sum_control=0;
	}
      if (p->actual == 400)
	{
	  p->r = r1 = calculo_r1(p->y,400);
	  if (p->autocorrelado)
	    {
	      if (r1 <= 0 ||
		  (r1 < 0.4 && 
		   ((p->r != 1.0 && p->r > r1)||
		    r1 > r1_200(p->y))))
		p->autocorrelado = FALSE;
#ifdef SEGUIMIENTO
              fprintf(stderr, "%d: %ld muestras\t%f r1\n", p->orden_promedio,
                      p->actual * p->tamanho, r1);
#endif
	    }

#ifdef CONTROL_VARIATES
	  /* en caso de variable de control asincrona, se comprueba si las
	     muestras est'an autocorrelacionadas, en cuyo caso se inhibe
	     la reduccion de varianza */
	  if(CV_ASINC || CV_S)
	    {
	      if (p->r_autocorrelado)
		{
		  /* si el número de bloques no da para 39 muestras, inhibimos */
		  if (p->r_control != 1.0 || p->nc >= 360)
		    {
		      control_t = malloc(sizeof(*control_t) * 1200);
		      for (i=0; i < p->nc-1; i++)
			control_t[i] = p->c[i+1] - p->c[i];
		      r_ctrl = calculo_r1(control_t, p->nc-1);
		      if (r_ctrl <= 0.0 || 
			  (r_ctrl < 0.4 && 
			   ((p->r_control != 1.0 && p->r_control > r_ctrl) ||
			    r_ctrl > r1_200(p->y))))
			{
			  p->r_autocorrelado = FALSE;
			  p->CV = TRUE;
			}
		      p->r_control = r_ctrl;
		      free(control_t);
#ifdef SEGUIMIENTO
		      fprintf(stderr, "%d: %f r1_control\n", p->orden_promedio,
			      r_ctrl);
#endif
		    }
		}

	      correlacion_cruzada(p);
	      
	      if (CV_ASINC)
		{
		  diezma(p->c, p->nc, 2);
		  if (!(p->nc % 2))
		    p->sum_control += p->tv;
		  p->nc = (p->nc+1)/2;
		  p->tv *= 2;
		}
	      else if (CV_S)
		agrupa(p->c, p->c, 2, 200);
	       
	    }
#endif

	  agrupa(p->y, p->y, 2, 200);
	  diezma(p->time, 400, 2);
	  p->tamanho *= 2;
	  p->actual = 200;
	}
      if (!p->autocorrelado && p->actual == p->comprobacion)
	{
	  analisis_est(p);
	  /* comprueba si alcanzo los requisitos */
	  if(p->precision_actual > p->precision)
	    {
              num_promedios--;
	      p->comprobacion = 0;

#ifdef FULL_ANALYSIS
	      if (reloj)
		p->tiempo_fin = *reloj;
#ifdef ADD_FILE_RESULTS
              if (p->CV)
                nombre = nombreCV;
              else
                nombre = nombreNormal;
              if (results = fopen(nombre, "a"))
                {
                  fprintf(results, "%lg\t%lg\t%ld\n", p->media_actual,
                          calidad_tabla(p->precision_actual, p->bloques_cobertura-1),
                          p->actual * p->tamanho);
                  fclose(results);
                }
              else
                fprintf(stderr, "bloques: No pude abrir fichero %s\n", nombre);
#endif /* ADD_FILE_RESULTS */
#endif /* FULL_ANALYSIS */
            }
	  else
	    {
              /* realiza una estimacion del numero de muestras necesarias */
	      r1 = p->precision / p->precision_actual;
	      r1 *= r1;
	      comp = (r1 + 1) * p->actual / 2 + 1;
	      if (p->actual >= 360)
		comp /= 2;
	      if (comp >= 360)
		p->comprobacion = 200;
	      else
		{
		  if ((t = comp % 40))
		    comp += 40 - t;
		  p->comprobacion = comp;
		}
	    }
#ifdef SEGUIMIENTO
          fprintf(stderr, "%d: %ld muestras\t%f delta_z", p->orden_promedio,
		  p->actual * p->tamanho, p->precision_actual - p->precision);
#ifdef CONTROL_VARIATES
          if (p->CV)
            fprintf(stderr, "\t%f a", p->a);
#endif
          fprintf(stderr, "\n");
#endif
          
	}
    }
  return !p->autocorrelado;
}

struct {
  char order_promedio;
  char num_samples;
  double promedio;
#ifdef CV_PROMEDIOS
  double sum_CV;
  unsigned long long num_CV;
#endif
} inter_sample;

void promedia(promedio p, double x, double llegada)
// En vez de llegada, S en caso de CV_S
{
  p->samples++;

  if (p->diferencia)
    x -= p->teorico;

#ifdef FULL_ANALYSIS
  if (x > p->max_value)
    p->max_value = x;
  if (x < p->min_value)
    p->min_value = x;
  if (p->fichero && p->samples < file_limit + 1)
    fprintf(p->fichero,"%f\n",x);
  p->sq += x * x;
#endif

  if (inter_file)
    {
      p->sum_tot += x;
      p->num_tot ++;
      if(p->samples == p->next_results)
	{
	  inter_sample.order_promedio = p->mul_1->orden_promedio;
	  inter_sample.num_samples = p->next_results_pot;
	  inter_sample.promedio = p->sum_tot/p->num_tot;
#ifdef CV_PROMEDIOS
	  inter_sample.sum_CV = p->reloj_control;
	  inter_sample.num_CV = p->num_control;
#endif
	  if (write(inter_file, &inter_sample, sizeof(inter_sample)) 
	      != sizeof(inter_sample))
	    {
	      fprintf(stderr, "Fallo al grabar en fichero inter_results\n");
	      exit (1);
	    }
	  p->next_results <<= 1;
	  p->next_results_pot ++;
	}
    }
  if (p->autocorrelado)
    {
      if (bloques(p->mul_1, x, llegada) || bloques(p->mul_2, x, llegada))
	{
	  p->autocorrelado = FALSE;
	  if (p->mul_1->autocorrelado)
	    {
	      free(p->mul_1);
	      p->mul_1 = p->mul_2;
	      p->mul_2 = NULL;
	    }
	  else
	    {
	      free(p->mul_2);
	      p->mul_2 = NULL;
	    }
	}
    }
  else
    bloques(p->mul_1, x, llegada);
}

#ifdef CONTROL_VARIATES
static void actualizaCV(struct bloques_muestras *p, double clock, double idle)
{
  unsigned i;
  if (!p->nc)
    {
      p->c[0] = clock-idle;
      p->nc = 1;
    }
  p->sum_control += idle;
  while (p->sum_control >= p->tv)
    {
      p->sum_control -= p->tv;
      if (p->nc == 1200)
	{
	  for (i=0; i < 1199; i++)
	    p->c[i] = p->c[i+1];
	  p->nc--;
	}
      p->c[p->nc] = clock - p->sum_control;
      p->nc++;
    }
}

void grupoCVasinc(double clock, double idle)
{
  struct analisis_estadistico *p;

  for (p = First; p; p = p->next)
    {
      actualizaCV(p->mul_1, clock, idle);
      if (p->mul_2) actualizaCV(p->mul_2, clock, idle);
    }
}
#endif

static struct bloques_muestras  *ULTIMO(struct bloques_muestras *a,
					struct bloques_muestras *b)
{
  if (a == NULL)
    return b;
  if (b == NULL)
    return a;
  return (a->actual < b->actual ? a : b);
}

/* RESULTADOS */

double estimacion(promedio p)
{
  analisis_est(p->mul_1);
  return p->mul_1->media_actual;
}

double calidad_fin(promedio p)
{
  analisis_est(p->mul_1);
  return calidad_tabla(p->mul_1->precision_actual, p->mul_1->bloques_cobertura-1);
}

double tolerancia_fin(promedio p)
{
  analisis_est(p->mul_1);
  return p->mul_1->error_margin * p->mul_1->precision 
    / p->mul_1->precision_actual;
}

unsigned long num_muestras(promedio p)
{
  return p->samples;
}

#ifdef FULL_ANALYSIS
void estado_promedio(promedio p)
{
  struct bloques_muestras *ultimo;

  if (p->mul_1->comprobacion)
    {     /* no ha finalizado el calculo */
      printf("Numero de muestras tomadas: %ld\n", p->samples);
      ultimo = ULTIMO(p->mul_1, p->mul_2);
      if (ultimo->autocorrelado)
	printf("Factor autocorrelacion: %.3f (autocorrelado hasta 0.4)\n",
	       ultimo->r);
      else
	printf("Calidad pedida: %.3f\t Calidad actual: %.2f\n",
	       calidad_tabla(ultimo->precision, ultimo->bloques_cobertura-1),
	       calidad_fin(p));
    }
  else 
    {     /* ha finalizado */
      printf("Ha Finalizado el calculo para esta variable.\n");
    }
}

void cierra_ficheros(promedio p)
{
  if (p->fichero && fclose(p->fichero))
    {
      fprintf(efp, "couldn't close file %s\n", p->nombre_fichero);
      exit(1);
    }
}

double desviacion(promedio p)
{
  double media,var;

  media = estimacion(p);
  var = p->sq / num_muestras(p) - media * media;
  return sqrt(var);
}

void promedios_stats(promedio p, FILE *fp)
{
  double media,var,precision;
  static double agregado = 0.0, min_calidad = 1.0, margen_final;
  static unsigned num_agregados = 0;
  
  if (!fp)
    fp = stdout;

  if (!p)
    {
      fprintf(fp, "Media agregada: %g\n", agregado / num_agregados);
      fprintf(fp, "Calidad minima: %g\n", min_calidad);
#ifdef CONTROL_VARIATES
      fprintf(fp, "CV_a: %g Correlacion cruzada(%d,%d): %g(%g)\n",
	      CVa, bloques_ccruz, tamanho_ccruz, ccruz, accruz);
#endif
      min_calidad = 1.0;
      agregado = 0.0;
      num_agregados = 0;
      return;
    }

  precision = calidad_fin(p);
  media = estimacion(p);

  agregado += media;
  num_agregados++;
  if (min_calidad > precision)
    min_calidad = precision;

  var = p->sq / num_muestras(p) - media * media;
  
  fprintf(fp, "Factor autocorrelacion 1o bloques: %f\n",
	  p->mul_1->r);
  if (precision > 0.0)
    {
      margen_final = tolerancia_fin(p);
      fprintf(fp, "Margen de error:    %g\tMargen final(%d):  %g\n",
	      p->mul_1->error_margin, p->mul_1->bloques_cobertura, margen_final);
      fprintf(fp, "Calidad final(%d):      %g\tCalidad:       %g\n",
	      p->mul_1->bloques_cobertura, precision,
	      calidad_tabla(p->mul_1->precision, p->mul_1->bloques_cobertura-1));
      fprintf(fp, "Numero de muestras: %6ld", num_muestras(p));
#ifdef CONTROL_VARIATES
      if (p->mul_1->CV && p->mul_1->bloques_a)
	fprintf(fp, " (%d)", p->mul_1->muestras_promedio);
#endif;
      fprintf(fp, "\n");

      if (reloj)
	fprintf(fp, "Fin calculo con reloj %f ms\n\n", p->mul_1->tiempo_fin);
    }
  else
    fprintf(fp, "Numero de muestras: %6ld\n", num_muestras(p));

  fprintf(fp, "\tPromedio:\t%g", media);

#ifdef CONTROL_VARIATES
  if (p->mul_1->CV && p->mul_1->bloques_a)
    fprintf(fp, " CV_a(%d): %g PromedioCtrl: %g MediaCtrl: %g\n",
	    p->mul_1->bloques_a, p->mul_1->a, p->mul_1->promedio_control,
	    p->mul_1->media_control);
  else
#endif
    fprintf(fp, "\n");

  fprintf(fp, "\tDesviacion:\t%g\n", desviacion(p));
  fprintf(fp, "\tValor maximo:\t%g\n", p->max_value);
  fprintf(fp, "\tValor minimo:\t%g\n", p->min_value);
}
#endif /* FULL_ANALYSIS */

void libera_promedio(promedio p)
{
#ifdef FULL_ANALYSIS
  cierra_ficheros(p);
#endif

  if (p->mul_1->comprobacion)
    num_promedios--;

  if (p->mul_1)
    free (p->mul_1);
  if (p->mul_2)
    free (p->mul_2);
  free (p);
}

#ifdef DEBUG

#include <limits.h>
#define REPETICIONES 100

#define	LARGEST_INT	2147483647
#define INVERSE_LARGEST_INT	((double)(1.0 / LARGEST_INT))

long semilla = 1;

void semilla_aleatorio(long seed)
{
  semilla = seed;
}

long aleatorio()
{
  long hi, lo, test;
  /*
    #define aleatorio_A 16807
    #define aleatorio_Q 127773
    #define aleatorio_R 2836
    */

#define aleatorio_A 48271
#define aleatorio_Q 44488
#define aleatorio_R 3399

  /*
    #define aleatorio_A 69621
    #define aleatorio_Q 30845
    #define aleatorio_R 23902
    */
  hi = semilla / aleatorio_Q;
  lo = semilla % aleatorio_Q;
  test = aleatorio_A * lo - aleatorio_R * hi;
  semilla = (test > 0)? test : test + LARGEST_INT;
  return semilla;
}


/*#define PRIMERA_PRUEBA
#define SEGUNDA_PRUEBA
#define TERCERA_PRUEBA
#define CUARTA_PRUEBA
#define QUINTA_PRUEBA
#define SEXTA_PRUEBA
#define SEXTA_B_PRUEBA
#define SEPTIMA_PRUEBA
#define NOVENA_PRUEBA
#define UNDECIMA_PRUEBA
#define DUODECIMA_PRUEBA
#define DECIMOTERCERA_PRUEBA
#define DECIMOCUARTA_PRUEBA
*/
#define DECIMOQUINTA_PRUEBA

main()
{
  promedio a;
  unsigned long i;
  double x, y;
  unsigned long aciertos=0, muestras = 0;
  double sum_precisiones = 0.0;

#ifdef PRIMERA_PRUEBA
  a = inic_promedio((float).90, (float).005);
  while (num_promedios)
    promedia(a, 0.0);
  printf("%g %g %d\n\n", estimacion(a), calidad_fin(a), num_muestras(a));
  libera_promedio(a);
#endif /* PRIMERA_PRUEBA */

#ifdef SEGUNDA_PRUEBA
  a = inic_promedio((float).90, (float).005);
  while (num_promedios)
    promedia(a, 2.0);
  for (i=0; i<1000000; i++)
    promedia(a, 2.0);
  printf("%g %g %d\n\n", estimacion(a), calidad_fin(a), num_muestras(a));
  libera_promedio(a);
#endif /* SEGUNDA_PRUEBA */

#ifdef TERCERA_PRUEBA
  a = inic_promedio((float).90, (float).01);
#ifdef FULL_ANALYSIS
  estado_promedio(a);
#endif
  for (i =0; num_promedios || i < 16312; i++)
    {
      promedia(a,aleatorio()/(double)LARGEST_INT);
      if (i==2405)
	{
#ifdef FULL_ANALYSIS
	  estado_promedio(a);
	  promedios_stats(a,stdout);
#else
          printf("%g %g %d\n", estimacion(a), calidad_fin(a), num_muestras(a));
#endif
	}
    }
#ifdef FULL_ANALYSIS
  estado_promedio(a);
  promedios_stats(a,stdout);
#else
  printf("%g %g %d\n", estimacion(a), calidad_fin(a), num_muestras(a));
#endif
  printf("\n");
  libera_promedio(a);
#endif /* TERCERA_PRUEBA */

#ifdef CUARTA_PRUEBA
  a = inic_promedio((float).99, (float).01);
  while (num_promedios)
    promedia(a, aleatorio()/(double)LARGEST_INT);
  printf("%g %g %d\n\n", estimacion(a), calidad_fin(a), num_muestras(a));
  libera_promedio(a);
#endif /* CUARTA_PRUEBA */

#ifdef QUINTA_PRUEBA
  a = inic_promedio(0.0, 0.01);
  printf("num_promedios: %d\n", num_promedios);
#ifdef FULL_ANALYSIS
  promedios_stats(a, stdout);
#endif
  for (i = 0; i < 9600; i++)
    promedia(a, aleatorio()/(double)LARGEST_INT);
#ifdef FULL_ANALYSIS
  promedios_stats(a, stdout);
#else
  printf("%g %g %d\n", estimacion(a), calidad_fin(a), num_muestras(a));
#endif
  libera_promedio(a);
#endif /* QUINTA_PRUEBA */

#ifdef SEXTA_PRUEBA
  semilla_aleatorio(27);
  for (i=0; i<REPETICIONES; i++)
    {
      a = inic_promedio(0.9, 0.0025);
      y = 0.0;
      while(num_promedios)
	{
	  y = aleatorio()/(double)LARGEST_INT + 0.9 * y;
	  promedia(a, y);
	}
      sum_precisiones += calidad_fin(a);
      y = estimacion(a);
      if (y >= 4.9875 && y <= 5.0125)
	aciertos ++;
      libera_promedio(a);
    }
  printf("%f%% aciertos, precision media %f\n", (float)aciertos / REPETICIONES * 100.0,
	 sum_precisiones / REPETICIONES);
#endif /* SEXTA_PRUEBA */

#ifdef SEXTA_B_PRUEBA
  semilla_aleatorio(27);
  for (i=0; i<REPETICIONES; i++)
    {
      a = inic_promedio(0.9, 0.01);
      y = 0.0;
      while(a->mul_1->autocorrelado)
	{
	  y = aleatorio()/(double)LARGEST_INT + 0.9 * y;
	  promedia(a, y);
	}
      sum_precisiones += calidad_fin(a);
      y = estimacion(a);
      if (y >= 4.95 && y <= 5.05)
	aciertos ++;
      libera_promedio(a);
    }
  printf("%f%% aciertos, precision media %f\n", (float)aciertos / REPETICIONES * 100.0,
	 sum_precisiones / REPETICIONES);
#endif /* SEXTA_B_PRUEBA */

#ifdef CONTROL_VARIATES

#ifdef SEPTIMA_PRUEBA
  a = inic_promedioCV(0.9,0.0001,-0.5,1.0/12);
  while(num_promedios)
    {
      y = aleatorio()/(double)LARGEST_INT;
      promediaCV(a, y, -y);
    }
  printf("a = %f\n", a->mul_1->a);
  promedios_stats(a, stdout);
  libera_promedio(a);
#endif /* SEPTIMA_PRUEBA */

#ifdef OCTAVA_PRUEBA
  a = inic_promedioCV(0.9,0.01,0.5,1.0/12);
  while(num_promedios)
    {
      y = aleatorio()/(double)LARGEST_INT;
      promediaCV(a, y, aleatorio()/(double)LARGEST_INT);
    }
  printf("a = %f\n", a->mul_1->a);
  promedios_stats(a, stdout);
  libera_promedio(a);
#endif /* OCTAVA_PRUEBA */

#ifdef NOVENA_PRUEBA
  semilla_aleatorio(27);
  a = inic_promedioCV(0.9,0.0001,0.5,1.0/12);
  y = 0.0;
  while(num_promedios)
    {
      x = aleatorio()/(double)LARGEST_INT;
      y = x + 0.9 * y + aleatorio()/(double)LARGEST_INT;
      promediaCV(a, y, x);
    }
  printf("a = %f\n", a->mul_1->a);
  promedios_stats(a, stdout);
  libera_promedio(a);
#endif /* NOVENA_PRUEBA */

#ifdef DECIMA_PRUEBA
  semilla_aleatorio(26287);
  for (i=0; i<REPETICIONES; i++)
    {
      a = inic_promedioCV(0.6, 0.001, 0.5, 1.0 / 12);
      a->mul_1->CV = a->mul_2->CV = FALSE;
      y = 0.0;
      while(num_promedios)
	{
	  x = aleatorio()/(double)LARGEST_INT;
	  y = x + 0.9 * y + aleatorio()/(double)LARGEST_INT;
	  promediaCV(a, y, x);
	}
      sum_precisiones += a->mul_1->precision_actual;
      y = estimacion(a);
      if (y >= 9.99 && y <= 10.01)
	aciertos ++;
      muestras += num_muestras(a);
      libera_promedio(a);
    }
  printf("%f%% aciertos, precision media %f, numero medio muestras %f\n",
	 aciertos / 10.0, sum_precisiones / REPETICIONES, muestras / REPETICIONES.0);
#endif /* DECIMA_PRUEBA */

#ifdef UNDECIMA_PRUEBA
  a = inic_promedioCVasinc(0.9,0.0001,-0.5);
  while(num_promedios)
    {
      y = aleatorio()/(double)LARGEST_INT;
      promedia(a, y);
      grupoCVasinc(a, -y, 1);
    }
  printf("a = %f\n", a->mul_1->a);
  promedios_stats(a, stdout);
  libera_promedio(a);
#endif /* UNDECIMA_PRUEBA */

#ifdef DUODECIMA_PRUEBA
  a = inic_promedioCVasinc(0.9,0.001,0.5);
  while(num_promedios)
    {
      y = aleatorio()/(double)LARGEST_INT;
      promedia(a, y);
      grupoCVasinc(a, aleatorio()/(double)LARGEST_INT, 1);
    }
  printf("a = %f\n", a->mul_1->a);
  promedios_stats(a, stdout);
  libera_promedio(a);
#endif /* DUODECIMA_PRUEBA */

#ifdef DECIMOTERCERA_PRUEBA
  a = inic_promedioCVasinc(0.9,0.001,0.5);
  y = 0.0;
  while(num_promedios)
    {
      x = aleatorio()/(double)LARGEST_INT;
      y = x + 0.9 * y + aleatorio()/(double)LARGEST_INT;
      promedia(a, y);
      grupoCVasinc(a, x, 1);
    }
  printf("a = %f\n", a->mul_1->a);
  promedios_stats(a, stdout);
  libera_promedio(a);
#endif /* DECIMOTERCERA_PRUEBA */

#ifdef DECIMOCUARTA_PRUEBA
  unsigned j;
  const lnMI = log(LARGEST_INT);
  double cum, z;
  unsigned n_cum;

  semilla_aleatorio(22587);
  for (i=0; i<REPETICIONES; i++)
    {
      j=n_cum=0;
      cum=0.0;
      a = inic_promedioCVasinc(0.9, 0.001, 0.5);
      /*      a->mul_1->CV = a->mul_2->CV = FALSE;*/
      y = x = 0.0;
      while(num_promedios)
	{
	  z = aleatorio()/(double)LARGEST_INT;
	  x = x * 0.99 + 0.01 * z;

	  if (j)
	    {
	      j--;
	      cum += x;
	      n_cum++;
	    }
	  else
	    {
	      if (n_cum) grupoCVasinc(a, cum, n_cum);
	      j =  - 4 * (log(aleatorio()) - lnMI) + 1;
	      cum = 0.0;
	      n_cum = 0;
	    }
	      
	  y = z + 0.9 * y + aleatorio()/(double)LARGEST_INT;
	  promedia(a, y);
	}
      sum_precisiones += a->mul_1->precision_actual;
      y = estimacion(a);
      if (y >= 9.99 && y <= 10.01)
	aciertos ++;
      muestras += num_muestras(a);
      libera_promedio(a);
    }
  printf("%f%% aciertos, precision media %f, numero medio muestras %f\n",
	 aciertos * 100 / (double) REPETICIONES,
	 calidad_tabla(sum_precisiones / REPETICIONES, 39), muestras / (double) REPETICIONES);
#endif /* DECIMOCUARTA_PRUEBA */

#endif /* CONTROL_VARIATES */

#ifdef DECIMOQUINTA_PRUEBA
  a = inic_promedio_dif((float).90, (float).01, (float).51);
#ifdef FULL_ANALYSIS
  estado_promedio(a);
#endif
  for (i =0; num_promedios || i < 16312; i++)
    {
      promedia(a,aleatorio()/(double)LARGEST_INT);
      if (i==2405)
	{
#ifdef FULL_ANALYSIS
	  estado_promedio(a);
	  promedios_stats(a,stdout);
#else
          printf("%g %g %d\n", estimacion(a), calidad_fin(a), num_muestras(a));
#endif
	}
    }
#ifdef FULL_ANALYSIS
  estado_promedio(a);
  promedios_stats(a,stdout);
#else
  printf("%g %g %d\n", estimacion(a), calidad_fin(a), num_muestras(a));
#endif
  printf("\n");
  libera_promedio(a);
#endif /* DECIMOQUINTA_PRUEBA*/
}

#endif
