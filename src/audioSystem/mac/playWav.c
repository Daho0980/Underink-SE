#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdatomic.h>

#include <AudioUnit/AudioUnit.h>

#include "playWav.h"

#include "soundStruct.h"
#include "wavFormat.h"

#define MIN(a,b) ((a)<(b) ? (a) : (b))


extern void updateAudioDataAmplitude(uint8_t* target, int bits, uint32_t size, float volume);

OSStatus _AURenderCallback(
    void                       *inRefCon     ,
    AudioUnitRenderActionFlags *ioActionFlags,
    const AudioTimeStamp       *inTimeStamp  ,
    UInt32                     inBusNumber   ,
    UInt32                     inNumberFrames,
    AudioBufferList            *ioData
) {
    inUserData_t *ctx = (inUserData_t*)inRefCon;

    if (
        atomic_load(ctx->finishFlag) ||
        atomic_load(ctx->pause)
    ) {
        for ( int j=0; j<ioData->mNumberBuffers; j++ ) {
            memset(
                ioData->mBuffers[j].mData, 0,
                ioData->mBuffers[j].mDataByteSize
            );
        }

        return noErr;
    }

    uint32_t bytesPerFrame = ctx->sampleSound.chunkSize
   ;uint64_t startOffset   = ctx->sampleSound.offset
   ;uint32_t bytesLeft     = ctx->size - startOffset
   ;
    uint32_t bytesToCopy = MIN((inNumberFrames*bytesPerFrame), bytesLeft);

    uint8_t *outBuffer = (uint8_t*)ioData->mBuffers[0].mData;
    memcpy(outBuffer, ctx->sampleSound.audioData+startOffset, bytesToCopy);

    // TODO: WE NEED FILTER CHAIN HERE

    updateAudioDataAmplitude(
        outBuffer,
        ctx->bits,
        bytesToCopy,
        *ctx->volume
    );
    
    ctx->sampleSound.offset += bytesToCopy;

    if ( bytesToCopy < (inNumberFrames*bytesPerFrame) ) {
        memset(
            outBuffer+bytesToCopy, 0,
            (inNumberFrames*bytesPerFrame)-bytesToCopy
        );

        atomic_store(ctx->finishFlag, true);
    }

    return noErr;
}

void play(AudioUnit *audioUnit,

          inUserData_t *inUserData,
          const char   mode[8]    ,

          SampleOrDataOnly data      ,
          uint32_t         size      ,
          int              sampleRate,
          int              channels  ,
          int              bits       ) {
    printf("play 함수 호출됨.\n");

    if ( strcmp(mode, "ALLINONE") == 0 ) {
        inUserData->sampleSound.audioData = data.dataOnly;
    }
    else if ( strcmp(mode, "SAMPLING") == 0 ) {
        inUserData->sampleSound.audioData = data.sampleData.audioData;
    }
    else {
        char tmp[9];
        memcpy(tmp, mode, 8);
        tmp[8] = '\0';
        fprintf(stderr,
            "[play]\x1b[31m(UndefinedMode)\x1b[0m '%s' 모드는 정의되지 않았습니다.\n",
            tmp
        );
        return;
    }
    inUserData->sampleSound.offset = 0
   ;inUserData->size               = size
   ;inUserData->bits               = bits
   ;
    printf("오디오 데이터 :\n");
    printf("\t데이터 위치 : \x1b[32m%p\x1b[0m\n", inUserData->sampleSound.audioData);
    printf("\t크기        : \x1b[32m%d\x1b[0m\n", size);
    printf("\t샘플레이트  : \x1b[32m%d\x1b[0m\n", sampleRate);
    printf("\t채널 수     : \x1b[32m%d\x1b[0m\n", channels);
    printf("\t비트 깊이   : \x1b[32m%d\x1b[0m\n", bits);

    AudioStreamBasicDescription clientFormat = {0};

    printf("오디오 포맷 설정 중... ");
    clientFormat.mSampleRate       = sampleRate
   ;clientFormat.mFormatID         = kAudioFormatLinearPCM
   ;clientFormat.mFramesPerPacket  = 1
   ;clientFormat.mChannelsPerFrame = channels
   ;
    switch (bits) {
        case 8:
            clientFormat.mFormatFlags    = kLinearPCMFormatFlagIsPacked
           ;clientFormat.mBitsPerChannel = 8;
           ;clientFormat.mBytesPerFrame  = channels
           ;
            break;
        case 16:
            clientFormat.mFormatFlags    = kLinearPCMFormatFlagIsSignedInteger |
                                           kLinearPCMFormatFlagIsPacked
           ;clientFormat.mBitsPerChannel = 16
           ;clientFormat.mBytesPerFrame  = channels * 2
           ;
            break;
        case 24:
            clientFormat.mFormatFlags    = kLinearPCMFormatFlagIsSignedInteger |
                                           kLinearPCMFormatFlagIsPacked
           ;clientFormat.mBitsPerChannel = 24
           ;clientFormat.mBytesPerFrame  = channels * 3
           ;
            break;
        case 32:
            clientFormat.mFormatFlags    = kLinearPCMFormatFlagIsSignedInteger |
                                           kLinearPCMFormatFlagIsPacked
           ;clientFormat.mBitsPerChannel = 32
           ;clientFormat.mBytesPerFrame  = channels * 4
           ;
            break;
            
        default:
            fprintf(stderr,
                "\n[play]\x1b[31m(UnsupportedBitDepth) 지원하지 않는 비트 깊이 : %d\n",
                bits
            );

            return;
    }
    clientFormat.mBytesPerPacket = clientFormat.mBytesPerFrame;
    inUserData->sampleSound.chunkSize = clientFormat.mBytesPerFrame;
    printf("완료\n");

    AudioComponentDescription desc = {0};
    desc.componentType = kAudioUnitType_Output;
    /**
     * macOS    : DefaultOutput
     * iOS/tvOS : RemoteIO
     */
    // macOS 기준으로 설계됨.
    desc.componentSubType = kAudioUnitSubType_DefaultOutput; 
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;

    AudioComponent outputComponent = AudioComponentFindNext(NULL, &desc);
    if ( outputComponent == NULL ) {
        fprintf(stderr,
            "[play]\x1b[31m(ComponentNotFound)\x1b[0m 기본 출력 장치를 찾을 수 없습니다.\n"
        );

        return;
    }

    printf("Audio Unit 인스턴스 생성 중... ");
    OSStatus status = AudioComponentInstanceNew(outputComponent, audioUnit);
    if ( status != noErr ) {
        fprintf(stderr,
            "\n[play]\x1b[31m(InstanceNewFailed)\x1b[0m Audio Unit 인스턴스 생성 실패: %d\n",
            (int)status
        );

        return;
    }
    printf("완료\n");

    printf("렌더링 콜백 설정 중... ");
    AURenderCallbackStruct renderCallback;
    renderCallback.inputProc       = _AURenderCallback
   ;renderCallback.inputProcRefCon = inUserData
   ;
    status = AudioUnitSetProperty(
        *audioUnit,
        kAudioUnitProperty_SetRenderCallback,
        kAudioUnitScope_Input,
        0, // Bus 0
        &renderCallback,
        sizeof(renderCallback)
    );
    if ( status != noErr ) {
        fprintf(stderr,
            "\n[play]\x1b[31m(SetCallbackFailed)\x1b[0m 렌더링 콜백 설정 실패: %d\n",
            (int)status
        );

        return;
    }
    printf("완료\n");

    printf("스트림 포맷 설정 중... ");
    status = AudioUnitSetProperty(
        *audioUnit,
        kAudioUnitProperty_StreamFormat,
        kAudioUnitScope_Input,
        0, // Bus 0
        &clientFormat,
        sizeof(clientFormat)
    );
    if ( status != noErr ) {
        fprintf(stderr,
            "\n[play]\x1b[31m(SetStreamFormatFailed)\x1b[0m 스트림 포맷 설정 실패: %d\n",
            (int)status
        );
        
        return;
    }
    printf("완료\n");
    
    printf("Audio Unit 초기화 중... ");
    status = AudioUnitInitialize(*audioUnit);
    if ( status != noErr ) {
        fprintf(stderr,
            "\n[play]\x1b[31m(InitializeFailed)\x1b[0m Audio Unit 초기화 실패: %d\n",
            (int)status
        );

        return;
    }
    printf("완료\n");

    printf("Audio Unit 시작 중... ");
    status = AudioOutputUnitStart(*audioUnit);
    if ( status != noErr ) {
        fprintf(stderr,
            "\n[play]\x1b[31m(StartFailed)\x1b[0m Audio Unit 시작 실패: %d\n",
            (int)status
        );

        return;
    }
    printf("완료\n");

    printf("사운드가 실행됩니다...\n");
}