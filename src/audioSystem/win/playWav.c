#define INITGUID
#include <stdio.h>
#include <math.h>

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

#define SAFE_RELEASE(punk)\
            if ( (punk) != NULL )\
                { (punk)->lpVtbl->Release(punk); (punk) = NULL; }


const int SAMPLE_RATE = 44100;
const int DURATION_SEC = 1;
const float FREQUENCY = 440.0f;

HRESULT PlaySineWave_C_Style() {
    HRESULT hr = S_OK;

    IMMDeviceEnumerator* pEnumerator   = NULL;
    IMMDevice*           pDevice       = NULL;
    IAudioClient*        pAudioClient  = NULL;
    IAudioRenderClient*  pRenderClient = NULL;
    WAVEFORMATEX*        pwfx          = NULL;

    UINT32 bufferFramecount  ;
    UINT32 numFramesPadding  ;
    BYTE*  pData             ;
    UINT32 numFramesAvailable;

    int i                                       ;
    int totalFrames = SAMPLE_RATE * DURATION_SEC;
    int currentFrame = 0                        ;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if ( FAILED(hr) ) {
        fprintf(stderr,
            "[PlaySineWave_C_Style]\x1b[31m(CoInitializeExFailed)\x1b[0m CoInitializeEx 실패. HR=\x1b[31m0x%lx\x1b[0m\n",
            hr
        );

        return hr;
    }

    hr = CoCreateInstance(
        &CLSID_MMDeviceEnumerator, NULL,
        CLSCTX_ALL, &IID_IMMDeviceEnumerator,
        (void**)&pEnumerator
    ); if ( FAILED(hr) ) goto exit;

    hr = pEnumerator->lpVtbl->GetDefaultAudioEndpoint(
        pEnumerator, eRender, eConsole, &pDevice
    ); if ( FAILED(hr) ) goto exit;

    hr = pDevice->lpVtbl->Activate(
        pDevice, &IID_IAudioClient, CLSCTX_ALL, NULL,
        (void**)&pAudioClient
    ); if ( FAILED(hr) ) goto exit;

    hr = pAudioClient->lpVtbl->GetMixFormat(
        pAudioClient, &pwfx
    ); if ( FAILED(hr) ) goto exit;

    REFERENCE_TIME hnsRequestedDuration = DURATION_SEC * 10000000;
    hr = pAudioClient->lpVtbl->Initialize(
        pAudioClient,
        AUDCLNT_SHAREMODE_SHARED,
        0,
        hnsRequestedDuration,
        0,
        pwfx,
        NULL
    ); if ( FAILED(hr) ) goto exit;

    hr = pAudioClient->lpVtbl->GetService(
        pAudioClient, &IID_IAudioRenderClient,
        (void**)&pRenderClient
    ); if ( FAILED(hr) ) goto exit;

    hr = pAudioClient->lpVtbl->GetBufferSize(pAudioClient, &bufferFramecount);
    if ( FAILED(hr) ) goto exit;

    hr = pAudioClient->lpVtbl->Start(pAudioClient);
    if ( FAILED(hr) ) goto exit;

    printf("[PlaySineWave_C_Style] 사인파 재생 시작...\n");

    while ( currentFrame < totalFrames ) {
        hr = pAudioClient->lpVtbl->GetCurrentPadding(pAudioClient, &numFramesPadding);
        if ( FAILED(hr) ) break;

        numFramesAvailable = bufferFramecount - numFramesPadding;

        if ( numFramesAvailable > (totalFrames-currentFrame) ) {
            numFramesAvailable = totalFrames - currentFrame;
        }

        if ( numFramesAvailable > 0 ) {
            hr = pRenderClient->lpVtbl->GetBuffer(
                pRenderClient, numFramesAvailable, &pData
            ); if ( FAILED(hr) ) break;

            float *pFloatData = (float*)pData  ;
            int   numChannels = pwfx->nChannels;

            for ( i=0; i<numFramesAvailable; i++ ) {
                float t      = (float)(currentFrame+i) / SAMPLE_RATE     ;
                float sample = sinf(2.0f * 3.1415926535f * FREQUENCY * t);
                for( int ch=0; ch<numChannels; ch++ ) {
                    *pFloatData++ = sample;
                }
            }
            currentFrame += numFramesAvailable;

            hr = pRenderClient->lpVtbl->ReleaseBuffer(
                pRenderClient, numFramesAvailable, 0
            ); if ( FAILED(hr) ) break;
        }

        Sleep(1);
    }

    Sleep(500);

    pAudioClient->lpVtbl->Stop(pAudioClient);

    exit:
        if ( pwfx != NULL ) CoTaskMemFree(pwfx);

        SAFE_RELEASE(pRenderClient);
        SAFE_RELEASE(pAudioClient);
        SAFE_RELEASE(pDevice);
        SAFE_RELEASE(pEnumerator);

        CoUninitialize();

        if ( FAILED(hr) ) {
            fprintf(stderr,
                "[PlaySineWave_C_Style]\x1b[31m(CoInitializeExFailed)\x1b[0m WASAPI에 오류가 발생헀습니다. HR=\x1b[31m0x%lx\x1b[0m\n",
                hr
            );
        } else {
            printf("재생 완료.\n");
        }

        return hr;
}

int main() {
    PlaySineWave_C_Style();

    return 0;
}