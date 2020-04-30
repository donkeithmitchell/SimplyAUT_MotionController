#include "pch.h"
#include "SimplyAUT_MotionController.h"
#include "Button.h"
#include "Misc.h"
#include "math.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// a collection of usefuyl functions

#define BSWAP32(x) ( ((x & 0xff000000) >> 24) | ((x & 0x00ff0000) >> 8) | ((x & 0x0000ff00) << 8) | ((x & 0x000000ff) << 24) )
#define BSWAP16(x) ( ((x & 0xff00) >> 8) | ((x & 0x00ff) << 8) )


static double PI = 4 * atan(1.0);


// 0 order: just the average
// 1 order: y = m x + b
// 2 order: y =  A x^2 + B x + c
int polyfit(const double* const dependentValues,
            const double* const independentValues,
            unsigned int        countOfElements,
            unsigned int        order,
            double*             coefficients)
{
    // Declarations...
    // ----------------------------------
    enum {maxOrder = 5};
    
    double B[maxOrder+1] = {0.0f};
    double P[((maxOrder+1) * 2)+1] = {0.0f};
    double A[(maxOrder + 1)*2*(maxOrder + 1)] = {0.0f};

    double x, y, powx;

    unsigned int ii, jj, kk;

    // Verify initial conditions....
    // ----------------------------------

    // This method requires that the countOfElements > 
    // (order+1) 
    if (countOfElements <= order)
        return -1;

    // This method has imposed an arbitrary bound of
    // order <= maxOrder.  Increase maxOrder if necessary.
    if (order > maxOrder)
        return -1;

    // Begin Code...
    // ----------------------------------

    // Identify the column vector
    for (ii = 0; ii < countOfElements; ii++)
    {
        x    = dependentValues[ii];
        y    = independentValues[ii];
        powx = 1;

        for (jj = 0; jj < (order + 1); jj++)
        {
            B[jj] = B[jj] + (y * powx);
            powx  = powx * x;
        }
    }

    // Initialize the PowX array
    P[0] = countOfElements;

    // Compute the sum of the Powers of X
    for (ii = 0; ii < countOfElements; ii++)
    {
        x    = dependentValues[ii];
        powx = dependentValues[ii];

        for (jj = 1; jj < ((2 * (order + 1)) + 1); jj++)
        {
            P[jj] = P[jj] + powx;
            powx  = powx * x;
        }
    }

    // Initialize the reduction matrix
    //
    for (ii = 0; ii < (order + 1); ii++)
    {
        for (jj = 0; jj < (order + 1); jj++)
        {
            A[(ii * (2 * (order + 1))) + jj] = P[ii+jj];
        }

        A[(ii*(2 * (order + 1))) + (ii + (order + 1))] = 1;
    }

    // Move the Identity matrix portion of the redux matrix
    // to the left side (find the inverse of the left side
    // of the redux matrix
    for (ii = 0; ii < (order + 1); ii++)
    {
        x = A[(ii * (2 * (order + 1))) + ii];
        if (x != 0)
        {
            for (kk = 0; kk < (2 * (order + 1)); kk++)
            {
                A[(ii * (2 * (order + 1))) + kk] = 
                    A[(ii * (2 * (order + 1))) + kk] / x;
            }

            for (jj = 0; jj < (order + 1); jj++)
            {
                if ((jj - ii) != 0)
                {
                    y = A[(jj * (2 * (order + 1))) + ii];
                    for (kk = 0; kk < (2 * (order + 1)); kk++)
                    {
                        A[(jj * (2 * (order + 1))) + kk] = 
                            A[(jj * (2 * (order + 1))) + kk] -
                            y * A[(ii * (2 * (order + 1))) + kk];
                    }
                }
            }
        }
        else
        {
            // Cannot work with singular matrices
            return -1;
        }
    }

    // Calculate and Identify the coefficients
    for (ii = 0; ii < (order + 1); ii++)
    {
        for (jj = 0; jj < (order + 1); jj++)
        {
            x = 0;
            for (kk = 0; kk < (order + 1); kk++)
            {
                x = x + (A[(ii * (2 * (order + 1))) + (kk + (order + 1))] *
                    B[kk]);
            }
            coefficients[ii] = x;
        }
    }

    return 0;
}


// used in qsort
int MinMaxI4(const void* i1, const void* i2)
{
    int val1 = *((int*)i1);
    int val2 = *((int*)i2);

    return val1 - val2;
}
// used in qsort
int MinMaxDP_X(const void* i1, const void* i2)
{
    const CDoublePoint* val1 = (CDoublePoint*)i1;
    const CDoublePoint* val2 = (CDoublePoint*)i2;

    if (val1->x > val2->x)
        return 1;
    else if (val1->x < val2->x)
        return -1;
    else
        return 0;
}

int MinMaxDP_Y(const void* i1, const void* i2)
{
    const CDoublePoint* val1 = (CDoublePoint*)i1;
    const CDoublePoint* val2 = (CDoublePoint*)i2;

    if (val1->y > val2->y)
        return 1;
    else if (val1->y < val2->y)
        return -1;
    else
        return 0;
}
int MinMaxR8(const void* i1, const void* i2)
{
    double val1 = *((double*)i1);
    double val2 = *((double*)i2);

    if (val1 > val2)
        return 1;
    else if (val1 < val2)
        return -1;
    else
        return 0;
}

int ConvertToWavFormat(const CArray<float, float>& trace, int samp_rate, int volume, CArray<char, char>& wav_vector)
{
    static double PI = 4 * atan(1.0);
    int i;
    // int NumSamples1 = min(32000, trace.GetSize());
    int NumSamples1 = trace.GetSize();

    int SampleRate1 = (int)samp_rate * 1; // max(atoi(m_szSpeed),1);
    short AudioFormat1 = 1; // PCM
    short NumChannels1 = 2; // mono
    short BitsPerSample1 = 16; // the data is signed shorts (2's complement)
    short BlockAlign1 = NumChannels1 * BitsPerSample1 / 8;
    int ByteRate1 = SampleRate1 * NumChannels1 * BitsPerSample1 / 8;

    int SubChunk1Size1 = 16;
    int SubChunk2Size1 = NumSamples1 * NumChannels1 * BitsPerSample1 / 8;
    int ChunkSize1 = 4 + (8 + SubChunk1Size1) + (8 + SubChunk2Size1); // this chunk contains the format and data chunks

    // define 
    wav_vector.SetSize(ChunkSize1 + 8 + 8);

    // RIFF chunk indicating that a wave file
    int offset = 0;
    memcpy(wav_vector.GetData() + offset, "RIFF", 4); offset += 4;
    memcpy(wav_vector.GetData() + offset, &ChunkSize1, sizeof(ChunkSize1)); offset += sizeof(ChunkSize1);
    memcpy(wav_vector.GetData() + offset, "WAVE", 4); offset += 4;

    // inidcate that the format chunk to follow (24 bytes) 
    memcpy(wav_vector.GetData() + offset, "fmt ", 4); offset += 4;
    memcpy(wav_vector.GetData() + offset, &SubChunk1Size1, sizeof(SubChunk1Size1)); offset += sizeof(SubChunk1Size1);
    memcpy(wav_vector.GetData() + offset, &AudioFormat1, sizeof(AudioFormat1)); offset += sizeof(AudioFormat1);
    memcpy(wav_vector.GetData() + offset, &NumChannels1, sizeof(NumChannels1)); offset += sizeof(NumChannels1);
    memcpy(wav_vector.GetData() + offset, &SampleRate1, sizeof(SampleRate1)); offset += sizeof(SampleRate1);
    memcpy(wav_vector.GetData() + offset, &ByteRate1, sizeof(ByteRate1)); offset += sizeof(ByteRate1);
    memcpy(wav_vector.GetData() + offset, &BlockAlign1, sizeof(BlockAlign1)); offset += sizeof(BlockAlign1);
    memcpy(wav_vector.GetData() + offset, &BitsPerSample1, sizeof(BitsPerSample1)); offset += sizeof(BitsPerSample1);

    double scale = volume / 10000.0; // m_sliderVolume.GetPos()/100.0;

    // the data must be -32768 -> +332767
    // scale so that the maximum 
    double maxVal = 0;
    for (i = 0; i < trace.GetSize(); ++i)
        maxVal = max(maxVal, fabs(trace[i]));

    // indicagte that this is a data chunk (8 + 4*nsamp bytes)
    memcpy(wav_vector.GetData() + offset, "data", 4); offset += 4;
    memcpy(wav_vector.GetData() + offset, &SubChunk2Size1, sizeof(SubChunk2Size1)); offset += sizeof(SubChunk2Size1);

    for (i = 0; i < trace.GetSize() && offset < wav_vector.GetSize() + 4; ++i)
    {
        // the data is 2's complement
        short val1 = (short)(32767 / 8 * scale * trace[i] / maxVal + 0.5);
        short val2 = BSWAP16(val1);
        // short val2 = ~val1 + 1;
        // short val2 = val1;
        memcpy(wav_vector.GetData() + offset, &val2, sizeof(val2)); offset += sizeof(val2);// left
        memcpy(wav_vector.GetData() + offset, &val2, sizeof(val2)); offset += sizeof(val2);// right
    }


    return wav_vector.GetSize();
}

void MySleep(DWORD dwMilliseconds, DWORD msgMask) 
{
    // The message loop lasts until we get a WM_QUIT message,
    // upon which we shall return from the function.
    clock_t t1 = clock();
    while ((DWORD)(clock() - t1) < dwMilliseconds)
    {
        // block-local variable 
        MSG msg;

        // Read all of the messages in this next loop, 
        // removing each message as we read it.
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            // If it's a quit message, we're out of here.
            if (msg.message == WM_QUIT)
                return;

            // Otherwise, dispatch the message.
            if( msg.message & msgMask)
              DispatchMessage(&msg);
        } // End of PeekMessage while loop.
    }

} // End of function.

DWORD MyWaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds, DWORD msgMask)
{
    DWORD ret = WAIT_TIMEOUT;
    clock_t t1 = clock();
    while (ret == WAIT_TIMEOUT && (dwMilliseconds == INFINITE || (DWORD)(clock() - t1) < dwMilliseconds) )
    {
        ret = WaitForSingleObject(hHandle, 0);
        if( ret == WAIT_TIMEOUT )
            MySleep(1, msgMask);
    }

    return ret;
}

CWinThread* AfxMyBeginThread(AFX_THREADPROC pfnThreadProc, LPVOID pParam,
    int nPriority/* = THREAD_PRIORITY_NORMAL*/, UINT nStackSize/* = 0*/,
    DWORD dwCreateFlags/* = 0*/, LPSECURITY_ATTRIBUTES lpSecurityAttrs/* = NULL*/)
{
    CWinThread* pThread = new CWinThread(pfnThreadProc, pParam);
    ASSERT_VALID(pThread);

    pThread->m_bAutoDelete = FALSE;

    if (!pThread->CreateThread(dwCreateFlags | CREATE_SUSPENDED, nStackSize,
        lpSecurityAttrs))
    {
        pThread->Delete();
        return NULL;
    }

    VERIFY(pThread->SetThreadPriority(nPriority));
    if (!(dwCreateFlags & CREATE_SUSPENDED))
        VERIFY(pThread->ResumeThread() != (DWORD)-1);

    return pThread;
}

/*
static complex8 conj(const complex8& x)
{
    complex8 ret;
    ret.real = x.real;
    ret.imag = -x.imag;
    return ret;
}

static void _fft(complex8 buf[], complex8 out[], int n, int step)
{
    if (step < n) 
    {
        _fft(out, buf, n, step * 2);
        _fft(out + step, buf + step, n, step * 2);

        for (int i = 0; i < n; i += 2 * step) 
        {
            complex8 t = cexp(-I * PI * i / n) * out[i + step];
            buf[i / 2] = out[i] + t;
            buf[(i + n) / 2] = out[i] - t;
        }
    }
}

void fft(complex8 buf[], int n)
{
    CArray<complex8, complex8> out;
    out.SetSize(n);

    for (int i = 0; i < n; i++) out[i] = buf[i];

    _fft(buf, out.GetData(), n, 1);
}

*/



