#include "stdafx.h"
#include "myRender.h"

VECTOR2::VECTOR2(float a, float b)
{
	u = a; v = b;
}

VECTOR3::VECTOR3(float a, float b, float c)
{
	x = a; y = b; z = c;
}

//Direct3Dの初期化関数
HRESULT InitD3D(HWND hWnd)
{
	// デバイスとスワップチェーンの作成
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;         //バックバッファの数
	sd.BufferDesc.Width = WINDOW_WIDTH;     //バッファの幅
	sd.BufferDesc.Height = WINDOW_HEIGHT;    //バッファの高さ
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //バッファのフォーマット
	sd.BufferDesc.RefreshRate.Numerator = 60;   //リフレッシュレート
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	D3D_FEATURE_LEVEL  FeatureLevel = D3D_FEATURE_LEVEL_11_0;


	if (FAILED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_REFERENCE, NULL, 0,
		&FeatureLevel, 1, D3D11_SDK_VERSION, &sd, &SwapChain, &Device, NULL, &DeviceContext)))
	{
		return FALSE;
	}
	//レンダーターゲットビューの作成
	ID3D11Texture2D *BackBuffer;
	SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&BackBuffer);
	Device->CreateRenderTargetView(BackBuffer, NULL, &RenderTargetView);
	BackBuffer->Release();
	DeviceContext->OMSetRenderTargets(1, &RenderTargetView, NULL);

	//ビューポートの設定
	D3D11_VIEWPORT vp;
	vp.Width = WINDOW_WIDTH;
	vp.Height = WINDOW_HEIGHT;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	DeviceContext->RSSetViewports(1, &vp);

	//hlslファイル読み込み
	ID3DBlob *pCompiledShaderL = NULL, *pCompiledShaderR = NULL;
	ID3DBlob *pErrors = NULL;
	//ブロブから頂点シェーダー作成
	if (FAILED(D3DX11CompileFromFile(L"shader.hlsl", NULL, NULL, "VSL", "vs_5_0", 0, 0, NULL, &pCompiledShaderL, &pErrors, NULL)) && FAILED(D3DX11CompileFromFile(L"shader.hlsl", NULL, NULL, "VSR", "vs_5_0", 0, 0, NULL, &pCompiledShaderR, &pErrors, NULL)))
	{
		MessageBox(0, L"頂点シェーダー読み込み失敗", NULL, MB_OK);
		return E_FAIL;
	}
	SAFE_RELEASE(pErrors);

	if (FAILED(Device->CreateVertexShader(pCompiledShaderL->GetBufferPointer(), pCompiledShaderL->GetBufferSize(), NULL, &VertexShader)))
	{
		SAFE_RELEASE(pCompiledShaderL);
		SAFE_RELEASE(pCompiledShaderR);
		MessageBox(0, L"頂点シェーダー作成失敗", NULL, MB_OK);
		return E_FAIL;
	}
	//頂点インプットレイアウトを定義 
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = sizeof(layout) / sizeof(layout[0]);

	//頂点インプットレイアウトを作成
	if (FAILED(Device->CreateInputLayout(layout, numElements, pCompiledShaderL->GetBufferPointer(), pCompiledShaderL->GetBufferSize(), &VertexLayoutL)) && FAILED(Device->CreateInputLayout(layout, numElements, pCompiledShaderR->GetBufferPointer(), pCompiledShaderR->GetBufferSize(), &VertexLayoutR)))
		return FALSE;
	//頂点インプットレイアウトをセット
	DeviceContext->IASetInputLayout(VertexLayoutL);
	DeviceContext->IASetInputLayout(VertexLayoutR);

	//ブロブからピクセルシェーダー作成
	if (FAILED(D3DX11CompileFromFile(L"shader.hlsl", NULL, NULL, "PS", "ps_5_0", 0, 0, NULL, &pCompiledShaderL, &pErrors, NULL)))
	{
		MessageBox(0, L"ピクセルシェーダー読み込み失敗", NULL, MB_OK);
		return E_FAIL;
	}
	SAFE_RELEASE(pErrors);
	if (FAILED(Device->CreatePixelShader(pCompiledShaderL->GetBufferPointer(), pCompiledShaderL->GetBufferSize(), NULL, &PixelShader)))
	{
		SAFE_RELEASE(pCompiledShaderL);
		MessageBox(0, L"ピクセルシェーダー作成失敗", NULL, MB_OK);
		return E_FAIL;
	}
	SAFE_RELEASE(pCompiledShaderL);
	SAFE_RELEASE(pCompiledShaderR);
	//四角形
	SimpleVertex vertices[] =
	{
		VECTOR3(-0.5f, -0.5f, 0.5f), VECTOR2(0, 1),
		VECTOR3(-0.5f, 0.5f, 0.5f), VECTOR2(0, 0),
		VECTOR3(0.5f, -0.5f, 0.5f), VECTOR2(1, 1),
		VECTOR3(0.5f, 0.5f, 0.5f), VECTOR2(1, 0),
	};
	D3D11_BUFFER_DESC bd;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = vertices;
	if (FAILED(Device->CreateBuffer(&bd, &InitData, &VertexBuffer)))
		return FALSE;

	//バーテックスバッファーをセット
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &stride, &offset);

	//プリミティブ・トポロジーをセット
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//テクスチャー用サンプラー作成
	D3D11_SAMPLER_DESC SamDesc;
	ZeroMemory(&SamDesc, sizeof(D3D11_SAMPLER_DESC));

	SamDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	SamDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	SamDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	Device->CreateSamplerState(&SamDesc, &SampleLinear);
	//テクスチャー作成
	if (FAILED(D3DX11CreateShaderResourceViewFromFile(Device, L"leftview.jpg", NULL, NULL, &TextureL, NULL)) && FAILED(D3DX11CreateShaderResourceViewFromFile(Device, L"rightview.jpg", NULL, NULL, &TextureR, NULL)))
	{
		return E_FAIL;
	}

	return S_OK;
}

VOID myRender()
{
	float ClearColor[4] = { 0, 0, 0, 1 }; //消去色
	DeviceContext->ClearRenderTargetView(RenderTargetView, ClearColor);//画面クリア 
	//使用するシェーダーの登録
	DeviceContext->VSSetShader(VertexShader, NULL, 0);
	DeviceContext->PSSetShader(PixelShader, NULL, 0);

	//テクスチャーをシェーダーに渡す
	DeviceContext->PSSetSamplers(0, 1, &SampleLinear);
	//DeviceContext->PSSetShaderResources(0, 1, &TextureL);
	DeviceContext->PSSetShaderResources(0, 1, &TextureR);

	//プリミティブをレンダリング
	DeviceContext->Draw(4, 0);
	SwapChain->Present(0, 0);//フリップ
}

VOID Cleanup()
{
	SAFE_RELEASE(SampleLinear);
	SAFE_RELEASE(TextureL);
	SAFE_RELEASE(VertexShader);
	SAFE_RELEASE(PixelShader);
	SAFE_RELEASE(VertexBuffer);
	SAFE_RELEASE(VertexLayoutL);
	SAFE_RELEASE(VertexLayoutR);
	SAFE_RELEASE(SwapChain);
	SAFE_RELEASE(RenderTargetView);
	SAFE_RELEASE(DeviceContext);
	SAFE_RELEASE(Device);
}

