#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "Misc.h"
#include "Filter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


static double PI = 4 * atan(1.0); 

#define  A  0.17
#define  FL1   2.0251
#define  FL2   0.236
#define  FC2   0.8223
#define  FL3   2.0251

CIIR_Filter::CIIR_Filter(int nsamp)
{
    m_work.SetSize(nsamp);
    for (int i = 0; i < 3; ++i)
    {
        m_coef_1[i] = 0;
        m_coef_2[i] = 0;
        m_coef_3[i] = 0;
        m_coef_4[i] = 0;
        m_coef_5[i] = 0;
        m_pFreq[i] = 0;
        m_pSrate[i] = 0;
    }
}

/*    trace -->   Input Data                                      */
/*    srate -->   Sample Rate                                     */
/*    freq  -->   Frequency                                       */
/*    type  -->   1 = double                                      */
/*                2 = double                                       */
void CIIR_Filter::IIR_HighCut(double* trace, int srate, double fFreq)
{
    if (fFreq > (srate / 2))
        return;

    if (srate != m_pSrate[0] || fFreq != m_pFreq[0])
    {
        double den, tp, fc, dtee;
        double a, b, c, d;
        double aa, bb, cc, dd, ee, ff;

        m_pSrate[0] = srate;
        m_pFreq[0] = fFreq;

        dtee = 1.0 / srate;
        tp = dtee * 2 * PI * fFreq / 2;
        fc = 2 * tan(tp) / (dtee * 2 * PI);
        den = 2 * PI * fc * dtee;

        a = 0.5 * (FL1 * FC2 * FL3 + FL1 * FC2 * FL2 + FC2 * FL2 * FL3) * pow((2.0 / den), 3.0);
        b = 0.5 * (FL1 * FC2 + FC2 * FL3 + 2.0 * FC2 * FL2) * pow((2.0 / den), 2.0);
        c = 0.5 * (FL1 + FC2 + FL3) * (2.0 / den);
        d = FC2 * FL2 * pow((2.0 / den), 2.0);

        aa = a + b + c + 1.0;
        bb = 3.0 - 3.0 * a - b + c;
        cc = 3.0 + 3.0 * a - b - c;
        dd = 1.0 - a + b - c;
        ee = 1.0 + d;
        ff = 3.0 - d;

        m_coef_3[0] = bb / aa;
        m_coef_4[0] = cc / aa;
        m_coef_5[0] = dd / aa;
        m_coef_1[0] = ee / aa;
        m_coef_2[0] = ff / aa;
    }

    IIR_hc_r4(trace, m_coef_1[0], m_coef_2[0], m_coef_3[0], m_coef_4[0], m_coef_5[0]);
}

void CIIR_Filter::IIR_LowCut(double* trace, int srate, double fFreq)
{
    if (fFreq > (srate / 2))
        return;

    if (srate != m_pSrate[1] || fFreq != m_pFreq[1])
    {
        double den, tp, fc, dtee;
        double a, b, c, d, e;
        double aa, bb, cc, dd, ee, ff;

        m_pSrate[1] = srate;
        m_pFreq[1] = fFreq;

        fFreq = 1.28f * fFreq;

        dtee = 1.0 / srate;
        tp = dtee * 2 * PI * fFreq / 2;
        fc = 2 * tan(tp) / (dtee * 2 * PI);
        den = 2 * PI * fc * dtee;

        a = pow((2.0 / den), 3.0) / 0.38208;
        b = 1.27995 * pow((2.0 / den), 2.0) / 0.38208;
        c = 0.874624 * (2.0 / den) / 0.38208;
        d = a;
        e = 0.0979603 * (2.0 / den) / 0.38208;

        aa = a + b + c + 1.0;
        bb = 3.0 - 3.0 * a - b + c;
        cc = 3.0 + 3.0 * a - b - c;
        dd = 1.0 - a + b - c;
        ee = d + e;
        ff = e - 3.0 * d;

        m_coef_3[1] = bb / aa;
        m_coef_4[1] = cc / aa;
        m_coef_5[1] = dd / aa;
        m_coef_1[1] = ee / aa;
        m_coef_2[1] = ff / aa;
    }

    IIR_lc_r4(trace, m_coef_1[1], m_coef_2[1], m_coef_3[1], m_coef_4[1], m_coef_5[1]);
}


void CIIR_Filter::IIR_lc_r4(double* trace,
    double coef1,
    double coef2,
    double coef3,
    double coef4,
    double coef5)
{
    int i;

    m_work[0] = (double)(trace[0] * coef1);
    m_work[1] = (double)(trace[1] * coef1 + trace[0] * coef2 - m_work[0] * coef3);
    m_work[2] = (double)(trace[2] * coef1 + (trace[1] - trace[0]) * coef2 -
        m_work[1] * coef3 - m_work[0] * coef4);

    int nsamp = GetSize();
    for (i = 3; i < nsamp; ++i)
    {
        m_work[i] = (double)((trace[i] - trace[i - 3]) * coef1 + (trace[i - 1] - trace[i - 2]) * coef2 -
            m_work[i - 1] * coef3 - m_work[i - 2] * coef4 - m_work[i - 3] * coef5);
    }

    m_work[nsamp - 1] = (double)(trace[nsamp - 1] * coef1);
    m_work[nsamp - 2] = (double)(trace[nsamp - 2] * coef1 + trace[nsamp - 1] * coef2 - m_work[nsamp - 1] * coef3);
    m_work[nsamp - 3] = (double)(trace[nsamp - 3] * coef1 + (trace[nsamp - 2] - trace[nsamp - 1]) * coef2 -
        m_work[nsamp - 2] * coef3 - m_work[nsamp - 1] * coef4);

    for (i = nsamp - 4; i >= 0; --i)
    {
        m_work[i] = (double)((trace[i] - trace[i + 3]) * coef1 + (trace[i + 1] - trace[i + 2]) * coef2 -
            m_work[i + 1] * coef3 - m_work[i + 2] * coef4 - m_work[i + 3] * coef5);
    }

    memcpy(trace, m_work.GetData(), sizeof(double) * nsamp);
}

void CIIR_Filter::IIR_hc_r4(double* trace,
    double coef1,
    double coef2,
    double coef3,
    double coef4,
    double coef5)
{
    int i;

    m_work[0] = (double)(trace[0] * coef1);
    m_work[1] = (double)(trace[1] * coef1 + trace[0] * coef2 - m_work[0] * coef3);
    m_work[2] = (double)(trace[2] * coef1 + (trace[1] + trace[0]) * coef2 -
        m_work[1] * coef3 - m_work[0] * coef4);

    int nsamp = GetSize();
    for (i = 3; i < nsamp; ++i)
    {
        m_work[i] = (double)((trace[i] + trace[i - 3]) * coef1 + (trace[i - 1] + trace[i - 2]) * coef2 -
            m_work[i - 1] * coef3 - m_work[i - 2] * coef4 - m_work[i - 3] * coef5);
    }

    m_work[nsamp - 1] = (double)(trace[nsamp - 1] * coef1);
    m_work[nsamp - 2] = (double)(trace[nsamp - 2] * coef1 + trace[nsamp - 1] * coef2 - m_work[nsamp - 1] * coef3);
    m_work[nsamp - 3] = (double)(trace[nsamp - 3] * coef1 + (trace[nsamp - 2] + trace[nsamp - 1]) * coef2 -
        m_work[nsamp - 2] * coef3 - m_work[nsamp - 1] * coef4);

    for (i = nsamp - 4; i >= 0; --i)
    {
        m_work[i] = (double)((trace[i] + trace[i + 3]) * coef1 + (trace[i + 1] + trace[i + 2]) * coef2 -
            m_work[i + 1] * coef3 - m_work[i + 2] * coef4 - m_work[i + 3] * coef5);
    }

    memcpy(trace, m_work.GetData(), sizeof(double) * nsamp);
}

void CIIR_Filter::Notch_r4(double* trace,
    double coef1,
    double coef2,
    double coef3)
{
    int i;
    m_work[0] = (double)(trace[0] * coef3);
    m_work[1] = (double)(trace[1] * coef3 + trace[0] * coef2 - m_work[0] * coef2);

    int nsamp = GetSize();
    for (i = 2; i < nsamp; ++i)
        m_work[i] = (double)(trace[i] * coef3 + trace[i - 1] * coef2 + trace[i - 2] * coef3 -
            m_work[i - 1] * coef2 - m_work[i - 2] * coef1);

    trace[nsamp - 1] = (double)(m_work[nsamp - 1] * coef3);
    trace[nsamp - 2] = (double)(m_work[nsamp - 2] * coef3 + m_work[nsamp - 1] * coef2 - trace[nsamp - 1] * coef2);

    for (i = nsamp - 3; i >= 0; --i)
        trace[i] = (double)(m_work[i] * coef3 + m_work[i + 1] * coef2 + m_work[i + 2] * coef3 -
            trace[i + 1] * coef2 - trace[i + 2] * coef1);
}