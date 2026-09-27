#include "GeneralInterface.h"
#include "EpollGpioEvent.h"
#include "VoiceRingPlay.h"
#include "LightControl.h"
#include "GpioControl.h"
#include "UserConfig.h"
#include "Unlock.h"
#include "Timer.h"
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>

#define LOCK_GPIO 34
#define GATE_GPIO 32
#define LOCK_3_GPIO 27

#define LOCK_3_detect_GPIO 25   //锁3开锁检测

static char LockGpio[] = {0, LOCK_GPIO, GATE_GPIO, LOCK_3_GPIO};
static void GateColse(void *u)
{
    printf("[%s]\n", __func__);
    GpioLevelSet(GATE_GPIO, GPIO_LEVEL_LOW);
    if (!TimerEnablestatus(LockTimer))
    {
        // KeyLightControl(TimerEnablestatus(MonitorTimer) || TimerEnablestatus(CommunicateTimer));
    }
}

static void LockColse(void *u)
{
    printf("[%s]\n", __func__);
    GpioLevelSet(LOCK_GPIO, GPIO_LEVEL_LOW);
    if (!TimerEnablestatus(GateTimer))
    {
        // KeyLightControl(TimerEnablestatus(MonitorTimer) || TimerEnablestatus(CommunicateTimer));
    }
}

static void Lock_3_Colse(void *u)
{
    printf("[%s]\n", __func__);
    GpioLevelSet(LOCK_3_GPIO, GPIO_LEVEL_LOW);
    if (!TimerEnablestatus(Lock_3_Timer))
    {
        // KeyLightControl(TimerEnablestatus(MonitorTimer) || TimerEnablestatus(CommunicateTimer));
    }
}

static int LockExitHandle(int Level)
{
    static struct timespec time;
    if (DiffClockTimeMs(&time) > 1 * 1000)
    {
        printf("[%s]Trigger:%d\n\n", __func__, Level);
        Unlock(UserConfigGet()->UnlockTime, LOCK_TYPE);
    }
    GetClockTimeMs(&time);
    return 0;
}
int LockExitEpollEventInit(struct EpollEvent *Event)
{
#define LOCK_EXIT_GPIO 25
    if (GpioOpen(LOCK_EXIT_GPIO, GPIO_DIR_IN, true) == false)
    {
        return -1;
    }
    GpioEdge(LOCK_EXIT_GPIO, FALLING_EDGE);
    char Path[64] = {0};
    memset(Path, 0, sizeof(Path));
    sprintf(Path, "/sys/class/gpio/gpio%d/value", LOCK_EXIT_GPIO);
    Event->Fd = open(Path, O_RDONLY);
    Event->TriggerLevel = 0;
    Event->EpollEventHandle = LockExitHandle;
    return 0;
}

static int GateExitHandle(int Level)
{
    static struct timespec time;
    if (DiffClockTimeMs(&time) > 1 * 1000)
    {
        // printf("[%s]Trigger:%d\n\n", __func__, Level);
        Unlock(UserConfigGet()->UngateTime, GATE_TYPE);
    }
    GetClockTimeMs(&time);
    return 0;
}
int GateExitEpollEventInit(struct EpollEvent *Event)
{
#define GATE_EXIT_GPIO 27
    if (GpioOpen(GATE_EXIT_GPIO, GPIO_DIR_IN, true) == false)
    {
        return -1;
    }
    GpioEdge(GATE_EXIT_GPIO, FALLING_EDGE);
    char Path[64] = {0};
    memset(Path, 0, sizeof(Path));
    sprintf(Path, "/sys/class/gpio/gpio%d/value", GATE_EXIT_GPIO);
    Event->Fd = open(Path, O_RDONLY);
    Event->TriggerLevel = 0;
    Event->EpollEventHandle = GateExitHandle;
    return 0;
}

/**
 * @description: 开锁
 * @param {int} time    开锁时间
 * @param {LockType} type   开锁类型
 * @return {*}0-失败，1-成功
 */
int Unlock(int time, LockType type)
{
    assert(type <= LOCK_3_TYPE);
    if (type == LOCK_TYPE)
    {
        if (SetTimer(time * 1000, LockTimer, LockColse, NULL))
        {
            if (!TimerEnablestatus(GateTimer) && UserConfigGet()->UnlockVoiceEn)
            {
                printf("[Unlock] Lock1 - About to play voice, Language:%d, VoiceIndex:%d\n",
                       UserConfigGet()->Language, UserConfigGet()->Language + UnlockEng);
                VoiceRingPlay(UserConfigGet()->Language + UnlockEng, VoiceDefVol);
            }
            printf("[%s] %ds\n", type == LOCK_TYPE ? "Unlock" : "Ungate", time);
            GpioLevelSet(LockGpio[type], GPIO_LEVEL_HIGH);
            // KeyLightControl(!(TimerEnablestatus(MonitorTimer) || TimerEnablestatus(CommunicateTimer)));
            return 1;
        }
    }else if (type == GATE_TYPE){
        if (SetTimer(time * 1000, GateTimer, GateColse, NULL))
        {
            if (!TimerEnablestatus(LockTimer) && UserConfigGet()->UnlockVoiceEn)
            {
                printf("bbbbbbbbbbbbbbbbbbbbb\n");
                VoiceRingPlay(UserConfigGet()->Language + UnlockEng, VoiceDefVol);
            }
            printf("[%s] %ds\n", type == LOCK_TYPE ? "Unlock" : "Ungate", time);
            GpioLevelSet(LockGpio[type], GPIO_LEVEL_HIGH);
            // KeyLightControl(!(TimerEnablestatus(MonitorTimer) || TimerEnablestatus(CommunicateTimer)));
            return 1;
        }
    }
    else if(type == LOCK_3_TYPE){
        if (SetTimer(time * 1000, Lock_3_Timer, Lock_3_Colse, NULL))
        {
            if (!TimerEnablestatus(LockTimer) && !TimerEnablestatus(GateTimer) && UserConfigGet()->UnlockVoiceEn)
            {
                printf("[Unlock] Lock3 - About to play voice, Language:%d, VoiceIndex:%d\n",
                       UserConfigGet()->Language, UserConfigGet()->Language + UnlockEng);
                VoiceRingPlay(UserConfigGet()->Language + UnlockEng, VoiceDefVol);
            }
            printf("lock3 %ds\n", time);
            GpioLevelSet(LOCK_3_GPIO, GPIO_LEVEL_HIGH);
            // KeyLightControl(!(TimerEnablestatus(MonitorTimer) || TimerEnablestatus(CommunicateTimer)));
            return 1;
        }
    }

    // if (SetTimer(time * 1000, type == LOCK_TYPE ? LockTimer : GateTimer, type == LOCK_TYPE ? LockColse : GateColse, NULL))
    // {
    //     if (!TimerEnablestatus(type == LOCK_TYPE ? GateTimer : LockTimer) && UserConfigGet()->UnlockVoiceEn)
    //     {
    //         VoiceRingPlay(UserConfigGet()->Language + UnlockEng, VoiceDefVol);
    //     }
    //     printf("[%s] %ds\n", type == LOCK_TYPE ? "Unlock" : "Ungate", time);
    //     GpioLevelSet(LockGpio[type], GPIO_LEVEL_HIGH);
    //     // KeyLightControl(!(TimerEnablestatus(MonitorTimer) || TimerEnablestatus(CommunicateTimer)));
    //     return 1;
    // }
    return 0;
}

void *gpio_det_task(void *arg)
{
    GPIO_LEVEL Level;
    while(1)
    {
        if (GpioLevelGet(LOCK_3_detect_GPIO, &Level) )
        {
            if (Level == GPIO_LEVEL_LOW)
            {
                Unlock(UserConfigGet()->Unlock_3_Time, LOCK_3_TYPE);
            }
        }
        usleep(1000);
    }
    return NULL;
}



/**
 * @description: 锁引脚初始化
 * @return {*}
 */
void LockGpioInit(void)
{
    for (int i = LOCK_TYPE; i < sizeof(LockGpio); i++)
    {
        if (GpioOpen(LockGpio[i], GPIO_DIR_LOW, false))
        {
        }
    }

    if (GpioOpen(LOCK_3_detect_GPIO, GPIO_DIR_IN, true) == false)
    {
        return;
    }
    printf("=====time3==================%d\n",UserConfigGet()->Unlock_3_Time);
    pthread_t thread_t;
	pthread_create(&thread_t, NULL, gpio_det_task, NULL);
    pthread_detach(thread_t);
}