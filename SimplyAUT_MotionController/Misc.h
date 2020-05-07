#pragma once

int polyfit(const double* const dependentValues,
	const double* const independentValues,
	unsigned int        countOfElements,
	unsigned int        order,
	double* coefficients);



class CDoublePoint
{
public:
	CDoublePoint() { x = y = FLT_MAX; }
	CDoublePoint(double _x, double _y) { x = _x; y = _y; }
	void Reset() { x = y = FLT_MAX; }
	BOOL IsSet()const { return x != FLT_MAX && y != FLT_MAX; }
	BOOL operator != (const CDoublePoint& xx) { return xx.x != x || xx.y != y; }

	double x, y;
};

struct complex8
{
	complex8() { real = imag = 0.0; }
	complex8& operator + (const complex8& x) { complex8 ret; ret.real = real + x.real; ret.imag = imag + x.imag; return ret; }
	complex8& operator - (const complex8& x) { complex8 ret; ret.real = real - x.real; ret.imag = imag - x.imag; return ret; }
	double real;
	double imag;
};



int MinMaxI4(const void* i1, const void* i2);
int MinMaxR8(const void* i1, const void* i2);
int MinMaxDP_X(const void* i1, const void* i2);
int MinMaxDP_Y(const void* i1, const void* i2);

void MySleep(DWORD dwMilliseconds, DWORD msgMask);
DWORD MyWaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds, DWORD msgMask);

int ConvertToWavFormat(const CArray<float, float>& trace, int samp_rate, int volume, CArray<char, char>& wav_vector);
double HammingWindow(double dist, double nWidth);

CWinThread* AfxMyBeginThread(AFX_THREADPROC pfnThreadProc, LPVOID pParam,
	int nPriority = THREAD_PRIORITY_NORMAL, UINT nStackSize = 0,
	DWORD dwCreateFlags = 0, LPSECURITY_ATTRIBUTES lpSecurityAttrs = NULL);
