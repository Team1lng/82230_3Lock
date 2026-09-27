/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-26 09:41:09
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-08-07 08:44:43
 * @FilePath: /project_3/src/UartIcCard.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "UserNetManage.h"
#include "VoiceRingPlay.h"
#include "SecurityMode.h"
#include "LightControl.h"
#include "UartControl.h"
#include "UartIcCard.h"
#include "UserConfig.h"
#include "UserCard.h"
#include "Unlock.h"
#include "Timer.h"
#include <string.h>

#define TTYSAK_DEVICE   "ttySAK2"
int UartIcCardCheck(const char *Data)
{
    if ((Data[0] ^ Data[1] ^ Data[2] ^ Data[3]) == Data[4])
    {
        return 0;
    }
    return -1;
}

/**********************************************************弱函数重定义************************************************************8*/
int UartIcCardModuleInit(int *DrvFd, int *DrvLen)
{
    *DrvFd = UartOpen(TTYSAK_DEVICE, 115200, 8, 1, 'n');
    if (*DrvFd < 0)
    {
        printf("open %s faild \n",TTYSAK_DEVICE);
        return -1;
    }
    *DrvLen = UART_IC_CARD_LEN;
    printf("%s  Succeed!!!\n", __func__);
    return 0;
}

int UartIcCardModuleHandle(int *DrvFd, char *Data)
{
    static int CardIndex = 0;
    if (UartIcCardCheck(Data) == -1)
    {
        return -1;
    }

    if (TimerEnablestatus(SecirityTriggerTimer))
    {
        return -1;
    }

    CardLightFlashes();

    printf("CARD : [%02x %02x %02x %02x %02x]\n", Data[0], Data[1], Data[2], Data[3], Data[4]);
    CardIndex = UserCardSearch(Data);
    if (CardIndex != -1)
    {
        printf("Card %d Exist,Code:", CardIndex);
        for (int i = 0; i < sizeof(UserCardGet()->Deck[CardIndex].Code); i++)
        {
            printf("%c", UserCardGet()->Deck[CardIndex].Code[i]);
        }
        printf("\n");
    }

    if (TimerEnablestatus(AddCardTimer))
    {
        int ret = 0;
        int IfNetManage = (TimerGet(AddCardTimer)->Data) ? 0 : 1;
        // signed char Index = IfNetManage == 0 ? *((char *)(TimerGet(AddCardTimer)->Data)) : -1;
        int Index = IfNetManage == 0 ? *((int *)(TimerGet(AddCardTimer)->Data)) : -1; //2025.6.16 hare
        // printf("IfNetManage=%d     Index=%d\n",IfNetManage,Index);

        if (CardIndex == -1)
        {
            if ((ret = UserCardAdd(Index, Data, 1)))
            {
                printf("Add Card [%d] Succeed!!!\n", Index);
                UserCardSave();
                RefreshTimer(30 * 1000, AddCardTimer);

                goto AddFinish;
                return 1;
            }
        }
        printf("Add Card [%d] Fail!!!\n", Index);

    AddFinish:
        VoiceRingPlay(ret ? Bi2 : Bi4, VoiceDefVol);
        if (IfNetManage)
        {
            NetManageShortPack(1, ManageAddCard, ret ? 1 : 9, 0);
        }
        return 0;
    }
    else if (TimerEnablestatus(DelCardTimer))
    {
        if (CardIndex != -1)
        {
            printf("Del Card Index:%d\n", CardIndex);
            if (UserCardSetPerm(CardIndex, 0))
            {
                UserCardSave();
                VoiceRingPlay(Bi3, VoiceDefVol);
                printf("Del Card [%d] Succeed!!!\n", CardIndex);
                return 1;
            }
        }
        VoiceRingPlay(Bi4, VoiceDefVol);
        printf("Del Card [%d] Fail!!!\n", CardIndex);
        return 0;
    }
    else if (TimerEnablestatus(ModifyCodeCardTimer))
    {
        TimerGet(ModifyCodeCardTimer)->Data = &CardIndex;
        printf("[ModifyCodeCardTimer] CardIndex:%d\n", CardIndex);
        RefreshTimer(30 * 1000, ModifyCodeCardTimer);
        VoiceRingPlay(Bi1, VoiceDefVol);
        return 1;
    }

    if (CardIndex != -1)
    {
        if (UserConfigGet()->LockWay == CardAndCodeWay)
        {
            if (TimerEnablestatus(CodeCardUnlockTimer))
            {
                TimerDestroy(CodeCardUnlockTimer);
                goto error;
            }
            else
            {
                SetTimer(30 * 1000, CodeCardUnlockTimer, NULL, &CardIndex);
                VoiceRingPlay(Bi1, VoiceDefVol);
                return 1;
            }
        }
        else
        {
            SecurityErrorReset();
            VoiceRingPlay(Bi1, VoiceDefVol);

            int perm = UserCardGet()->Deck[CardIndex].Perm;
            printf("[UartIcCard] CardIndex:%d, Perm:0x%X (LOCK_TYPE:0x%X, GATE_TYPE:0x%X, LOCK_3_TYPE:0x%X)\n",
                   CardIndex, perm, LOCK_TYPE, GATE_TYPE, LOCK_3_TYPE);

            if (perm & LOCK_TYPE)
            {
                printf("[UartIcCard] Opening LOCK_TYPE\n");
                Unlock(UserConfigGet()->UnlockTime, LOCK_TYPE);
            }
            if (perm & GATE_TYPE)
            {
                printf("[UartIcCard] Opening GATE_TYPE\n");
                Unlock(UserConfigGet()->UngateTime, GATE_TYPE);
            }
            if (perm & LOCK_3_TYPE)
            {
                printf("[UartIcCard] Opening LOCK_3_TYPE\n");
                Unlock(UserConfigGet()->Unlock_3_Time, LOCK_3_TYPE);
            }
            if (!(perm & (LOCK_TYPE | GATE_TYPE | LOCK_3_TYPE)))
            {
                printf("[UartIcCard] No valid lock type matched!\n");
            }

            printf("Verify Card Succeed!!! Permissions[%s]\n", perm == LOCK_TYPE ? "Lock" : perm == GATE_TYPE ? "Gate" : "Total");
            return 1;
        }
    }
error:
    VoiceRingPlay(Bi4, VoiceDefVol);
    SecurityErrorUpdate();
    printf("Verify Card  Fail!!!\n");
    return 0;
}

int UartIcCardModuleClear(int Fd)
{
    UartClear(Fd);
    return 0;
}
/***********************************************************************************************************************/