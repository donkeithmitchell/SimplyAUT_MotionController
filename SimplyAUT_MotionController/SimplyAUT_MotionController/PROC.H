#pragma once


// only test if in debug and this defined
#ifdef _DEBUG
#ifndef _WIN64
//#define TEST_PROC	
#endif
#endif


double*		dvector(int nl, int nh);
double*		dnhvector(int nl,int nh);
double**	dmatrix(int nrl, int nrh, int ncl, int nch);
void		free_dvector(double*v, int nl, int );
void		free_dvector(double*v, int nl, int );
void		free_dmatrix(double** m, int nrl, int, int ncl, int);
int			dsvdcmp(double** a=NULL,int b=0,int c=0,double* d=NULL,double** e=NULL);
void		dsvbksb(double** a=NULL,double* b=NULL,double** c=NULL,int d=0,int e=0,double* f=0,double* g=NULL);

int			sfuf(const double* a=NULL,const double* b=NULL,int c=0,double d=0.0,double e=0.0,const double* f=NULL,int g=0,double* h=NULL);
void		sfkf(const double* a=NULL,const double* b=NULL,int c=0,double d=0.0,double* e=NULL);
void		dsvdufit(const double* a=NULL,const double* b=NULL, double* c=NULL,int d=NULL,double* e=NULL,int f=0,
						double** g=NULL,double** h=NULL,double* i=NULL,double* j=NULL,
						void(*)(double,double*,int,double*)=NULL,double* l=NULL);
void		dsvdfit(const double* a=NULL, const double* b=NULL,double* c=NULL,int d=0,double* e=NULL,int f=0,
						double** g=NULL,double** h=NULL,double* i=NULL,double* j=NULL,
						void (*)(double,double*,int)=NULL );
double		pythag(double a,double b);
float		pythag(float a,float b);
void		dsvdfit(double*,double*,double*,int,double*,int,double**,double**,double*,double*,
						void (*)(double,double*,int) );


