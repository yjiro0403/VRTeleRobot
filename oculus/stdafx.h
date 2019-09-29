// stdafx.h : �W���̃V�X�e�� �C���N���[�h �t�@�C���̃C���N���[�h �t�@�C���A�܂���
// �Q�Ɖ񐔂������A�����܂�ύX����Ȃ��A�v���W�F�N�g��p�̃C���N���[�h �t�@�C��
// ���L�q���܂��B
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Windows �w�b�_�[����g�p����Ă��Ȃ����������O���܂��B
// Windows �w�b�_�[ �t�@�C��:
#include <windows.h>

// C �����^�C�� �w�b�_�[ �t�@�C��
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include "ipc.h"
#include "ipc_msg.h"
//#include "hoge.h"

// TODO: �v���O�����ɕK�v�Ȓǉ��w�b�_�[�������ŎQ�Ƃ��Ă��������B
#include <cstdint>
#include <vector>
#include <new>

#include <iostream>
#include <fstream>

//�摜��荞��
#include <opencv.hpp>

using namespace std;

//Oculus��Rendering
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
#define DIRECTINPUT_VERSION     0x0800          // DirectInput�̃o�[�W�����w��
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