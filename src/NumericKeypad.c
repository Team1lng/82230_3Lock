/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-24 15:33:36
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-11-01 14:50:38
 * @FilePath: /82225-EPC/src/NumericKeypad.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "GeneralInterface.h"
#include "NumericKeypad.h"
#include "SecurityMode.h"
#include "VoiceRingPlay.h"
#include "LightControl.h"
#include "GpioControl.h"
#include "UserConfig.h"
#include "UserCard.h"
#include "Unlock.h"
#include "Timer.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>

#define Atoi(c) (c - 48)
#define IsDigit(c) ((c) >= '0' && (c) <= '9')
#define DEFAULT_FACTORY_SET_FLAG  9

static ActionRoute RoutesMap[ActionTotal];

void KeypadLightDisable(void *us)
{
    if (TimerEnablestatus(KeypadTimer))
    {
        TimerDestroy(KeypadTimer);
    }
    KeypadLightControl(0);
}

void KeypadLightEnable(void)
{
    extern int DarkModeStatus(void);
    int LightTime = DarkModeStatus() ? (UserConfigGet()->NumKeyLightTime ? UserConfigGet()->NumKeyLightTime : 86400) * 1000 : 10 * 1000;
    if (TimerEnablestatus(KeypadTimer))
    {
        RefreshTimer(LightTime, KeypadTimer);
    }
    else
    {
        SetTimer(LightTime, KeypadTimer, KeypadLightDisable, NULL);
    }
    KeypadLightControl(1);
}
/*********************************************************************************************************************************/

static void PushRouteStack(KeyAction Actiom);
static void PopRouteStack(void);
static RouteStack ActionStack;

static void PrintfActionStackMap(void)
{
    PrintCentered("\033[33mActionStack Top\033[0m", 49);
    for(int i = ActionStack.CurrentRoute;i >= 0 ;i --)
    {
        PrintCentered(ActionStack.Routes[i]->ActionStr, 40);
    }
    PrintCentered("\033[33mActionStack Bottom\033[0m", 49);
}

static void ModifyCardCodeTimerHandle(void *us)
{
    struct timespec time;
    GetClockTimeMs(&time);
    printf("ModifyCodeCardTimer[Disable]\n");
    printf("No operation for 30 seconds, exit management mode!!! [%ld]\n",time.tv_sec);
    PopRouteStack();
}

static void AdminOvertimeHandle(void *us)
{
    struct timespec time;
    GetClockTimeMs(&time);
    printf("No operation for 30 seconds, exit management mode!!! [%ld]\n",time.tv_sec);
    while (ActionStack.CurrentRoute != KeyStandby)
    {
        PopRouteStack();
    }
    VoiceRingPlay(Bi3, VoiceDefVol);
}

void StandbyError(void)
{
    printf("\n");
    VoiceRingPlay(Bi4, VoiceDefVol);
    return;
}

void ErrorHandle(void)
{
    printf("%s\n", __func__);
    PopRouteStack();
    PopRouteStack();
    VoiceRingPlay(Bi4, VoiceDefVol);
    return;
}

int EnterStandby(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    if(TimerEnablestatus(AdminOutTimer))
    {
        printf("[%s][%d] Destroy AdminOutTimer[Currnet Status %s] !!!!!\n",__func__,__LINE__,TimerEnablestatus(AdminOutTimer) ? "true":"false");
        TimerDestroy(AdminOutTimer);
    }
    return 1;
}

int CodeUnlock(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    printf("[Curret %s Unlock] \n", UserConfigGet()->LockWay == CardAndCodeWay ? "CardAndCode" : UserConfigGet()->LockWay == CardOrCodeWay ? "CardOrCode"
                                                                                                                                           : "CardWay");
    int Permission = NONE_TYPE;
    if (UserConfigGet()->PublicUnlockEn)
    {
        printf("KeyAttr->Cursor====%d        KeyAttr->Buff=======%s\n",KeyAttr->Cursor,KeyAttr->Buff);
        if((KeyAttr->Cursor - 1) == strlen(UserConfigGet()->UnlockCode))
        {
            if (memcmp(KeyAttr->Buff, UserConfigGet()->UnlockCode, KeyAttr->Cursor - 1) == 0)
            {
                Permission |= LOCK_TYPE;
            }
        }
            
        if((KeyAttr->Cursor - 1) == strlen(UserConfigGet()->UngateCode))
        {
            if (memcmp(KeyAttr->Buff, UserConfigGet()->UngateCode, KeyAttr->Cursor - 1) == 0)
            {
                Permission |= GATE_TYPE;
            }
        }
        
        if((KeyAttr->Cursor - 1) == strlen(UserConfigGet()->Unlock_3_Code))
        {
            if (memcmp(KeyAttr->Buff, UserConfigGet()->Unlock_3_Code, KeyAttr->Cursor - 1) == 0)
            {
                Permission |= 4;  //LOCK_3_TYPE ~ 4
            }
        }

        if(Permission != NONE_TYPE)
        {
            goto finish;
        }
        printf("Public Unlock Code:%s\n", UserConfigGet()->UnlockCode);
        printf("Public Ungate Code:%s\n", UserConfigGet()->UngateCode);
        printf("Public Ungate Code:%s\n", UserConfigGet()->Unlock_3_Code);
    }

    if (UserConfigGet()->LockWay == CardWay)
    {
        goto error;
    }
    else if (UserConfigGet()->LockWay == CardAndCodeWay)
    {
        if (!TimerEnablestatus(CodeCardUnlockTimer))
            goto error;

        if (memcmp(CARD_INITIAL_CODE, KeyAttr->Buff, KeyAttr->Cursor - 1) == 0)
            goto error;

        int *CardIndex = TimerGet(CodeCardUnlockTimer)->Data;
        Permission = CardCodeVerify(*CardIndex, KeyAttr->Buff, KeyAttr->Cursor - 1);

        /* 无论成功与否结束本次密码加卡开锁 */
        TimerDestroy(CodeCardUnlockTimer);
    }
    else
    {
        Permission = CardCodePermission(KeyAttr->Buff, KeyAttr->Cursor - 1);
    }

finish:
    if (Permission)
    {
        SecurityErrorReset();
        VoiceRingPlay(Bi1, VoiceDefVol);

        if (Permission & LOCK_TYPE)
            Unlock(UserConfigGet()->UnlockTime, LOCK_TYPE);
        if (Permission & GATE_TYPE)
            Unlock(UserConfigGet()->UngateTime, GATE_TYPE);
        if (Permission & 4)    //LOCK_3_TYPE ~ 4
            Unlock(UserConfigGet()->Unlock_3_Time, LOCK_3_TYPE);
        return 1;
    }

error:
    SecurityErrorUpdate();
    printf("[%s][%d]\n",__func__,__LINE__);
    return 0;
}

int ExitStandbyReady(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    if (TimerEnablestatus(ModifyCodeCardTimer))
    {
        printf("ModifyCodeCardTimer[Disable]\n");
        TimerDestroy(ModifyCodeCardTimer);
    }
    return 1;
}

int StandbyReadyMode(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    if (!TimerEnablestatus(ModifyCodeCardTimer))
    {
        printf("ModifyCodeCardTimer[Enable]\n");
        SetTimer(30 * 1000, ModifyCodeCardTimer, ModifyCardCodeTimerHandle, NULL);
    }
    PushRouteStack(KeyStandbyReady);
    return 1;
}

int ReturnRoute(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    VoiceRingPlay(Bi2, VoiceDefVol);
    PopRouteStack();
    return 1;
}

int ReturnStandby(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    while (ActionStack.CurrentRoute != KeyStandby)
    {
        PopRouteStack();
    }
    VoiceRingPlay(Bi2, VoiceDefVol);
    return 1;
}

int EnterModifyCardCode(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    // printf("[%s] Cursor:%d,TimerEm:%d\n", __func__, KeyAttr->Cursor, TimerEnablestatus(ModifyCodeCardTimer));
    if (KeyAttr->Cursor - 1 != sizeof(UserCardGet()->Deck[0].Code))
        goto error;

    if (!TimerEnablestatus(ModifyCodeCardTimer))
        goto error;

    int *CardIndex = TimerGet(ModifyCodeCardTimer)->Data;
    if (CardIndex)
    {
        if (CardCodeVerify(*CardIndex, KeyAttr->Buff, KeyAttr->Cursor - 1))
        {
            if (TimerEnablestatus(ModifyCodeCardTimer))
            {
                printf("ModifyCodeCardTimer[Disable]\n");
                TimerDestroy(ModifyCodeCardTimer);
            }
            PushRouteStack(KeyNewCardCode);
            VoiceRingPlay(Bi2, VoiceDefVol);
            return 1;
        }
    }
error:
TimerDestroy(ModifyCodeCardTimer);
    return 0;
}

int EnterAffirmCardCode(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    if (KeyAttr->Cursor - 1 != sizeof(UserCardGet()->Deck[0].Code) && CurrRoute->RouteData)
    {
        return 0;
    }

    static char Buf[32] = {0};
    memset(Buf, 0, sizeof(Buf));
    memcpy(Buf, KeyAttr->Buff, KeyAttr->Cursor - 1);
    CurrRoute->RouteData = Buf;
    PushRouteStack(KeyAffiCardCode);
    VoiceRingPlay(Bi1, VoiceDefVol);
    return 1;
}

int EnterCardCodeVerify(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    if (KeyAttr->Cursor - 1 != sizeof(UserCardGet()->Deck[0].Code) && CurrRoute->RouteData)
    {
        goto error;
    }
    int *CardIndex = TimerGet(ModifyCodeCardTimer)->Data;

    if (memcmp(KeyAttr->Buff, CurrRoute->RouteData, KeyAttr->Cursor - 1) == 0)
    {
        printf("[Verify Pass!!! Modify Card[%d] Code Succeed]!!\n", *CardIndex);
        memcpy(UserCardGet()->Deck[*CardIndex].Code, KeyAttr->Buff, KeyAttr->Cursor - 1);
        UserCardSave();
        PopRouteStack();
        PopRouteStack();
        PopRouteStack();
        VoiceRingPlay(Bi2, VoiceDefVol);
        return 1;
    }
error:
    PopRouteStack();
    PopRouteStack();
    PopRouteStack();
    return 0;
}

int NewCardCodeVerify(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    return 1;
}

int EnterAdminMode(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    if (KeyAttr->Cursor - 1 != strlen(UserConfigGet()->AdminCode))
    {
        printf("[%s]Not enough password bits!!\n", __func__);
        return 0;
    }

    if (memcmp(KeyAttr->Buff, UserConfigGet()->AdminCode, strlen(UserConfigGet()->AdminCode)) == 0)
    {
        if (TimerEnablestatus(ModifyCodeCardTimer))
        {
            printf("ModifyCodeCardTimer[Disable]\n");
            TimerDestroy(ModifyCodeCardTimer);
        }
        PushRouteStack(KeyAdmin);
        printf("[%s][%d] Open AdminOutTimer !!!!!\n",__func__,__LINE__);
        SetTimer(30 * 1000, AdminOutTimer, AdminOvertimeHandle, NULL);
        VoiceRingPlay(Bi2, VoiceDefVol);
        return 1;
    }
    return 0;
}

int ExitAdminMode(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    if(TimerEnablestatus(AdminOutTimer))
    {
        printf("[%s][%d] Destroy AdminOutTimer[Currnet Status %s] !!!!!\n",__func__,__LINE__,TimerEnablestatus(AdminOutTimer) ? "true":"false");
        TimerDestroy(AdminOutTimer);
    }
    return 1;
}

int RestoreFactory(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    UserConfigReset();
    RefreshTimer(30 * 1000, AdminOutTimer);
    VoiceRingPlay(Bi1, VoiceDefVol);
    return 1;
}

int UnlockVoiceSwitch(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    if (Atoi(KeyAttr->Buff[2]) != 1 && Atoi(KeyAttr->Buff[2]) != 0)
        return 0;

    UserConfigGet()->UnlockVoiceEn = Atoi(KeyAttr->Buff[2]);
    UserConfigSave();  //hare set
    UserCardSave();
    VoiceRingPlay(Bi2, VoiceDefVol);
    return 1;
}

int EnterNewAdminCode(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    PushRouteStack(KeyNewAdminCode);
    RefreshTimer(30 * 1000, AdminOutTimer);
    VoiceRingPlay(Bi2, VoiceDefVol);
    return 1;
}

int EnterAffirmAdminCode(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    RefreshTimer(30 * 1000, AdminOutTimer);
    static char Buff[ADMIN_CODE_LEN] = {0};
    if (KeyAttr->Cursor - 1 != strlen(UserConfigGet()->AdminCode))
    {
        printf("[%s]Not enough password bits!!\n", __func__);
        return 0;
    }
    memset(Buff, 0, sizeof(Buff));
    memcpy(Buff, KeyAttr->Buff, KeyAttr->Cursor - 1);
    CurrRoute->RouteData = Buff;
    PushRouteStack(KeyAffiAdminCode);
    VoiceRingPlay(Bi1, VoiceDefVol);
    return 1;
}

int EnterAdminCodeVerify(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    RefreshTimer(30 * 1000, AdminOutTimer);
    if (!CurrRoute->RouteData || KeyAttr->Cursor - 1 != ADMIN_CODE_LEN)
        return 0;

    if (memcmp(KeyAttr->Buff, CurrRoute->RouteData, KeyAttr->Cursor - 1) == 0)
    {
        printf("[Verify Pass!!! Modify Admin Code Succeed]!!\n");
        memcpy(UserConfigGet()->AdminCode, KeyAttr->Buff, KeyAttr->Cursor - 1);
        UserConfigSave();
        PopRouteStack();
        PopRouteStack();
        VoiceRingPlay(Bi2, VoiceDefVol);
        return 1;
    }
    printf("[AdminCodeVerify Fail]!!\n");
    return 0;
}

int EnterNewLockCode(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    static bool DefaultUnlockSetFlag;
    DefaultUnlockSetFlag = false;

    /* 
    *修改默认出厂Gate开锁密码 022 + DEFAULT_FACTORY_SET_FLAG
    *修改用户Gate开锁密码 022
    */
    if(KeyAttr->Cursor - 1 == 4 && Atoi(KeyAttr->Buff[3]) == DEFAULT_FACTORY_SET_FLAG)
    {
        DefaultUnlockSetFlag = true;
    }
    else if(KeyAttr->Cursor - 1 != 3)
    {
        return 0;
    }
    printf("[%s][%d]%d,%s,%d\n",__func__,__LINE__,KeyAttr->Cursor - 1,KeyAttr->Buff,DefaultUnlockSetFlag);
    RefreshTimer(30 * 1000, AdminOutTimer);
    PushRouteStack(KeyNewLockCode);
    VoiceRingPlay(Bi2, VoiceDefVol);

    RoutesMap[KeyNewLockCode].RouteData = &DefaultUnlockSetFlag;
    return 1;
}

int EnterAffirmLockCode(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    RefreshTimer(30 * 1000, AdminOutTimer);
    static char Buff[UNLOCK_CODE_MAX_LEN + 1] = {0};
    if ((KeyAttr->Cursor - 1 > UNLOCK_CODE_MAX_LEN) || (KeyAttr->Cursor - 1 < UNLOCK_CODE_MIN_LEN))
    {
        printf("[%s]The number of password bits is not in the valid range!!\n", __func__);
        return 0;
    }
    memset(Buff, 0, sizeof(Buff));
    memcpy(Buff, KeyAttr->Buff, KeyAttr->Cursor -1);
    PushRouteStack(KeyAffiLockCode);
    RoutesMap[KeyAffiLockCode].RouteData = Buff;
    VoiceRingPlay(Bi1, VoiceDefVol);
    return 1;
}

int EnterLockCodeVerify(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    RefreshTimer(30 * 1000, AdminOutTimer);
    if (!CurrRoute->RouteData || KeyAttr->Cursor - 1 != strlen((char *)CurrRoute->RouteData) || KeyAttr->Cursor - 1 > sizeof(UserConfigGet()->UnlockCode))
    {
        // printf("[%s][%d][%s][%s]\n",__func__,__LINE__,KeyAttr->Buff,CurrRoute->RouteData);
        printf("[%s]The password length is different between the two times!!? [%p][%d][%d]\n", __func__,CurrRoute->RouteData,strlen((char *)CurrRoute->RouteData),KeyAttr->Cursor - 1);
        return 0;
    }
    // printf("[%s][%d][%s][%s]\n",__func__,__LINE__,KeyAttr->Buff,CurrRoute->RouteData);
    if (memcmp(KeyAttr->Buff, CurrRoute->RouteData, KeyAttr->Cursor -1) == 0)
    {
        bool DefaultUnlockSetFlag = RoutesMap[KeyNewLockCode].RouteData ? (*(bool *)RoutesMap[KeyNewLockCode].RouteData) : false;
        if(DefaultUnlockSetFlag)
        {
            memset(UserDefaultConfigGet()->UnlockCode,0,sizeof(UserDefaultConfigGet()->UnlockCode));
            memcpy(UserDefaultConfigGet()->UnlockCode, CurrRoute->RouteData, strlen((char *)CurrRoute->RouteData));
            UserDefaultConfigSave();
            printf("[Verify Pass!!! Modify Factory Default Lock Code Succeed]!!\n");
        }

        printf("[Verify Pass!!! Modify Lock Code Succeed]!!\n");
        memset(UserConfigGet()->UnlockCode,0,sizeof(UserConfigGet()->UnlockCode));
        memcpy(UserConfigGet()->UnlockCode, CurrRoute->RouteData, strlen((char *)CurrRoute->RouteData));

        UserConfigSave();
        PopRouteStack();
        PopRouteStack();
        VoiceRingPlay(Bi2, VoiceDefVol);
        return 1;
    }
    printf("[LockCodeVerify Fail]!!\n");
    return 0;
}

int EnterNewGateCode(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    static bool DefaultUnlockSetFlag;
    DefaultUnlockSetFlag = false;
    /* 
    *修改默认出厂Lock开锁密码 011 + DEFAULT_FACTORY_SET_FLAG
    *修改用户Lock开锁密码 011
    */
    if(KeyAttr->Cursor - 1 == 4 && Atoi(KeyAttr->Buff[3]) == DEFAULT_FACTORY_SET_FLAG)
    {
        DefaultUnlockSetFlag = true;
    }
    else if(KeyAttr->Cursor - 1 != 3)
    {
        return 0;
    }
    RefreshTimer(30 * 1000, AdminOutTimer);
    PushRouteStack(KeyNewGateCode);
    VoiceRingPlay(Bi2, VoiceDefVol);

    RoutesMap[KeyNewGateCode].RouteData = &DefaultUnlockSetFlag;
    return 1;
}

int EnterAffirmGateCode(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    RefreshTimer(30 * 1000, AdminOutTimer);
    static char Buff[UNLOCK_CODE_MAX_LEN + 1] = {0};
    if ((KeyAttr->Cursor - 1 > UNLOCK_CODE_MAX_LEN) || (KeyAttr->Cursor - 1 < UNLOCK_CODE_MIN_LEN))
    {
        printf("[%s]The number of password bits is not in the valid range!!\n", __func__);
        return 0;
    }
    memset(Buff, 0, sizeof(Buff));
    memcpy(Buff, KeyAttr->Buff, KeyAttr->Cursor -1);
    PushRouteStack(KeyAffiGateCode);
    RoutesMap[KeyAffiGateCode].RouteData = Buff;
    VoiceRingPlay(Bi1, VoiceDefVol);
    return 1;
}

int EnterGateCodeVerify(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    RefreshTimer(30 * 1000, AdminOutTimer);
    if (!CurrRoute->RouteData || KeyAttr->Cursor - 1 != strlen((char *)CurrRoute->RouteData) || KeyAttr->Cursor - 1 > sizeof(UserConfigGet()->UngateCode))
    {
        printf("[%s]The password length is different between the two times!!? [%p]\n", __func__,CurrRoute->RouteData);
        return 0;
    }

    if (memcmp(KeyAttr->Buff, CurrRoute->RouteData, KeyAttr->Cursor -1) == 0)
    {
        bool DefaultUnlockSetFlag = RoutesMap[KeyNewGateCode].RouteData ? (*(bool *)RoutesMap[KeyNewGateCode].RouteData) : false;
        if(DefaultUnlockSetFlag)
        {
            memset(UserDefaultConfigGet()->UngateCode,0,sizeof(UserDefaultConfigGet()->UngateCode));
            memcpy(UserDefaultConfigGet()->UngateCode, CurrRoute->RouteData, strlen((char *)CurrRoute->RouteData));
            UserDefaultConfigSave();
            printf("[Verify Pass!!! Modify Factory Default Gate Code Succeed]!!\n");
        }

        printf("[Verify Pass!!! Modify Gate Code Succeed]!!\n");
        memset(UserConfigGet()->UngateCode,0,sizeof(UserConfigGet()->UngateCode));
        memcpy(UserConfigGet()->UngateCode, CurrRoute->RouteData, strlen((char *)CurrRoute->RouteData));
        
        UserConfigSave();
        PopRouteStack();
        PopRouteStack();
        VoiceRingPlay(Bi2, VoiceDefVol);
        return 1;
    }
    printf("[GateCodeVerify Fail]!!\n");
    return 0;
}
/* 作者:hare*************************************************************************修改lock_3密码*********************************************************************************** */
int EnterNewLock_3_Code(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    static bool DefaultUnlockSetFlag;
    DefaultUnlockSetFlag = false;

    /* 
    *修改默认出厂lock_3开锁密码 033 + DEFAULT_FACTORY_SET_FLAG
    *修改用户lovk_3开锁密码 033
    */
    if(KeyAttr->Cursor - 1 == 4 && Atoi(KeyAttr->Buff[3]) == DEFAULT_FACTORY_SET_FLAG)
    {
        DefaultUnlockSetFlag = true;
    }
    else if(KeyAttr->Cursor - 1 != 3)
    {
        return 0;
    }
    printf("[%s][%d]%d,%s,%d\n",__func__,__LINE__,KeyAttr->Cursor - 1,KeyAttr->Buff,DefaultUnlockSetFlag);
    RefreshTimer(30 * 1000, AdminOutTimer);
    PushRouteStack(KeyNewLock_3_Code);
    VoiceRingPlay(Bi2, VoiceDefVol);

    RoutesMap[KeyNewLock_3_Code].RouteData = &DefaultUnlockSetFlag;
    return 1;
}

int EnterAffirmLock_3_Code(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    RefreshTimer(30 * 1000, AdminOutTimer);
    static char Buff[UNLOCK_CODE_MAX_LEN] = {0};
    if ((KeyAttr->Cursor - 1 > UNLOCK_CODE_MAX_LEN) || (KeyAttr->Cursor - 1 < UNLOCK_CODE_MIN_LEN))
    {
        printf("[%s]The number of password bits is not in the valid range!!\n", __func__);
        return 0;
    }
    memset(Buff, 0, sizeof(Buff));
    memcpy(Buff, KeyAttr->Buff, KeyAttr->Cursor -1);
    PushRouteStack(KeyAffiLock_3_Code);
    RoutesMap[KeyAffiLock_3_Code].RouteData = Buff;
    VoiceRingPlay(Bi1, VoiceDefVol);
    return 1;
}

int EnterLock_3_CodeVerify(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    RefreshTimer(30 * 1000, AdminOutTimer);
    if (!CurrRoute->RouteData || KeyAttr->Cursor - 1 != strlen((char *)CurrRoute->RouteData) || KeyAttr->Cursor - 1 > sizeof(UserConfigGet()->Unlock_3_Code))
    {
        printf("[%s]The password length is different between the two times!!? [%p][%d][%d]\n", __func__,CurrRoute->RouteData,strlen((char *)CurrRoute->RouteData),KeyAttr->Cursor - 1);
        return 0;
    }
    // printf("[%s][%d][%s][%s]\n",__func__,__LINE__,KeyAttr->Buff,CurrRoute->RouteData);
    if (memcmp(KeyAttr->Buff, CurrRoute->RouteData, KeyAttr->Cursor -1) == 0)
    {
        bool DefaultUnlockSetFlag = RoutesMap[KeyNewLock_3_Code].RouteData ? (*(bool *)RoutesMap[KeyNewLock_3_Code].RouteData) : false;
        if(DefaultUnlockSetFlag)
        {
            memset(UserDefaultConfigGet()->Unlock_3_Code,0,sizeof(UserDefaultConfigGet()->Unlock_3_Code));
            memcpy(UserDefaultConfigGet()->Unlock_3_Code, CurrRoute->RouteData, strlen((char *)CurrRoute->RouteData));
            UserDefaultConfigSave();
            printf("[Verify Pass!!! Modify Factory Default Lock Code Succeed]!!\n");
        }

        printf("[Verify Pass!!! Modify Lock Code Succeed]!!\n");
        memset(UserConfigGet()->Unlock_3_Code,0,sizeof(UserConfigGet()->Unlock_3_Code));
        memcpy(UserConfigGet()->Unlock_3_Code, CurrRoute->RouteData, strlen((char *)CurrRoute->RouteData));

        UserConfigSave();
        PopRouteStack();
        PopRouteStack();
        VoiceRingPlay(Bi2, VoiceDefVol);
        return 1;
    }
    printf("[LockCodeVerify Fail]!!\n");
    return 0;
}
/* ************************************************************************************************************************************************************************ */

/* ****************************************************************************锁3开锁时间设置******************************************************************************* */
int SetUnlock_3_Time(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    bool DefaultUnlockTimeFlag = false;
    /* 
    *修改默认出厂Lock_3开锁时间 2 + DEFAULT_FACTORY_SET_FLAG + ***(Time)
    *修改用户Lock_3开锁时间 2 + ***(Time)
    */
   DefaultUnlockTimeFlag = false;
    if(KeyAttr->Cursor - 1 == 5 && Atoi(KeyAttr->Buff[1]) == DEFAULT_FACTORY_SET_FLAG)
    {
        DefaultUnlockTimeFlag = true;
    }
    else if(KeyAttr->Cursor - 1 != 4)
    {
        return 0;
    }

    RefreshTimer(30 * 1000, AdminOutTimer);
    UserConfigGet()->Unlock_3_Time = Atoi(KeyAttr->Buff[1 + DefaultUnlockTimeFlag]) * 100 + Atoi(KeyAttr->Buff[2 + DefaultUnlockTimeFlag]) * 10 + Atoi(KeyAttr->Buff[3 + DefaultUnlockTimeFlag]);
    UserConfigSave();
    if(DefaultUnlockTimeFlag)
    {
        UserDefaultConfigGet()->Unlock_3_Time = UserConfigGet()->Unlock_3_Time;
        UserDefaultConfigSave();
    }
    VoiceRingPlay(Bi2, VoiceDefVol);
    printf("Modify %s Unlock_3 Time :%dS\n", DefaultUnlockTimeFlag ? "Default Factory" : "", UserConfigGet()->Unlock_3_Time);
    return 1;
}

/* ************************************************************************************************************************************************************************ */
int SetUnlockTime(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    bool DefaultUnlockTimeFlag = false;
    /* 
    *修改默认出厂Lock开锁时间 2 + DEFAULT_FACTORY_SET_FLAG + ***(Time)
    *修改用户Lock开锁时间 2 + ***(Time)
    */
   DefaultUnlockTimeFlag = false;
    if(KeyAttr->Cursor - 1 == 5 && Atoi(KeyAttr->Buff[1]) == DEFAULT_FACTORY_SET_FLAG)
    {
        DefaultUnlockTimeFlag = true;
    }
    else if(KeyAttr->Cursor - 1 != 4)
    {
        return 0;
    }

    RefreshTimer(30 * 1000, AdminOutTimer);
    if (Atoi(KeyAttr->Buff[1 + DefaultUnlockTimeFlag]) * 100 + Atoi(KeyAttr->Buff[2 + DefaultUnlockTimeFlag]) * 10 + Atoi(KeyAttr->Buff[3 + DefaultUnlockTimeFlag]) == 0)  //hare set
    {
        return 0;
    }
    UserConfigGet()->UnlockTime = Atoi(KeyAttr->Buff[1 + DefaultUnlockTimeFlag]) * 100 + Atoi(KeyAttr->Buff[2 + DefaultUnlockTimeFlag]) * 10 + Atoi(KeyAttr->Buff[3 + DefaultUnlockTimeFlag]);
    UserConfigSave();
    if(DefaultUnlockTimeFlag)
    {
        UserDefaultConfigGet()->UnlockTime = UserConfigGet()->UnlockTime;
        UserDefaultConfigSave();
    }
    VoiceRingPlay(Bi2, VoiceDefVol);
    printf("Modify %s Unlock Time :%dS\n", DefaultUnlockTimeFlag ? "Default Factory" : "", UserConfigGet()->UnlockTime);
    return 1;
}

int SetUngateTime(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    bool DefaultUnlockTimeFlag = false;
    /* 
    *修改默认出厂Gate开锁时间 4 + DEFAULT_FACTORY_SET_FLAG + ***(Time)
    *修改用户Gate开锁时间 4 + ***(Time)
    */
   DefaultUnlockTimeFlag = false;
    if(KeyAttr->Cursor - 1 == 5 && Atoi(KeyAttr->Buff[1]) == DEFAULT_FACTORY_SET_FLAG)
    {
        DefaultUnlockTimeFlag = true;
    }
    else if(KeyAttr->Cursor - 1 != 4)
    {
        return 0;
    }

    RefreshTimer(30 * 1000, AdminOutTimer);
    if (Atoi(KeyAttr->Buff[1 + DefaultUnlockTimeFlag]) * 100 + Atoi(KeyAttr->Buff[2 + DefaultUnlockTimeFlag]) * 10 + Atoi(KeyAttr->Buff[3 + DefaultUnlockTimeFlag]) == 0)  //hare set
    {
        return 0;
    }
    UserConfigGet()->UngateTime = Atoi(KeyAttr->Buff[1 + DefaultUnlockTimeFlag]) * 100 + Atoi(KeyAttr->Buff[2 + DefaultUnlockTimeFlag]) * 10 + Atoi(KeyAttr->Buff[3 + DefaultUnlockTimeFlag]);
    UserConfigSave();
    if(DefaultUnlockTimeFlag)
    {
        UserDefaultConfigGet()->UngateTime = UserConfigGet()->UngateTime;
        UserDefaultConfigSave();
    }
    VoiceRingPlay(Bi2, VoiceDefVol);
    printf("Modify %s UnGate Time :%dS\n", DefaultUnlockTimeFlag ? "Default Factory" : "", UserConfigGet()->UngateTime);
    return 1;
}

int SetBacklightTime(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    RefreshTimer(30 * 1000, AdminOutTimer);
    if (KeyAttr->Cursor != 4)
        return 0;

    UserConfigGet()->NumKeyLightTime = Atoi(KeyAttr->Buff[1]) * 10 + Atoi(KeyAttr->Buff[2]);
    UserConfigSave();
    KeypadLightEnable();
    VoiceRingPlay(Bi2, VoiceDefVol);
    printf("Modify Backlight Time :%dS\n", UserConfigGet()->NumKeyLightTime);
    return 1;
}

int SetLockWay(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    RefreshTimer(30 * 1000, AdminOutTimer);
    if (KeyAttr->Cursor != 4)
        return 0;

    int LockWay = Atoi(KeyAttr->Buff[1]) * 10 + Atoi(KeyAttr->Buff[2]);
    if (LockWay != CardWay && LockWay != CardOrCodeWay && LockWay != CardAndCodeWay)
    {
        return 0;
    }

    UserConfigGet()->LockWay = LockWay;
    UserConfigSave();
    VoiceRingPlay(Bi2, VoiceDefVol);
    printf("Modify Lock Way  [%s]\n", LockWay == CardWay ? "CardWay" : LockWay == CardOrCodeWay ? "CardOrCodeWay"
                                                                                                : "CardAndCodeWay");
    return 1;
}

int SetSafeMode(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    RefreshTimer(30 * 1000, AdminOutTimer);
    if (KeyAttr->Cursor != 4)
        return 0;

    int SafeMode = Atoi(KeyAttr->Buff[1]) * 10 + Atoi(KeyAttr->Buff[2]);
    if (SafeMode != CloseSafe && SafeMode != LockMode && SafeMode != AlarmMode)
    {
        return 0;
    }
    UserConfigGet()->SafeMode = SafeMode;
    UserConfigSave();
    VoiceRingPlay(Bi2, VoiceDefVol);
    printf("Modify Safe Mode [%s]\n", UserConfigGet()->SafeMode == CloseSafe ? "CloseSafe" : UserConfigGet()->SafeMode == LockMode ? "LockMode"
                                                                                                                                   : "AlarmMode");
    return 1;
}

int SetPublicUnlockEn(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    RefreshTimer(30 * 1000, AdminOutTimer);
    if (KeyAttr->Cursor != 4)
        return 0;

    int PublicUnlockEn = Atoi(KeyAttr->Buff[1]) * 10 + Atoi(KeyAttr->Buff[2]);
    if (PublicUnlockEn != 1 && PublicUnlockEn != 0)
    {
        return 0;
    }
    UserConfigGet()->PublicUnlockEn = PublicUnlockEn;
    UserConfigSave();
    VoiceRingPlay(Bi2, VoiceDefVol);
    printf("Modify Safe Mode [%s]\n", UserConfigGet()->PublicUnlockEn ? "Enable" : "Disable");
    return 1;
}

void CloseAddDelCardMode(void *us)
{
    printf("[Exit Add or Del User Card]!!!!\n\n");
    PopRouteStack();
}

int AddUserCard(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    RefreshTimer(30 * 1000, AdminOutTimer);
    static int CardIndex;
    if (KeyAttr->Cursor != 5)
        return 0;

    if ((CardIndex = Atoi(KeyAttr->Buff[1]) * 100 + Atoi(KeyAttr->Buff[2]) * 10 + Atoi(KeyAttr->Buff[3])) >= DECK_SIZE_MAX)
        return 0;

    if (TimerEnablestatus(AddCardTimer))
        return 0;

    if (DeckIndexPerm(CardIndex))
        return 0;

    SetTimer(30 * 1000, AddCardTimer, CloseAddDelCardMode, &CardIndex);
    PushRouteStack(KeyAddCard);
    VoiceRingPlay(Bi2, VoiceDefVol);
    CardLightFlashes();  //刷卡灯闪烁 hare 2025.4.25
    return 1;
}

int AddMoreUserCard(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    if (KeyAttr->Cursor != 4)
        return 0;

    if (!TimerEnablestatus(AddCardTimer))
        return 0;

    static int *CardIndex;
    CardIndex = (int *)(TimerGet(AddCardTimer)->Data);

    if (CardIndex && (*CardIndex = (KeyAttr->Buff[0] - 48) * 100 + Atoi(KeyAttr->Buff[1]) * 10 + Atoi(KeyAttr->Buff[2])) >= DECK_SIZE_MAX)
        return 0;

    if (DeckIndexPerm(*CardIndex))
        return 0;

    RefreshTimer(30 * 1000, AdminOutTimer);
    RefreshTimer(30 * 1000, AddCardTimer);
    VoiceRingPlay(Bi2, VoiceDefVol);
    return 1;
}

int ExitAddUserCard(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    printf("[Exit Add User Card]!!!!\n\n");
    TimerDestroy(AddCardTimer);
    return 1;
}

int DelUserCard(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    RefreshTimer(30 * 1000, AdminOutTimer);
    static int CardIndex;
    if (KeyAttr->Cursor != 5)
        return 0;

    CardIndex = Atoi(KeyAttr->Buff[1]) * 100 + Atoi(KeyAttr->Buff[2]) * 10 + Atoi(KeyAttr->Buff[3]);

    if (CardIndex == 888)
    {
        UserDeckFormat();
        VoiceRingPlay(Bi3, VoiceDefVol);
        printf("User Deck Format Succeed!!!\n");
        return 1;
    }
    else if (CardIndex == 999)
    {
        if (TimerEnablestatus(DelCardTimer))
            return 0;

        SetTimer(30 * 1000, DelCardTimer, CloseAddDelCardMode, NULL);
        PushRouteStack(KeyDelCard);
        VoiceRingPlay(Bi2, VoiceDefVol);
        CardLightFlashes();  //刷卡灯闪烁 hare 2025.4.25
        return 1;
    }
    else if (CardIndex < DECK_SIZE_MAX)
    {
        UserCardSetPerm(CardIndex, 0);
        VoiceRingPlay(Bi3, VoiceDefVol);
        printf("User Card %d Del Succeed!!!\n", CardIndex);
        return 1;
    }

    return 0;
}
int ExitDelUserCard(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    printf("[Exit Del User Card]!!!!\n\n");
    RefreshTimer(1, DelCardTimer);
    return 1;
}

int SetLanguage(Keyboard *KeyAttr, struct ActionRoute *CurrRoute)
{
    int ret = 0;
    int Digits = KeyAttr->Cursor - 1; /* 去掉结尾的 '#' */
    int DefaultFlag = 0;
    int Start = 1; /* 语言号在 Buff 中的起始下标 */
    int Language = -1;

    /* 9 + 9 + ... 为出厂默认指令；90 ~ 99 不是合法语言号，故此处优先按出厂指令解析 */
    if (Digits >= 3 && Atoi(KeyAttr->Buff[1]) == DEFAULT_FACTORY_SET_FLAG)
    {
        DefaultFlag = 1;
        Start = 2;
    }

    switch (Digits - Start)
    {
    case 1:
        if (IsDigit(KeyAttr->Buff[Start]))
        {
            Language = Atoi(KeyAttr->Buff[Start]);
        }
        break;
    case 2:
        if (IsDigit(KeyAttr->Buff[Start]) && IsDigit(KeyAttr->Buff[Start + 1]))
        {
            Language = Atoi(KeyAttr->Buff[Start]) * 10 + Atoi(KeyAttr->Buff[Start + 1]);
        }
        break;
    default:
        break;
    }

    if (Language >= 0 && Language < LanguageTotal)
    {
        UserConfigGet()->Language = Language;
        UserConfigSave();
        if (DefaultFlag)
        {
            UserDefaultConfigGet()->Language = Language;
            UserDefaultConfigSave();
        }
        VoiceRingPlay(Bi2, VoiceDefVol);
        printf("Modify %sLanguage [%d] %s\n", DefaultFlag ? "Default " : "", Language, LanguageName(Language));
        ret = 1;
    }

    RefreshTimer(30 * 1000, AdminOutTimer);
    return ret;
}

static ActionRoute RoutesMap[ActionTotal] = {
    [KeyStandby] = {"[Standby Mode]", NULL, EnterStandby, StandbyError, NULL, {KeyStandbyReady, KeyCodeUnlock}, 2},
    [KeyCodeUnlock] = {"[Code Unlock]", NULL, CodeUnlock, StandbyError, NULL, {0}, 0},
    [KeyStandbyReady] = {"[KeyStandbyReady Mode]", "*", StandbyReadyMode, ErrorHandle, ExitStandbyReady, {KeyAdmin, KeyNewCardCode}, 2},
    [KeyStandbyReturn] = {"[Return Standby]", "*", ReturnStandby, ErrorHandle, NULL, {0}, 0},
    [KeyReturn] = {"[Return]", "*", ReturnRoute, NULL, NULL, {0}, 0},

    [KeyNewCardCode] = {"[New Card Code]", NULL, EnterModifyCardCode, ErrorHandle, NULL, {KeyStandbyReturn, KeyAffiCardCode}, 2},
    [KeyAffiCardCode] = {"[Affirm Card Code]", NULL, EnterAffirmCardCode, ErrorHandle, NULL, {KeyReturn, KeyCardCodeVerify}, 2},
    [KeyCardCodeVerify] = {"[Card Code Verify]", NULL, EnterCardCodeVerify, ErrorHandle, NULL, {0}, 0},

    [KeyAdmin] = {
        "[Admin Mode]",
        NULL,
        EnterAdminMode,
        ErrorHandle,
        ExitAdminMode,
        {KeyStandbyReturn,
         KeyReset,
         KeyUnlockVoiceSwitch,
         KeyNewAdminCode,
         KeyNewLockCode,
         KeyNewGateCode,
         KeyUnlockTime,
         KeyUngateTime,
         KeyBacklightTime,
         KeyLockWay,
         KeySafeMode,
         KeyPublicUnlockEn,
         KeyAddCard,
         KeyDelCard,
         KeyLanguage,
         KeyNewLock_3_Code,
         KeyUnlock_3_Time},
        17},

    [KeyReset] = {"[restore factory setting]", "099", RestoreFactory, NULL, NULL, {0}, 0},

    [KeyUnlockVoiceSwitch] = {"[Unlock Voice Switch]", "08", UnlockVoiceSwitch, ErrorHandle, NULL, {0}, 0},

    [KeyNewAdminCode] = {"[New Admin Code]", "077#", EnterNewAdminCode, ErrorHandle, NULL, {KeyReturn, KeyAffiAdminCode}, 2},
    [KeyAffiAdminCode] = {"[Affirm New Admin Code]", NULL, EnterAffirmAdminCode, ErrorHandle, NULL, {KeyReturn, KeyAdminCodeVerify}, 2},
    [KeyAdminCodeVerify] = {"[New Admin Code Verify]\n", NULL, EnterAdminCodeVerify, ErrorHandle, NULL, {0}, 0},

    [KeyNewLockCode] = {"[New Lock Code]", "011", EnterNewLockCode, ErrorHandle, NULL, {KeyReturn, KeyAffiLockCode}, 2},
    [KeyAffiLockCode] = {"[Affirm New Lock Code]", NULL, EnterAffirmLockCode, ErrorHandle, NULL, {KeyReturn, KeyLockCodeVerify}, 2},
    [KeyLockCodeVerify] = {"[New Lock Code Verify]", NULL, EnterLockCodeVerify, ErrorHandle, NULL, {0}, 0},

    [KeyNewGateCode] = {"[New Gate Code]", "022", EnterNewGateCode, ErrorHandle, NULL, {KeyReturn, KeyAffiGateCode}, 2},
    [KeyAffiGateCode] = {"[Affirm New Gate Code]", NULL, EnterAffirmGateCode, ErrorHandle, NULL, {KeyReturn, KeyGateCodeVerify}, 2},
    [KeyGateCodeVerify] = {"[New Gate Code Verify]", NULL, EnterGateCodeVerify, ErrorHandle, NULL, {0}, 0},

    [KeyBacklightTime] = {"[Setting Backlight Time]", "1", SetBacklightTime, ErrorHandle, NULL, {0}, 0},

    [KeyUnlockTime] = {"[Setting UnLock Time]", "2", SetUnlockTime, ErrorHandle, NULL, {0}, 0},

    [KeyLockWay] = {"[Setting Lock Way]", "3", SetLockWay, ErrorHandle, NULL, {0}, 0},
    [KeyUngateTime] = {"[Setting UnGate Time]", "4", SetUngateTime, ErrorHandle, NULL, {0}, 0},

    [KeySafeMode] = {"[Setting Safe Mode]", "5", SetSafeMode, ErrorHandle, NULL, {0}, 0},
    [KeyPublicUnlockEn] = {"[Setting Public Unlock En]\n", "6", SetPublicUnlockEn, ErrorHandle, NULL, {0}, 0},

    [KeyAddCard] = {"[Add User Card]", "7", AddUserCard, ErrorHandle, ExitAddUserCard, {KeyReturn, KeyAddMoreCard}, 2},
    [KeyAddMoreCard] = {"[Add More User Card]", NULL, AddMoreUserCard, ErrorHandle, ExitAddUserCard, {KeyReturn}, 1},
    [KeyDelCard] = {"[Del User Card]", "8", DelUserCard, ErrorHandle, ExitDelUserCard, {KeyReturn}, 1},

    [KeyLanguage] = {"[Modify Language]", "9", SetLanguage, ErrorHandle, NULL, {0}, 0},

    [KeyNewLock_3_Code] = {"[New Lock_3 Code]", "033", EnterNewLock_3_Code, ErrorHandle, NULL, {KeyReturn, KeyAffiLock_3_Code}, 2},               //hare  2025.5.6
    [KeyAffiLock_3_Code] = {"[Affirm New Lock_3 Code]", NULL, EnterAffirmLock_3_Code, ErrorHandle, NULL, {KeyReturn, KeyLock_3_CodeVerify}, 2},
    [KeyLock_3_CodeVerify] = {"[New Lock_3 Code Verify]", NULL, EnterLock_3_CodeVerify, ErrorHandle, NULL, {0}, 0},

    [KeyUnlock_3_Time] = {"[Setting UnLock_3 Time]", "0", SetUnlock_3_Time, ErrorHandle, NULL, {0}, 0},    
};
/*********************************************************************************************************************************/

static void PushRouteStack(KeyAction Actiom)
{
    assert(Actiom >= KeyStandby && Actiom < ActionTotal);
    ActionRoute **CurrRoute = &(ActionStack.Routes[ActionStack.CurrentRoute]);

    if ((*CurrRoute) == NULL)
    {
        (*CurrRoute) = &RoutesMap[Actiom];
        printf("Enter %s\n", (*CurrRoute)->ActionStr);
    }
    else
    {
        for (int i = 0; i < (*CurrRoute)->NextRouteCount; i++)
        {
            if ((*CurrRoute)->NextRoute[i] == Actiom)
            {
                RoutesMap[Actiom].RouteData = (*CurrRoute)->RouteData;
                ActionStack.CurrentRoute++;
                printf("%s", (*CurrRoute)->ActionStr);
                ActionStack.Routes[ActionStack.CurrentRoute] = &RoutesMap[Actiom];

                struct timespec time;
                GetClockTimeMs(&time);
                printf(" => %s [%ld]\n", ActionStack.Routes[ActionStack.CurrentRoute]->ActionStr,time.tv_sec);
                break;
            }
        }
    }
}


static void PopRouteStack(void)
{
    if (ActionStack.CurrentRoute)
    {
        if (ActionStack.Routes[ActionStack.CurrentRoute]->ExitHandle)
            ActionStack.Routes[ActionStack.CurrentRoute]->ExitHandle(NULL, NULL);

        ActionStack.CurrentRoute--;
        
        struct timespec time;
        GetClockTimeMs(&time);
        printf("%s [%ld]\n", ActionStack.Routes[ActionStack.CurrentRoute]->ActionStr,time.tv_sec);

    }
}

int KeypadProess(Keyboard *KeyAttr)
{
    assert(KeyAttr->Cursor > 0);
    ActionRoute **CurrRoute = &(ActionStack.Routes[ActionStack.CurrentRoute]);
    for (int i = 0; i < (*CurrRoute)->NextRouteCount; i++)
    {
        ActionRoute Route = RoutesMap[(*CurrRoute)->NextRoute[i]];
        // printf("KeypadProess:%s,NextRouteCount:%d,Route.Command:%p\n", Route.ActionStr, Route.NextRouteCount, Route.Command);
        // for (int i = 0; i < Route.NextRouteCount; i++)
        // {
        // printf("NextRoute[%d]:%d\n", i, Route.NextRoute[i]);
        // }
        if (Route.Process)
        {
            if (Route.Command == NULL || memcmp(Route.Command, KeyAttr->Buff, strlen(Route.Command)) == 0)
            {
                if (Route.Process(KeyAttr, (*CurrRoute)))
                {
                    PrintfActionStackMap();
                    return 1;
                }
            }
        }
    }
    
    if ((*CurrRoute)->ErrorHandle)
        (*CurrRoute)->ErrorHandle();

    return 0;
}

int NumericKeypadInit(void)
{
    memset(&ActionStack, 0, sizeof(ActionStack));
    PushRouteStack(KeyStandby);
    return 1;
}