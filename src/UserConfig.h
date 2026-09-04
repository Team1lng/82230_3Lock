/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-26 14:42:21
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-11-01 14:50:04
 * @FilePath: /82225-EPC/src/UserConfig.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

#include <stdbool.h>

#define USER_DATA_PATH "/etc/config/UserConfig.cfg"
#define USER_DEFAUILT_DATA_PATH "/etc/config/UserDefaultConfig.cfg"
#define ADMIN_CODE_LEN 6
#define UNLOCK_CODE_MAX_LEN 6
#define UNLOCK_CODE_MIN_LEN 4

typedef enum
{
    CardWay,
    CardOrCodeWay,
    CardAndCodeWay
} UnlockWay;

typedef enum
{
    CloseSafe,
    LockMode,
    AlarmMode,
} SecurityMode;

typedef enum
{
    English,
    Chinese,
    Germany,
    Hebrew,
    Polish,
    Portugal,
    Spain,
    French,
    Japanese,
    Ltaly,
    Dutch,
    Slovakia,
    Arabic,
    LanguageTotal,
} Language;

typedef struct
{
    char AdminCode[ADMIN_CODE_LEN + 1];
    char UnlockCode[UNLOCK_CODE_MAX_LEN + 1];
    char UngateCode[UNLOCK_CODE_MAX_LEN + 1];
    char PublicUnlockEn;
    char UnlockVoiceEn;
    int UnlockTime;
    int UngateTime;
    int NumKeyLightTime;
    Language Language;
    UnlockWay LockWay;
    SecurityMode SafeMode;
    int Unlock_3_Time;
    char Unlock_3_Code[UNLOCK_CODE_MAX_LEN + 1];
    bool cmd_status;   //开机指纹模块发送0XF5指令时亮绿灯的标志位
} UserConfig;

/**
 * @description: 保存默认用户配置
 * @return {*}
 */
int UserDefaultConfigSave(void);

/**
 * @description: 保存用户配置
 * @return {*}
 */
int UserConfigSave(void);

/**
 * @description: 获取用户配置
 * @return {*}
 */
UserConfig *UserConfigGet(void);

/**
 * @description: 获取默认用户配置
 * @return {*}
 */
UserConfig *UserDefaultConfigGet(void);

/**
 * @description: 用户配置初始化
 * @return {*}
 */
int UserConfigInit(void);

/**
 * @description: 恢复用户默认配置
 * @return {*}
 */
int UserConfigReset(void);

/***********喂狗*************/
void watchdog_open(void);

void watch_dog_close(void);

void watch_dog_feed(void);

unsigned long os_get_second(void);
#endif