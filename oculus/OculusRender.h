#include "stdafx.h"
#include "OculusStruct.hpp"

void Render::Init(int vpW, int vpH, LUID* pLuid){
	HRESULT result;

	winSizeH = vpH;
	winSizeW = vpW;

	RECT size = { 0, 0, vpW, vpH };
	AdjustWindowRect(&size, WS_OVERLAPPEDWINDOW, false);
	const UINT flags = SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW;
	if (!SetWindowPos(Window, nullptr, 0, 0, size.right - size.left, size.bottom - size.top, flags))
		return;

	IDXGIFactory * DXGIFactory = nullptr;
	result = CreateDXGIFactory1(__uuidof(IDXGIFactory), (void**)(&DXGIFactory));
	if (FAILED(result))
		return;

	IDXGIAdapter * Adapter = nullptr;
	for (UINT iAdapter = 0; DXGIFactory->EnumAdapters(iAdapter, &Adapter) != DXGI_ERROR_NOT_FOUND; ++iAdapter)
	{
		DXGI_ADAPTER_DESC adapterDesc;
		Adapter->GetDesc(&adapterDesc);
		if ((pLuid == nullptr) || memcmp(&adapterDesc.AdapterLuid, pLuid, sizeof(LUID)) == 0)
			break;
		Adapter->Release();
	}
	auto DriverType = Adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE;

	//Create swap chain
	DXGI_SWAP_CHAIN_DESC chaindsc; //スワップチェイン
	ZeroMemory(&chaindsc, sizeof(chaindsc));
	chaindsc.BufferCount = 2;
	chaindsc.BufferDesc.Width = winSizeW;
	chaindsc.BufferDesc.Height = winSizeH;
	chaindsc.BufferDesc.RefreshRate.Numerator = 1;
	//chaindsc.BufferDesc.RefreshRate.Denominator = 60;
	chaindsc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	chaindsc.SampleDesc.Count = 1;
	chaindsc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	chaindsc.OutputWindow = Window;
	chaindsc.Windowed = true;

	D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
	result = D3D11CreateDeviceAndSwapChain(Adapter, DriverType, nullptr, D3D11_CREATE_DEVICE_DEBUG && 0, &feature_level, 1, D3D11_SDK_VERSION, &chaindsc, &pSwapChain, &pDevice, nullptr, &pContext);
	if (FAILED(result))
		return;

	//Create Back Buffer
	//Output Merger Stage
	result = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&pBackbuffer);
	result = RENDER.pDevice->CreateRenderTargetView(pBackbuffer, nullptr, &pBackbufferRT);
	if (FAILED(result))
		return;

	//Main depth buffer
	MainDepthBuffer = new DepthBuffer(pDevice, winSizeW, winSizeH);
	//pContext->OMSetRenderTargets(1, &pBackbufferRT, MainDepthBuffer->TexDsv);
	pContext->OMSetRenderTargets(1, &pBackbufferRT, nullptr);

	//Buffer for shader constants
	UniformBufferGen = new DataBuffer(pDevice, D3D11_BIND_VERTEX_BUFFER, vertexL, sizeof(float) * 4 * 6);
	pContext->VSSetConstantBuffers(0, 1, &UniformBufferGen->D3DBuffer);

	// Set max frame latency to 1
	IDXGIDevice1* DXGIDevice1 = nullptr;
	result = RENDER.pDevice->QueryInterface(__uuidof(IDXGIDevice1), (void**)&DXGIDevice1);
	DXGIDevice1->SetMaximumFrameLatency(1);
	DXGIDevice1->Release();

	// Buffer for shader constants
	//UniformBufferGen = new DataBuffer(pDevice, D3D11_BIND_CONSTANT_BUFFER, NULL, UNIFORM_DATA_SIZE);
}


/*
void Oculus::Init(){
ovrResult ovr_result;
HRESULT result;

//Rendering Set
for (int eye = 0; eye < 2; eye++)
{
ovrSizei idealSize = ovr_GetFovTextureSize(session, (ovrEyeType)eye, hmdDesc.DefaultEyeFov[eye], 1.0f);
pEyeRenderTexture[eye] = new OculusTexture();
pEyeRenderTexture[eye]->Init(session, idealSize.w, idealSize.h);

//CreateRender
pEyeRenderTexture[eye]->Init(session, idealSize.w, idealSize.h);
pEyeDepthBuffer[eye] = new DepthBuffer(RENDER.pDevice, idealSize.w, idealSize.h);
eyeRenderViewport[eye].Pos.x = 0;
eyeRenderViewport[eye].Pos.y = 0;
eyeRenderViewport[eye].Size = idealSize;
bool flag;
flag = pEyeRenderTexture[eye]->TextureSet;
}

//Create a mirror
td.ArraySize = 1;
td.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
td.Width = RENDER.winSizeW;
td.Height = RENDER.winSizeH;
td.Usage = D3D11_USAGE_DEFAULT;
td.SampleDesc.Count = 1;
td.MipLevels = 1;
ovr_result = ovr_CreateMirrorTextureD3D11(session, RENDER.pDevice, &td, 0, &mirrorTexture);
if (OVR_FAILURE(ovr_result))
return;

eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);
hmdToEyeViewOffset[0] = eyeRenderDesc[0].HmdToEyeViewOffset;
hmdToEyeViewOffset[1] = eyeRenderDesc[1].HmdToEyeViewOffset;
}*/