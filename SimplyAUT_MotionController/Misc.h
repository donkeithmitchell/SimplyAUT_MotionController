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

	double x, y;
};