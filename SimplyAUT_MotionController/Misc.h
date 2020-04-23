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



int MinMaxI4(const void* i1, const void* i2);
int MinMaxR8(const void* i1, const void* i2);
int MinMaxDP_X(const void* i1, const void* i2);
int MinMaxDP_Y(const void* i1, const void* i2);

int ConvertToWavFormat(const CArray<float, float>& trace, int samp_rate, int volume, CArray<char, char>& wav_vector);
