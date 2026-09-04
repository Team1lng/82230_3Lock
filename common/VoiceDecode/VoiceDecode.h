/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-14 08:23:19
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-11-21 15:58:18
 * @FilePath: /project_3/include/list/VoiceDecode.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _VOICERINGTONE_H_
#define _VOICERINGTONE_H_
#include <stdatomic.h>

typedef void (*VoiceCallBackFunc)(void *);

typedef enum
{
    VOICE_TYPE_MP3,
    VOICE_TYPE_PCM,
    VOICE_TYPE_UNKNOWN,
} VoiceType;

typedef struct
{
    unsigned char *Data;   // frame data
    unsigned int Len;      // frame len in bytes
    unsigned long long Ts; // timestamp(ms)
    unsigned long SeqNo;   // current frame sequence no.
} VoiceFrame;

typedef struct
{
    atomic_int Valid;     // 原子整型，用于标记该结构体是否有效
    char FilePath[128];   // 文件路径字符串，固定128字节
    char *BackPcmFile;    // 指向一个PCM文件的指针（可能是备用文件）
    int Volume;           // 音量
    VoiceType Type;       // 声音类型，VoiceType可能是枚举或自定义类型

    VoiceCallBackFunc Start;   // 回调函数指针，在开始播放时调用
    VoiceCallBackFunc End;     // 回调函数指针，在播放结束时调用
} VoiceInfo;

/**
 * @description: 获取语音铃声信息 [需要函数重定义]
 * @param {VoiceInfo} info   节点信息
 * @return {*}
 */
__attribute__((weak)) int VoiceInfoImport(VoiceInfo *info);

/**
 * @description: 铃声音频数据写入 [需要函数重定义]
 * @param {VoiceFrame} frame   音频数据
 * @return {*}
 */
__attribute__((weak)) int VoiceDataExport(VoiceFrame *frame);

/**
 * @description: 铃声音频开始回调 [需要函数重定义]
 * @param {void} arg   私有数据
 * @return {*}
 */
__attribute__((weak)) void VoiceDecodeStart(void *arg);

/**
 * @description: 铃声音频结束回调 [需要函数重定义]
 * @param {void} arg   私有数据
 * @return {*}
 */
__attribute__((weak)) void VoiceDecodeEnd(void *arg);

/**
 * @description: 语音解码状态
 * @return {*}
 */
int VoiceDecodeStatus(void);

/**
 * @description: 初始化语音铃声提示
 * @return {*}
 */
int VoiceDecodeInit(void);
#endif