/*------------------------------------------------------------------------*/
/*  
    The model is initialized by reading the parameter file and storing values
    into the tp and user data str structures.
    The user(s) sending data have their IDU lists initialized to contain the
    IDU's to send, their event queues are then initialized to send the data
    in their IDU list.
    Timers are initialized.

    ----------------------------------------------------------------------------*/


char InitSid[] = "@(#)init.c	4.13 07/24/98";
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/param.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

//#undef HZ
#include "defs.h"
#include "types.h"
#include "globals.h"

struct distrib *p_riyaz; /*Puntero a la estructura de definicion de la long.
			   de trama de alta prioridad segun 
			   el modelo de Riyaz*/

static struct tipo_tabla *primera_tabla = NULL;

/*--------------------------------------------------------------------------*/
/* This gets the next line from the input parameter file.  The line must
   start with a digit or #, otherwise its ignored */
static int get_a_line(int fdes, char *s)
{
  char c;
  for (;;)
    {
      if (read(fdes, &c, 1) == 0)
	return (EOF);
      if (isdigit(c) != 0 || c == '#' || c == '-')
	{
	  *s = c;
	  do
	    {
	      s++;
	      if (read(fdes, s, 1) == 0)
		return (EOF);
	    } while (*s != '\n');
	  *s = '\0';
	  return (TRUE);
	}
      else
	{
	  while (c != '\n')
	    if (read(fdes, &c, 1) == 0)
	      return (EOF);
	}
    }
}

/*--------------------------------------------------------------------------*/
/* The event priority line is in "buf";  each number is stored separatly
   in the prio_tab array.
*/
static int read_priorities(int fildes)
{
  int count = 0;
  int num, i, j, gz = FALSE;
  char buf[LINELENGTH], *ss;

  for (i = 0; i < no_of_nets; i++)
    {
      if (get_a_line(fildes, buf) == EOF)
	{
	  fprintf(efp, "param file ends after net event prio\n");
	  return (ERROR);
	}
      ss = buf;
      num = (*ss++ & 017) * 100;
      num += (*ss++ & 017) * 10;
      num += (*ss++ & 017);	/* make first3 chars an int -SES #1 */

      if (num != i && !(num == 111 && i == 0))
	{
	  fprintf(efp, "priorities specification: "
		  "nodes must be in order and option 111"
		  " only valid at first line\n");
	  return (ERROR);
	}
      if (num == 111)
	{
	  num = 0;
	  gz = TRUE;
	}

      while (*++ss)
	{
	  if (*ss >= '0' && *ss <= '9')
	    {
	      if (count >= NUM_SNT_EVENTS)
		break;
	      s[num].prio_tab[count] = *ss & 017;
	  
	      count++;
	    }
	}

      if (count < NUM_SNT_EVENTS)
	{
	  fprintf(efp, "Wrong number of subnet event priorities\n");
	  return (ERROR);
	}
      
      if (gz)
	{
	  for (num = 1; num <no_of_nets; num++)
	    for (j = 0; j < count; j++)
	      s[num].prio_tab[j] = s[0].prio_tab[j];
	  break;
	}
    }
  return 0;
}

/*--------------------------------------------------------------------------*/
static void lee_tabla(struct distrib *distribuc)
{
  int valor, i, id;
  float interv;
  char buf[LINELENGTH];
  static int llave = 0;
  static struct tipo_tabla *tabla;
  
  if (llave == 0)
    {
      llave = 1;
      if((tabla=malloc(sizeof(struct tipo_tabla)))==NULL)
	p_error("insuficiente memoria HEAP para la estructura"
		"tabla de valores \n");

      if((tabla->valor=malloc(sizeof(float)*500))==NULL)
	p_error("insuficiente memoria HEAP para la matriz de valores\n");

      if ((id = open("tabla.tbl", 0)) < 0)
	p_error("Tabla de tiempos entre llegadas inexistente.");
      
      if (get_a_line(id, buf) == EOF)
	p_error("Tabla erronea \n");
      sscanf(buf, "%f", &interv);
      tabla->intervalo = interv;
      i = 0;
      do
	{
	  if (get_a_line(id, buf) == EOF)
	    p_error("Tabla aerronea \n");
	  sscanf(buf, "%d", &valor);
	  tabla->valor[i++] = 1000.0 / valor;
	}
      while (buf[0] != '#');
      tabla->limite = i - 2;
      close(id);
    }
  distribuc->tabla = tabla;
  distribuc->indice = uniform(0.0, tabla->limite);
  /* Value1, tiempo de comienzo del intervalo, iniciado a 0 */
  distribuc->value1 = uniform(-tabla->intervalo, 0.0);
  /* Lectura del primer valor -> value2 */
  distribuc->value2 = distribuc->tabla->valor[distribuc->indice];
}

#ifdef SSIZE_MAX
#define TAM (SSIZE_MAX > 65536? 65536: SSIZE_MAX)
#else
#define TAM 65536
#endif

static int lectura(char *buf, 
#ifdef FLOAT
		   float
#else
		   double
#endif
		   **mues)
{
  int ret_u, t, num_a, n = 0, lineas = 0;
  char *a, *b;
  char aa[TAM+1];
#ifdef FLOAT
  float
#else
    double
#endif
    x, *y, *muestras;

  if ((ret_u = open(buf, O_RDONLY)) == -1)
    {
      fprintf(efp, "cannot open file %s for input\nerror %d\n",
	      buf,errno);
      p_error(NULL);
    }

  t = TAM;
  while ((num_a=read(ret_u,aa,t)))
    for (a = aa, b = a+num_a; a!=b; a++)
      if (*a == '\n')
	lineas++;

  if ((muestras=malloc(sizeof(float)*lineas)) == NULL)
    {
      fprintf(efp, "Insuficiente memoria HEAP para la matriz de valores %s\n",
	      buf);
      return(ERROR);
    }
  *mues = muestras;

  if (lseek(ret_u, 0, SEEK_SET))
    {
      fprintf(efp, "Error en reapertura del fichero %s\n", buf);
      return(ERROR);
    }
  a = aa;
  y = muestras;
  while ((num_a=read(ret_u,a,t-num_a)))
    {
      a[num_a] = '\0';
      a = aa;
      while (1)
	{
	  x = strtod(a, &b);
	  if (!(*b))
	    {
	      strcpy(aa,a);
	      num_a = strlen(aa);
	      a = aa + num_a;
	      break;
	    }
	  if (a == b)
	    {
	      a = aa;
	      num_a = 0;
	      break;
	    }
	  *y++ = x;

	  if (++n > lineas)
	    {
	      fprintf(efp, "Fichero %s ha cambiado\n", buf);
	      exit(1);
	    }
	  a = b;
	}
    }

  if (close(ret_u))
    {
      fprintf(efp, "Fichero %s no se ha podido cerrar\n", buf);

      exit(1);
    }
  return n;
}

static void lee_fichero(struct distrib *distribuc, char *filename)
{
#ifdef FLOAT
  float
#else
    double
#endif
    *y, *z;
  struct tipo_tabla *tabla;
  double sum;
  
  for (tabla = primera_tabla; tabla != NULL && strcmp(tabla->name, filename);
       tabla = tabla->sig);

  if (tabla == NULL)
    {
      if((tabla=malloc(sizeof(struct tipo_tabla)))==NULL)
	p_error("insuficiente memoria HEAP para la estructura"
		" tabla de valores\n");

      tabla->sig = primera_tabla;
      primera_tabla = tabla;
      tabla->uses = 0;
      tabla->order = 0;
      tabla->name = strdup(filename);
      if (tabla->name == NULL)
	p_error("insuficiente memoria HEAP para nombre del fichero\n");

      /*      fprintf(stderr, "Leyendo fichero %s\n", filename);*/
      tabla->limite = lectura(filename, &tabla->valor) -1;
      /*      fprintf(stderr, "Numero de datos: %d\n", tabla->limite + 1);*/
      
      for (sum=0, y=tabla->valor, z=y+tabla->limite; y<=z; sum += *y++);
      tabla->media = sum / (tabla->limite + 1);
      /*      fprintf(stderr, "Media del fichero %s es %f\n", filename, tabla->media);*/
    }
  tabla->uses++;
  distribuc->tabla = tabla;
}


static void inicia_binario(struct distrib *distribuc, char *filename,
			   double tamano)
{
  struct tipo_tabla *tabla;
  struct stat pos;
  static int minimo = INT_MAX;
  int i, m;


  if((tabla=malloc(sizeof(struct tipo_tabla)))==NULL)
    p_error("insuficiente memoria HEAP para la estructura"
	    " tabla de valores\n");
  
  tabla->sig = NULL;
  primera_tabla = tabla;
  tabla->name = strdup(filename);
  if (tabla->name == NULL)
    p_error("insuficiente memoria HEAP para nombre del fichero\n");
  if ((tabla->uses = open(filename, O_RDONLY)) == -1)
    {
      fprintf(efp, "cannot open file %s for input\nerror %d\n",
	      filename,errno);
      p_error(NULL);
      exit (1);
    }

  if (fstat(tabla->uses, &pos))
    {
      fprintf(efp, "error while getting stat information of %s\nerror %d\n",
	      filename,errno);
      p_error(NULL);
      exit (1);
    }
  if (pos.st_size/sizeof(*tabla->valor) < minimo)
    {
      minimo = pos.st_size/sizeof(*tabla->valor);
      for (i=0, m = minimo>>1; m; i++, m>>=1);
      paso_final = 1<<i;
    }
  /* sustituyo _POSIX_SSIZE_MAX por 2^18 = 262144 */
  tabla->order = tabla->limite = 
    (tamano< 262144 ? 262144 :tamano)/sizeof(*tabla->valor);
  if ((tabla->valor=malloc(sizeof(*tabla->valor)*tabla->limite)) == NULL)
    {
      fprintf(efp, "Insuficiente memoria HEAP para la matriz de valores %s\n",
	      filename);
      p_error(NULL);
    }

  distribuc->tabla = tabla;
}

static void inicia_chd(struct distrib* distrib, const char *buf)
{
  double ftemp, oa, ob, fa, fb;
  char ctemp, dorigin, dfinal, filename[LINELENGTH];
  int opts;
  double (* F)(double,double,double);
  double (*inv)(double,double,double);

  opts = sscanf(buf, "%lf %c %s %c %lf %lf %c %lf %lf", &ftemp, &ctemp, filename,
		&dorigin, &oa, &ob, &dfinal, &fa, &fb);

  if (opts != 3 && opts != 9)
    p_error("Error en especificacion de opciones CHD\n");

  if (opts == 3)
    sscanf(chdopts, "%c %lf %lf %c %lf %lf",
	   &dorigin, &oa, &ob, &dfinal, &fa, &fb);

  switch(dorigin)
    {
    case 'U':
    case 'u':  
      F = F_uni;
      break;
    case 'E':
    case 'e':
      F = F_exp2;
      break;
    case 'N':
    case 'n':
      if (oa == 0 && ob == 1)
	F = F_nor_0_1;
      else
	F = F_nor;
      break;
    case 'L':
    case 'l':
      F = F_log_nor;
      break;
    case 'W':
    case 'w':
      F = F_wei;
      break;
    case 'G':
    case 'g':
      F = F_gam;
      break;
    case 'B':
    case 'b':
      F = F_bet;
      break;
    case 'P':
    case 'p':
      F = F_pareto;
      break;
    case 'S':
    case 's':
      F = F_poisson2;
      break;
    default:
      p_error("Error: La distribucion de origen introducida no existe\n");
    }		  

  switch(dfinal)
    {
    case 'U':
    case 'u':
      inv = uni_inv_diff;
      fb -= fa;
      break;
    case 'E':
    case 'e':
      inv = exp_inv2;
      break;
    case 'N':
    case 'n':
      inv = nor_inv;
      break;
    case 'L':
    case 'l':
      inv = lognor_inv;
      break;
    case 'W':
    case 'w':
      inv = wei_inv;
      break;
    case 'G':
    case 'g':
      inv = gam_inv;
      break;
    case 'B':
    case 'b':
      inv = bet_inv;
      break;
    case 'P':
    case 'p':
      inv = pareto_inv;
      break;
    default:
      p_error( "Error: La distribucion de destino introducida no existe\n");
    }		  
  distrib->F = F;
  distrib->value1 = oa;
  distrib->value2 = ob;
  distrib->inv = inv;
  distrib->value3 = fa;
  distrib->value4 = fb;
}

/*---------------------------------------------------------------------------*/
/*  The distribution type is is determined.                                  */
/*  AQUI HAY QUE HACER MODIFICACIONES                                        */
static int find_type(char ch)
{
  switch (ch)
    {
    case 'H': return (DOBLE_DET);
    case 'h': return (DOBLE_DET);
    case 'C': return (CONSTANT);
    case 'c': return (CONSTANT);
    case 'U': return (UNIFORM);
    case 'u': return (UNIFORM);
    case 'N': return (NORMAL);
    case 'n': return (NORMAL);
    case 'P': return (POISSON);
    case 'p': return (POISSON);
    case 'E': return (EXPONENTIAL);
    case 'e': return (EXPONENTIAL);
    case 'G': return (GAMMA);
    case 'g': return (GAMMA);
    case 'T': return (TRIANGULAR);
    case 't': return (TRIANGULAR);
    case 'A': return (TABULAR);
    case 'a': return (FILE_SMPLS);
    case 'V': return (VELOCIDAD_UNIFORME);
    case 'v': return (VELOCIDAD_UNIFORME);
    case 'S': return (SINCRONO);
    case 's': return (SINCRONO);
    case 'r': return(RIYAZ95_B);              /*riyaz95. BAJA PRIORIDAD*/
    case 'R': return(RIYAZ95_A);              /*riyaz95. ALTA PRIORIDAD*/
    case 'b': return(BINARY_FILE);
    case 'B': return(BRADY69);                /*brady69*/
    case 'i': return(SRIRAM86);
    case 'I': return(SRIRAM86);               /*sriram86*/
    case 'k': return(KRUNZ95);
    case 'K': return(KRUNZ95);                /*krunz95*/
    case 'o': return(TORBEY91);
    case 'O': return(TORBEY91);               /*torbey91*/
    case 'x': return(GRUBER82);
    case 'X': return(GRUBER82);               /*gruber82*/
    case 'y': return(YEGENOGLU92);
    case 'Y': return(YEGENOGLU92);            /*yegenoglu92*/
    case 'w': return(WEILLBULL);              
    case 'W': return(WEILLBULL);              /*Weillbull*/
    case 'z': return(PARETO);
    case 'Z': return(PARETO);                 /*Pareto*/
    case 'd': return(TIEMPO_TELNET_1);
    case 'D': return(TIEMPO_TELNET_1);        /*tiempo entre ll. telnet 1*/
    case 'f': return(TIEMPO_TELNET_2);
    case 'F': return(TIEMPO_TELNET_2);        /*tiempo entre ll. telnet 2*/
    case 'l': return(LOGNORMAL);
    case 'L': return(LOGNORMAL);              /*lognormal*/
    case 'q': return(TAM_PKT_SMTP);
    case 'Q': return(TAM_PKT_SMTP);           /*tamanho de un pkt smtp*/
    case 'm': return(TIEMPO_FTP);       /*FTP.   tiempo entre llegadas*/       
    case 'M': return(PKT_FTP);          /*FTP.    Long.trama */
    case 'J': return(TAM_PKT_NNTP);
    case 'j': return(TAM_PKT_NNTP);
    default:
      return (ERROR);
    }
}

/*--------------------------------------------------------------------------*/
/*
  An initialization line containing traffic generation information
  is read and stored in the traffic data structure.
  AQUI HAY QUE HACER MODIFICACIONES.
  */
/*--------------------------------------------------------------------------*/

static int read_msg_line(int fd, struct traffic *traf_p)
{
  char buf[LINELENGTH], ctemp, filename[LINELENGTH];
  float prob;
  int entero;
  double ftemp, ftemp1, ftemp2, ftemp3,
    ftemp4,ftemp5,ftemp6,ftemp7,ftemp8,ftemp9; /*ELIMINA LAS QUE SOBRAN*/
#ifdef INDEP_SEEDS
  int seed1, seed2;
  extern int seedOne, seedTwo;
  static int seeds = 0;
  if (!seeds)
    seeds = seed;
#endif
  
  if((traf_p->msg = malloc(sizeof(struct distrib)))==NULL)
    {
      fprintf(efp, "insuficiente memoria HEAP para\n"
	      " la estructura distribucion de la longitud de mensajes\n");
      return(ERROR);
    }
  
  if((traf_p->arrive=malloc(sizeof(struct distrib)))==NULL)
    {
      fprintf(efp, "insuficiente memoria HEAP para la estructura\n");
      fprintf(efp, " distribucion del tiempo entre llegadas\n");
      return(ERROR);
    }
  
  /* Lectura de parametros para el tamanho de las tramas */
  if (get_a_line(fd, buf) == EOF)
    {
      fprintf(efp, "param file ends reading msg type descriptions");
      return (ERROR);
    }
  
  sscanf(buf, "%f %c %lf", &prob, &ctemp, &ftemp);
  traf_p->prob_of_msg = prob;
  if ((traf_p->msg->type = find_type(ctemp)) < 0)
    {
      fprintf(efp, "error finding distribution type\n");
      return (ERROR);
    }
#ifdef INDEP_SEEDS
  seed1 = seedOne;
  seed2 = seedTwo;
  semilla_aleatorio(++seeds);
  traf_p->msg->seed1 = seedOne;
  traf_p->msg->seed2 = seedTwo;
  semilla_aleatorio(++seeds);
  traf_p->arrive->seed1 = seedOne;
  traf_p->arrive->seed2 = seedTwo;
  seedOne = seed1;
  seedTwo = seed2;
#endif

  traf_p->msg->value1 = ftemp;
  traf_p->msg->value2 = 0;
  traf_p->msg->value3 = 0;
  traf_p->msg->value4 = 0;
  traf_p->msg->value5 = 0;
  traf_p->msg->value6 = 0;
  traf_p->msg->value7 = 0;
  traf_p->msg->value8 = 0;
  traf_p->msg->value9 = 0;
  traf_p->msg->value10 = 0;  
  traf_p->msg->value11 = 0;
  traf_p->msg->value12 = 0;
  traf_p->msg->value13 = 0;
  traf_p->msg->value14 = 0;
  traf_p->msg->value15 = 0;
  traf_p->msg->indice = 0;

  if (traf_p->msg->type == FILE_SMPLS)
    {
      sscanf(buf, "%lf %c %s", &ftemp, &ctemp, filename);
      lee_fichero(traf_p->msg, filename);
    }

  if (traf_p->msg->type == BINARY_FILE)
    {
      sscanf(buf, "%lf %c %s", &ftemp, &ctemp, filename);
      inicia_binario(traf_p->msg, filename, ftemp);
      if (chd)
	inicia_chd(traf_p->msg, buf);
    }
  
  if (traf_p->msg->type == DOBLE_DET)
    {
      sscanf(buf,"%f %c %lf %lf %lf",&prob,&ctemp,&ftemp,&ftemp1,&ftemp2);
      traf_p->msg->value2 = ftemp1;	/* segundo valor */
      traf_p->msg->value3 = ftemp2;	/* probabilidad */
    }

  if ((traf_p->msg->type == UNIFORM) ||
      (traf_p->msg->type == CONSTANT) ||
      (traf_p->msg->type == NORMAL) ||
      (traf_p->msg->type == GAMMA) ||
      (traf_p->msg->type == SINCRONO) ||
      (traf_p->msg->type == LOGNORMAL) ||
      (traf_p->msg->type == PARETO) ||
      (traf_p->msg->type == WEILLBULL))
    {
      sscanf(buf, "%f %c %lf %lf", &prob, &ctemp, &ftemp, &ftemp1);
      traf_p->msg->value2 = ftemp1;
    }
  if (traf_p->msg->type == TRIANGULAR)
    {
      sscanf(buf,"%f %c %lf %lf %lf",&prob,&ctemp,&ftemp,&ftemp1,&ftemp2);
      traf_p->msg->value2 = ftemp1;	/* extremo derecho */
      traf_p->msg->value3 = ftemp2;	/* centro */
    }
  if (traf_p->msg->type == RIYAZ95_B)
    {
      if (p_riyaz == NULL)
	{
	  fprintf(efp, "Error en la definicion del modelo de Riyaz.");
	  return (ERROR);
	}
      else 
	{
	  traf_p->msg->otro = p_riyaz;
	}
    }
  if (traf_p->msg->type == RIYAZ95_A)
    {
      sscanf(buf, "%lf %c %lf %lf %lf %lf ",&ftemp,&ctemp,
	     &ftemp1, &ftemp2, &ftemp3, &ftemp4);
      traf_p->msg->value1 = ftemp1;	/*  I1  */
      traf_p->msg->value2 = ftemp2;	/*  I2  */
      traf_p->msg->value3 = ftemp3;	/*  P1  */
      traf_p->msg->value4 = ftemp4;     /*  P2  */
    
      if (get_a_line(fd, buf) == EOF)
	{
	  fprintf(efp, "param file ends reading msg type descriptions");
	  return (ERROR);
	}
      sscanf(buf, "%lf %lf %lf ", &ftemp5, &ftemp6, &ftemp7);

      traf_p->msg->value5 = ftemp5;  /*  B1   */
      traf_p->msg->value6 = ftemp6;  /*  B2   */
      traf_p->msg->value7 = ftemp7;  /*  p    */
      traf_p->msg->value8 = 0;       /* cambio_escena */
      traf_p->msg->value9 = 1;       /* GOPde10       */
      traf_p->msg->value10 = 0;      /* Tamanho de la trama de baja prioridad.
					Se definira un puntero de la
					estructura de long.pkt de baja
					prioridad a este campo.*/
      p_riyaz = traf_p->msg;         /* Puntero a la estructura de long.pkt 
					de alta prioridad*/
      traf_p->msg->value13 = 0;      /* tramas */
      traf_p->msg->value14 = 0;      /* tramasB */
      traf_p->msg->value15 = 10;     /* fin */
    }
  if (traf_p->msg->type == YEGENOGLU92)
    {
      traf_p->msg->value1 = 0;          /* estado cadena de Markov */
      sscanf(buf, "%lf %c %lf %lf %lf %lf ",&ftemp,&ctemp,
	     &ftemp1, &ftemp2, &ftemp3, &ftemp4);
      traf_p->msg->value4 = ftemp1;	/*  a1  */
      traf_p->msg->value5 = ftemp2;	/*  a2  */
      traf_p->msg->value6 = ftemp3;	/*  a3  */
      traf_p->msg->value7 = ftemp4;     /*  m1  */
    
      if (get_a_line(fd, buf) == EOF)
	{
	  fprintf(efp, "param file ends reading msg type descriptions");
	  return (ERROR);
	}
      sscanf(buf, "%lf %lf %lf %lf %lf ", 
	     &ftemp5, &ftemp6,&ftemp7, &ftemp8,&ftemp9);
      traf_p->msg->value8 = ftemp5;  /*  m2   */
      traf_p->msg->value9 = ftemp6;  /*  m3   */
      traf_p->msg->value10 = ftemp7; /*  d1   */
      traf_p->msg->value11 = ftemp8; /*  d2   */
      traf_p->msg->value12 = ftemp9; /*  d3   */
    }
  if (traf_p->msg->type == KRUNZ95)
    {
      sscanf(buf, "%lf %c %lf %lf %lf %lf ",&ftemp,&ctemp,
	     &ftemp1, &ftemp2, &ftemp3, &ftemp4);
      traf_p->msg->value1 = ftemp1;	/*  mI  */
      traf_p->msg->value2 = ftemp2;	/*  dI  */
      traf_p->msg->value3 = ftemp3;	/*  mP  */
      traf_p->msg->value4 = ftemp4;     /*  dP  */
    
      if (get_a_line(fd, buf) == EOF)
	{
	  fprintf(efp, "param file ends reading msg type descriptions");
	  return (ERROR);
	}
      sscanf(buf, "%lf %lf ", &ftemp5, &ftemp6);
      traf_p->msg->value5 = ftemp5;  /*  mB   */
      traf_p->msg->value6 = ftemp6;  /*  dB   */
      traf_p->msg->value14 = 0;      /* tramas */
      traf_p->msg->value15 = 0;      /* tramasB */
    }
  if (traf_p->msg->type == PKT_FTP)
    {
      sscanf(buf,"%f %c %lf %lf",&prob,&ctemp,&ftemp,&ftemp1);
      traf_p->msg->value1 = ftemp;	/* MTU */
      traf_p->msg->value2 = ftemp1;	/* cabecera */
      traf_p->msg->value3 = 1.0;      /* inicio_tx_item */
      traf_p->msg->value4 = 1.0;      /* inicio_ses_ftp */
      traf_p->msg->value5 = 3.0;      /* flag */
      traf_p->msg->value13 = 0;       /* pkt_final */
      traf_p->msg->value14 = 0;       /* n_pkts */
      traf_p->msg->value15 = 0;       /* items */

    } 

  /*--------- Lectura de parametros para el tiempo entre llegadas---------*/

  if (get_a_line(fd, buf) == EOF)
    {
      fprintf(efp, "param file ends reading msg type descriptions");
      return (ERROR);
    }
  sscanf(buf, "%lf %c", &ftemp, &ctemp);
  if ((traf_p->arrive->type = find_type(ctemp)) < 0)
    {
      fprintf(efp, "error finding distribution type");
      return (ERROR);
    }
  if (traf_p->arrive->type == TABULAR)
    {
      lee_tabla(traf_p->arrive);
      return NONE;
    }
  
  traf_p->arrive->value1 = ftemp;
  traf_p->arrive->value2 = 0;
  traf_p->arrive->value3 = 0;
  traf_p->arrive->value4 = 0;
  traf_p->arrive->value5 = 0;
  traf_p->arrive->value6 = 0;
  traf_p->arrive->value7 = 0;
  traf_p->arrive->value8 = 0;
  traf_p->arrive->value9 = 0;
  traf_p->arrive->value10 = 0;
  traf_p->arrive->value11 = 0;
  traf_p->arrive->value12 = 0;
  traf_p->arrive->value13 = 0;
  traf_p->arrive->value14 = 0;
  traf_p->arrive->value15 = 0;

  traf_p->arrive->indice = 0;

  if (traf_p->arrive->type == FILE_SMPLS)
    {
      sscanf(buf, "%lf %c %s", &ftemp, &ctemp, filename);
      lee_fichero(traf_p->arrive, filename);
    }

  if (traf_p->arrive->type == BINARY_FILE)
    {
      sscanf(buf, "%lf %c %s", &ftemp, &ctemp, filename);
      inicia_binario(traf_p->arrive, filename, ftemp);
      if (chd)
	inicia_chd(traf_p->arrive, buf);
    }

  if (traf_p->arrive->type == TIEMPO_FTP)
    {
      sscanf(buf, "%lf %c %lf %lf %lf", &ftemp, &ctemp, &ftemp1, 
	     &ftemp2 , &ftemp3);
      traf_p->arrive->value2 = ftemp1;	/* T  */
      traf_p->arrive->value3 = ftemp2;	/* m_off2 */
      traf_p->arrive->value4 = ftemp3;  /* d_off2 */
      traf_p->arrive->value6 = 0.0;     /* inicio_siguiente */
      traf_p->arrive->otro = traf_p->msg; /*Puntero de la estructura de 
					    tiempo entre llegadas a la de
					    longitud de trama */
    }
  if (traf_p->arrive->type == SRIRAM86)
    {
      sscanf(buf, "%lf %c %lf %lf ", &ftemp, &ctemp, &ftemp1, &ftemp2 );
      traf_p->arrive->value2 = ftemp1;	/* T  */
      traf_p->arrive->value3 = ftemp2;	/* media_silencio */
    }
  if (traf_p->arrive->type == BRADY69)
    {
      sscanf(buf, "%lf %c %lf %lf ", &ftemp, &ctemp, &ftemp1, &ftemp2 );
      traf_p->arrive->value2 = ftemp1;	/* media_talkspurt  */
      traf_p->arrive->value3 = ftemp2;	/*  T   */
      traf_p->arrive->value4 = 0.0;     /* double *paquetes */
      traf_p->arrive->value14=1;        /* int *silencio    */        
      traf_p->arrive->value15=1;        /* int *primer_paquete */
    }

 
  if (traf_p->arrive->type == TORBEY91)
    {
      sscanf(buf, "%lf %c %lf %lf %lf %i ",&ftemp,&ctemp,
	     &ftemp1, &ftemp2, &ftemp3,&entero );
      traf_p->arrive->value2 = ftemp1;	/*  media_off  */
      traf_p->arrive->value3 = ftemp2;	/*  p1  */
      traf_p->arrive->value4 = ftemp3;  /*  p2  */
      traf_p->arrive->value13 = entero; /*  uniforme */ 
     
      traf_p->arrive->value5 = 0;       /*  numero */ 
      traf_p->arrive->value14 = 1;      /*  on  */
      traf_p->arrive->value15= 1;       /*  primera_vez */
    }
 
  if (traf_p->arrive->type == GRUBER82)
    {
      sscanf(buf, "%lf %c %lf %lf %lf %lf ",&ftemp,&ctemp,
	     &ftemp1, &ftemp2, &ftemp3, &ftemp4);
      traf_p->arrive->value2 = ftemp1;	/*  r1  */
      traf_p->arrive->value3 = ftemp2;	/*  c1  */
      traf_p->arrive->value4 = ftemp3;  /*  r2  */
      traf_p->arrive->value5 = ftemp4;  /*  c2  */   
      if (get_a_line(fd, buf) == EOF)
	{
	  fprintf(efp, "param file ends reading msg type descriptions");
	  return (ERROR);
	}
      sscanf(buf, "%lf  ",&ftemp5);
      traf_p->arrive->value6 = ftemp5;  /*  T   */
      traf_p->arrive->value15=1;        /* int *silencio */
    }
  if (traf_p->arrive->type == TIEMPO_TELNET_1)
    {
      traf_p->arrive->value2 = 0;	/*  tiempo_siguiente */
      traf_p->arrive->value3 = 0;	/*  duracion  */
      traf_p->arrive->value13 = 1;      /*  primer_byte  */
    }

  if (traf_p->arrive->type == TIEMPO_TELNET_2)
    {
      sscanf(buf, "%lf %c %lf %lf ",&ftemp,&ctemp,
	     &ftemp1, &ftemp2);

      traf_p->arrive->value2 = ftemp1;	/*  m_tam    */
      traf_p->arrive->value3 = ftemp2;	/*  d_tam    */
      traf_p->arrive->value4 = 0;	/*  tiempo_siguiente */
      traf_p->arrive->value13 = 1;      /*  primer_byte  */
      traf_p->arrive->value14 = 0;      /*  tamano  */

    }
  if ((traf_p->arrive->type == UNIFORM) ||
      (traf_p->arrive->type == CONSTANT) ||	/* SESA-SWAR #4 */
      (traf_p->arrive->type == NORMAL) ||
      (traf_p->arrive->type == GAMMA) ||
      (traf_p->arrive->type == LOGNORMAL) ||
      (traf_p->arrive->type == PARETO) ||
      (traf_p->arrive->type == WEILLBULL))
    {
      sscanf(buf, "%lf %c %lf", &ftemp, &ctemp, &ftemp1);
      traf_p->arrive->value2 = ftemp1;
    }
  if (traf_p->arrive->type == TRIANGULAR)
    {
      sscanf(buf, "%lf %c %lf %lf %lf", &ftemp, 
	     &ctemp, &ftemp1, &ftemp2, &ftemp3);
      traf_p->arrive->value2 = ftemp1;	/* extremo derecho */
      traf_p->arrive->value3 = ftemp2;	/* centro */
      traf_p->arrive->value4 = ftemp3;	/* intervalo */
      traf_p->arrive->value5 = uniform(-ftemp3, 0.0);
      traf_p->arrive->indice = 1;	/* indica primera vez de llamada */
    }
  
  if (traf_p->arrive->type == VELOCIDAD_UNIFORME)
    {
      sscanf(buf, "%lf %c %lf %lf", &ftemp, &ctemp, &ftemp1, &ftemp3);
      traf_p->arrive->value2 = ftemp1;	/* extremo derecho */
      traf_p->arrive->value4 = ftemp3;	/* intervalo */
      traf_p->arrive->value5 = uniform(-ftemp3, 0.0);
      traf_p->arrive->indice = 1;	/* indica primera vez de llamada */
    }
  if (traf_p->arrive->type == SINCRONO)
    {
      sscanf(buf, "%lf %c %lf", &ftemp, &ctemp, &ftemp1);
      traf_p->arrive->value1 = ftemp;     /* delta_t de control */
      traf_p->arrive->value3 = ftemp1;    /* delta_t medio trafico de voz */
      traf_p->arrive->value2 = uniform(0.0,ftemp);    /* siguiente llegada
							 de control */
      traf_p->arrive->value4 = expont(ftemp1);
      /* siguiente llegada de voz */
      traf_p->msg->otro = traf_p->arrive; /* puntero a arrive desde msg */
      traf_p->arrive->otro = NULL;        /* para distinguir el arrive
					     del msg*/
    }

  if (traf_p->arrive->type == DOBLE_DET)
    {
      fprintf(efp, "No tiene sentido usar DOBLE_DET para"
	      " tpo. entre llegadas\n");  
      return (ERROR);
    }
  
  return (1);
}

/*--------------------------------------------------------------------------*/
/* The dynamic fields of the traffic generation structure are
   initialized.
*/
static void init_fields(struct traffic *traf_p, int cid, int source, int dest,
			int type)
{
  traf_p->tsdu_cnt = MAX_INT;
  
  traf_p->cur_tcnt = 0;
  traf_p->cur_tarrive = 0.0;
  traf_p->t_flag = (cid << CSHIFT) | (dest << DSHIFT) | (source << SSHIFT);
  traf_p->t_flag |= type;
  traf_p->command_cnt = 0;
  traf_p->tsdu_ptr = NULL;
}

/*--------------------------------------------------------------------------*/
static void init_conns(int i)
{
  /* initialize connection info. */
  
  c[i].c_end1 = -1;
  c[i].c_end2 = -1;

  c[i].inter_cv.next_results = 0x40000;
  c[i].inter_cv.reloj = 0;
  c[i].inter_cv.sumW = 0;
  c[i].inter_cv.sumTTRT = 0;
  c[i].inter_cv.num_rot = 0;
  c[i].inter_cv.sumSis = 0;
  c[i].inter_cv.num_samples = 0;
  c[i].inter_cv.cs = NULL;
  c[i].inter_cv.cs_remaining = 0;
  c[i].inter_cv.next = NULL;
  c[i].inter_cv.c = i; 
  c[i].sumSis = 0;
  c[i].num_samples = 0;
  c[i].sumW = 0;

  c[i].retardo = NULL;
  c[i].perdidas = NULL;
  c[i].r_pkt_num = c[i].r_frame_num = c[i].s_frame_num =
    c[i].r_well_num = c[i].left_size = c[i].seq_num = 0;
  c[i].s_pkt_num = -1;

  c[i].ak_head = NORMAL_HEAD_SIZE;
  c[i].xak_head = NORMAL_HEAD_SIZE;
  c[i].dt_head = NORMAL_HEAD_SIZE;
  c[i].xdt_head = NORMAL_HEAD_SIZE;
  c[i].command_cnt = 0;
  c[i].command_dist = NULL;
}

/*--------------------------------------------------------------------------*/
/*Data specified in traffic definition in the input parameter file
  is loaded into user(s) IDU list(s)
  */
static int init_data(int fd, char *string)
{
  int cid, end1, end2, cnt;

  sscanf(string, "%d %d %d %d", &cid, &end1, &end2, &cnt);

  if (cid != no_conns)
    {
      fprintf(efp, "Numero de conexion no introducido"
	      " correlativamente(0...)\n");
      return(ERROR);
    }

  if(c == NULL)
    {
      if((c=malloc(sizeof(struct conn_info)))==NULL)
	{
	  fprintf(efp, "Conexion 0: insuficiente memoria HEAP para"
		  " la matriz de conexiones\n");
	  return(ERROR);
	}
    }
  else
    if((c=(struct conn_info *)realloc(c,sizeof(struct conn_info)*(cid + 1)))
       ==NULL)
      {
	fprintf(efp, "Conexion %d insuficiente memoria HEAP para"
		" la matriz de conexiones\n",cid);
	return(ERROR);
      }
  init_conns(cid);
  
  c[cid].c_end1 = end1;
  c[cid].c_end2 = end2;
  
  if (read_msg_line(fd, &c[cid].tgen) < 0)
    return (ERROR);
  
  init_fields(&c[cid].tgen, cid, end1, end2, DT);
  
  if (cnt != 0)
    c[cid].tgen.tsdu_cnt = cnt;
  return NONE;
}

/*--------------------------------------------------------------------------*/
static void init_cpu(int j)
{
  int i;
  
  for (i = 0; i < j+1; i++)
    cpus[i] = 0.0;
}

/*--------------------------------------------------------------------------*/
static void init_stats()
{
  int i, j;
  
  for (j = 0; j < no_conns; j++)
    {
      stats[j].rxdts = 0;
      stats[j].oxdts = 0;
      stats[j].aks = 0;
      stats[j].rdts = 0;
      stats[j].odts = 0;
      stats[j].xaks = 0;
      stats[j].ak_bits = 0;
      stats[j].xak_bits = 0;
      stats[j].dt_bits = 0;
      stats[j].xdt_bits = 0;
      stats[j].nhdr_bits = 0;
      stats[j].info_bits = 0;
      stats[j].net_messages = 0;
      stats[j].retrans_max = 0;
      stats[j].dt_errors = 0;
      stats[j].xdt_errors = 0;
      stats[j].ak_errors = 0;
      stats[j].xak_errors = 0;
      stats[j].dt_drops = 0;
      stats[j].xdt_drops = 0;
      stats[j].ak_drops = 0;
      stats[j].xak_drops = 0;
      stats[j].release_waits = 0;
      stats[j].tx_waits = 0;
      stats[j].normal_waits = 0;
      stats[j].ndata_bits = 0;
      stats[j].snhdr_bits = 0;
      stats[j].subnet_messages = 0;
      stats[j].sndata_bits = 0;
      
      for (i = 0; i <= BUCKETS; i++)
	stats[j].histo_retrans[i] = 0;
    }
}

/*--------------------------------------------------------------------------*/
static void init_stat_anal()
{
  int i;
  
  for (i = 0; i < no_of_nets; i++)
    s[i].ttrt = NULL;
}

/*--------------------------------------------------------------------------*/
/* Find the lowest connection id # which is defined                         */
static void find_start_cid()
{
  int i;
  for (i = 0; i < no_conns; i++)
    if ((c[i].c_end1 != -1) && (c[i].c_end2 != -1))
      {
	start_cid = i;
	return;
      }
}

/*--------------------------------------------------------------------------*/
static void inic_retardo(int temp, double ftemp, double ftemp2, double ftemp3,
			 int temp2)
{
  char buf2[LINELENGTH]="";

  sprintf(buf2, "retardo%d", temp);

  if (temp2 == 4)
    c[temp].retardo = inic_promedio_dif(ftemp, ftemp2, ftemp3);
  else
    if (!CV_ASINC && !CV_S && !CV_U)
      {
	if (files)
	  c[temp].retardo = inic_promedio_files(ftemp, ftemp2,
						buf2);
	else
	  c[temp].retardo = inic_promedio(ftemp, ftemp2);
      }
    else
      {
	c[temp].retardo = (promedio) !NULL; /* la inicializacion se
					       har'a en fast.c */
	c[temp].calidad = ftemp;
	c[temp].tolerancia = ftemp2;
	if (files)
	  strcpy(c[temp].nombre_fich, buf2);
      }
}



/*--------------------------------------------------------------------------*/
static int read_params(int fildes)
{
  char buf[LINELENGTH];
  int end_of_data, data_init, i, j;
  int found, value, net_no, media_no;
  int conn, count, itemp, itemp2;
  int media_num[MAX_NO_MEDIA];
  long int temp, temp2, temp3, temp4;
  double ftemp, ftemp1, ftemp2, ftemp3, ftemp4, ftemp5, ftemp6, ftemp7, ftemp8;
  char ctemp;
  end_of_data = data_init = FALSE;
  found = FALSE;
  
  no_of_nets = no_of_media = 0;
  s=NULL;
  /*which users interface to which tps is defined in the 
    input parameter file */

  while (end_of_data == FALSE)
    {
      
      if (get_a_line(fildes, buf) == EOF)
	{
	  fprintf(efp,"Parametros erroneos en la relacion de"
		  " network y FDDI con el medio. \n");
	  return (ERROR);
	}
      
      /* # is the signal for end of relationship list in param file */
      if (buf[0] != '#')
	{
	  sscanf(buf, "%d %d", &net_no, &value);
	  if(net_no != no_of_nets && !(net_no == -1 && no_of_nets == 0))
	    {
	      fprintf(efp, "Numero de net no introducido"
		      " correlativamente(0...) u opcion -1 no en la"
		      "primera linea\n");
	      return(ERROR);
	    }
	  if (net_no == -1)
	    {
	      no_of_nets =  value;
	      value = 0;
	      for (j = 0; j < no_of_media; j++)
		if (media_num[j] == value)
		  found = TRUE;
	      if (found == FALSE)
		{
		  if (no_of_media >= MAX_NO_MEDIA)
		    {
		      fprintf(efp, "medias exceed max number of medias\n");
		      return (ERROR);
		    }
		  media_num[no_of_media] = value;
		  no_of_media++;
		}
	      else
		found = FALSE;

	      if((s=(struct subnet_info *)
		  malloc(sizeof(struct subnet_info)*no_of_nets))==NULL)
		{
		  fprintf(efp, "Nodo %d insuficiente memoria HEAP para"
			  " la matriz de subnets\n",net_no);
		  return(ERROR);
		}

	      for (net_no = 0; net_no < no_of_nets; net_no++)
		s[net_no].media_interface = value;
	      if (get_a_line(fildes,buf) == EOF || *buf != '#')
		{
		  fprintf(efp, "param file ends after subnet list"
			  " specification, or -1 not only line\n");
		  return (ERROR);
		}
	      end_of_data = TRUE;
	    }
	  else
	    {
	      if(s == NULL)
		{
		  if((s=malloc(sizeof(struct subnet_info)))==NULL)
		    {
		      fprintf(efp, "Nodo 0: insuficiente memoria HEAP para"
			      " la matriz de subnets\n");
		      return(ERROR);
		    }
		}
	      else
		if((s=(struct subnet_info *)
		    realloc(s,sizeof(struct subnet_info)*(net_no + 1)))==NULL)
		  {
		    fprintf(efp, "Nodo %d insuficiente memoria HEAP para"
			    " la matriz de subnets\n",net_no);
		    return(ERROR);
		  }
	  
	      s[net_no].media_interface = value;
	      no_of_nets++;
	      for (j = 0; j < no_of_media; j++)
		if (media_num[j] == value)
		  found = TRUE;
	      if (found == FALSE)
		{
		  if (no_of_media >= MAX_NO_MEDIA)
		    {
		      fprintf(efp, "medias exceed max number of medias\n");
		      return (ERROR);
		    }
		  media_num[no_of_media] = value;
		  no_of_media++;
		}
	      else
		found = FALSE;
	    }
	}
      else
	{
	  if (no_of_nets == 0)
	    {
	      fprintf(efp, "no net-media relations specified\n");
	      return (ERROR);
	    }
	  end_of_data = TRUE;
	}
    }

  for (net_no = 0; net_no < no_of_nets; net_no++)
    s[net_no].subnet_number = net_no;
  
  if((n=malloc(sizeof(struct net_info)*(net_no + 1)))==NULL)
    {
      fprintf(efp, "insuficiente memoria HEAP para la matriz de nets[%d]\n",
	      net_no);
      return(ERROR);
    }
  
  if((clocks=malloc(sizeof(double) * 2 * no_of_nets * 2))==NULL)
    {
      fprintf(efp, "Clocks: insuficiente memoria HEAP para"
	      " la matriz de clocks[%d]\n", value);
      return(ERROR);
    } 
  no_clocks = 0;
  
  for (i = 0; i < no_of_nets; i++)
    {
      if (get_a_line(fildes, buf) == EOF)
	{
	  fprintf(efp, "param file ends after tp-clock relationship\n");
	  return (ERROR);
	}
      
      sscanf(buf, "%d %d", &net_no, &value);
      
      /* net clock point to clock # in clock array */
      /*
	SWAR SES #5 n[net_no].clock_ptr = &clocks[value];
	*/

      if (net_no == -1)
	if (no_clocks != 0)
	  {
	    fprintf(efp, "net-clock specification: "
		    "option -1 only valid at first line\n");
	    return (ERROR);
	  }
	else
	  {
	    no_clocks = no_of_nets;
	    for (net_no = 0; net_no < no_of_nets; net_no++)
	      {
		n[net_no].clock_ptr = &clocks[net_no][0];
		n[net_no].clk_index = net_no;
		*n[net_no].clock_ptr = INFINITY;
	      }
	    break;
	  }

      if(value > no_clocks)
	{
	  fprintf(efp, "Numero de reloj no introducido"
		  " correlativamente(0...)\n");
	  return(ERROR);
	}
      no_clocks = value + 1;
      
      n[net_no].clock_ptr = &clocks[value][0];
      
      n[net_no].clk_index = value;
      *n[net_no].clock_ptr = INFINITY;
    }
  
  if((cpus=malloc(sizeof(double)*(value+1)))==NULL)
    {
      fprintf(efp, "insuficiente memoria HEAP para la matriz de cpus\n");
      return(ERROR);
    }
  
  /* initialize the CPU utilization measurement areas */
  init_cpu(value);
  
  /* subnet clock #'s */
  for (i = 0; i < no_of_nets; i++)
    {
      if (get_a_line(fildes, buf) == EOF)
	{
	  fprintf(efp, "param file ends after net-clock relationship\n");
	  return (ERROR);
	}
      sscanf(buf, "%d %d %lf", &net_no, &value, &ftemp);
      
      /* subnet# is same as net # */
      /* subnet clock pointer points to clock in clock array */
      /*
	SWAR SES #5 s[net_no].clock_ptr = &clocks[value];
	*/

      if (net_no == -1)
	if (value != -1 && i != 0)
	  {
	    fprintf(efp, "subnet-clock specification: "
		    "option -1 -1 only valid at first line\n");
	    return (ERROR);
	  }
	else
	  {
	    for (net_no = 0; net_no < no_of_nets; net_no++)
	      {
		s[net_no].clock_ptr = &clocks[net_no][0];
		/* added SES #5 */ clocks[net_no][1] = ftemp * 1000;
		s[net_no].clk_index = net_no;
		*s[net_no].clock_ptr = INFINITY;
	      }
	    break;
	  }

      s[net_no].clock_ptr = &clocks[value][0];
      /* added SES #5 */ clocks[value][1] = ftemp * 1000;
      s[net_no].clk_index = value;
      *s[net_no].clock_ptr = INFINITY;
    }

  /* Objective number and Experiment number */
  if (get_a_line(fildes, buf) == EOF)
    {
      fprintf(efp, "param file ends after tp description\n");
      return (ERROR);
    }
  sscanf(buf, "%ld %d", &temp, &value);
  
  /*-----------DATA TRAFFIC SPECIFICATION-----------------------------------*/
  no_conns = 0;
  c = NULL;
  end_of_data = FALSE;
  while (end_of_data == FALSE)
    {
      if (get_a_line(fildes, buf) == EOF)
	{
	  fprintf(efp, "param file ends before end of traffic data found\n");
	  return (ERROR);
	}
      
      /* # signals end of traffic specification */
      if (buf[0] != '#')
	{
	  if (init_data(fildes, buf) < 0)
	    return (ERROR);
	  data_init = TRUE;
	  no_conns++;
	}
      else
	{
	  if (data_init == FALSE)
	    {
	      fprintf(efp, "no traffic specified\n");
	      return (ERROR);
	    }
	  end_of_data = TRUE;
	}
    }

  first_waitingCV = NULL;
  Inter_CV_file = 0;

  if (inter_est)
    {
      sprintf(buf,"/tmp/inter_results%d.log",inter_est);
      if ((Inter_CV_file = open(buf, O_WRONLY|O_CREAT|O_EXCL, 00644)) == -1)
	{
	  fprintf(efp, "cannot open file %s for output\n", buf);
	  exit(1);
	}
      faltan = no_conns;
      for (i = 0; i<no_conns; i++)
	{
	  if ((c[i].inter_cv.cs = malloc(no_conns*sizeof(int))) == NULL)
	    {
	      fprintf(efp, "insuficiente memoria HEAP para la matriz stat\n");
	      return(ERROR);
	    }
	}
    }

  if((stats=malloc(sizeof(struct measures)*(no_conns)))==NULL)
    {
      fprintf(efp, "insuficiente memoria HEAP para la matriz stat\n");
      return(ERROR);
    }

  /* initialize the event statistics arrays */
  init_stats();
  
  /* initialize the statistic analysis */
  init_stat_anal();

  find_start_cid();

  /* Parameters for statistic analysis */
  /* Nodo TTRT */
  inic_parametros(&sim_clock, file_limit, 0, efp);
  
  if (get_a_line(fildes, buf) == EOF)
    {
      fprintf(efp, "param file doesn't specificate parameters for"
	      " statistic analysis\n");
      return (ERROR);
    }
  if (buf[0] != '#')
    {
      sscanf(buf, "%d %lf %lf", &net_no, &ftemp, &ftemp2);
      if (net_no > no_of_nets + 1)
	{
	  fprintf(efp, "param file has a bad TTRT node\n");
	  return (ERROR);
	}
      if (!anal_est)
	ftemp = 0.0;
      if (files)
	s[net_no].ttrt = inic_promedio_files(ftemp, ftemp2, "trt");
      else
	s[net_no].ttrt = inic_promedio(ftemp, ftemp2);
    }
  
  /* Conexiones de medida de probabilidad de perdida de paquetes */
  end_of_data = FALSE;
  while (end_of_data == FALSE)
    {
      if (get_a_line(fildes, buf) == EOF)
	{
	  fprintf(efp, "fichero de entrada termina antes de especificar"
		  " probabilidad de perdida de conexiones\n");
	  return (ERROR);
	}
      if (buf[0] != '#')
	{
	  temp2 = sscanf(buf, "%ld %lf %lf %lf", &temp, &ftemp, &ftemp2,
			 &ftemp3);
	  if (temp > start_cid + no_conns - 1 || c[temp].perdidas != NULL)
	    {
	      fprintf(efp, "conexion de medida de perdidas inexistente"
		      " o repetido\n");
	      return (ERROR);
	    }
	    
	  /*	    for (aux = buf; isdigit(*aux); aux++);
	   *aux = (char) 0;
	   strcpy(buf2, "perdidas");
	   strcat(buf2, buf);
	    
	   if (files)
	   s[net_no].llenado = inic_promedio_files(0.0, 0.1, buf2);
	   else */
	  if (temp2 == 4)
	    c[temp].perdidas = inic_promedio_dif(ftemp, ftemp2, ftemp3);
	  else
	    c[temp].perdidas = inic_promedio(ftemp, ftemp2);
	}
      else
	end_of_data = TRUE;
    }
  /* Conexiones de medida de retardo */
  end_of_data = FALSE;
  count = 0;
  while (end_of_data == FALSE)
    {
      if (get_a_line(fildes, buf) == EOF)
	{
	  fprintf(efp, "fichero de entrada termina antes de especificar"
		  " conexiones de retardo\n");
	  return (ERROR);
	}
      if (buf[0] != '#')
	{
	  count++;
	  temp2 = sscanf(buf, "%ld %lf %lf %lf", &temp, &ftemp, &ftemp2,
			 &ftemp3);
	  if (!anal_est)
	    ftemp = 0.0;

	  if (temp == -1)
	    if (count != 1)
	      {
		fprintf(efp, "mean delay measures specification: "
			"option -1 only valid at first line\n");
		return (ERROR);
	      }
	    else
	      {
		for (temp = start_cid; temp < start_cid + no_conns; temp++)
		  inic_retardo(temp, ftemp, ftemp2, ftemp3, temp2);
		if (get_a_line(fildes,buf) == EOF || *buf != '#')
		  {
		    fprintf(efp, "param file ends after delay measures"
			    " specification, or -1 not only line\n");
		    return (ERROR);
		  }
		end_of_data = TRUE;
	      }
	  else
	    {
	      if (temp > start_cid + no_conns - 1 || c[temp].retardo != NULL)
		{
		  fprintf(efp, "conexion de retardo inexistente o repetida\n");
		  return (ERROR);
		}
	      inic_retardo(temp, ftemp, ftemp2, ftemp3, temp2);
	    }
	}
      else
	end_of_data = TRUE;
    }

  /* media description */
  
  for (i = 0; i < no_of_media; i++)
    {
      if (get_a_line(fildes, buf) == EOF)
	{
	  fprintf(efp, "param file ends after traffic data\n");
	  return (ERROR);
	}
      sscanf(buf, "%d %c %lf %lf %ld %d", &media_no, &ctemp, &ftemp, &ftemp2,
	     &temp, &value);
      m[media_no].error_rate = ftemp;
      m[media_no].prop_delay = ftemp2;
      m[media_no].link_speed = temp / (double) MILLISEC;
      m[media_no].crc_on = value;
      
      
      /* Read in remainder based upon medium type */
      
      switch (ctemp)
	{
	case 'L':
	case 'l':
	  m[media_no].medium_type = LINK;
	  break;
	case 'C':
	case 'c':
	  m[media_no].medium_type = CONTENTION;
	  break; 
	case 'T':
	case 't':
	  m[media_no].medium_type = TOKEN;
	  break;
	case 'F':
	case 'f':
	  /* latency and token length */
	  /* frame preamble length */
	  
	  if (get_a_line(fildes, buf) == EOF)
	    {
	      fprintf(efp, "param file ends after first line of medium %d\n",
		      media_no);
	      return (ERROR);
	    }
	  sscanf(buf, "%lf %ld %ld %lf %ld %ld", &ftemp, &temp, &temp2,
		 &ftemp2, &temp3, &temp4);
	  if (TTRT == 0.0) TTRT = ftemp2;
	  if(fddi_ii)
	    {
	      if (temp3 < 0)
		{
		  fprintf(efp, "Numero de llamadas debe ser > 0.\n");
		  return(ERROR);
		}
	      m[media_no].no_of_calls = temp3;
	      if (temp4 < 0 || temp4 >= no_of_nets)
		{
		  fprintf(efp, "Estacion maestra invalida.\n");
		  return(ERROR);
		}
	      m[media_no].master = temp4;    
	    }
	  
	  m[media_no].fddi_head_end_latency = ftemp;
	  m[media_no].preamble = temp;
	  m[media_no].token_length = temp2 * 8
	    / m[media_no].link_speed;
	  /* Determine # of subnets on this medium */
	  
	  count = 0;
	  for (j = 0; j < no_of_nets; j++)
	    if (s[j].media_interface == media_no)
	      count++;
	  	  
	  /* subnet characteristics */
	  
	  for (j = 0; j < count; j++)
	    {
	      if (get_a_line(fildes, buf) == EOF)
		{
		  fprintf(efp, "param file ends after FDDI SPECIFIC"
			  " PARAMETERS\n");
		  return (ERROR);
		}
	      sscanf(buf, "%d %d %lf", &net_no,
		     &value, &ftemp);

	      if(get_a_line(fildes,buf) == EOF)
		{
		  fprintf(efp,"param file ends after FDDI number of"
			  " stations\n");
		  return(ERROR);
		}

	      sscanf(buf,"%lf %lf %lf %lf %lf %lf %lf %lf", 
		     &ftemp8, 
		     &ftemp7, &ftemp6, &ftemp5,
		     &ftemp4, &ftemp3, &ftemp2, &ftemp1);

	      if ((TTRT<ftemp7) || (ftemp7<ftemp6) || (ftemp6<ftemp5) ||
		  (ftemp5<ftemp4) || (ftemp4<ftemp3) || (ftemp3<ftemp2) ||
		  (ftemp2<ftemp))
		{
		  fprintf(efp,"Un temporizador de determinada prioridad no\n"
			  "puede contener un valor mayor, que otro de mayor\n"
			  );
		  fprintf(efp,"prioridad\n");
		  return(ERROR);
		}
	      
	      if (net_no == -1)
		if (value != -1 || count != no_of_nets)
		  {
		    fprintf(efp, "FDDI priority thresholds: "
			    "option -1 -1 not valid\n");
		    return (ERROR);
		  }	  
		else
		  {
		    for (net_no = 0; net_no < no_of_nets; net_no++)
		      {
			s[net_no].logical_next_station = net_no+1;
			s[net_no].token_proc_time = ftemp;

			s[net_no].target_time_syn = ftemp8;
			s[net_no].target_time[6] = ftemp7;
			s[net_no].target_time[5] = ftemp6;
			s[net_no].target_time[4] = ftemp5;
			s[net_no].target_time[3] = ftemp4;
			s[net_no].target_time[2] = ftemp3;
			s[net_no].target_time[1] = ftemp2;
			s[net_no].target_time[0] = ftemp1;
		      }
		    s[no_of_nets-1].logical_next_station = 0;
		    break;
		  }

	      s[net_no].logical_next_station = value;
	      s[net_no].token_proc_time = ftemp;

	      s[net_no].target_time_syn = ftemp8;
	      s[net_no].target_time[6] = ftemp7;
	      s[net_no].target_time[5] = ftemp6;
	      s[net_no].target_time[4] = ftemp5;
	      s[net_no].target_time[3] = ftemp4;
	      s[net_no].target_time[2] = ftemp3;
	      s[net_no].target_time[1] = ftemp2;
	      s[net_no].target_time[0] = ftemp1;
	    }

	  /* limits of size of messages for SIZE_SELECT */
	  /*
	    if(get_a_line(fildes,buf) == EOF) { fprintf(efp,"param file ends
	    after TRT's\n"); return(ERROR); } for (pri=NO_LEVELS_PRI;
	    pri>=0; pri--) { sscanf(buf,"%d", &value);
	    limit_size[pri]=value; }
	    */
	  
	  if(fddi_ii)
	    m[media_no].medium_type = HYBRID_RING;
	  else
	    m[media_no].medium_type = TOKEN_HI_SPEED;
	  break;
	}
    }

  

  /* event priorities */
  for (i = 0; i < no_of_nets; i++)
    {
      n[i].prio_tab[0] = 1;
      n[i].prio_tab[1] = 1;
    }
  
  if (read_priorities(fildes) == ERROR)
    return(ERROR);
  
  /* subnet send and receive buffer space and header size */
  
  for (i = 0; i < no_of_nets; i++)
    {
      if (get_a_line(fildes, buf) == EOF)
	{
	  fprintf(efp, "param file ends after net send q bound\n");
	  return (ERROR);
	}
      sscanf(buf, "%d %d %d %d", &net_no, &value, &itemp, &itemp2);
      if (net_no != i && !(net_no == -1 && i == 0))
	{
	  fprintf(efp, "send and receive buffers: "
		  "subnets must be in order and option -1"
		  " only valid at first line\n");
	  return (ERROR);
	}
      if (net_no == -1)
	{
	  for (net_no = 0; net_no < no_of_nets; net_no++)
	    {
	      s[net_no].send_buf_bound = value;
	      s[net_no].rec_buf_bound = itemp;
	      s[net_no].subnet_head = itemp2;
	      s[net_no].max_size = 4500 - itemp2;
	    }
	  break;
	}
      else
	{
	  s[net_no].send_buf_bound = value;
	  s[net_no].rec_buf_bound = itemp;
	  s[net_no].subnet_head = itemp2;
	  s[net_no].max_size = 4500 - itemp2;
	}
    }
  
  /* connection information */
  
  for (i = 0; i < no_conns; i++)
    {
      if (get_a_line(fildes, buf) == EOF)
	{
	  fprintf(efp, "param file ends after net send & rec buf space\n");
	  return (ERROR);
	}
      sscanf(buf, "%d %d", &conn, &value);
      if (conn != i && !(conn == -1 && i == 0))
	{
	  fprintf(efp, "connection qos: "
		  "connections must be in order and option -1"
		  " only valid at first line\n");
	  return (ERROR);
	}
      if (conn == -1)
	{
	  for (conn = 0; conn < no_conns; conn++)
	    c[conn].qos = value;
	  break;
	}
      else
	c[conn].qos = value;
    }

  /* subnet write time per octet to net-sn interface */
  
  for (i = 0; i < no_of_nets; i++)
    {
      if (get_a_line(fildes, buf) == EOF)
	{
	  fprintf(efp, "param file ends after net proc receive msg time\n");
	  return (ERROR);
	}
      sscanf(buf, "%d %lf", &net_no, &ftemp);
      if (net_no != i && !(net_no == -1 && i == 0))
	{
	  fprintf(efp, "subnet write time: "
		  "subnets must be in order and option -1"
		  " only valid at first line\n");
	  return (ERROR);
	}
      if (net_no == -1)
	{
	  for (net_no = 0; net_no < no_of_nets; net_no++)
	    /* SWAR SES #5 */
	    s[net_no].write_time = ftemp / clocks[s[net_no].clk_index][1];
	  break;
	}
      else
	s[net_no].write_time = ftemp / clocks[s[net_no].clk_index][1];
    }

  /* subnet read and buffer time per octet from net-sn interface */

  for (i = 0; i < no_of_nets; i++)
    {
      if (get_a_line(fildes, buf) == EOF)
	{
	  fprintf(efp, "param file ends after sn write time\n");
	  return (ERROR);
	}
      sscanf(buf, "%d %lf", &net_no, &ftemp);
      if (net_no != i && !(net_no == -1 && i == 0))
	{
	  fprintf(efp, "subnet read and buffer time: "
		  "subnets must be in order and option -1"
		  " only valid at first line\n");
	  return (ERROR);
	}
      if (net_no == -1)
	{
	  for (net_no = 0; net_no < no_of_nets; net_no++)
	    /* SWAR SES #5 */
	    s[net_no].read_buf_time = 
	      ftemp / clocks[s[net_no].clk_index][1];
	  break;
	}
      else
	s[net_no].read_buf_time = ftemp / clocks[s[net_no].clk_index][1];
    }

  /* subnet process send message time */

  for (i = 0; i < no_of_nets; i++)
    {
      if (get_a_line(fildes, buf) == EOF)
	{
	  fprintf(efp, "param file ends after sn read & buf time\n");
	  return (ERROR);
	}
      sscanf(buf, "%d %lf", &net_no, &ftemp);
      if (net_no != i && !(net_no == -1 && i == 0))
	{
	  fprintf(efp, "subnet process send time: "
		  "subnets must be in order and option -1"
		  " only valid at first line\n");
	  return (ERROR);
	}
      if (net_no == -1)
	{
	  for (net_no = 0; net_no < no_of_nets; net_no++)
	    /* SWAR SES #5 */
	    s[net_no].proc_send_time = ftemp / clocks[s[net_no].clk_index][1];
	  break;
	}
      else
	s[net_no].proc_send_time = ftemp / clocks[s[net_no].clk_index][1];
    }
  
  /* subnet process receive message time */
  
  for (i = 0; i < no_of_nets; i++)
    {
      if (get_a_line(fildes, buf) == EOF)
	{
	  fprintf(efp, "param file ends after subnet proc send msg time\n");
	  return (ERROR);
	}
      sscanf(buf, "%d %lf", &net_no, &ftemp);
      if (net_no != i && !(net_no == -1 && i == 0))
	{
	  fprintf(efp, "subnet process receive time: "
		  "subnets must be in order and option -1"
		  " only valid at first line\n");
	  return (ERROR);
	}
      if (net_no == -1)
	{
	  for (net_no = 0; net_no < no_of_nets; net_no++)
	    /* SWAR SES #5 */
	    s[net_no].proc_recv_time =
	      ftemp / clocks[s[net_no].clk_index][1];
	  break;
	}
      else
	s[net_no].proc_recv_time =
	  ftemp / clocks[s[net_no].clk_index][1];
    }
  
  /* everything read in properly */
  
  return (TRUE);
}

/*--------------------------------------------------------------------------*/
/* Inicializa el numero de nodos entre los extremos de una conexion         */
static void init_conns_2()
{
  int conn, cont, end1, end2;
  
  for (conn = 0; conn < no_conns; conn++)
    {
      end1 = c[conn].c_end1;
      end2 = c[conn].c_end2;
      for (cont = 0; end1 != end2; 
	   end1 = s[end1].logical_next_station, cont++);
      c[conn].between = cont;
    }
}

/*--------------------------------------------------------------------------*/
/* User event queue is initialized.                                         */
static void init_event()
{
  int i;
  int connect_id;
  double ftemp;

  primer_msg = INFINITY;
  for (i = 0; i < no_conns; i++)
    {
      connect_id = start_cid + i;
      ftemp = Q_msg(c[connect_id].c_end1, connect_id, &c[connect_id].tgen);
      if (ftemp < primer_msg)
	primer_msg = ftemp;
      /* for (i=0; i < no_of_nets; i++) if(n[i].event_q_head != NULL)
	 n[i].event_q_head->xtime = 0.0;	*/
    }
  tkn.flags = 1;
  if(fddi_ii)
    i = m[0].master;
  else
    /* determine which subnet gets the token */
    for (i = 0; i < no_of_nets - 1; i++)	/* SES */
      if (bernoulli(1.0 / no_of_nets))
	break;
  /* schedule receipt of token by subnet */
  schedule(SNT, EVENT3, 0.0, &tkn, i);
}

/*--------------------------------------------------------------------------*/
static void init_net()
{
  int i, j;
  /* set default/starting values for network parameters */
  
  for (i = 0; i < no_of_nets; i++)
    {
      n[i].event_q_head = NULL;
      n[i].enable = TRUE;
      n[i].net_from_sn_intf = 0;
      n[i].s_interface_free = 0.0;
      for (j = 0; j < MAX_PRIORITY; j++)
	{
	  n[i].n_queue[j].head = NULL;
	  n[i].n_queue[j].end = NULL;
	}
    }
}

/*--------------------------------------------------------------------------*/
static void init_subnets()
{
  int i, j;
  /* set default/starting values for subnets */
  
  for (i = 0; i < no_of_nets; i++)
    {
      s[i].enable = TRUE;
      s[i].event_q_head = NULL;
      s[i].sn_from_net_intf = 0;
      s[i].no_net_msg_sched = TRUE;
      s[i].order = 0;
      
      s[i].interface_free = 0.0;
      
      s[i].send_buf_size = 0;
      s[i].rec_buf_size = 0;
      s[i].mem_drops = 0;
      s[i].crc_drops = 0;
      
      for (j = 0; j < MAX_PRIORITY; j++)
	{
	  s[i].sn_queue[j].head = NULL;
	  s[i].sn_queue[j].end = NULL;
	}
      
#ifdef SATELLITE
      s[i].Rx_Q.head = NULL;
      s[i].Rx_Q.end = NULL;
#endif
      
      s[i].tokens = 0;
      s[i].last_visit = 0.0;
      s[i].Rx_Q.head = NULL;
      s[i].Rx_Q.end = NULL;
      s[i].Rx_Q.dt_start = NULL;
      
      for (j = 0; j <= NO_LEVELS_PRI; j++)
	{
	  s[i].Tx_Q[j].head = NULL;
	  s[i].Tx_Q[j].end = NULL;
	  s[i].Tx_Q[j].dt_start = NULL;
	  s[i].last_token_time[j] = 0.0;
	}
    }
}

/*--------------------------------------------------------------------------*/
static void init_media()
{
  int i;

  for (i = 0; i < no_of_media; i++)
    {
      m[i].byte_ok_prob = pow((1.0 - m[i].error_rate), 8.0);
      
      m[i].head = NULL;
      m[i].tail = NULL;
      
      /* Para FDDI-II  */
      
      m[i].packet_bandwidth = 0.0;
      m[i].iso_bandwidth = 0.0;
      m[i].header_bandwidth = 0.0;
      m[i].current_link_speed = 0.0;
      m[i].channels = 0;
      m[i].master_delay = 0.0;
      
    }
}

/*--------------------------------------------------------------------------*/
void init(char *file)
{
  int fd;
  char cad[200];

  if ((fd = open(file, 0)) < 0)
    {
      sprintf(cad, "bad parameter filename %s", file);
      p_error(cad);
    }
  /* read in the parameters */

  if (read_params(fd) < 0)
    {
      close(fd);
      p_error("error reading parameter file");
    }
  close(fd);
  /* init wall clock */
  sim_clock = 0.0;
  metric_clock = 0.0;
  dead_clock = 0.0;
  
  /* Number of TSDUs received set to zero */
  tsdus_rcvd = 0;
  
  /* Set initial execution status */
  octs_rcvd = 0;
  evts_exec = 0;
  retrans = 0;
  xpd_cnt = 0;
  err_cnt = 0;
  
  modactivity = FALSE;

  /* init tp and network structures with default values */
  init_net();
  init_subnets();
  init_media();
  /* init connections */
  init_conns_2();
  /* init event queues for those users sending data */

  if (!fast && !gg1)
    init_event();
  
  /* Allocate PS & CS bandwidth for FDDI-II*/
  if(fddi_ii)
    channel_allocator(m, no_of_nets);
}
