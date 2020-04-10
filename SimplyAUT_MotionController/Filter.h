#pragma once
class CIIR_Filter
{
public:
	CIIR_Filter(int nsamp);

	void IIR_HighCut(double* tin, int srate, double freq);
	void IIR_LowCut(double* tin, int srate, double freq);
	void AveragingFilter(double* tin, int width);
	void MedianFilter(double* tin, int width);
	void Butter_lc_r4(double* tin, double* work1, double* work2, double* coef, int nsamp);
	int GetLowCutCoeff(int srate, double* coef, int LC);

private:
	int GetSize()const { return m_work.GetSize(); }

	void IIR_lc_r4(double* trace,
		double coef1,
		double coef2,
		double coef3,
		double coef4,
		double coef5);

	void IIR_hc_r4(double* trace,
		double coef1,
		double coef2,
		double coef3,
		double coef4,
		double coef5);

	void Notch_r4(double* trace,
		double coef1,
		double coef2,
		double coef3);

	double  m_coef_1[3],
		m_coef_2[3],
		m_coef_3[3],
		m_coef_4[3],
		m_coef_5[3];
	double m_pFreq[3];
	int m_pSrate[3];

	CArray<double, double> m_work;
};

