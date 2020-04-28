#include "pch.h"
#include "PROC.H"
//#include <string.h>
//#include <math.h>
//#include <stdlib.h>


#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static void polint( float* xa, float* ya, int N, float x, float* y, float* dy );
static void polcof( float* xa, float* ya, int N, float* cof );
#define SQR(a)		( (a)*(a) )
#define SIGN(a,b)	( ((b) >= 0.0) ? fabs(a) : -fabs(a) )

#define TOL 1.0e-10	//was 1.0e-5, but results were not as good as with Matlab
#define NR_END 1


double* dvector(int nl, int nh)
{
	//allocate a double vector with subsscript rangev[nl..nh]
	double* v = new double [nh-nl+1+NR_END];
	return (v == NULL) ? NULL :v - nl + NR_END;
}
double* dnhvector(int nl,int nh)
{
	//allocate a double vector with subsscript rangev[nl..nh] 
	double* v = new double [nh-nl+1+NR_END];
	return (v == NULL) ? NULL : v-nl+NR_END;
}

double** dmatrix(int nrl, int nrh, int ncl, int nch)
{ 
	int nrow = nrh-nrl+1;
	int ncol = nch-ncl+1;

	double** m = new double*[nrow+NR_END]; //conversion may loose significant digits huge *
	if( m == NULL )
		return NULL;

	m += NR_END;
	m -= nrl;

	m[nrl] = new double [(long)nrow*ncol+NR_END];  //*** long is important
	if( m[nrl] == NULL )
	{
		delete m;
		return NULL;
	}
	m[nrl] += NR_END;
	m[nrl] -= ncl;

	for(int i=nrl+1; i <= nrh; i++) 
		m[i] = m[i-1] + ncol;

	return m;
}

void free_dvector(double *v, int nl, int )
{
	if( v != NULL )
		delete (v+nl-NR_END);
}
void free_dmatrix(double** m, int nrl, int, int ncl, int)
{
	if( m != NULL )
	{
		delete  (m[nrl] + ncl-NR_END);
		delete  (m+nrl-NR_END);
	}
}

static void sineUF(double t, double* line,int ,double* param);
static void sineF(double xi, double *line,int );

int sfuf(
const double*	x,		// time vector
const double*	y,		// data samples
int				nData,	// number of samples
double			freq,	// estimate of frequency
double			FLSC,	// full scale value
const double*	tol,	// vector of tolerances [amp dc phase freq]
int				nMax,	// maximum iterations
double*			param)	// output parameters    [amp dc phase freq]
{
	const int ma = 4;	//The sinefit Unknown freq. need a 4 column matrix
	double  chi2;
	double  prvParam[4];	// holds previous param
	//dsvdufit for fixed max. iterations and tol
	double  para[3];		// Aim1,Bim1,delta wi: to be passed to sineUF

	static double*  sig= NULL;
	static double*  a  = NULL;
	static double** AU = NULL;
	static double** V  = NULL;
	static double*  W  = NULL;
	static int n0 = 0;

	if( x == NULL )
	{
		free_dvector(sig,1,ma);			sig= NULL;
		free_dmatrix(AU,1,nData,1,ma);	AU = NULL;
		free_dmatrix(V,1,ma,1,ma);		V  = NULL;
		free_dvector(W,1,ma);			W  = NULL;
		free_dvector(a,1,ma);			a  = NULL;
		n0 = 0;

		sfkf();
		dsvdufit();
		return 0;
	}

	if( sig == NULL )
	{
		sig = dvector(1,nData);		//Stand. Deviation
		a  = dnhvector(1,ma);		//Space for the coefficient of the basis functions
		AU = dmatrix(1,nData,1,ma); //Transformation matrix
									//and unitary  mat. (on the return)
		V  = dmatrix(1,ma,1,ma);	//Unitary marix
		W  = dnhvector(1,ma);
		n0 = nData;
	}
	else if( nData > n0 )
	{
		free_dmatrix(AU,1,nData,1,ma);
		AU = dmatrix(1,nData,1,ma); //Transformation matrix
		n0 = nData;
	}

	int i=1; 
	for(i=1; i <= nData; i++) 
		sig[i] = 1.0;

	//Angular frequency
	//////////////////////////////
	const double pi = acos(-1.0);
	const double pi2 = 2*pi;

	//in dsvdcmp, called from sfkf.  It is a bug of numrec???
	sfkf( x, y, nData, freq, a);

	if( nMax > 2 )
		freq += 0.0001;

	double Wi = pi2 * freq;

	int j=0;
	for(j=0; j < 4; j++) 
		prvParam[j] = 0.0;

	//a contains A0 B0 C0
	//delta w for first step  is 0
	////////////////////////////////
	a[4] = 0.0;

	for(i=1; i <= nMax; ++i)
	{
		Wi += a[4];	// wi=w(i-1) + deltaw(i-1)
		para[0] = a[1];
		para[1] = a[2];
		para[2] = Wi;
		dsvdufit(x,y,sig,nData,a,ma,AU,V,W,&chi2,sineUF,para);

		//Get result in form [Amp DC PHASE FREQ]  (Amp*Cos( 2piFREQ+PHASE) + DC)
		//from A0*Cos(2piFREQ)+B0*Sin(2piFREQ)+DC)
		param[0] = sqrt(a[1]*a[1] +a[2]*a[2]);
		param[1] = a[3];
		param[2] = atan(-a[2]/a[1]);	//A0>=0 and B0<=0
		if( a[1] >= 0 && a[2] > 0) 
			param[2] += pi2;			//A0>=0 and B0>0
		if( a[1] < 0)
			param[2] += pi;				//A0<0, any B0
		param[3] = Wi / pi2;

		// check the tolerances, I only care about frequency
		if(	(fabs(param[3]-prvParam[3]) < tol[3]*param[3])	&&	//FREQ
		    (fabs(param[0]-prvParam[0]) < tol[0]*param[0])	&&	//AMP
			(fabs(param[1]-prvParam[1]) < tol[1]*FLSC)		&&	//DC
			(fabs(param[2]-prvParam[2]) < tol[2]*pi2)		)	//PHASE
		{
			break;
		}

		for(int j=0; j < 4; j++) 
			prvParam[j] = param[j];
	} 

	return (i <= nMax) ? i : -1;
}
//////////////////////////////////////////////////////////////////////////////
//Sine Fit Unknown Frequency
static void sineUF(double t, double* line,int ,double* para)
{
	//Define the function for filling the Matrix (one line at a time)
	//line is [1..col] but para is [0...2]
	double Wit	= t * para[2];
	double Bim1t= t * para[1];
	double Aim1t=t * para[0];
	line[1] = cos(Wit); //line [1...col]
	line[2] = sin(Wit); //line [1...col]
	line[3] = 1.0; //line [1...col]
	line[4] = Bim1t * line[1] - Aim1t*line[2];
}

//////////////////////////////////////////////////////////////////////////////
//Sine Fit for Known Frequency
static void sineF(double xi, double *line,int )
{
	//Define the function for filling the Matrix (one line at a time)
	//Note that xi=Wo*xi
	line[1] = cos(xi); //line [1...col]
	line[2] = sin(xi); //line [1...col]
	line[3] = 1.0; //line [1...col]
}


void sfkf(
const double*	x,		// data vectors [1..nData]
const double*	y, 
int				nData, 
double			freq,	// estimate of frequency
double*			ABC)	// vector[1..3] A0 B0 C0
{
	int ma = 3;	//number of col: cos sin 1

	//Grabs workspace and results memory
	static double*  sig= NULL;
	static double** AU = NULL;
	static double** V  = NULL;
	static double*  W  = NULL;
	static int n0 = 0;

	if( x == NULL )
	{
		//Clear memory
		free_dmatrix(AU,1,nData,1,ma);	AU = NULL;
		free_dmatrix(V,1,ma,1,ma);		V  = NULL;
		free_dvector(W,1,ma);			W  = NULL;
		free_dvector(sig,1,nData);		sig= NULL;
		dsvdfit();
		n0 = 0;
		return;
	}
	if( sig == NULL )
	{
		sig = dvector(1,nData);//Probably not needed
		AU  = dmatrix(1,nData,1,ma); //Transformation matrix and unitary
		V   = dmatrix(1,ma,1,ma); //Unitary matrix
		W   = dnhvector(1,ma);
		n0 = nData;
	}
	else if( nData > n0 )
	{
		free_dmatrix(AU,1,nData,1,ma);
		free_dvector(sig,1,nData);
		sig = dvector(1,nData);//Probably not needed
		AU  = dmatrix(1,nData,1,ma); //Transformation matrix and unitary
		n0 = nData;
	}

	double chi2;
	for(int i=1; i <= nData; i++)
		sig[i] = 1.0; //will become soon a null ptr
														//when  looking at efficiency
	//dsvdfit
	ABC[1]=freq;		//Pass the freq. to func
	dsvdfit(x,y,sig,nData,ABC,ma,AU,V,W,&chi2,sineF);
							 //Get A0 B0 C0 in ABC
}

void dsvdfit(
const double*	x,		// data points
const double*	y,
double*			sig,	// individual std of each data point (all 1 if not known)
int				ndata,	// rows
double*			a,		// coefficients to be estimated with L.S. method A[x]*a= b[y]
						// on input a[1] contains  the KNOWN frequency
int				ma,		// columns
double**		u,		// workspace on input, svd on output
double**		v,
double*			w,
double*			chisq,	// kind of residuals (the smaller the best) chisq=Ax-b
void (*funcs)(double,double*,int) )	// return the ma basis fcns evaluated at x
{
	static double* b = NULL;
	static double* afunc = NULL;
	static int n0 = 0;

	if( x == NULL )
	{
		free_dvector(b,1,ndata);	b     = NULL;
		free_dvector(afunc,1,ma);	afunc = NULL;
		dsvdcmp();
		dsvbksb();
		n0 = 0;
		return;
	}

	if( b == NULL )
	{
		b     = dvector(1,ndata);
		afunc = dnhvector(1,ma);
		n0 = ndata;
	}
	else if( ndata > n0 )
	{
		free_dvector(b,1,ndata);
		b     = dvector(1,ndata);
		n0 = ndata;
	}

	double pi2 = 8.0*atan(1.0);
	double Wo = pi2 * a[1];		//Angular frequency

	for(int i=1;i <= ndata; i++)			//Build the matrix  A[x]
	{
		(*funcs)(Wo*x[i],afunc,ma);
		double tmp = 1.0/sig[i];
		for(int j=1; j<= ma; j++)
			u[i][j] = afunc[j]*tmp;
		b[i] = y[i]*tmp;
	}
	dsvdcmp(u,ndata,ma,w,v);

	double wmax = 0.0; 						//Edit singular values, using TOL
	int j=1;
	for(j=1; j <= ma; j++)
	{
		if( w[j] > wmax)
			wmax=w[j];
	}
	double thresh = TOL*wmax;
	for(j=1; j <= ma; j++)
	{
		if( w[j] < thresh)
			w[j] = 0.0;
	}

	dsvbksb(u,w,v,ndata,ma,b,a);

	*chisq=0.0;							//Evaluate chi-square.
}


void dsvdufit(
const double*	x,		// data points
const double*	y,
double*			sig,	// individual std of each data point (all 1 if not known)
int				ndata,	// rows
double*			a,		// coefficients to be estimated with L.S. method A[x]*a= b[y]
int				ma,		// columns
double**		u,		// workspace on input, svd on output
double**		v,
double*			w,
double*			chisq,	// kind of residuals (the smaller the best) chisq=Ax-b
void			(*funcs)(double,double*,int,double*), // return the ma basis fcns evaluated at x
double*			param ) // parameters to pass on to funcs.  Note index :[0..2]
{
	static double* b	 = NULL;
	static double* afunc = NULL;
	static int n0 = 0;

	if( x == NULL )
	{
		free_dvector(b,1,ndata);	b     = NULL;
		free_dvector(afunc,1,ma);	afunc = NULL;
		dsvdcmp();
		dsvbksb();
		n0 = 0;
		return;
	}

	if( b == NULL )
	{
		b		= dvector(1,ndata);
		afunc	= dnhvector(1,ma);
		n0 = ndata;
	}
	else if( ndata > n0 )
	{
		free_dvector(b,1,ndata);
		b		= dvector(1,ndata);
		n0 = ndata;
	}

	for(int i = 1; i <= ndata; i++)			//Build the matrix  A[x]
	{
		(*funcs)(x[i],afunc,ma,param);
		double tmp = 1.0/sig[i];
		for(int j=1; j <= ma; j++) 
			u[i][j] = afunc[j]*tmp;
		b[i]=y[i]*tmp;
	}
	dsvdcmp(u,ndata,ma,w,v);

	double wmax = 0.0;    						//Edit singular values, using TOL
	int j=1;
	for(j=1; j <= ma; j++)
	{
		if( w[j] > wmax ) 
			wmax = w[j];
	}

	double thresh = TOL*wmax;
	for(j=1; j <= ma; j++)
	{
		if( w[j] < thresh )
			w[j] = 0.0;
	}

	dsvbksb(u,w,v,ndata,ma,b,a);

	*chisq=0.0;							//Evaluate chi-square.
}

#pragma optimize( "", off)

//////////////////////////////////////////////////////////////////////////////
int dsvdcmp(
double**	a,
int			m,	//	rows
int			n,	//  columns 	*** assume never neede huge ****
double*		w,
double**	v )
{
	static double* rv1 = dnhvector(1,n);
	static int n0=0;
	

	if( a == NULL )
	{
		free_dvector(rv1,1,n); rv1 = NULL;
		n0 = 0;
		return 0;
	}

	if( rv1 == NULL || n > n0 )
	{
		free_dvector(rv1,1,n);
		rv1 = dnhvector(1,n);
		n0 = n;
	}


	int flag, its, j, jj, k, l, nm;
	double c, f,  h, s, x, y, z;

	double g = 0.0;
	double scale = 0.0;
	double anorm = 0.0;

	int i=1;
	for(i=1; i <= n; i++)
	{ 
		l = i+1;
		rv1[i] = scale*g;
		g = 0.0;
		s = 0.0;
		scale = 0.0;
		if( i <= m)
		{
			int k=0;
			for(k=i; k <= m; k++)
				scale += fabs(a[k][i]);
			if( scale != 0.0 )
			{
				for(k=i; k <= m; k++)
				{
					a[k][i] /= scale;
					s += a[k][i]*a[k][i];
				}
				f = a[i][i];
				g = - SIGN(sqrt(s),f);
				h=f*g-s;
				a[i][i]=f-g;
				for (j=l;j<=n;j++)
				{
					for (s=0.0,k=i;k<=m;k++) 
						s += a[k][i]*a[k][j];
					f=s/h;
					for (k=i;k<=m;k++) 
						a[k][j] += f*a[k][i];
				}
				for (k=i;k<=m;k++) 
					a[k][i] *= scale;
			}
		}
		w[i]=scale*g;
		g=s=scale=0.0;
		if( i <= m && i != n )
		{
			for(k=l; k <= n; k++)
				scale += fabs(a[i][k]);
			if( scale != 0.0 )
			{
				for(k=l; k <= n; k++)
				{
					a[i][k] /= scale;
					s += a[i][k]*a[i][k];
				}
				f=a[i][l];
				g = -SIGN(sqrt(s),f);
				h=f*g-s;
				a[i][l]=f-g;
				for (k=l;k<=n;k++) 
					rv1[k]=a[i][k]/h;
				for (j=l;j<=m;j++)
				{
					for (s=0.0,k=l;k<=n;k++)
						s += a[j][k]*a[i][k];
					for (k=l;k<=n;k++) 
						a[j][k] += s*rv1[k];
				}
				for (k=l;k<=n;k++) 
					a[i][k] *= scale;
			  }
		 }
		 anorm = __max(anorm,(fabs(w[i]) + fabs(rv1[i])));
	}
	for (i=n;i>=1;i--)
	{ 
		if (i<n)
		{
			if (g)
			{
				for (j=l;j<=n;j++) 
					v[j][i]=(a[i][j]/a[i][l])/g;
				for (j=l;j<=n;j++)
				{
					for (s=0.0,k=l;k<=n;k++) 
						s += a[i][k]*v[k][j];
					for (k=l;k<=n;k++) 
						v[k][j] += s*v[k][i];
				}
			}
			for (j=l;j<=n;j++) 
				v[i][j]=v[j][i]=0.0;
		}
		v[i][i]=1.0;
		g=rv1[i];
		l=i;
	}
	for (i=__min(m,n);i>=1;i--)
	{
		l=i+1;
		g=w[i];
		for (j=l;j<=n;j++) 
			a[i][j]=0.0;
		if (g)
		{
			g=1.0/g;
			for (j=l;j<=n;j++)
			{
				for (s=0.0,k=l;k<=m;k++) 
					s+= a[k][i]*a[k][j];
				f=(s/a[i][i])*g;
				for (k=i;k<=m;k++) 
					a[k][j] += f*a[k][i];
			}
			for (j=i;j<=m;j++) 
				a[j][i] *= g;
		}
		else
			for (j=i;j<=m;j++)
			a[j][i]=0.0;
		++a[i][i];
	}

	for (k=n;k>=1;k--)
	{
		for (its=1;its<=30;its++)
		{
			flag=1;
			for (l=k;l>=1;l--)
			{
				nm=l-1;
				if ((double)(fabs(rv1[l])+anorm) ==anorm)
				{
					flag=0;
					break;
				}
				if ((double)(fabs(w[nm])+anorm)==anorm)  
					break;
			}
			if (flag)
			{
				c=0.0;
				s=1.0;
				for (i=l;i<=k;i++)
				{
					f=s*rv1[i];
					rv1[i]=c*rv1[i];
					if ((double)(fabs(f)+anorm) == anorm) 
						break;
					g=w[i];
					h=pythag(f,g);
					w[i]=h;
					h=1.0/h;
					c=g*h;
					s= -f*h;
					for (j=1;j<=m;j++)
					{
						y=a[j][nm];
						z=a[j][i];
						a[j][nm]=y*c+z*s;
						a[j][i]=z*c-y*s;
					}
				}
			}

			z=w[k];
			if (l==k)            //Convergence
			{
				if (z<0.0)
				{
					w[k] = -z;
					for(j=1;j<=n;j++)  
						v[j][k] = -v[j][k];
				}
				break;
			}
	
			if (its==30) 
				return -1;
			x=w[l];
			nm=k-1;
			y=w[nm];
			g=rv1[nm];
			h=rv1[k];
			f=((y-z)*(y+z)+(g-h)*(g+h))/(2.0*h*y);
			g=pythag(f,1.0);
			f=((x-z)*(x+z)+h*((y/(f+SIGN(g,f)))-h))/x;
			c=s=1.0;					//Next QR transformation:
			for (j=l;j<=nm;j++)
			{
				i=j+1;
				g=rv1[i];
				y=w[i];
				h=s*g;
				g=c*g;
				z=pythag(f,h);
				rv1[j]=z;
				c=f/z;
				s=h/z;
				f=x*c+g*s;
				g = g*c-x*s;
				h=y*s;
				y *=c;
				for (jj=1;jj<=n;jj++)
				{
					x=v[jj][j];
					z=v[jj][i];
					v[jj][j]=x*c+z*s;
					v[jj][i]=z*c-x*s;
				}
				z=pythag(f,h);
				w[j]=z;
				if (z)
				{
					z=1.0/z;
					c=f*z;
					s=h*z;
				}
				f=c*g+s*y;
				x=c*y-s*g;
				for (jj=1;jj<=m;jj++)
				{
					y=a[jj][j];
					z=a[jj][i];
					a[jj][j]=y*c+z*s;
					a[jj][i]=z*c-y*s;
				}
			}
			rv1[l]=0.0;
			rv1[k]=f;
			w[k]=x;
		}
	}
	return 0;
}

#pragma optimize( "", on)

void dsvbksb(
double**	u,	// A from A*x=b but after SVD: A=U*w*V
double*		w, 
double**	v, 
int			m,	// rows
int			n,	// columns
double*		b,	// from A*x = b
double*		x )	// from A*x = b
{
	static double* tmp = NULL;
	static int n0=0;

	if( u == NULL )
	{
		free_dvector(tmp,1,n); tmp = NULL;
		n0 = 0;
		return;
	}

	if( tmp == NULL || n > n0 )
	{
		free_dvector(tmp,1,n);
		tmp = dnhvector(1,n);
		n0 = n;
	}

	int j=1;
	for (j=1;j<=n;j++)			//Calculate U'B
	{
		double s=0.0;
		if (w[j])
		{
			for (int i=1;i<=m;i++) 
				s += u[i][j]*b[i];
			s /= w[j];
		}
		tmp[j] = s;
	}
	for (j=1;j<=n;j++)			//Multiply by V
	{
		double s=0.0;
		for (int jj=1;jj<=n;jj++) 
			s += v[j][jj]*tmp[jj];
		x[j] = s;
	}
}
static void polcof( float* xa, float* ya, int N, float* cof )
{
	float* x = new float[N+1];
	float* y = new float[N+1];

	memcpy( x, xa, 4*(N+1) );
	memcpy( y, ya, 4*(N+1) );

	for(int j=0; j <= N; ++j)
	{
		float dy;
		polint(x-1,y-1, N+1-j, 0.0f, cof+j, &dy);
		// subtracct '1' from pointers to x & y because (polint) uses dimensions
		// [1..N], we extrapolate to x = 0
		float xmin = (float)1.0e38;
		int k = -1;
		int i=0;
		for(i=0; i <= N-j; ++i)
		{
			if( fabs(x[i]) < xmin ) 
			{
				xmin = (float)fabs(x[i]);
				k = i;
			}
			if( x[i] )
				y[i] = (y[i] - cof[j]) / x[i];
		}
		for(i=k+1; i <= N-j; ++i)
		{
			y[i-1] = y[i];
			x[i-1] = x[i];
		}
	}
	delete y;
	delete x;
}

/*Compute (a.^2 +b.^2).^.5
without destructive underflow or overflow
*/
float pythag(float a,float b)
{
	float absa = (float)fabs(a);
	float absb = (float)fabs(b);
	if (absa > absb)
		return absa * (float)sqrt(1.0 + SQR(absb/absa));
	else 
		return (absb == 0.0f ? 0.0f : absb * (float)sqrt(1.0 + SQR(absa/absb)));
}
/*Compute (a.^2 +b.^2).^.5
without destructive underflow or overflow
*/
double pythag(double a,double b)
{
	double absa = fabs(a);
	double absb = fabs(b);
	if (absa > absb)
		return absa*sqrt(1.0 + SQR(absb/absa));
	else
		return (absb == 0.0 ? 0.0 : absb*sqrt(1.0 + SQR(absa/absb)));
}

static void polint( float* xa, float* ya, int N, float x, float* y, float* dy )
{
	float dif = (float)fabs(x-xa[0]);
	float* c = new float[N];
	float* d = new float[N];

	int ns = 1;
	for(int i=1; i < N; ++i)
	{
		float dift = (float)fabs(x - xa[i]);
		if( dift < dif )
		{
			ns = i+1;
			dif = dift;
		}
		c[i] = ya[i];
		d[i] = ya[i];
	}

	*y = ya[--ns];
	for(int m=0; m < N-1; m++)
	{
		for(int i=0; i < N-m; ++i)
		{
			float ho = xa[i] - x;
			float hp = xa[i+m] - x;
			float w  = c[i+1] - d[i];
			float den = ho-hp;
			if( den == 0.0 ) continue;

			den = w/den;
			d[i] = hp * den;
			c[i] = ho * den;
		}
		*y += (*dy=(2*ns < (N-m) ? c[ns+1] : d[--ns]));
	}

	delete c;
	delete d;
}

