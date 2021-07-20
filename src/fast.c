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

char FastSid[] = "@(#)fast.c	4.8 01/25/99";

static void fast_summary(const char *fname, FILE *mfp)
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

  fprintf(mfp, "\tNodos: %4d\n", no_of_nets);
  fprintf(mfp, "\tSemilla: %ld\n", seed);
  fprintf(mfp, "\tTTRT = %.1f\n", TTRT);
  if (anal_est)
    fprintf(mfp, "\tTransitorio: %d ms\n", clk_limit);
  fprintf(mfp, "\tFichero de Entrada: %s\n", fname);
  fprintf(mfp, "\n\t Version rapida del simulador\n");
  for (i = 0; i < no_of_media; i++)
  {
    fprintf(mfp, "\nMedium Throughput: %.3f bps.\n",
            ((m[i].byte_count * 8) / sim_clock) * MILLISEC);
  }
  fprintf(mfp, "\n");
  if (fddi_ii)
  {
    fprintf(mfp, "\n ***  PARAMETROS DE INTERES PARA FDDI-II  ***\n");
    fprintf(mfp, "\nEstacion Master: %d\n", m[0].master);
    fprintf(mfp, "Latencia del anillo: %f ms -> %d ciclo(s)\n", m_delay,
            m_cycles);
    fprintf(mfp, "Retardo LAB buffer: %f ms\n", m[0].master_delay);
    fprintf(mfp, "BW Trafico isocrono: %f Kbps", m[0].iso_bandwidth);
    fprintf(mfp, " (%2.2f%c)\n",
            m[0].iso_bandwidth / m[0].current_link_speed * 100, 37);
    fprintf(mfp, "# WBCs ocupados: %d de %d (%2.2f%c)\n", m_no_of_channels,
            m_wbcs, m_no_of_channels * 100.0 / m_wbcs, 37);
    fprintf(mfp, "BW Trafico de Paquetes: %f Kbps ", m[0].packet_bandwidth);
    fprintf(mfp, "(%2.2f%c)\n",
            m[0].packet_bandwidth / m[0].current_link_speed * 100, 37);
    fprintf(mfp, "Longitud del testigo: %f ms\n", m[0].token_length);
  }
}

static void fast_imprime(const char *nombre_fichero, const char *fn)
{
  FILE *mfp;
  int i;

  mfp = fopen(nombre_fichero, "w");

  /* print a summary of the run */
  fast_summary(fn, mfp);

  fprintf(mfp, "\n\n*** ESTADISTICAS DE TIEMPO DE ESPERA EN COLA (ms) ***\n");
  for (i = 0; i < no_conns; i++)
  {
    if (c[i].retardo != NULL)
    {
      fprintf(mfp, "\n\nCONEXION %d\n", i);
      promedios_stats(c[i].retardo, mfp);
    }
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
    p_error("Control Variates no soportado para esta distribucion\n");
    return (ERROR);
  }
}

static void fast_init(void)
{
  struct subnet_info *sn;
  struct conn_info *conn;
  struct media_info *media;
  long int msg_size;
  int i, j, k;

  double ciclo, utilizacion, lambda, aux, aux2;

  for (i = 0; i < no_of_nets; i++)
  {
    sn = &s[i];
    sn->min_ex_event = INFINITY;
    for (j = 0; j <= NO_LEVELS_PRI; j++)
      sn->clases[j] = NULL;
  }

  conn = &c[no_conns - 1];
  sn = &s[conn->c_end2];
  media = &m[sn->media_interface];
  media->token_time = media->prop_delay + media->fddi_head_end_latency;

  /* Calculo del tiempo medio desocupado por paquete */
  if (CV_ASINC)
  {
    utilizacion = 0.0;
    lambda = 0.0;
    for (i = 0; i < no_conns; i++)
    {
      conn = &c[i];
      sn = &s[conn->c_end1];
      aux = ControlAsinc(conn->tgen.msg);
      aux += sn->subnet_head + media->preamble;
      aux *= 8;
      aux2 = ControlAsinc(conn->tgen.arrive);
      aux /= aux2;
      utilizacion += aux;
      lambda += 1 / aux2;
    }
    utilizacion /= media->link_speed;
    ciclo = (1 - utilizacion) / lambda;
  }

  for (i = 0; i < no_conns; i++)
  {
    conn = &c[i];
    sn = &s[conn->c_end1];
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

    sn->clases[conn->qos]->waiting = 0.0;
    sn->clases[conn->qos]->retardo = conn->retardo;

    msg_size = get_distrib_val(sn->clases[conn->qos]->msg, 0.0, 0);
    if (msg_size > sn->max_size)
    {
      sn->clases[conn->qos]->sig_size = sn->max_size;
      sn->clases[conn->qos]->left_size = msg_size - sn->max_size;
    }
    else
    {
      sn->clases[conn->qos]->sig_size = msg_size;
      sn->clases[conn->qos]->left_size = 0;
    }

    sn->clases[conn->qos]->ant_size = 0;
    sn->clases[conn->qos]->intertime =
        sn->clases[conn->qos]->llegada =
            get_distrib_val(sn->clases[conn->qos]->arrive, 0.0, msg_size);
    sn->clases[conn->qos]->sig_llegada =
        sn->clases[conn->qos]->llegada +
        get_distrib_val(sn->clases[conn->qos]->arrive, 0.0, msg_size);

    sn->clases[conn->qos]->ant_tx = 0.0;
    sn->clases[conn->qos]->num_vuelta = 0;

    /* Selecciona mínimo tiempo de llegada de todos los tráficos del nodo */
    if (sn->clases[conn->qos]->llegada < sn->min_ex_event)
      sn->min_ex_event = sn->clases[conn->qos]->llegada;
  }

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
  /* determine which subnet gets the token */
  for (nodo_inicial = 0; nodo_inicial < no_of_nets - 1; nodo_inicial++)
    if (bernoulli((float)(1.0 / (float)no_of_nets)))
      break;
  /*  free(c); */
  free(n);
}

void fast_metrics(char *fn)
{

  int i;

  for (i = 0; i < no_of_nets; i++)
  {
    if (files && s[i].ttrt != NULL)
      cierra_ficheros(s[i].ttrt);
    /*      if (files && s[i].llenado != NULL)
	      cierra_ficheros(s[i].llenado);*/
  }
  for (i = 0; i < no_conns; i++)
    if (files && c[i].retardo != NULL)
      cierra_ficheros(c[i].retardo);

  fast_imprime("simmetric.log", fn);

  if (anal_est)
    p_error("Simulacion FAST finalizada, tras alcanzarse los requisitos de"
            " analisis estadistico solicitados");
  else
    p_error("Simulacion FAST finalizada, tras alcanzarse el limite de reloj");
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

void fast_fddi(char *fn)
{
  double vueltas, t_vuelta, t_vueltas, THT, target, amt1, token_time;
  int sin_transmitir, msg_size, pri, media_no, late;
  struct subnet_info *sn, *sn_1, *sn_ult;
  struct trafico *clase;

  /* variables para el c'alculo del tiempo entre oportunidades de
     transmisi'on, para el metodo CV con variable de control tiempo
     de rotaci'on del testigo
     */
  unsigned long numero_vueltas = 0, vueltas_ciclotx, primera;
  double ciclotx;

  semilla_aleatorio(seed);
  fast_init();
  sn_ult = &s[no_of_nets - 1]; /* ultima subred */
  media_no = sn_ult->media_interface;
  token_time = m[media_no].token_time; /* tiempo entre transmision y recepcion
					  del testigo */
  t_vuelta = token_time * no_of_nets;  /* tiempo minimo de rotacion del testigo
				       */

  if (fddi_ii)
    t_vuelta += m[media_no].master_delay;

  sn = &s[nodo_inicial]; /* primera subred con posesion del testigo */
  sin_transmitir = 0;
  do
  {
    if (sn->min_ex_event > sim_clock) /* la estacion no tiene tramas para
					   transmitir */
    {
      sin_transmitir++; /* incrementa el numero de estaciones sucesivas
			       sin transmitir */
      if (sn->min_ex_event < sig_event)
        sig_event = sn->min_ex_event; /* calculo del minimo tiempo de
					     llegada de mensaje a la red */
      sn->last_visit = sim_clock;
      if (sin_transmitir == no_of_nets) /* si se ha dado una vuelta sin
					       que transmita nadie */
      {
        /* se incrementa el reloj con el numero de vueltas en que nadie
		 transmite */
        vueltas = floor((sig_event - sim_clock) / t_vuelta);
        if (vueltas > 0.0)
        {
          numero_vueltas += vueltas;

          t_vueltas = vueltas * t_vuelta;
          sim_clock += t_vueltas;
          if (CV_ASINC)
            grupoCVasinc(sim_clock, t_vueltas);

          for (sn_1 = s; sn_1 <= sn_ult; sn_1++)
            sn_1->last_visit += t_vueltas;
        }
        sig_event = sn->min_ex_event;
        sin_transmitir = 1;
      }
    }
    else /* la estacion tiene algo que transmitir */
    {
      THT = sim_clock - sn->last_visit;
      if (THT > TTRT) /* Testigo llega tarde */
      {
        late = TRUE;
        sn->last_visit = sim_clock + TTRT - THT; /* No se resetea
							  el TRT */
      }
      else
      {
        late = FALSE;
        sn->last_visit = sim_clock; /* Reseteamos el TRT */
      }
      sn->min_ex_event = INFINITY;
      for (pri = NO_LEVELS_PRI; pri >= 0; pri--) /* bucle de transmision
							para todos los
							traficos */
      {
        if (sn->clases[pri] != NULL)
        {
          switch (pri)
          {
          case NO_LEVELS_PRI:
            target = sn->target_time_syn;
            break;

          case NO_LEVELS_PRI - 1:
            target = (late) ? 0.0 : TTRT - THT;
            break;

          default:
            target = (late) ? 0.0 : sn->target_time[pri] - THT;
            break;
          }

          clase = sn->clases[pri];

          primera = TRUE;
          while (target > 0.0 && clase->llegada <= sim_clock)
          /* transmite tramas */
          {
            if (primera)
            {
              primera = FALSE;
              ciclotx = sim_clock - clase->ant_tx;
              vueltas_ciclotx = numero_vueltas - clase->num_vuelta;
              clase->ant_tx = sim_clock;
              clase->num_vuelta = numero_vueltas;
            }

            msg_size = clase->sig_size + sn->subnet_head;
            m[media_no].byte_count += msg_size;
            msg_size += m[media_no].preamble;
            if (fddi_ii)
              amt1 = (msg_size * 8) /
                     m[media_no].packet_bandwidth;
            else
              amt1 = (msg_size * 8) /
                     m[media_no].link_speed;
            if (pri != NO_LEVELS_PRI)
              THT += amt1;
            clase->waiting += sim_clock - clase->llegada;

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

            if (clase->retardo != NULL && !transitorio && !clase->left_size)
              promedia(clase->retardo, clase->waiting,
                       clase->llegada);

            sim_clock += amt1;
            target -= amt1;

            if (clase->left_size)
            {
              if (clase->left_size > sn->max_size)
              {
                clase->sig_size = sn->max_size;
                clase->left_size -= sn->max_size;
              }
              else
              {
                clase->sig_size = clase->left_size;
                clase->left_size = 0;
              }
              clase->llegada = sim_clock;
            }
            else
            {
              clase->waiting = 0.0;
              clase->sig_size = get_distrib_val(clase->msg,
                                                clase->llegada,
                                                0);

              clase->sumSis += clase->sig_size;
              clase->num_samples++;
              if (inter_est)
                if (clase->num_samples == clase->inter_cv.next_results)
                  begin_record_cv(clase);
                else if (clase->num_samples == clase->inter_cv.next_results << 1)
                  p_error("Clase empieza grabacion CV antes de"
                          " terminar con la anterior\n");

              if (clase->sig_size > sn->max_size)
              {
                clase->left_size = clase->sig_size -
                                   sn->max_size;
                clase->sig_size = sn->max_size;
              }
              clase->intertime = get_distrib_val(clase->arrive,
                                                 clase->llegada,
                                                 clase->sig_size);
              clase->llegada = clase->sig_llegada;
              clase->sig_llegada += clase->intertime;
              if (inter_est)
                acum_cv(clase);
            }
          }
          if (clase->llegada < sn->min_ex_event)
            sn->min_ex_event = clase->llegada;
        }
      }
      sig_event = sn->min_ex_event;
      sin_transmitir = 0;
    }
    sim_clock += token_time;
    if (CV_ASINC)
      grupoCVasinc(sim_clock, token_time);

    if (transitorio && sim_clock > clk_limit)
      transitorio = FALSE;
    if (sn == sn_ult)
    {
      numero_vueltas++;
      sn = s;
    }
    else
      sn++;

    if (pause_point == ON)
      tpause("Interrumpido desde el terminal");

    if (volcado_parcial)
    {
      volcado_parcial = OFF;
      fast_imprime("actual.log", fn);
    }
  } while ((anal_est ? num_promedios : sim_clock < clk_limit) && sigue_programa);

#ifdef FILES_MMAPPED
  binary_control();
#endif

  if (xmode == BATCH)
    fast_metrics(fn);
  else if (!sigue_programa)
    tpause("Terminado debido a senhal TERM");
  else if (anal_est)
    tpause("Alcanzados los requisitos de margen de error y"
           " nivel de confianza");
  else
    tpause("Alcanzado limite de reloj");
}
