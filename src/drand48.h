#ifndef __drand48_h__
#define __drand48_h__
extern double drand48();
extern double erand48(/* unsigned short xsubi[3] */);
extern long lrand48();
extern long nrand48(/* unsigned short xsubi[3] */);
extern long mrand48();
extern long jrand48(/* unsigned short xsubi[3] */);
extern void srand48(/* long seedval */);
extern unsigned short *seed48(/* unsigned short seed16v[3] */);
extern void lcong48(/* unsigned short param[7] */);
#endif
