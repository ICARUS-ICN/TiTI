/*
  NewVersion @(#)promedios.h	1.11 06/30/98 ETSIT-IT
*/

#ifndef PROMEDIOS_H

#define PROMEDIOS_H

#define CV_PROMEDIOS

#ifndef PROMEDIOS_C
typedef void * promedio;
const extern
#endif
unsigned num_promedios;

promedio inic_promedio(float calidad, float tolerancia_relativa);
/* funcion encargada de inicializar una variable de tipo promedio
   antes de dar comienzo a la estimacion: 
   admite calidad 0.0 => no afecta a num_promedios;
   no admite calidad 1.0 */

promedio inic_promedio_dif(float calidad, float tolerancia_relativa,
			   double analitico);
/* idem anterior pero calcula la diferencia con respecto a un valor
   analitico, exacto (no debe converger) o aproximado (nos dar'a una
   estimaci'on de la diferencia */

void promedia(promedio, double muestra, double llegada);
/* computa la muestra en la estimacion del promedio */

double estimacion(promedio);
/* devuelve la estimacion realizada del promedio */

double desviacion(promedio);
/* devuelve la estimacion de la desviacion tipica de la
   v.a. promedida, sin intervalo de confianza */

double calidad_fin(promedio);
/* devuelve la calidad final en la estimacion del promedio, fijando
   la tolerancia relativa */

double tolerancia_fin(promedio);
/* devuelve la tolerancia relativa final en la estimacion del promedio,
   fijando la calidad */

unsigned long num_muestras(promedio);
/* devuelve el numero de muestras computadas hasta el momento */

void libera_promedio(promedio);
/* elimina las estructuras de datos asociadas al promedio, incluyendo
   en su caso el cierre de ficheros */

#define FULL_PROMEDIOS
#define CV_PROMEDIOS

#ifdef FULL_PROMEDIOS
void inic_parametros(double *clock, int file_limit, int inter_est,
		     FILE* fichero_error);

promedio inic_promedio_files(float calidad, float tolerancia_relativa,
			     const char *nombre_fichero);
/* idem inic_promedio, pero comprueba files para saber si inicializa
   los ficheros */

void estado_promedio(promedio);
/* imprime el estado, acabado o no, y en este ultimo caso una estimacion
   de muestras necesarias */

void cierra_ficheros(promedio);
/* de estar abiertos, cierra los ficheros de muestras */

void promedios_stats(promedio, FILE *);

#endif /* FULL_PROMEDIOS */

#ifdef CV_PROMEDIOS

promedio inic_promedioCVasinc(float calidad, float tolerancia_relativa,
			      double MediaControl, double Tv);
/* idem. inic_promedioCV, pero para variable de control asincrona, es
   decir, no hay correspondencia muestra a muestra entre la variable a
   promediar y la de control */

promedio inic_promedioCVsinc(float calidad, float tolerancia_relativa,
			      double MediaControl);
/* idem. inic_promedioCV, pero para variable de control asincrona, es
   decir, no hay correspondencia muestra a muestra entre la variable a
   promediar y la de control */

void grupoCVasinc(double clock, double idle);
/* env'ia suma de muestras de la variable de control */

#ifdef FULL_PROMEDIOS
promedio inic_promedioCVasinc_files(float calidad, float tolerancia_relativa,
				    double MediaControl, double Tv,
				    const char *nombre_fichero);
/* obvio */
promedio inic_promedioCVsinc_files(float calidad, float tolerancia_relativa,
				    double MediaControl,
				    const char *nombre_fichero);
/* obvio */
#endif /* FULL_PROMEDIOS */
#endif /* CV_PROMEDIOS */

#endif /* PROMEDIOS_H */
