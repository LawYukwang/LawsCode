// CloudmusicUC2MP3.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <WinInet.h>

#pragma comment(lib, "WinInet.lib")

typedef struct ID3V2 
{
	char Header[3];
	char Ver;
	char Reversion;
	char Flag;
	char Size[4];
}MP3Header;

typedef struct ID3V1
{
	char Tag[3];        // ID3V1标识符“TAG”的Ascii码
	char Title[30];     // 歌曲名
	char Artist[30];    // 歌手名
	char Album[30];     // 专辑名
	char Year[4];       // 日期信息
	char Comment[30];   // 注释信息
	char Genre;         // 歌曲风格（字节型数值）
}MP3Trailer;

int _tmain(int argc, _TCHAR* argv[])
{
	FILE *ucFile, *mpFile;
	MP3Header *mp3Header;
	MP3Trailer *mp3Trailer;
	ucFile = fopen ("C:\\Users\\Administrator\\AppData\\Local\\Netease\\CloudMusic\\Cache\\Cache\\449818741-128-a01cace34f2df73c384bbcfe3e30b827.uc","rb");

	char *url = "https://api.imjad.cn/cloudmusic/?type=detail&id=449818741";

	
	if (ucFile == NULL)
	{
		return 1;
	}
	fseek (ucFile, 0, SEEK_END);
	long lSize = ftell (ucFile);
	rewind(ucFile);
	BYTE *Head = new BYTE[sizeof(MP3Header)];
	size_t len = fread(Head, 1, sizeof(MP3Header), ucFile);
	for (int i = 0;i<len;i++)
	{
		Head[i] ^= 0xa3;
	}
	mp3Header = (MP3Header*)Head;

	rewind(ucFile);
	fseek(ucFile, -128, SEEK_END);
	BYTE *Trail = new BYTE[sizeof(MP3Trailer)];
	len = fread(Trail, 1, sizeof(MP3Trailer), ucFile);
	for (int i = 0;i<len;i++)
	{
		Trail[i] ^= 0xa3;
	}
	mp3Trailer = (MP3Trailer*)Trail;

	rewind(ucFile);
	BYTE *transB = new BYTE[lSize];
	len = fread(transB, 1, lSize, ucFile);
	for (int i = 0;i<len;i++)
	{
		transB[i] ^= 0xa3;
	}


	mpFile = fopen ("C:\\Users\\Administrator\\AppData\\Local\\Netease\\CloudMusic\\Cache\\Cache\\449818741-128-a01cace34f2df73c384bbcfe3e30b827.mp3","wb");
	fwrite (transB, len, 1, mpFile);
	return 0;
}

