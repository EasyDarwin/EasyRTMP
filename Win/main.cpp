#define _CRTDBG_MAP_ALLOC
#include <stdio.h>

#ifdef _WIN32
#include "windows.h"
#else
#include <string.h>
#include <unistd.h>
#endif
#include <stdio.h> 
#include <iostream> 
#include <time.h> 
#include <stdlib.h>
#include "EasyStreamClientAPI.h"
#include "EasyRTMPAPI.h"
#include <list>
#include "SpsDecode.h"

#ifdef _WIN32
#pragma comment(lib,"libEasyRTMP.lib")
#pragma comment(lib,"libEasyStreamClient.lib")
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif


#define MAX_RTMP_URL_LEN 256

typedef struct __EASY_CLIENT_OBJ_T
{
	Easy_Handle streamClientHandle;
	Easy_Handle pusherHandle;
	EASY_MEDIA_INFO_T mediainfo;


	EASY_RTP_CONNECT_TYPE	connectType;
	char	szURL[MAX_PATH];
	char	szOutputFormat[MAX_PATH];
	char	szOutputAddress[MAX_PATH];
	int		nTimeout;
	int		logLevel;

	int		gopTotal;
	int		GOP;

	FILE* fVideoOut;
	FILE* fAudioOut;
}EASY_CLIENT_OBJ_T;


int __EasyRTMP_Callback(int _frameType, char* pBuf, EASY_RTMP_STATE_T _state, void* _userPtr)
{
	switch (_state)
	{
	case EASY_RTMP_STATE_CONNECTING:
		printf("EasyRTMP Connecting...\n");
		break;
	case EASY_RTMP_STATE_CONNECTED:
		printf("EasyRTMP Connected...\n");
		break;
	case EASY_RTMP_STATE_CONNECT_FAILED:
		printf("EasyRTMP Connect failed...\n");
		break;
	case EASY_RTMP_STATE_CONNECT_ABORT:
		printf("EasyRTMP Connect abort...\n");
		break;
	case EASY_RTMP_STATE_DISCONNECTED:
		printf("EasyRTMP Disconnect...\n");
		break;
	}

	return 0;
}

static int writeFile(const char* _fileName, void* _buf, int _bufLen)
{
	FILE* fp = NULL;
	if (NULL == _buf || _bufLen <= 0) return (-1);

	fp = fopen(_fileName, "ab+"); // 必须确保是以 二进制写入的形式打开

	if (NULL == fp)
	{
		return (-1);
	}

	fwrite(_buf, _bufLen, 1, fp); //二进制写

	fclose(fp);
	fp = NULL;

	return 0;
}

static int readFile(const char* _fileName, void* _buf, int _bufLen)
{
	FILE* fp = NULL;
	if (NULL == _buf || _bufLen <= 0) return (-1);

	fp = fopen(_fileName, "rb"); // 必须确保是以 二进制读取的形式打开 

	if (NULL == fp)
	{
		return (-1);
	}

	fread(_buf, _bufLen, 1, fp); // 二进制读

	fclose(fp);
	return 0;
}

int Easy_APICALL __EasyStreamClientCallBack(void* channelPtr, int frameType, void* pBuf, EASY_FRAME_INFO* frameInfo)
{
	Easy_Bool bRet = 0;
	int iRet = 0;
	EASY_CLIENT_OBJ_T* pEasyClient = (EASY_CLIENT_OBJ_T*)channelPtr;

	if (frameType == EASY_SDK_VIDEO_FRAME_FLAG || frameType == EASY_SDK_AUDIO_FRAME_FLAG)
	{
		if (frameInfo && frameInfo->length && frameType == EASY_SDK_AUDIO_FRAME_FLAG)
		{

		}


		if (pEasyClient->logLevel >= 1)
		{
#if 1
			char szVideo[64] = { 0 };
			sprintf(szVideo, "GOP[%d] [%dx%d] Frame Type[%s]", pEasyClient->GOP, frameInfo->width, frameInfo->height, frameInfo->type == 1 ? "I" : "P");

			char* p = (char*)pBuf;
			printf("%s %s Size[%d] %02X %02X %02X %02X %02X %02X %02X %02X\tTimestamp:%u.%u\n",
				frameType == EASY_SDK_VIDEO_FRAME_FLAG ? "Video" : "\t\t\t\tAudio",
				frameType == EASY_SDK_VIDEO_FRAME_FLAG ? szVideo : "",
				frameInfo->length,
				(unsigned char)p[0], (unsigned char)p[1], (unsigned char)p[2], (unsigned char)p[3],
				(unsigned char)p[4], (unsigned char)p[5], (unsigned char)p[6], (unsigned char)p[7],
				frameInfo->timestamp_sec, frameInfo->timestamp_usec);
#endif
		}

		if (frameInfo && frameInfo->length && frameType == EASY_SDK_VIDEO_FRAME_FLAG)
		{
			if (frameInfo->type == EASY_SDK_VIDEO_FRAME_I)
			{
				pEasyClient->GOP = pEasyClient->gopTotal;
				pEasyClient->gopTotal = 1;

				//printf("Keyframe ...\n");
			}
			else
			{
				pEasyClient->gopTotal++;
			}


			if (NULL == pEasyClient->fVideoOut && 0 == strncmp(pEasyClient->szOutputFormat, "file", 4))
			{
				//if (frameInfo->type == EASY_SDK_VIDEO_FRAME_I)
				{
					char videoFilename[260] = { 0 };
					sprintf(videoFilename, "%s_video", pEasyClient->szOutputAddress);
					char* p = (char*)(videoFilename + (int)strlen(videoFilename));
					if (frameInfo->codec == EASY_SDK_VIDEO_CODEC_H264)			p += sprintf(p, ".h264");
					else if (frameInfo->codec == EASY_SDK_VIDEO_CODEC_H265)		p += sprintf(p, ".h265");
					else if (frameInfo->codec == EASY_SDK_VIDEO_CODEC_MJPEG)	p += sprintf(p, ".mjpeg");
					else if (frameInfo->codec == EASY_SDK_VIDEO_CODEC_MPEG4)	p += sprintf(p, ".mpeg");
					else
					{
						p += sprintf(p, ".unknown");
					}

					pEasyClient->fVideoOut = fopen(videoFilename, "wb");
				}
			}

			if (NULL != pEasyClient->fVideoOut)
			{
				fwrite(pBuf, 1, frameInfo->length, pEasyClient->fVideoOut);
				fflush(pEasyClient->fVideoOut);

			}
		}
		else if (frameInfo && frameInfo->length && frameType == EASY_SDK_AUDIO_FRAME_FLAG)
		{
			if (NULL == pEasyClient->fAudioOut && 0 == strncmp(pEasyClient->szOutputFormat, "file", 4))
			{
				char audioFilename[260] = { 0 };
				sprintf(audioFilename, "%s_audio.aac", pEasyClient->szOutputAddress);
				pEasyClient->fAudioOut = fopen(audioFilename, "wb");
			}

			if (NULL != pEasyClient->fAudioOut)
			{
				fwrite(pBuf, 1, frameInfo->length, pEasyClient->fAudioOut);
				fflush(pEasyClient->fAudioOut);
			}
		}

		if (0 == strncmp(pEasyClient->szOutputFormat, "rtmp", 4))
		{
			if (frameInfo && frameInfo->length && strlen(pEasyClient->szOutputAddress) > 0)
			{
#if 1
				//printf("video timestamp = %ld : %06ld \n", frameInfo->timestamp_sec, frameInfo->timestamp_usec);
				if (pEasyClient->pusherHandle == NULL)
				{
					pEasyClient->pusherHandle = EasyRTMP_Create();
					EasyRTMP_SetCallback(pEasyClient->pusherHandle, __EasyRTMP_Callback, NULL);
					bRet = EasyRTMP_Connect(pEasyClient->pusherHandle, pEasyClient->szOutputAddress);
					if (!bRet)
					{
						printf("Fail to EasyRTMP_Connect ...\n");
					}

					iRet = EasyRTMP_InitMetadata(pEasyClient->pusherHandle, &pEasyClient->mediainfo, 1024);
					if (iRet < 0)
					{
						printf("Fail to InitMetadata ...\n");
					}
				}

				if (frameInfo->type == 0x01)
				{
					char vps[512] = { 0 };
					int vpsLength = 0;
					char sps[256] = { 0 };
					int spsLength = 0;
					char pps[128] = { 0 };
					int ppsLength = 0;
					int idrPos = 0;
					unsigned char profile = 0;
					GetH265VPSandSPSandPPS((char*)pBuf, frameInfo->length,
						(char*)vps, (int*)&vpsLength,
						(char*)sps, (int*)&spsLength,
						(char*)pps, (int*)&ppsLength, &idrPos, &profile);

					//if (vpsLength > 0 && spsLength > 0)// && ppsLength > 0)
					{

						bool update = false;
						if (vpsLength > 0 && ((0 != memcmp(vps, pEasyClient->mediainfo.u8Vps, vpsLength)) || (vpsLength != pEasyClient->mediainfo.u32VpsLength)))
						{
							memset(pEasyClient->mediainfo.u8Vps, 0x00, sizeof(pEasyClient->mediainfo.u8Vps));
							memcpy(pEasyClient->mediainfo.u8Vps, vps, vpsLength);
							pEasyClient->mediainfo.u32VpsLength = vpsLength;

							printf("%s Size[%d]",
								"VPS",
								vpsLength);
							char* p = (char*)pEasyClient->mediainfo.u8Vps;
							for (int k = 0; k < vpsLength; k++)
							{
								printf(" %X", (unsigned char)p[k]);
							}
							printf("\n");

							update = true;
						}

						if (spsLength > 0 && ((0 != memcmp(sps, pEasyClient->mediainfo.u8Sps, spsLength)) || (spsLength != pEasyClient->mediainfo.u32SpsLength)))
						{
							memset(pEasyClient->mediainfo.u8Sps, 0x00, sizeof(pEasyClient->mediainfo.u8Sps));
							memcpy(pEasyClient->mediainfo.u8Sps, sps, spsLength);
							pEasyClient->mediainfo.u32SpsLength = spsLength;

							printf("%s Size[%d]",
								"SPS",
								spsLength);
							char* p = (char*)pEasyClient->mediainfo.u8Sps;
							for (int k = 0; k < spsLength; k++)
							{
								printf(" %X", (unsigned char)p[k]);
							}
							printf("\n");

							update = true;
						}

						if (ppsLength > 0 && ((0 != memcmp(pps, pEasyClient->mediainfo.u8Pps, ppsLength)) || (ppsLength != pEasyClient->mediainfo.u32PpsLength)))
						{
							memset(pEasyClient->mediainfo.u8Pps, 0x00, sizeof(pEasyClient->mediainfo.u8Pps));
							memcpy(pEasyClient->mediainfo.u8Pps, pps, ppsLength);
							pEasyClient->mediainfo.u32PpsLength = ppsLength;

							printf("%s Size[%d]",
								"PPS",
								ppsLength);
							char* p = (char*)pEasyClient->mediainfo.u8Pps;
							for (int k = 0; k < ppsLength; k++)
							{
								printf(" %X", (unsigned char)p[k]);
							}
							printf("\n");

							update = true;
						}

						if (update)
						{
							iRet = EasyRTMP_InitMetadata(pEasyClient->pusherHandle, &pEasyClient->mediainfo, 1024);
							if (iRet < 0)
							{
								printf("Fail to InitMetadata ...\n");
							}
						}
					}
				}


				if (pEasyClient->pusherHandle)
				{
					EASY_AV_Frame avFrame;
					memset(&avFrame, 0, sizeof(EASY_AV_Frame));
					avFrame.u32AVFrameFlag = frameType;
					avFrame.u32AVFrameLen = frameInfo->length;
					avFrame.pBuffer = (unsigned char*)pBuf;
					avFrame.u32AVFrameType = frameInfo->type;
					avFrame.u32PTS = frameInfo->pts;
					avFrame.u32TimestampSec = frameInfo->timestamp_sec;
					avFrame.u32TimestampUsec = frameInfo->timestamp_usec;

#ifdef _DEBUG__
					int idx = 0;
					static unsigned char tstBuf[1024 * 1024 * 2];
					tstBuf[idx++] = 0x00;
					tstBuf[idx++] = 0x00;
					tstBuf[idx++] = 0x00;
					tstBuf[idx++] = 0x01;
					tstBuf[idx++] = 0x06;
					tstBuf[idx++] = 0x0A;
					tstBuf[idx++] = 0x0B;
					tstBuf[idx++] = 0x0C;
					tstBuf[idx++] = 0x0D;
					tstBuf[idx++] = 0x0E;
					tstBuf[idx++] = 0x0F;
					memcpy(tstBuf + idx, pBuf, frameInfo->length);
					idx += frameInfo->length;
					avFrame.u32AVFrameLen = idx;
					avFrame.pBuffer = (unsigned char*)tstBuf;

#endif

#ifdef _DEBUG__
					static FILE* f = fopen("20240720.h265", "wb");
					if (f)
					{
						fwrite(pBuf, 1, frameInfo->length, f);
					}
#endif

					/*if (frameType == EASY_SDK_VIDEO_FRAME_FLAG)
					{
						static int h264index = 0;
						printf("%s pBuf=%02X %02X %02X %02X %02X %02X %02X \n", __FUNCTION__,
							avFrame.pBuffer[0], avFrame.pBuffer[1], avFrame.pBuffer[2], avFrame.pBuffer[3], avFrame.pBuffer[4], avFrame.pBuffer[5], avFrame.pBuffer[6]);
						char filename[128] = { 0 };
						sprintf(filename, "frame_%d.h264", h264index++);
						writeFile(filename, pBuf, frameInfo->length);
					}*/

					iRet = EasyRTMP_SendPacket(pEasyClient->pusherHandle, &avFrame);
					if (iRet < 0)
					{
						printf("Fail to EasyRTMP_SendH264Packet(I-frame) ...\n");
					}
				}
#endif
			}
		}
	}
	else if (frameType == EASY_SDK_MEDIA_INFO_FLAG)//回调出媒体信息
	{
		//if(pBuf != NULL)
		//{
		//	memcpy(&pEasyClient->mediainfo, pBuf, sizeof(EASY_MEDIA_INFO_T));
		//	printf("RTSP DESCRIBE Get Media Info: video:%u fps:%u audio:%u channel:%u sampleRate:%u spslen: %d ppslen:%d\n", 
		//		pEasyClient->mediainfo.u32VideoCodec, pEasyClient->mediainfo.u32VideoFps, 
		//		pEasyClient->mediainfo.u32AudioCodec, pEasyClient->mediainfo.u32AudioChannel, pEasyClient->mediainfo.u32AudioSamplerate,
		//		pEasyClient->mediainfo.u32SpsLength, pEasyClient->mediainfo.u32PpsLength);
		//}

		static unsigned int priorCodecId = 0;

		if (pEasyClient->mediainfo.u32VideoCodec != priorCodecId && priorCodecId > 0)
		{
			if (pEasyClient->pusherHandle)
			{
				EasyRTMP_Release(pEasyClient->pusherHandle);
				pEasyClient->pusherHandle = NULL;
			}
		}

		if (pEasyClient->pusherHandle)
		{
			iRet = EasyRTMP_InitMetadata(pEasyClient->pusherHandle, &pEasyClient->mediainfo, 1024);
			if (iRet < 0)
			{
				printf("Fail to InitMetadata ...\n");
			}
		}

		priorCodecId = pEasyClient->mediainfo.u32VideoCodec;
	}
	else if (frameType == EASY_SDK_EVENT_FRAME_FLAG)
	{
		if (frameInfo->codec == EASY_STREAM_CLIENT_STATE_DISCONNECTED)
		{
			printf("channel source stream disconnected!\n");
		}
		else if (frameInfo->codec == EASY_STREAM_CLIENT_STATE_CONNECTED)
		{
			printf("channel source stream connected!\n");
		}
		else if (frameInfo->codec == EASY_STREAM_CLIENT_STATE_EXIT)
		{
			printf("channel source stream exit!\n");
		}
	}
	else if (frameType == EASY_SDK_SNAP_FRAME_FLAG)
	{
		//char jpgname[128] = { 0 };
		//static int index = 0;
		//sprintf(jpgname, "channel_%d.jpg", index++);
		//FILE* file = fopen(jpgname, "wb+");
		//if (file)
		//{
		//	fwrite(pBuf, 1, frameInfo->length, file);
		//	fclose(file);
		//	file = NULL;
		//}
	}

	return 0;
}



int Easy_APICALL __EasyDownloadCallBack(void* userptr, const char* path)
{
	if (path)
	{
		printf("%s : %s\n", __FUNCTION__, path);
	}
	else
	{
		printf("%s : download failed!\n", __FUNCTION__);
	}
	return 0;
}

int PrintPrompt(char const* progName)
{
	printf("%s -m udp -d rtsp://srcAddr -s file/rtmp -f rtmp://dstAddr\n", progName);
	printf("-m: tcp or udp\n");
	printf("-d: rtsp、m3u8、rtmp、http\n");
	printf("-s: file、rtmp\n");
	printf("-f: fileName、rtmp://dstAddr\n");
	printf("-t: timeout(seconds)\n");
	printf("-l: log level  1:print(default)  2:print+file\n");

	printf("%s -m tcp -d rtsp://admin:admin@192.168.99.100/ch1 -s file -f cam1_ch1\n\n", progName);

	return 0;
}


//-m tcp -d rtsp://192.168.1.100/ch1 -s rtmp -f rtmp://127.0.0.1:10035/hls/ch1 -t 30
int main(int argc, char* argv[])
{
	int size = 338;
	char buff[1024] = { 0 };
	int i = 0;

#ifdef _WIN32
	extern char* optarg;
#endif

	int iret = 0;

	if (argc < 2)
	{
		PrintPrompt(argv[0]);
		getchar();
		return 0;
	}


	EASY_CLIENT_OBJ_T		easyClientObj;
	memset(&easyClientObj, 0x00, sizeof(EASY_CLIENT_OBJ_T));

	easyClientObj.connectType = EASY_RTP_OVER_TCP;
	easyClientObj.nTimeout = 5;
	easyClientObj.logLevel = 1;

	for (int i = 0; i < argc; i++)
	{
		if (0 == strncmp(argv[i], "-m", 2))
		{
			if (argc >= i + 1)
			{
				if ((0 == strncmp(argv[i + 1], "udp", 3)) ||
					(0 == strncmp(argv[i + 1], "UDP", 3)))
				{
					easyClientObj.connectType = EASY_RTP_OVER_UDP;
				}
			}
		}
		else if (0 == strncmp(argv[i], "-d", 2))
		{
			if (argc >= i + 1)
				snprintf(easyClientObj.szURL, sizeof(easyClientObj.szURL), "%s", argv[i + 1]);
		}
		else if (0 == strncmp(argv[i], "-s", 2))
		{
			if (argc >= i + 1)
				snprintf(easyClientObj.szOutputFormat, sizeof(easyClientObj.szOutputFormat), "%s", argv[i + 1]);
		}
		else if (0 == strncmp(argv[i], "-f", 2))
		{
			if (argc >= i + 1)
				snprintf(easyClientObj.szOutputAddress, sizeof(easyClientObj.szOutputAddress), "%s", argv[i + 1]);
		}
		else if (0 == strncmp(argv[i], "-t", 2))
		{
			if (argc >= i + 1)
				easyClientObj.nTimeout = atoi(argv[i + 1]);
		}
		else if (0 == strncmp(argv[i], "-l", 2))
		{
			if (argc >= i + 1)
				easyClientObj.logLevel = atoi(argv[i + 1]);
		}
	}

	//memset(easyClientObj.szURL, 0x00, sizeof(easyClientObj.szURL));
	//strcpy(easyClientObj.szURL, "rtsp://124.42.239.202:1554/pag://11.10.150.183:7302:1f9af7693fca48caa13289d5a0501108:1:MAIN:TCP?streamform=rtp");
	//strcpy(easyClientObj.szURL, "rtsp://admin:fzzy@123@111.9.60.238:11206/Streaming/Channels/102");


	printf("Connect Type: %s\n", easyClientObj.connectType == EASY_RTP_OVER_TCP ? "TCP" : "UDP");
	printf("Connect Address: %s\n", easyClientObj.szURL);
	printf("Output Format: %s\n", easyClientObj.szOutputFormat);
	printf("Output Address: %s\n", easyClientObj.szOutputAddress);
	printf("Timeout: %d\n", easyClientObj.nTimeout);
	printf("LogLevel: %d\n", easyClientObj.logLevel);

	if ((int)strlen(easyClientObj.szURL) < 4)
	{
		PrintPrompt(argv[0]);
		return 0;
	}

	if (0 == strcmp(easyClientObj.szOutputAddress, "\0"))
	{
		PrintPrompt(argv[0]);
		return 0;
	}

	if (0 == strncmp(easyClientObj.szOutputFormat, "rtmp", 4))
	{

	}

	int userptr = 1;
	EasyStreamClient_Init(&easyClientObj.streamClientHandle, easyClientObj.logLevel);

	if (!easyClientObj.streamClientHandle)
	{
		printf("Initial fail.\n");
		return 0;
	}

	EasyStreamClient_SetAudioEnable(easyClientObj.streamClientHandle, 1);

	EasyStreamClient_SetCallback(easyClientObj.streamClientHandle, __EasyStreamClientCallBack);


	EasyStreamClient_OpenStream(easyClientObj.streamClientHandle, easyClientObj.szURL, easyClientObj.connectType, (void*)&easyClientObj, 1000, easyClientObj.nTimeout, 1);

	printf("按回车键退出...\n");
	getchar();

	if (easyClientObj.streamClientHandle)
	{
		EasyStreamClient_Deinit(easyClientObj.streamClientHandle);
		easyClientObj.streamClientHandle = NULL;
	}

	if (easyClientObj.pusherHandle)
	{
		EasyRTMP_Release(easyClientObj.pusherHandle);
		easyClientObj.pusherHandle = NULL;
	}

	return 0;
}

