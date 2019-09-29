#include "stdafx.h"
#include "OculusStruct.hpp"

//キー入力
const int UP = 2490368;
const int DOWN = 2621440;
const int ESC = 27;
const int CV_WAITKEY_SPACE = 32;
const int CV_WAITKEY_TAB = 9;

//マルチスレッド処理
bool g_flag_img_set;
bool g_flag_robot_mv;
bool g_flag_getHmd_state;
HANDLE img_set_thread;
HANDLE robot_mv_thread;
HANDLE getHmd_state_thread;

//multi-threaded communication
CRITICAL_SECTION g_cs;

void img_setHandler();

Scene	* roomScene = nullptr;

bool roop_flag = true;

unsigned __stdcall robot_mv(void *arg)
{
	float prev_Rot = 0;

	while (g_flag_robot_mv){
		//ロボットの制御
		//旋回角度
		//不安定なのときに0
		g_hoge01.setRotVel = pose.Orientation.y * -180;

		EnterCriticalSection(&g_csipc);
		IPC_publishData(HOGE01_MSG, &g_hoge01);
		LeaveCriticalSection(&g_csipc);

		prev_Rot = pose.Orientation.y;

		Sleep(50);
	}

	return 0;
}

ovrSession HMD;


unsigned __stdcall getHmd_state(void *arg)
{
	while (g_flag_getHmd_state){
		double frameTime = ovr_GetPredictedDisplayTime(HMD, 0);

		ovrTrackingState hmdState = ovr_GetTrackingState(HMD, frameTime, ovrTrue);
		if (hmdState.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked))
			pose = hmdState.HeadPose.ThePose;
		Sleep(50);

	}

	return 0;
}

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR, int)
{
	ovrPosef         EyeRenderPose[2];
	ovrTexture	* mirrorTexture = nullptr;
	OculusTexture	* pEyeRenderTexture[2] = { nullptr, nullptr };
	DepthBuffer    * pEyeDepthBuffer[2] = { nullptr, nullptr };
	D3D11_TEXTURE2D_DESC td = {};
	//Texture pTexture[2]; //画像を表示するテクスチャ

	//windowクラスの生成
	bool flag = RENDER.InitWindow(hinst, L"title");
	if (flag == false)
		return -1;

	ShowWindow(RENDER.Window, SW_SHOW);

	//ipc_setting
	InitializeCriticalSection(&g_csipc);
	ipc_init();
	g_hoge01.setVel = 0;

	ovrResult result = ovr_Initialize(nullptr);

	ovrGraphicsLuid luid;
	result = ovr_Create(&HMD, &luid);
	if (!OVR_SUCCESS(result)){
		ovr_Shutdown();
		return -1;
	}

	ovrHmdDesc hmdDesc = ovr_GetHmdDesc(HMD);

	// Setup Device and Graphics
	// Note: the mirror window can be any size, for this sample we use 1/2 the HMD resolution
	RENDER.Init(hmdDesc.Resolution.w / 2, hmdDesc.Resolution.h / 2, reinterpret_cast<LUID*>(&luid));

	//Make the eye render buffers
	ovrRecti	eyeRenderViewport[2];

	for (int eye = 0; eye < 2; ++eye)
	{
		ovrSizei idealSize = ovr_GetFovTextureSize(HMD, (ovrEyeType)eye, hmdDesc.DefaultEyeFov[eye], 1.0f);
		pEyeRenderTexture[eye] = new OculusTexture();
		if (!pEyeRenderTexture[eye]->Init(HMD, idealSize.w, idealSize.h))
		{
			return -1;
		}
		pEyeDepthBuffer[eye] = new DepthBuffer(RENDER.pDevice, idealSize.w, idealSize.h);
		eyeRenderViewport[eye].Pos.x = 0;
		eyeRenderViewport[eye].Pos.y = 0;
		eyeRenderViewport[eye].Size = idealSize;
		if (!pEyeRenderTexture[eye]->TextureSet)
		{
			return -1;
		}
	}

	//get oculus head tracking
	ovr_ConfigureTracking(HMD, ovrTrackingCap_Orientation |
		ovrTrackingCap_MagYawCorrection |
		ovrTrackingCap_Position, 0);

	// Create a mirror to see on the monitor.
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	td.Width = RENDER.winSizeW;
	td.Height = RENDER.winSizeH;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.SampleDesc.Count = 1;
	td.MipLevels = 1;
	result = ovr_CreateMirrorTextureD3D11(HMD, RENDER.pDevice, &td, 0, &mirrorTexture);
	if (!OVR_SUCCESS(result))
	{
		return -1;
	}
	//create the model scene
	roomScene = new Scene;

	// Setup VR components, filling out description
	ovrEyeRenderDesc eyeRenderDesc[2];
	eyeRenderDesc[0] = ovr_GetRenderDesc(HMD, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
	eyeRenderDesc[1] = ovr_GetRenderDesc(HMD, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);

	bool isVisible = true;
	float RGBA[] = { 0, 0, 0, 1 };

	//multi-thread
	unsigned threadID;
	//g_flag_img_set  = true;
	g_flag_robot_mv = true;
	g_flag_getHmd_state = true;
	InitializeCriticalSection(&g_cs);

	/*if ((img_set_thread = (HANDLE)_beginthreadex(NULL, 0, &img_set, NULL, 0, &threadID)) == 0)
		perror("pthread_create()\n");
		*/
	if ((robot_mv_thread = (HANDLE)_beginthreadex(NULL, 0, &robot_mv, NULL, 0, &threadID)) == 0)
		perror("pthread_create()\n");

	if ((getHmd_state_thread = (HANDLE)_beginthreadex(NULL, 0, &getHmd_state, NULL, 0, &threadID)) == 0)
		perror("pthread_create()\n");

	ovrVector3f      HmdToEyeViewOffset[2] = { eyeRenderDesc[0].HmdToEyeViewOffset,
		eyeRenderDesc[1].HmdToEyeViewOffset };

	//Main loop
	while (roop_flag){
		MSG msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		double           sensorSampleTime = ovr_GetTimeInSeconds();

		ovr_CalcEyePoses(pose, HmdToEyeViewOffset, EyeRenderPose);

		//キー入力処理
		int key = cv::waitKey(10);
		switch (key)
		{
		case UP:
			//進行
			cout << "KEYUP" << endl;
			g_hoge01.setVel = 60;
			break;
		case DOWN:
			g_hoge01.setVel = 0;
			break;
		case ESC:
			roop_flag = false;
		default:
			//g_hoge01.setVel = 0;
			break;
		}

		if (isVisible){
			// Increment to use next texture, just before writing
			//Clear and set up render-target

			roomScene->TextureSet();

			//Render Scene to Eye Buffers
			for (int eye = 0; eye < 2; eye++){
				pEyeRenderTexture[eye]->AdvanceToNextTexture();
				//Oculus
				int texIndex = pEyeRenderTexture[eye]->TextureSet->CurrentIndex;
				//To Oculus Rendering
				RENDER.pContext->OMSetRenderTargets(1, &pEyeRenderTexture[eye]->TexRtv[texIndex], nullptr);
				RENDER.pContext->ClearRenderTargetView(pEyeRenderTexture[eye]->TexRtv[texIndex], RGBA);
				RENDER.SetViewport((float)eyeRenderViewport[eye].Pos.x, (float)eyeRenderViewport[eye].Pos.y,
					(float)eyeRenderViewport[eye].Size.w, (float)eyeRenderViewport[eye].Size.h);

				roomScene->Render(eye);
				RENDER.pContext->Draw(6, 0);
			}
		}

		// Initialize our single full screen Fov layer.
		ovrLayerEyeFov ld = {};
		ld.Header.Type = ovrLayerType_EyeFov;
		ld.Header.Flags = 0;

		for (int eye = 0; eye < 2; ++eye)
		{
			ld.ColorTexture[eye] = pEyeRenderTexture[eye]->TextureSet;
			ld.Viewport[eye] = eyeRenderViewport[eye];
			ld.Fov[eye] = hmdDesc.DefaultEyeFov[eye];
			ld.RenderPose[eye] = EyeRenderPose[eye];
			ld.SensorSampleTime = sensorSampleTime;
		}

		// Submit frame with one layer we have.
		ovrLayerHeader* layers = &ld.Header;
		result = ovr_SubmitFrame(HMD, 0, nullptr, &layers, 1);
		isVisible = (result == ovrSuccess);

		//Render mirror
		ovrD3D11Texture* tex = (ovrD3D11Texture*)mirrorTexture;
		RENDER.pContext->CopyResource(RENDER.pBackbuffer, tex->D3D11.pTexture);
		RENDER.pSwapChain->Present(0, 0);
	}


	/* Close IPC */
	g_flag_img_set = false;
	g_flag_robot_mv = false;
	g_flag_getHmd_state = false;
	WaitForSingleObject((HANDLE)img_set_thread, 100);
	WaitForSingleObject((HANDLE)robot_mv_thread, 100);
	WaitForSingleObject((HANDLE)g_flag_getHmd_state, 100);

	ipc_close();
	DeleteCriticalSection(&g_csipc);

	delete roomScene;
	if (mirrorTexture) ovr_DestroyMirrorTexture(HMD, mirrorTexture);
	for (int eye = 0; eye < 2; ++eye)
	{
		delete pEyeRenderTexture[eye];
		delete pEyeDepthBuffer[eye];
	}

	//RENDER.ReleaseDevice();
	ovr_Destroy(HMD);
	ovr_Shutdown();

	return(0);
}