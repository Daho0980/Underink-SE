#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>

#include <AudioUnit/AudioUnit.h>

#include "stdcmd.h"
#include "audioSystemArgs.h"
#include "audioSystem/audioChannel.h"

#include "audioEnv.h"

#include "playWav.h"
#include "commandQueue.h"


typedef struct {
    AudioUnit*        audioUnit ;
    AudioChannelData* data      ;
    struct LocalVol*  localVol  ;
    atomic_bool*      pause     ;
    atomic_bool*      finishFlag;
    AudioManager*     mgr       ;
    bool*             hasRequest;
} CommandHandlerContext;

extern struct LocalVol LocalVol_init();
extern void updateVolume(float* localCurrVolume, float localBaseVolume, float globalVolume);
extern void play(AudioUnit* audioUnit, inUserData_t* inUserData, const char mode[8], AudioModeUnion data, uint32_t size, int sampleRate, int channels, int bits);

void destroyAllChannelThreads(AudioManager* mgr, int index, bool force);
AudioChannel_t* audioChannel(void* arg);

static void _cleanupAudioUnit(AudioUnit* audioUnit);
static void _commandHandler_ALLINONE(uint32_t cmd, CommandHandlerContext* context);
static void _commandHandler_SAMPLING(uint32_t cmd, CommandHandlerContext* context);
static AudioModeUnion _getSoundData_ALLINONE(uint8_t* audioData);
static AudioModeUnion _getSoundData_SAMPLING(uint8_t* audioData);
static void _playMode_ALLINONE(AudioUnit* audioUnit, inUserData_t* inUserData, AudioModeUnion* soundData, Sound* sound);
static void _playMode_SAMPLING(AudioUnit* audioUnit, inUserData_t* inUserData, AudioModeUnion* soundData, Sound* sound);


void destroyAllChannelThreads(AudioManager* mgr, int index, bool force) {
    for ( int i=(index-1); i>=0; i-- ) {
        printf("[destroyAllChannelThreads] \x1b[33m%d\x1b[0m번째 스레드 파괴 중...", i);
        AudioChannelData* target = &mgr->threads[i];

        atomic_store(&target->running, false);
        pthread_cond_signal(&target->cond);
        if ( force ) commandPush(target, pkcmd_u8(cmd_stop()));
        
        while ( !atomic_load(&target->finished) ) {
            usleep(1000);
        }

        pthread_mutex_destroy(&target->mutex);
        pthread_cond_destroy(&target->cond);
        printf(" 완료\n");
    }

    free(mgr->threads);
    printf("[destroyAllChannelThreads]\x1b[32m(SUCCESS)\x1b[0m 모든 스레드가 파괴되었습니다.\n");
}

AudioChannel_t* audioChannel(void* arg) {
   ;AudioChannelData* data = (AudioChannelData*)((ThreadArg*)arg)->thread
   ;AudioManager*     mgr  = (AudioManager*)    ((ThreadArg*)arg)->mgr
   ;
    AudioUnit audioUnit = NULL;

    atomic_bool finished = false;
    atomic_bool pause    = false;

    struct LocalVol localVol = LocalVol_init();

    CommandHandlerContext ctx;
    ctx.audioUnit  = &audioUnit
   ;ctx.data       = data
   ;ctx.localVol   = &localVol
   ;ctx.pause      = &pause
   ;ctx.finishFlag = &finished
   ;ctx.mgr        = mgr
   ;ctx.hasRequest = &data->hasRequest
   ;
    void (*playmode)(
        AudioUnit*      audioUnit ,
        inUserData_t*   inUserData,
        AudioModeUnion* soundData ,
        Sound*          sound
    )
   ;AudioModeUnion (*getSoundData)(uint8_t* audioIndex)
   ;void           (*commandHandler)(uint32_t cmd, CommandHandlerContext* context)
   ;
    if ( memcmp(mgr->mode, "ALLINONE", 8) == 0 ) {
        playmode       = _playMode_ALLINONE
       ;getSoundData   = _getSoundData_ALLINONE
       ;commandHandler = _commandHandler_ALLINONE
       ;
    }
    else if ( memcmp(mgr->mode, "SAMPLING", 8) == 0 ) {
        playmode       = _playMode_SAMPLING
       ;getSoundData   = _getSoundData_SAMPLING
       ;commandHandler = _commandHandler_SAMPLING
       ;
    }
    else {
        fprintf(stderr,
            "[audioChannel]\x1b[31m(UndefinedMode)\x1b[0m 정의되지 않은 모드입니다.\n"
        );

        pthread_mutex_unlock(&data->mutex);
        free(arg);

        return NULL;
    }

    pthread_mutex_lock(&data->mutex);

    while ( atomic_load(&data->running) ) {
        while ( !data->hasRequest ) {
            pthread_cond_wait(&data->cond, &data->mutex);
            if ( !atomic_load(&data->running) ) goto cleanup;
        }

        char totalTag[
            (MAX_TAG_LEN*MAX_TAG) + 
            (2*(MAX_TAG-1))       +
            1
        ]; totalTag[0] = '\0';
        {
            char (*tagIter)[MAX_TAG_LEN] = data->request.tags
           ;int  count                   = 0
           ;
            if ( **tagIter != '\0' ) {
                strcpy(totalTag, *tagIter);
                tagIter++;
                count  ++;
            }
            
            while ( **tagIter!='\0' && count<MAX_TAG ) {
                strcat(totalTag, ", ");
                strcat(totalTag, *tagIter);
                tagIter++;
                count  ++;
            }
        } /** totalTag - 디버깅용입니다! */

        {
            Sound* soundData = data->request.soundData;
            pthread_mutex_unlock(&data->mutex);

            inUserData_t inUserData;
            inUserData.finishFlag = &finished
           ;inUserData.pause      = &pause
           ;inUserData.volume     = &localVol.currVolume
           ;
            AudioModeUnion playChunk = getSoundData(soundData->origin);
    
            playmode(
                &audioUnit,
                &inUserData,
                &playChunk,
                soundData
            );
        }

        printf(
            "[Thread %d(\x1b[33m%s\x1b[0m)] 재생 시작\n",
            data->id,
            totalTag
        );

        while ( !atomic_load(&finished) ) {
            if ( !(
                isCommandQueueEmpty_u32(&data->command) &&
                isCommandQueueEmpty_u8(&data->command)
            ) ) {
                commandHandler(commandPop(&data->command), &ctx);
            } else {
                usleep(1000);
                
                if ( localVol.fadeActive ) {
                    localVol.baseVolume += localVol.decayFactor;
                    updateVolume(
                        &localVol.currVolume,
                        localVol.baseVolume,
                        mgr->volume
                    );

                    if ( (localVol.target-localVol.baseVolume)*localVol.decayFactor <= 0.0f ) {
                        localVol.baseVolume = localVol.target;
                        localVol.fadeActive = false;
                        
                        if ( localVol.decayFactor<0.0f && localVol.baseVolume<=0.0f ) {
                            atomic_store(&finished, true);
                        }
                    }

                }
            }
        }

        pthread_mutex_lock(&data->mutex);

        _cleanupAudioUnit(&audioUnit);
        printf("[Thread %d(\x1b[33m%s\x1b[0m)] 재생 완료\n", data->id, totalTag);

        finish_request:
            data->hasRequest = false;
            free(data->request.soundData->origin);
            free(data->request.soundData);
            atomic_store(&pause, false);
            atomic_store(&finished, false);
    }

    cleanup:
        printf("\x1b[34m승상\x1b[0m께서\x1b[31m빈찬합\x1b[0m을보내주시다니\x1b[33m더살아무엇하리\x1b[0m!!!!!!!!!!!!\n");
        pthread_mutex_unlock(&data->mutex);
        free(arg);
        _cleanupAudioUnit(&audioUnit);
        AudioComponentInstanceDispose(audioUnit);
        atomic_store(&data->finished, true);

        return NULL;
}

void _cleanupAudioUnit(AudioUnit* audioUnit) {
    if ( !audioUnit || !*audioUnit ) return;

    AudioOutputUnitStop(*audioUnit);
    AudioUnitUninitialize(*audioUnit);
}

void _commandHandler_ALLINONE(uint32_t cmd, CommandHandlerContext* context) {
    switch ( (uint8_t)(cmd>>28) ) {
        case CMD_CONTINUE: break;
        case CMD_PAUSE   :
        case CMD_STOP    :
            atomic_store(context->finishFlag, true);
            _cleanupAudioUnit(context->audioUnit);

            break;

        case CMD_SET_VOLUME:
            updateVolume(
                &context->localVol->currVolume,
                context->localVol->baseVolume ,
                context->mgr->volume
            );

            break;

        case CMD_FADE: break;

        case CMD_ERROR:
            fprintf(stderr,
                "[commandHandler]\x1b[31m(ErrorCommand)\x1b[0m 에러 처리 명령이 들어왔습니다. : %u\n",
                cmd
            );

            break;
    }

    return;
}

void _commandHandler_SAMPLING(uint32_t cmd, CommandHandlerContext* context) {
    switch ( (uint8_t)(cmd>>28) ) {
        case CMD_CONTINUE:
            if ( context->hasRequest ) {
                atomic_store(context->pause, false);
            } else {
                fprintf(stderr,
                    "[_commandHandler_SAMPLING]\x1b[31m(RequestNotFound)\x1b[0m 요청이 존재하지 않습니다.\n"
                );
            }

            break;

        case CMD_PAUSE:
            atomic_store(context->pause, true);

            break;

        case CMD_STOP:
            atomic_store(context->finishFlag, true);

            break;

        case CMD_SET_VOLUME:
            updateVolume(
                &context->localVol->currVolume,
                context->localVol->baseVolume ,
                context->mgr->volume
            );

            break;

        case CMD_FADE: {
            struct LocalVol* LV = context->localVol;

           ;uint16_t duration_ms = (uint16_t)(cmd&0xFFFF)
           ;float    target_vol  = VOLUME_SCALE * (float)((cmd>>16)&0x0FFF)
           ;
            LV->fadeActive  = true
           ;LV->decayFactor = (target_vol-LV->baseVolume) / (float)duration_ms
           ;LV->target      = target_vol
           ;
            break;
        }

        case CMD_ERROR:
            fprintf(stderr,
                "[commandHandler]\x1b[31m(ErrorCommand)\x1b[0m 에러 처리 명령이 들어왔습니다. : %u\n",
                cmd
            );

            break;
    }

    return;
}

AudioModeUnion _getSoundData_ALLINONE(uint8_t* audioIndex) {
    AudioModeUnion soundData = { .audioOrigin=audioIndex };

    return soundData;
}
AudioModeUnion _getSoundData_SAMPLING(uint8_t* audioIndex) {
    AudioFrameConfig a_config = initAudioFrameConfig(
        audioIndex,
        gAudioEnv.samplingFrameSize
    );
    AudioModeUnion soundData = { .config=a_config };

    return soundData;
}

void _playMode_ALLINONE(
    AudioUnit*        audioUnit ,
    inUserData_t*     inUserData,
    AudioModeUnion* soundData ,
    Sound*            sound
) {
    play(
        audioUnit,
        inUserData,
        "ALLINONE",
        *soundData,
        sound->size,
        sound->sampleRate,
        sound->channels,
        sound->bits
    );
}
void _playMode_SAMPLING(
    AudioUnit*        audioUnit ,
    inUserData_t*     inUserData,
    AudioModeUnion* soundData ,
    Sound*            sound
) {
    play(
        audioUnit,
        inUserData,
        "SAMPLING",
        *soundData,
        sound->size,
        sound->sampleRate,
        sound->channels,
        sound->bits
    );
}