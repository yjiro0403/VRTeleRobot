// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、または
// 参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Windows ヘッダーから使用されていない部分を除外します。
// Windows ヘッダー ファイル:
#include <windows.h>

// C ランタイム ヘッダー ファイル
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include "ipc.h"
#include "ipc_msg.h"
//#include "hoge.h"

// TODO: プログラムに必要な追加ヘッダーをここで参照してください。
#include <cstdint>
#include <vector>
#include <new>

#include <iostream>
#include <fstream>

//画像取り込み
#include <opencv.hpp>

using namespace std;

//OculusとRendering
#include "resource.h"
//Oculus
#include <OVR_CAPI.h>
#include "OVR_CAPI_D3D.h"
#include "OVR.h"

using namespace OVR;

#define OVR_Win32_DirectXAppUtil_h
//DirectX
#include <d3dx11.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include "DirectXMath.h"
#define DIRECTINPUT_VERSION     0x0800          // DirectInputのバージョン指定
#define INITGUID

#include <stdio.h>
#include <signal.h>
#include <process.h>

#include <conio.h>

#pragma warning(default:4716)
#define MODULE_NAME "ipc_test01"

HANDLE hMutex;

#define MAX01	10
#define MAX02	20


typedef struct hoge01 {
	int		setVel;
	int		setRotVel;
} hoge01;


typedef struct hoge02 {
	int		d;
	double	f[MAX02];
} hoge02;


void ipc_init(void);
void ipc_close(void);

IPC_RETURN_TYPE Dummy;

#ifndef _ipc_
#define _ipc_
#include "ipc.hpp"
#endif _ipc_