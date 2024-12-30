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
//// Windows ͷ�ļ�:
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

/* �����¼����Ͷ��� */
typedef enum __EASY_RTMP_STATE_T
{
    EASY_RTMP_STATE_CONNECTING   =   1,     /* ������ */
    EASY_RTMP_STATE_CONNECTED,              /* ���ӳɹ� */
    EASY_RTMP_STATE_CONNECT_FAILED,         /* ����ʧ�� */
    EASY_RTMP_STATE_CONNECT_ABORT,          /* �����쳣�ж� */
    EASY_RTMP_STATE_PUSHING,                /* ������ */
    EASY_RTMP_STATE_DISCONNECTED,           /* �Ͽ����� */
    EASY_RTMP_STATE_ERROR
} EASY_RTMP_STATE_T;

/*
	_frameType:		EASY_SDK_VIDEO_FRAME_FLAG/EASY_SDK_AUDIO_FRAME_FLAG/EASY_SDK_EVENT_FRAME_FLAG/...	
	_pBuf:			�ص������ݲ��֣������÷���Demo
	_frameInfo:		֡�ṹ����
	_userPtr:		�û��Զ�������
*/
typedef int (*EasyRTMPCallBack)(int _frameType, char *pBuf, EASY_RTMP_STATE_T _state, void *_userPtr);

#ifdef __cplusplus
extern "C" 
{
#endif
	/* ����RTMP����Session �������;�� */
	EasyRTMP_API Easy_Handle Easy_APICALL EasyRTMP_Create(void);

	/* �������ݻص� */
	EasyRTMP_API Easy_I32 Easy_APICALL EasyRTMP_SetCallback(Easy_Handle handle, EasyRTMPCallBack _callback, void * _userptr);

	/*�����ӿ� ����RTMP���͵Ĳ�����Ϣ */
	EasyRTMP_API Easy_I32 Easy_APICALL EasyRTMP_Init(Easy_Handle handle, const char *url, EASY_MEDIA_INFO_T*  pstruStreamInfo, Easy_U32 bufferKSize);

	/* ����RTMP���͵Ĳ�����Ϣ */
	EasyRTMP_API Easy_I32 Easy_APICALL EasyRTMP_InitMetadata(Easy_Handle handle, EASY_MEDIA_INFO_T*  pstruStreamInfo, Easy_U32 bufferKSize);
	
	/* ����RTMP������ */
	EasyRTMP_API Easy_Bool Easy_APICALL EasyRTMP_Connect(Easy_Handle handle, const char *url);

	/* ����H264��AAC�� */
	EasyRTMP_API Easy_U32 Easy_APICALL EasyRTMP_SendPacket(Easy_Handle handle, EASY_AV_Frame* frame);

    /* ��ȡ��������С */
    EasyRTMP_API Easy_I32 Easy_APICALL EasyRTMP_GetBufInfo(Easy_Handle handle, int* usedSize, int* totalSize);

	/* ֹͣRTMP���ͣ��ͷž�� */
	EasyRTMP_API void Easy_APICALL EasyRTMP_Release(Easy_Handle handle);

#ifdef __cplusplus
};
#endif

#endif