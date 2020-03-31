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

void CIIR_Filter::Butter_lc_r4(double* tin, double* work1, double* work2, double* coef, int nsamp)
{
    int i;

    work1[0] = coef[0] * tin[0];
    work1[1] = coef[0] * tin[1] + coef[1] * tin[0] - coef[3] * work1[0];

    for (i = 2; i < nsamp; ++i)
    {
        work1[i] = coef[0] * tin[i] + coef[1] * tin[i - 1] + coef[2] * tin[i - 2] -
            coef[3] * work1[i - 1] - coef[4] * work1[i - 2];
    }

    work2[nsamp - 1] = coef[0] * work1[nsamp - 1];
    work2[nsamp - 2] = coef[0] * work1[nsamp - 2] + coef[1] * work1[nsamp - 1] - coef[3] * work2[nsamp - 1];

    tin[nsamp - 1] = (double)work2[nsamp - 1];
    tin[nsamp - 2] = (double)work2[nsamp - 2];

    for (i = nsamp - 3; i >= 0; --i)
    {
        work2[i] = coef[0] * work1[i] + coef[1] * work1[i + 1] + coef[2] * work1[i + 2] -
            coef[3] * work2[i + 1] - coef[4] * work2[i + 2];

        tin[i] = (double)work2[i];
    }
}

// use matlab to solve as 
// [B,A] = butter(2, .8 * LC / srate / 2, 'high')
// 
// at second order have 6dB / octave, thus:
// (LC1 / LC2 - 1) * 6 dB/Oct = 1.5 dB
// LC2 = 0.8 * LC1
// this will insure that at LC1, are only down 1.5 dB, and 3.0 dB on two passes
// use the matlab file aram_lc.m to generate the following coefficients
////////////////////////////////////////////////////////////////////////////////////////

// as this is the acquire low-cut, do not change values to greater prcision
static double LowCut3[] = {
  100.0f, 8.9884553e-001f, -1.7976911f, 8.9884553e-001f, 1.0f, -1.7874325f, 8.0794959e-001f,
  125.0f, 9.1822800e-001f, -1.8364560f, 9.1822800e-001f, 1.0f, -1.8297581f, 8.4315388e-001f,
  150.0f, 9.3137886e-001f, -1.8627577f, 9.3137886e-001f, 1.0f, -1.8580433f, 8.6747213e-001f,
  200.0f, 9.4808079e-001f, -1.8961616f, 9.4808079e-001f, 1.0f, -1.8934641f, 8.9885899e-001f,
  250.0f, 9.5824473e-001f, -1.9164895f, 9.5824473e-001f, 1.0f, -1.9147452f, 9.1823372e-001f,
  300.0f, 9.6508099e-001f, -1.9301620f, 9.6508099e-001f, 1.0f, -1.9289423f, 9.3138168e-001f,
  333.0f, 9.6848633e-001f, -1.9369727f, 9.6848633e-001f, 1.0f, -1.9359793f, 9.3796603e-001f,
  400.0f, 9.7369481e-001f, -1.9473896f, 9.7369481e-001f, 1.0f, -1.9466975f, 9.4808171e-001f,
  500.0f, 9.7889992e-001f, -1.9577998f, 9.7889992e-001f, 1.0f, -1.9573546f, 9.5824511e-001f,
  600.0f, 9.8238544e-001f, -1.9647709f, 9.8238544e-001f, 1.0f, -1.9644606f, 9.6508117e-001f,
  800.0f, 9.8675978e-001f, -1.9735196f, 9.8675978e-001f, 1.0f, -1.9733442f, 9.7369487e-001f,
  1000.0f, 9.8939373e-001f, -1.9787875f, 9.8939373e-001f, 1.0f, -1.9786750f, 9.7889995e-001f,
  1200.0f, 9.9115360e-001f, -1.9823072f, 9.9115360e-001f, 1.0f, -1.9822289f, 9.8238545e-001f,
  1600.0f, 9.9335783e-001f, -1.9867157f, 9.9335783e-001f, 1.0f, -1.9866715f, 9.8675978e-001f,
  2000.0f, 9.9468273e-001f, -1.9893655f, 9.9468273e-001f, 1.0f, -1.9893372f, 9.8939373e-001f,
  2400.0f, 9.9556697e-001f, -1.9911339f, 9.9556697e-001f, 1.0f, -1.9911143f, 9.9115360e-001f,
  3200.0f, 9.9667338e-001f, -1.9933468f, 9.9667338e-001f, 1.0f, -1.9933357f, 9.9335783e-001f,
  4000.0f, 9.9733782e-001f, -1.9946756f, 9.9733782e-001f, 1.0f, -1.9946686f, 9.9468273e-001f,
  4800.0f, 9.9778102e-001f, -1.9955620f, 9.9778102e-001f, 1.0f, -1.9955571f, 9.9556697e-001f,
    -1.0f };

static int CalculateCoefficient(double freq, double srate, double* coef)
{
    // calculate for 0.8 so only 1.5 dB down at desired freq
    freq *= 0.8f;

    double dtu = tan(PI * freq / srate);
    double dtmp1 = sqrt(2.0) * dtu;
    double dtmp2 = pow(dtu, 2);
    double dtmp3 = dtmp2 + dtmp1 + 1.0;

    coef[0] = 1.0 / dtmp3;
    coef[1] = -2.0 * coef[0];
    coef[2] = coef[0];
    coef[3] = (2.0 * dtmp2 - 2.0) / dtmp3;
    coef[4] = (dtmp2 - dtmp1 + 1.0) / dtmp3;

    return 0;
}

static int GetCoeff(const double* lowCut, int srate, double* coef)
{
    int i;
    for (i = 0; lowCut[7 * i + 0] > 0.0 && (int)(lowCut[7 * i + 0]) != srate; ++i);

    if (lowCut[7 * i + 0] < 0.0f || (int)(lowCut[7 * i + 0]) != srate)
        return -1;

    coef[0] = (double)lowCut[7 * i + 1];   // B0
    coef[1] = (double)lowCut[7 * i + 2];   // B1
    coef[2] = (double)lowCut[7 * i + 1];   // B0
    coef[3] = (double)lowCut[7 * i + 5];   // A1
    coef[4] = (double)lowCut[7 * i + 6];   // A2

    // assert_coef(lowCut, (double)srate, coef);
    return 0;
}

// have checked that the above formulae gives same values within reasonable tolerance
// to the matlab values, continue to use existing coefficients for 3 Hz though
int CIIR_Filter::GetLowCutCoeff(int srate, double* coef, int LC)
{
    switch (LC)
    {
    case 0:
        return -1;
    case 1: // force to 1.25 Hz not 1Hz
        return CalculateCoefficient(1.25f, srate, coef);
    case 3:
        return GetCoeff(LowCut3, srate, coef);
    default:
        return CalculateCoefficient((double)LC, srate, coef);
    }
}

