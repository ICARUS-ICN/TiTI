#include "defs.h"
#include "globals.h"
#include "types.h"
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>

char GG1[] = "%W% %G%";
/*
Sobre una configuración con una sóla conexión, procede sin paso de testigo y
simula una cola G/G/1 a secas
 */

static void gg1_summary(const char *fname, FILE *mfp)
{
  struct tms csecs;
  float cpu_secs;
  int i;

  fprintf(mfp, "Comienzo de la ejecucion:\t%s", ctime(&wtime));
  times(&csecs);
  print_elapsed(mfp);

  cpu_secs = ((float)((csecs.tms_utime + csecs.tms_stime) -
                      (start_cpu.tms_utime + start_cpu.tms_stime))) /
             HZ;

  fprintf(mfp, "%10.3f segundos de CPU usados\n", cpu_secs + .0005);
  fprintf(mfp, "%10.3f segundos simulados\n", sim_clock / MILLISEC + .0005);
  fprintf(mfp, "%10.0f octetos recibidos\n\n", m[0].byte_count);

  fprintf(mfp, "\tSemilla: %ld\n", seed);
  if (anal_est)
    fprintf(mfp, "\tTransitorio: %d ms\n", clk_limit);
  fprintf(mfp, "\tFichero de Entrada: %s\n", fname);
  fprintf(mfp, "\n\t Version rapida del simulador para G/G/1: 1 nodo, 1 medio\n");
  fprintf(mfp, "\nMedium Throughput: %.3f bps.\n",
          ((m[0].byte_count * 8) / sim_clock) * MILLISEC);
  fprintf(mfp, "\n");
}

static void gg1_imprime(const char *nombre_fichero, const char *fn)
{
  FILE *mfp;
  int i;

  mfp = fopen(nombre_fichero, "w");

  /* print a summary of the run */
  gg1_summary(fn, mfp);

  fprintf(mfp, "\n\n*** ESTADISTICAS DE TIEMPO DE ESPERA EN COLA (ms) ***\n");
  if (c[0].retardo != NULL)
  {
    fprintf(mfp, "\n\nCONEXION\n");
    promedios_stats(c[0].retardo, mfp);
  }

  fprintf(mfp, "\n\nMEDIA AGREGADA\n");
  promedios_stats(NULL, mfp);

  fclose(mfp);
}

static double ControlAsinc(struct distrib *p)
{
  switch (p->type)
  {
  case UNIFORM:
    return (p->value1 + p->value2) / 2;

  case NORMAL:
    return p->value1;

  case POISSON:
    return p->value1;

  case EXPONENTIAL:
    return p->value1;

  case GAMMA:
    return p->value1 * p->value2;

  case CONSTANT:
    return p->value1;

  case TRIANGULAR:
  case TABULAR:
  case VELOCIDAD_UNIFORME:
  default:
    p_error("Control Variates asincrono no soportado para esta distribucion\n");
    return (ERROR);
  }
}

double S;

static void gg1_init(void)
{
  struct subnet_info *sn;
  struct conn_info *conn;
  struct media_info *media;
  int i, j, k;

  double ciclo, utilizacion, lambda, aux, aux2;

  if (no_of_nets != 1 || no_conns != 1)
  {
    p_error("Sólo G/G/1: ha de haber un nodo y una conexión.");
  }
  sn = &s[0];
  sn->min_ex_event = INFINITY;
  conn = &c[0];
  /* Calculo del tiempo medio desocupado por paquete */
  if (CV_ASINC)
  {
    aux = ControlAsinc(conn->tgen.msg);
    aux2 = ControlAsinc(conn->tgen.arrive);
    aux /= aux2;
    utilizacion = aux;
    lambda = 1 / aux2;
    if (utilizacion >= 1)
    {
      p_error("Utilización mayor o igual que 1.0: en G/G/1 distribución de S y no de L");
    }
    ciclo = (1 - utilizacion) / lambda;
  }

  /* Sólo habrá una qos por haber sólo una conexión */
  sn->clases[conn->qos] = (struct trafico *)malloc(sizeof(struct trafico));
  sn->clases[conn->qos]->msg = conn->tgen.msg;
  sn->clases[conn->qos]->arrive = conn->tgen.arrive;

  sn->clases[conn->qos]->inter_cv = conn->inter_cv;
  sn->clases[conn->qos]->sumSis = conn->sumSis;
  sn->clases[conn->qos]->num_samples = conn->num_samples;

  if (CV_ASINC && conn->retardo != NULL)
    if (files)
      conn->retardo =
          inic_promedioCVasinc_files(conn->calidad, conn->tolerancia,
                                     ControlAsinc(conn->tgen.arrive),
                                     (1 - utilizacion) * ControlAsinc(conn->tgen.arrive),
                                     conn->nombre_fich);
    else
      conn->retardo =
          inic_promedioCVasinc(conn->calidad, conn->tolerancia,
                               ControlAsinc(conn->tgen.arrive),
                               (1 - utilizacion) * ControlAsinc(conn->tgen.arrive));

  if (CV_S && conn->retardo != NULL)
    if (files)
      conn->retardo =
          inic_promedioCVsinc_files(conn->calidad, conn->tolerancia,
                                    ControlAsinc(conn->tgen.msg),
                                    conn->nombre_fich);
    else
      conn->retardo =
          inic_promedioCVsinc(conn->calidad, conn->tolerancia,
                              ControlAsinc(conn->tgen.msg));

  if (CV_U && conn->retardo != NULL)
    if (files)
      conn->retardo =
          inic_promedioCVsinc_files(conn->calidad, conn->tolerancia,
                                    ControlAsinc(conn->tgen.arrive),
                                    conn->nombre_fich);
    else
      conn->retardo =
          inic_promedioCVsinc(conn->calidad, conn->tolerancia,
                              ControlAsinc(conn->tgen.arrive));

  sn->clases[conn->qos]->waiting = 0.0;
  sn->clases[conn->qos]->retardo = conn->retardo;

  /* Ahora S no L */
  sn->clases[conn->qos]->S = get_distrib_val(sn->clases[conn->qos]->msg, 0.0, 0);
  sn->clases[conn->qos]->intertime =
      sn->clases[conn->qos]->llegada =
          get_distrib_val(sn->clases[conn->qos]->arrive, 0.0, 0);
  sn->clases[conn->qos]->sig_llegada =
      sn->clases[conn->qos]->llegada +
      get_distrib_val(sn->clases[conn->qos]->arrive, 0.0, 0);

  /* Debería cumplirse siempre: */
  if (sn->clases[conn->qos]->llegada < sn->min_ex_event)
    sn->min_ex_event = sn->clases[conn->qos]->llegada;

  /* comprueba num_promedios */
  if (anal_est)
  {
    k = 0;
    for (i = 0; i < no_of_nets; i++)
      for (j = 0; j <= NO_LEVELS_PRI; j++)
        if (s[i].clases[j] != NULL && s[i].clases[j]->retardo != NULL)
          k++;
    if (k != num_promedios)
      p_error("No promediable en modelo fast alguna de"
              " las v.a.s seleccionadas");
  }
  free(n);
}

void gg1_metrics(char *fn)
{

  int i;
  if (files && c[0].retardo != NULL)
    cierra_ficheros(c[0].retardo);

  gg1_imprime("simmetric.log", fn);

  if (anal_est)
    p_error("Simulacion G/G/1 finalizada, tras alcanzarse los requisitos de"
            " analisis estadistico solicitados");
  else
    p_error("Simulacion G/G/1 finalizada, tras alcanzarse el limite de reloj");
}

static struct
{
  unsigned connection;
  unsigned long num_samples_pot; /* k */
  double reloj;                  /* recepcion tarea 2^k */
  double sumW;
  double sumTTRT;                /* hasta comienzo de transmision de */
  unsigned long long num_rot;    /* la tarea 2^k */
  unsigned long long sumSis;     /* suma Sij recibidos hasta comienzo de envio de */
  unsigned long num_samples_all; /* la tarea 2^k; excluida tarea 2^k */
} sampleCV;

static inline void record_cv(struct Inter_CV_Sample *p)
{
  int i, j;

  for (i = 0, j = p->next_results; j != 1; j >>= 1, i++)
    ;
  sampleCV.num_samples_pot = i;

  sampleCV.sumSis = p->sumSis;
  if (faltan && p->next_results == paso_final)
  {
    faltan--;
    if (!faltan)
      sigue_programa = FALSE;
  }

  p->next_results <<= 1;
  sampleCV.connection = p->c;
  sampleCV.reloj = p->reloj;
  sampleCV.sumW = p->sumW;
  sampleCV.sumTTRT = p->sumTTRT;
  sampleCV.num_rot = p->num_rot;
  sampleCV.num_samples_all = p->num_samples;

  if (write(Inter_CV_file, &sampleCV,
            sizeof(sampleCV)) != sizeof(sampleCV))
  {
    fprintf(stderr, "Fallo al grabar en "
                    "fichero inter_results\n");
    exit(1);
  }
  if (first_waitingCV != p)
  {
    fprintf(stderr, "Error de concordancia"
                    " al grabar en inter_results"
                    "\n");
    exit(1);
  }
  first_waitingCV = first_waitingCV->next;
}

/* Llamada por todos los traficos al gestionar una nueva llegada de trama */
/* Actualiza los contadores CV con los datos de esta clase para aquellas
   clases que estan esperando actualizacion para grabar en fichero */
static inline void acum_cv(struct trafico *clase)
{
  struct Inter_CV_Sample *p;
  for (p = first_waitingCV;
       p && clase->sig_llegada >= p->reloj; p = p->next)
    if (p->cs[clase->inter_cv.c])
    {
      p->cs[clase->inter_cv.c] = 0;
      p->sumSis += clase->sumSis - clase->sig_size;
      p->num_samples += clase->num_samples - 1;
      if (!--(p->cs_remaining))
        record_cv(p);
    }
}

/* Llamada por la propia clase al alcanzar el siguiente punto de grabacion.
   Se esperara a ser actualizada por si misma y por las demas clases */
static inline void begin_record_cv(struct trafico *clase)
{
  struct Inter_CV_Sample *p, *aux;
  int i;

  for (p = first_waitingCV, aux = NULL; p;
       p = p->next)
    aux = p;
  if (aux)
    aux->next = &clase->inter_cv;
  else
    first_waitingCV = &clase->inter_cv;
  clase->inter_cv.next = NULL;
  clase->inter_cv.reloj = clase->sig_llegada;
  clase->inter_cv.sumSis = 0;
  clase->inter_cv.num_samples = 0;
  clase->inter_cv.cs_remaining = no_conns + 1;
  for (i = 0; i < no_conns; i++)
    clase->inter_cv.cs[i] = 1;
  acum_cv(clase);
}

void gg1_node(char *fn)
{
  double vueltas, t_vuelta, t_vueltas, THT, target, amt1, token_time, S;
  int sin_transmitir, pri, media_no, late;
  struct subnet_info *sn, *sn_1, *sn_ult;
  struct trafico *clase;

  /* variables para el c'alculo del tiempo entre oportunidades de
     transmisi'on, para el metodo CV con variable de control tiempo
     de rotaci'on del testigo
     */
  unsigned long numero_vueltas = 0, vueltas_ciclotx, primera;
  double ciclotx, prevS = 0;

  semilla_aleatorio(seed);
  gg1_init();

  sn = &s[0]; /* primera y única subred */
  for (pri = NO_LEVELS_PRI; pri >= 0; pri--)
  {
    clase = sn->clases[pri];
    if (clase != NULL)
      break; /* Sólo 1 tráfico en G/G/1 */
  }
  do
  {
    if (clase->llegada > sim_clock) /* periodo de inactividad */
      if (CV_ASINC)
        grupoCVasinc(sim_clock, clase->llegada - sim_clock); /* Computa tiempo desocupado */

    /* avanzamos a cuando la estacion tiene algo que transmitir */
    sim_clock = clase->llegada;
    /* transmitimos hasta siguiente periodo vacío */
    while (clase->llegada <= sim_clock)
    /* transmite tramas */
    {
      S = clase->S;
      clase->waiting = sim_clock - clase->llegada;

      if (verbose)
        printf("%f:SNT %d snd PKT rtd %g\n",
               sim_clock, sn->subnet_number,
               clase->waiting);

      if (Inter_CV_file && !clase->left_size)
      {
        /* almacena para salvar: W, sumTTRT y n_vueltas_T */
        clase->sumW += clase->waiting;
        if (clase->num_samples == clase->inter_cv.next_results)
        {
          clase->inter_cv.sumW = clase->sumW;
          clase->inter_cv.sumTTRT = sn->last_visit;
          clase->inter_cv.num_rot = clase->num_vuelta;
          if (!--(clase->inter_cv.cs_remaining))
            record_cv(&clase->inter_cv);
        }
      }

      if (clase->retardo != NULL && !transitorio)
        if (CV_S || CV_U)
          promedia(clase->retardo, clase->waiting, CV_S ? prevS : clase->intertime);
        else
          promedia(clase->retardo, clase->waiting, clase->llegada);

      prevS = S;
      sim_clock += S; /* en G/G/1 usamos S en vez de L */
      clase->S = get_distrib_val(clase->msg,
                                 clase->llegada,
                                 0);
      clase->intertime = get_distrib_val(clase->arrive,
                                         clase->llegada,
                                         clase->sig_size);
      clase->llegada = clase->sig_llegada;
      clase->sig_llegada += clase->intertime;
      if (inter_est)
        acum_cv(clase); // CV asincrono
    }

    if (transitorio && sim_clock > clk_limit)
      transitorio = FALSE;

    if (pause_point == ON)
      tpause("Interrumpido desde el terminal");

    if (volcado_parcial)
    {
      volcado_parcial = OFF;
      gg1_imprime("actual.log", fn);
    }
  } while ((anal_est ? num_promedios : sim_clock < clk_limit) && sigue_programa);

#ifdef FILES_MMAPPED
  binary_control();
#endif

  if (xmode == BATCH)
    gg1_metrics(fn);
  else if (!sigue_programa)
    tpause("Terminado debido a senhal TERM");
  else if (anal_est)
    tpause("Alcanzados los requisitos de margen de error y"
           " nivel de confianza");
  else
    tpause("Alcanzado limite de reloj");
}
