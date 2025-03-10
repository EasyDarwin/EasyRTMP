/*
	Copyright (c) 2012-2024 EasyDarwin.ORG.  All rights reserved.
	Github: https://github.com/EasyDarwin
	WEChat: EasyDarwin
	Website: http://www.easydarwin.org
*/
#ifndef _Easy_RTMP_API_H
#define _Easy_RTMP_API_H

#include "EasyTypes.h"

#ifdef _WIN32
#define EasyRTMP_API  __declspec(dllexport)
#define Easy_APICALL  __stdcall 
#else
#define EasyRTMP_API
#define Easy_APICALL 
#endif

#define	libEasyRTMP_PROG_NAME	"libEasyRTMP v3.0.24.1205"

//#ifdef _WIN32
//#include <stdio.h>
////#include <tchar.h>
//// Windows 头文件:
//
////#include <windows.h>
////#include <time.h>
//
//#if _MSC_VER>=1900
//#ifdef __cplusplus 
//extern "C"
//#endif 
//FILE* __cdecl __iob_func(unsigned i);
//#endif /* _MSC_VER>=1900 */
//#endif

/* 推送事件类型定义 */
typedef enum __EASY_RTMP_STATE_T
{
    EASY_RTMP_STATE_CONNECTING   =   1,     /* 连接中 */
    EASY_RTMP_STATE_CONNECTED,              /* 连接成功 */
    EASY_RTMP_STATE_CONNECT_FAILED,         /* 连接失败 */
    EASY_RTMP_STATE_CONNECT_ABORT,          /* 连接异常中断 */
    EASY_RTMP_STATE_PUSHING,                /* 推流中 */
    EASY_RTMP_STATE_DISCONNECTED,           /* 断开连接 */
    EASY_RTMP_STATE_ERROR
} EASY_RTMP_STATE_T;

/*
	_frameType:		EASY_SDK_VIDEO_FRAME_FLAG/EASY_SDK_AUDIO_FRAME_FLAG/EASY_SDK_EVENT_FRAME_FLAG/...	
	_pBuf:			回调的数据部分，具体用法看Demo
	_frameInfo:		帧结构数据
	_userPtr:		用户自定义数据
*/
typedef int (*EasyRTMPCallBack)(int _frameType, char *pBuf, EASY_RTMP_STATE_T _state, void *_userPtr);

#ifdef __cplusplus
extern "C" 
{
#endif
	/* 创建RTMP推送Session 返回推送句柄 */
	EasyRTMP_API Easy_Handle Easy_APICALL EasyRTMP_Create(void);

	/* 设置数据回调 */
	EasyRTMP_API Easy_I32 Easy_APICALL EasyRTMP_SetCallback(Easy_Handle handle, EasyRTMPCallBack _callback, void * _userptr);

	/*新增接口 创建RTMP推送的参数信息 */
	EasyRTMP_API Easy_I32 Easy_APICALL EasyRTMP_Init(Easy_Handle handle, const char *url, EASY_MEDIA_INFO_T*  pstruStreamInfo, Easy_U32 bufferKSize);

	/* 创建RTMP推送的参数信息 */
	EasyRTMP_API Easy_I32 Easy_APICALL EasyRTMP_InitMetadata(Easy_Handle handle, EASY_MEDIA_INFO_T*  pstruStreamInfo, Easy_U32 bufferKSize);
	
	/* 连接RTMP服务器 */
	EasyRTMP_API Easy_Bool Easy_APICALL EasyRTMP_Connect(Easy_Handle handle, const char *url);

	/* 推送H264或AAC流 */
	EasyRTMP_API Easy_U32 Easy_APICALL EasyRTMP_SendPacket(Easy_Handle handle, EASY_AV_Frame* frame);

    /* 获取缓冲区大小 */
    EasyRTMP_API Easy_I32 Easy_APICALL EasyRTMP_GetBufInfo(Easy_Handle handle, int* usedSize, int* totalSize);

	/* 停止RTMP推送，释放句柄 */
	EasyRTMP_API void Easy_APICALL EasyRTMP_Release(Easy_Handle handle);

#ifdef __cplusplus
};
#endif

#endif
