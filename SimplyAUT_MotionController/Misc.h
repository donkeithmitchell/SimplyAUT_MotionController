#pragma once


class CDoublePoint
{
public:
	CDoublePoint() { x = y = FLT_MAX; }
	void Reset() { x = y = FLT_MAX; }
	BOOL IsSet()const { return x != FLT_MAX && y != FLT_MAX; }

	double x, y;
};