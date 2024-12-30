#ifndef _SPS_DECODE_H
#define _SPS_DECODE_H

#ifndef UINT
#define UINT unsigned int
#endif

#ifndef BYTE
#define BYTE unsigned char
#endif

#ifndef DWORD
#define DWORD unsigned long
#endif

#include "SpsDecode.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "sps_pps.h"
#include "h265_stream.h"

inline UINT Ue(BYTE *pBuff, UINT nLen, UINT &nStartBit)
{
	//计算0bit的个数
	UINT nZeroNum = 0;
	while (nStartBit < nLen * 8)
	{
		if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8))) //&:按位与，%取余
		{
			break;
		}
		nZeroNum++;
		nStartBit++;
	}
	nStartBit++;


	//计算结果
	DWORD dwRet = 0;
	for (UINT i = 0; i<nZeroNum; i++)
	{
		dwRet <<= 1;
		if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
		{
			dwRet += 1;
		}
		nStartBit++;
	}
	return (1 << nZeroNum) - 1 + dwRet;
}

inline int Se(BYTE *pBuff, UINT nLen, UINT &nStartBit)
{
	int UeVal = Ue(pBuff, nLen, nStartBit);
	double k = UeVal;
	int nValue = ceil(k / 2);//ceil函数：ceil函数的作用是求不小于给定实数的最小整数。ceil(2)=ceil(1.2)=cei(1.5)=2.00
	if (UeVal % 2 == 0)
		nValue = -nValue;
	return nValue;
}

inline DWORD u(UINT BitCount, BYTE * buf, UINT &nStartBit)
{
	DWORD dwRet = 0;
	for (UINT i = 0; i<BitCount; i++)
	{
		dwRet <<= 1;
		if (buf[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
		{
			dwRet += 1;
		}
		nStartBit++;
	}
	return dwRet;
}

inline bool h264_decode_sps(BYTE * buf, unsigned int nLen, int &width, int &height)
{
	h264_sps_t sps_t;
	memset(&sps_t, 0, sizeof(h264_sps_t));
	/*
	* 某些高的计算有缺少比如1080，针对不同的sps，可能会存在算出1088,或者1084，//默认值为1，如果sps中不包含chroma_format_idc
	*/
	sps_t.i_chroma_format_idc = 1;  

	int iRet = h264_sps_read(buf, nLen, &sps_t);
	//printf("F:%s,L:%d sps_t.i_chroma_format_idc = %d\n",__FUNCTION__,__LINE__, sps_t.i_chroma_format_idc);
#if 1
	if ((iRet >= 0 || iRet == -1000) && (sps_t.i_mb_width > 1 && sps_t.i_mb_height > 1))
	{
		//根据H264手册Table6 - 1及7.4.2.1.1，参考mkvtoolnix代码，比如稳妥的计算方法如下：
		// 宽高计算公式
		width = (sps_t.i_mb_width) * 16;
		height = (2 - sps_t.b_frame_mbs_only) * (sps_t.i_mb_height) * 16;

		if (sps_t.b_crop)
		{
			unsigned int crop_unit_x;
			unsigned int crop_unit_y;
			if (0 == sps_t.i_chroma_format_idc) // monochrome
			{
				crop_unit_x = 1;
				crop_unit_y = 2 - sps_t.b_frame_mbs_only;
			}
			else if (1 == sps_t.i_chroma_format_idc) // 4:2:0
			{
				crop_unit_x = 2;
				crop_unit_y = 2 * (2 - sps_t.b_frame_mbs_only);
			}
			else if (2 == sps_t.i_chroma_format_idc) // 4:2:2
			{
				crop_unit_x = 2;
				crop_unit_y = 2 - sps_t.b_frame_mbs_only;
			}
			else // 3 == sps.chroma_format_idc   // 4:4:4
			{
				crop_unit_x = 1;
				crop_unit_y = 2 - sps_t.b_frame_mbs_only;
			}
			width -= crop_unit_x * (sps_t.crop.i_left + sps_t.crop.i_right);
			height -= crop_unit_y * (sps_t.crop.i_top + sps_t.crop.i_bottom);
		}
		return true;
	}
#else
	if ((iRet >= 0 || iRet == -1000) && (sps_t.i_mb_width > 1 && sps_t.i_mb_height > 1))
	{
		width = sps_t.i_mb_width * 16;
		height = sps_t.i_mb_height * 16;
		return true;
	}
#endif
	return false;
}

inline bool h265_decode_sps(BYTE * buf, unsigned int nLen, int &width, int &height)
{
	if (!buf || nLen == 0)
		return false;
#if 0
	h265_read_sps_rbsp2((unsigned char*)(buf), nLen + 4,
		(int *)&width, (int *)&width, NULL);
#else
	h265_read_sps_rbsp2((unsigned char*)(buf), nLen,
		(int *)&width, (int *)&height, NULL);
#endif
	return true;
}

//输入的pbuf必须包含start code(00 00 00 01)
inline int GetH265VPSandSPSandPPS(char *pbuf, int bufsize, char *_vps, int *_vpslen, char *_sps, int *_spslen, char *_pps, int *_ppslen, int* _startPos, unsigned char* profile)
{
	char vps[512] = { 0 }, sps[512] = { 0 }, pps[128] = { 0 };
	int vpslen = 0, spslen = 0, ppslen = 0, i = 0, iStartPos = 0, ret = -1;
	int iFoundVPS = 0, iFoundSPS = 0, iFoundPPS = 0, iFoundSEI = 0;
	if (NULL == pbuf || bufsize<4)	return -1;

#ifdef _DEBUG
	FILE* f = fopen("h265_txt.txt", "wb");
	if (f)
	{
		fwrite(pbuf, 1, bufsize, f);
		fclose(f);
	}

#endif

	for (i = 0; i<bufsize-5; i++)
	{
		if (  ((unsigned char)pbuf[i] == 0x00 && (unsigned char)pbuf[i + 1] == 0x00 &&
			  (unsigned char)pbuf[i + 2] == 0x00 && (unsigned char)pbuf[i + 3] == 0x01)  ||
			((unsigned char)pbuf[i] == 0x00 && (unsigned char)pbuf[i + 1] == 0x00 &&
				(unsigned char)pbuf[i + 2] == 0x01))
		{
			int offset = 4;
			unsigned char nalType = (unsigned char)pbuf[i + 4];
			if ((unsigned char)pbuf[i + 2] == 0x01)
			{
				nalType = (unsigned char)pbuf[i + 3];
				offset = 3;
			}

			//printf("0x%X\n", (unsigned char)pbuf[i + 4]);
			switch (nalType)
			{
			case 0x40:		//VPS
			{
				iFoundVPS = 1;
				iStartPos = i + offset;

				iFoundSEI = 0x00;
			}
			break;
			case 0x42:		//SPS
			{
				if (iFoundVPS == 0x01 && i>4)
				{
					vpslen = i - iStartPos;
					if (vpslen>256)	return -1;          //vps长度超出范围
					memset(vps, 0x00, sizeof(vps));
					memcpy(vps, pbuf + iStartPos, vpslen);
				}

				iStartPos = i + offset;
				iFoundSPS = 1;
				i += 1;
			}
			break;
			case 0x44:		//PPS
			{
				if (iFoundSPS == 0x01 && i>4)
				{
					spslen = i - iStartPos;
					if (spslen>256)	return -1;
					memset(sps, 0x0, sizeof(sps));
					memcpy(sps, pbuf + iStartPos, spslen);
				}

				iStartPos = i + offset;
				iFoundPPS = 1;
				i += 1;
			}
			break;
			case 0x4E:		//Prefix SEI  
			case 0x50:		//Suffix SEI 
			case 0x20:		//I frame 16
			case 0x22:		//I frame 17
			case 0x24:		//I frame 18
			case 0x26:		//I frame 19
			case 0x28:		//I frame 20
			case 0x2A:		//I frame 21(acturally we should find naltype 16-21)
			{
				if (iFoundPPS == 0x01 && i>4)
				{
					ppslen = i - iStartPos;
					if (ppslen>256)	return -1;
					memset(pps, 0x0, sizeof(pps));
					memcpy(pps, pbuf + iStartPos, ppslen);
				}
				iStartPos = i + offset;
				iFoundSEI = 1;
				i += 1;

				if (_startPos)	*_startPos = iStartPos;
			}
			break;
			default:
				break;
			}
		}

		if (iFoundSEI == 0x01 && iFoundPPS ==0x01)		break;
	}
	if (iFoundVPS == 0x01)
	{
		if (vpslen < 1)
		{
			if (bufsize < sizeof(vps))
			{
				vpslen = bufsize - 4;
				memset(vps, 0x00, sizeof(vps));
				memcpy(vps, pbuf + 4, vpslen);
			}
		}

		if (vpslen > 0)
		{
			if (NULL != _vps)   memcpy(_vps, vps, vpslen);
			if (NULL != _vpslen)    *_vpslen = vpslen;
		}

		ret = 0;
	}
	if (iFoundSPS == 0x01)
	{
		if (spslen < 1)
		{
			if (bufsize < sizeof(sps))
			{
				spslen = bufsize - 4;
				memset(sps, 0x00, sizeof(sps));
				memcpy(sps, pbuf + 4, spslen);
			}
		}

		if (spslen > 0)
		{
			if (NULL != _sps)   memcpy(_sps, sps, spslen);
			if (NULL != _spslen)    *_spslen = spslen;

			if (profile)	*profile = (unsigned char)_sps[1];
		}

		ret = 0;
	}

	if (iFoundPPS == 0x01)
	{
		if (ppslen < 1)
		{
			if (bufsize < sizeof(pps))
			{
				ppslen = bufsize - 4;
				memset(pps, 0x00, sizeof(pps));
				memcpy(pps, pbuf + 4, ppslen);	//pps
			}
		}
		if (ppslen > 0)
		{
			if (NULL != _pps)   memcpy(_pps, pps, ppslen);
			if (NULL != _ppslen)    *_ppslen = ppslen;
		}
		ret = 0;
	}

	return ret;
}

inline int find_nal_unit(unsigned char* buf, int size, int* nal_start, int* nal_end)
{
	int i;
	// find start
	*nal_start = 0;
	*nal_end = 0;

	i = 0;
	while (   //( next_bits( 24 ) != 0x000001 && next_bits( 32 ) != 0x00000001 )
		(buf[i] != 0 || buf[i + 1] != 0 || buf[i + 2] != 0x01) &&
		(buf[i] != 0 || buf[i + 1] != 0 || buf[i + 2] != 0 || buf[i + 3] != 0x01)
		)
	{
		i++; // skip leading zero
		if (i + 4 >= size) { return 0; } // did not find nal start
	}

	if (buf[i] != 0 || buf[i + 1] != 0 || buf[i + 2] != 0x01) // ( next_bits( 24 ) != 0x000001 )
	{
		i++;
	}

	if (buf[i] != 0 || buf[i + 1] != 0 || buf[i + 2] != 0x01) { /* error, should never happen */ return 0; }
	i += 3;
	*nal_start = i;

	while (   //( next_bits( 24 ) != 0x000000 && next_bits( 24 ) != 0x000001 )
		(buf[i] != 0 || buf[i + 1] != 0 || buf[i + 2] != 0) &&
		(buf[i] != 0 || buf[i + 1] != 0 || buf[i + 2] != 0x01)
		)
	{
		i++;
		// FIXME the next line fails when reading a nal that ends exactly at the end of the data
		if (i + 3 >= size)
		{
			*nal_end = size;
			return (*nal_end - *nal_start);
		} // did not find nal end, stream ended first
	}

	*nal_end = i;
	return (*nal_end - *nal_start);
}

inline int GetH264SPSandPPS(char *pbuf, int bufsize, char *_sps, int *_spslen, char *_pps, int *_ppslen, int *_startPos, unsigned char *profile)
{
	int i = 0, ret = -1;
	int iFoundIDR = 0;
	int splitLen = 0;
	int iFoundNalu = 0;
	int spsStartPos = 0;
	int spsEndPos = 0;
	int ppsStartPos = 0;
	int ppsEndPos = 0;

	if (NULL == pbuf || bufsize<4)	return -1;

	if (_sps == NULL && _spslen == NULL && _pps == NULL && _ppslen == NULL && _startPos == NULL)
	{
		return -2;
	}

	if (_startPos)	*_startPos = -1;

	for (i = 0; i<bufsize - 4; i++)
	{
		iFoundNalu = 0;
		if ((unsigned char)pbuf[i] == 0x00 && (unsigned char)pbuf[i + 1] == 0x00 &&
			(unsigned char)pbuf[i + 2] == 0x00 && (unsigned char)pbuf[i + 3] == 0x01)
		{
			iFoundNalu = 1;
			splitLen = 4;
		}

		if ((unsigned char)pbuf[i] == 0x00 && (unsigned char)pbuf[i + 1] == 0x00 && (unsigned char)pbuf[i + 2] == 0x01)
		{
			iFoundNalu = 1;
			splitLen = 3;
		}

		if (iFoundNalu == 1)
		{
			if (spsStartPos > 0 && spsEndPos == 0)
			{
				spsEndPos = i;
			}
			if (ppsStartPos > 0 && ppsEndPos == 0)
			{
				ppsEndPos = i;
			}

			unsigned char naltype = ((unsigned char)pbuf[i + splitLen] & 0x1F);
			if (naltype == 7)       //sps
			{
				spsStartPos = i + splitLen;
				spsEndPos = 0;
			}
			else if (naltype == 8)	//pps
			{
				ppsStartPos = i + splitLen;
				ppsEndPos = 0;
			}
			else if (naltype == 5)	//sei || idr
			{
				iFoundIDR = 1;
				if (_startPos)	*_startPos = i;

				break;
			}

			i += splitLen;
		}
	}

	if (spsStartPos > 0)
	{
		if (spsEndPos == 0)
		{
			spsEndPos = bufsize;
		}

		if (spsEndPos > 512)	return -3;

		if (NULL != _spslen)    *_spslen = spsEndPos - spsStartPos;
		if (NULL != _sps)   memcpy(_sps, pbuf + spsStartPos, *_spslen);
		if (profile)	*profile = (unsigned char)_sps[1];

		ret = 0;
	}

	if (ppsStartPos > 0)
	{
		if (ppsEndPos == 0)
		{
			ppsEndPos = bufsize;
		}

		if (NULL != _ppslen)    *_ppslen = ppsEndPos - ppsStartPos;
		if (NULL != _pps && *_ppslen<128)   memcpy(_pps, pbuf + ppsStartPos, *_ppslen);

		ret = 0;
	}

	return ret;
}

#endif