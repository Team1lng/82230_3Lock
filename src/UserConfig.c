/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-26 14:42:21
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-07-22 10:02:24
 * @FilePath: /82225-EPC/src/UserConfig.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "UserConfig.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include <stdbool.h>
#include <../common/watch_dog/ak_drv_wdt.h>
#include <ak_common.h>

#define DEFAULT_UNLOCK_TIME 1

static UserConfig UserConf;
static UserConfig UserConfDefault = {
    .AdminCode = {'9', '9', '9', '9', '9', '9', '\0'},
    .UnlockCode = {'1', '2', '3', '4', '5', '6', '\0'},
    .UngateCode = {'4', '5', '6', '7', '8', '9', '\0'},
    .PublicUnlockEn = 1,
    .UnlockVoiceEn = 1,
    .UngateTime = DEFAULT_UNLOCK_TIME,
    .UnlockTime = DEFAULT_UNLOCK_TIME,
    .NumKeyLightTime = 0,
    .Language = Arabic,
    .LockWay = CardOrCodeWay,
    .SafeMode = CloseSafe,
    .Unlock_3_Time = DEFAULT_UNLOCK_TIME,
    .Unlock_3_Code = {'2', '3', '4', '5', '6', '7', '\0'},
    .cmd_status = true,
};

/**
 * @description: 保存默认用户配置
 * @return {*}
 */
int UserDefaultConfigSave(void)
{
    int fd = open(USER_DEFAUILT_DATA_PATH, O_WRONLY | O_CREAT);
    if (fd < 0)
    {
        printf("write open %s fail \n", USER_DEFAUILT_DATA_PATH);
        return 0;
    }

    write(fd, &UserConfDefault, sizeof(UserConfig));

    close(fd);
    system("fsync -d " USER_DEFAUILT_DATA_PATH);
    return 1;
}

/**
 * @description: 保存用户配置
 * @return {*}
 */
int UserConfigSave(void)
{
    int fd = open(USER_DATA_PATH, O_WRONLY | O_CREAT);
    if (fd < 0)
    {
        printf("write open %s fail \n", USER_DATA_PATH);
        return 0;
    }

    write(fd, &UserConf, sizeof(UserConfig));

    close(fd);
    system("fsync -d " USER_DATA_PATH);
    return 1;
}

/**
 * @description: 获取用户配置
 * @return {*}
 */
UserConfig *UserConfigGet(void)
{
    return &UserConf;
}

/**
 * @description: 获取默认用户配置
 * @return {*}
 */
UserConfig *UserDefaultConfigGet(void)
{
    return &UserConfDefault;
}

/**
 * @description: 用户配置初始化
 * @return {*}
 */
int UserConfigInit(void)
{
    // UserDefaultConfigSave();    //2025.4.18 hare   修复门口机恢复默认设置时无效的问题
    int DefaultFd = open(USER_DEFAUILT_DATA_PATH,O_CREAT | O_RDONLY);

    if(DefaultFd)
    {
        UserConfig TmpUserConf;
        if(read(DefaultFd, &TmpUserConf, sizeof(TmpUserConf)))
        {
            UserConfDefault.Language = TmpUserConf.Language;
            UserConfDefault.UnlockTime = TmpUserConf.UnlockTime;
            UserConfDefault.UngateTime = TmpUserConf.UngateTime;
            UserConfDefault.Unlock_3_Time = TmpUserConf.Unlock_3_Time;
            printf("lock=====%d      gate======%d      lock3========%d\n", UserConfDefault.UnlockTime, UserConfDefault.UngateTime, UserConfDefault.Unlock_3_Time);
            memcpy(UserConfDefault.UnlockCode,TmpUserConf.UnlockCode,sizeof(TmpUserConf.UnlockCode));
            memcpy(UserConfDefault.UngateCode,TmpUserConf.UngateCode,sizeof(TmpUserConf.UngateCode));
            memcpy(UserConfDefault.Unlock_3_Code,TmpUserConf.Unlock_3_Code,sizeof(TmpUserConf.Unlock_3_Code));
        }
        close(DefaultFd);
    }

    int fd = open(USER_DATA_PATH, O_RDONLY);
    if (fd < 0)
    {
        UserConf = UserConfDefault;
        UserConfigSave();
        return 0;
    }

    read(fd, &UserConf, sizeof(UserConfig));

    close(fd);

    return 1;
}

/**
 * @description: 恢复用户默认配置
 * @return {*}
 */
int UserConfigReset(void)
{
    printf("[%s]\n", __func__);
    int Language = UserConf.Language;
    UserConf = UserConfDefault;
    UserConf.Language = Language;
    UserConfigSave();
    return 1;
}

/******************喂狗*************************/
static bool watch_dog_flag = false;

void watchdog_open(void)
{
	ak_drv_wdt_open(5);  //liu: 10 -> 5
	watch_dog_flag = true;
}

void watch_dog_close(void)
{
	if (watch_dog_flag)
	{
        printf("/n*************关门放狗**************/n");
		ak_drv_wdt_close();
		watch_dog_flag = false;
	}
}

void watch_dog_feed(void)
{
	if (watch_dog_flag == true)
	{
		ak_drv_wdt_feed();
	}
}

// 获取当前时间秒
unsigned long os_get_second(void)
{
	struct ak_timeval tv;
	ak_get_ostime(&tv);
	return tv.sec;
}