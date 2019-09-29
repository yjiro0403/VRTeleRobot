#include "stdafx.h"

CRITICAL_SECTION g_csipc;
hoge01	g_hoge01;
hoge02	g_hoge02;

//cppのeyeと対応させる
#define LEFT	0
#define RIGHT	1

using namespace DirectX;

//頂点作成
//x,y,u,v
const float vertexL[] = {
	-1, -1, 0, 1,
	-1, 1, 0, 0,
	1, -1, 1, 1,

	1, 1, 1, 0,
	1, -1, 1, 1,
	-1, 1, 0, 0,
};

const float vertexR[] = { 0, -1, 0, 1,
0, 1, 0, 0,
1, -1, 1, 1,
1, 1, 1, 0,
1, -1, 1, 1,
0, 1, 0, 0,
};

//ファイルのサイズと内容を格納
typedef struct file{
	int size;
	int *cso;
}FILES;

FILES Vertex(char* fi){
	ifstream fin(fi, ios::in | ios::binary);

	/*
	if (!fin)
	return ;
	*/
	int size = fin.seekg(0, ios::end).tellg();
	fin.seekg(0, ios::beg);

	int* cso = new int[size];

	fin.read((char *)cso, size);

	fin.close();

	FILES file;

	file.size = size;
	file.cso = cso;

	return file;
}

struct DepthBuffer{
	ID3D11DepthStencilView * TexDsv;

	DepthBuffer(ID3D11Device * Device, int sizeW, int sizeH, int sampleCount = 1)
	{
		DXGI_FORMAT format = DXGI_FORMAT_D32_FLOAT;
		D3D11_TEXTURE2D_DESC dsDesc;
		dsDesc.Width = sizeW;
		dsDesc.Height = sizeH;
		dsDesc.MipLevels = 1;
		dsDesc.ArraySize = 1;
		dsDesc.Format = format;
		dsDesc.SampleDesc.Count = sampleCount;
		dsDesc.SampleDesc.Quality = 0;
		dsDesc.Usage = D3D11_USAGE_DEFAULT;
		dsDesc.CPUAccessFlags = 0;
		dsDesc.MiscFlags = 0;
		dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		ID3D11Texture2D * Tex;
		Device->CreateTexture2D(&dsDesc, NULL, &Tex);
		Device->CreateDepthStencilView(Tex, NULL, &TexDsv);
	}
	~DepthBuffer()
	{
		TexDsv->Release();
	}
};
struct DataBuffer
{
	ID3D11Buffer * D3DBuffer;
	size_t         Size;

	DataBuffer(ID3D11Device * Device, D3D11_BIND_FLAG use, const void* buffer, size_t size) : Size(size)
	{
		D3D11_BUFFER_DESC desc;   memset(&desc, 0, sizeof(desc));
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.BindFlags = use;
		desc.ByteWidth = (unsigned)size;
		D3D11_SUBRESOURCE_DATA sr;
		sr.pSysMem = buffer;
		sr.SysMemPitch = sr.SysMemSlicePitch = 0;
		Device->CreateBuffer(&desc, buffer ? &sr : NULL, &D3DBuffer);
	}
	~DataBuffer()
	{
		D3DBuffer->Release();
	}
};

//OculusとPCモニタ上でRenderingを行うのに必要な構造体
struct Render
{
	//OculusとPCモニタ上で共通の要素を記述
	HWND				Window;
	int winSizeH, winSizeW;
	ID3D11Device *		pDevice;
	ID3D11DeviceContext* pContext;
	IDXGISwapChain *	pSwapChain;
	//D3D11_VIEWPORT viewport;
	DepthBuffer* MainDepthBuffer;
	ID3D11Texture2D* pBackbuffer;
	ID3D11RenderTargetView * pBackbufferRT;
	DataBuffer* UniformBufferGen;
	//FILES		fin;

	static const int UNIFORM_DATA_SIZE = 2000;

	//forRot
	unsigned char	UniformData[UNIFORM_DATA_SIZE];

	~Render()
	{
		//終了処理
		pSwapChain->Release();
		pDevice->Release();
		pContext->Release();
	}

	bool InitWindow(HINSTANCE hinst, LPCWSTR title)
	{
		WNDCLASSEX wnd;
		ZeroMemory(&wnd, sizeof(WNDCLASSEX));

		wnd.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		wnd.cbSize = sizeof(wnd);
		wnd.lpszClassName = L"TestClass";
		wnd.lpfnWndProc = DefWindowProc;
		wnd.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wnd.hCursor = LoadCursor(NULL, IDC_ARROW);
		wnd.hInstance = hinst;
		wnd.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);

		if (!RegisterClassEx(&wnd))
			return false;

		//Input Assemble Stage
		Window = CreateWindowEx(0, wnd.lpszClassName, L"window", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, hinst, nullptr);
		if (Window == nullptr)
			return false;

		return true;
	}
	void Init(int vpW, int vpH, LUID* pLuid)
	{
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
		result = pDevice->CreateRenderTargetView(pBackbuffer, nullptr, &pBackbufferRT);
		if (FAILED(result))
			return;

		// Set max frame latency to 1
		IDXGIDevice1* DXGIDevice1 = nullptr;
		result = pDevice->QueryInterface(__uuidof(IDXGIDevice1), (void**)&DXGIDevice1);
		DXGIDevice1->SetMaximumFrameLatency(1);
		DXGIDevice1->Release();

	}
	void SetViewport(float vpX, float vpY, float vpW, float vpH)
	{
		D3D11_VIEWPORT D3Dvp;
		D3Dvp.Width = vpW;    D3Dvp.Height = vpH;
		D3Dvp.MinDepth = 0;   D3Dvp.MaxDepth = 1;
		D3Dvp.TopLeftX = vpX; D3Dvp.TopLeftY = vpY;
		//D3Dvp.TopLeftX = 0; D3Dvp.TopLeftY = 0;
		pContext->RSSetViewports(1, &D3Dvp);
	}
};

//global Rendering state
static struct Render RENDER;

const string StreamAddress_LEFT = "http://192.168.11.36:8080/?action=snapshot?dummy=param.mjpeg";
const string StreamAddress_RIGHT = "http://192.168.11.36:9080/?action=snapshot?dummy=param.mjpeg";

//受け取る画像の大きさを設定
//const int received_img_width = 1280, received_img_height = 720;
const int received_img_width  = 640;
const int received_img_height = 480;
const int img_width  = 400;										//切り抜いてHMDに投影する画像の横幅
const int img_height = 350;										//切り抜いてHMDに投影する画像の横幅
const int x_max = received_img_width - img_width;				//左端の切り抜き画像が移動できる最大の位置
const int y_max = received_img_height - img_height;				//左端の切り抜き画像が移動できる最大の位置
const int img_centor_x = received_img_width/2-img_width/2;		//中心位置を切り抜いた時の左の位置
const int img_centor_y = received_img_height / 2 - img_height / 2;
const int centorVal = 2;										
int cut_x  = 0;
int prev_x = 0;

//頭の角度を取得
ovrPosef pose;

cv::Mat img_output(int LR){
	cv::Mat img;
	if (LR == LEFT){
		cv::VideoCapture cam(StreamAddress_LEFT);
		cam >> img;
		//img = cv::imread("leftview_640_480.jpg");
	}
	else{
		//cv::VideoCapture cam(StreamAddress_RIGHT);
		//cam >> img;
		img = cv::imread("rightview_640_480.jpg");

	}

	int head_x = pose.Orientation.y * 180 + img_centor_x;
	int head_y = pose.Orientation.x * 180 + img_centor_y;
	if (abs(head_x - prev_x) > 10){
		cut_x = head_x;
	}

	if (abs(cut_x - img_centor_x) < 10){
		cut_x = img_centor_x;
	} else if (cut_x < img_centor_x - 10){
		cut_x += centorVal;
	} else{
		cut_x -= centorVal;
	}

	if (cut_x > x_max)
		cut_x = x_max;
	else if (cut_x < 0)
		cut_x = 0;
	
	if (head_y > y_max)
		head_y = y_max;
	else if (head_y < 0)
		head_y = 0;

	//cv::Mat cut_img = img;												//画像抽出なしの処理
	cv::Mat cut_img(img, cv::Rect(cut_x, head_y, img_width, img_height));	//画像抽出ありの処理	

	prev_x = head_x;

	//デバッガ用処理
	//切り取った部分をわかりやすく表示
	cv::rectangle(img, cv::Rect(cut_x, head_y, img_width, img_height),cv::Scalar(0,0,200),5,4);	//画像抽出ありの処理

	if (LR == LEFT){
		cv::namedWindow("looking_area_LEFT", CV_WINDOW_AUTOSIZE | CV_WINDOW_FREERATIO);
		cv::imshow("looking_area_LEFT", img);
	} else {
		cv::namedWindow("looking_area_RIGHT", CV_WINDOW_AUTOSIZE | CV_WINDOW_FREERATIO);
		cv::imshow("looking_area_RIGHT", img);
	}

	return cut_img;
}

//img Set
//ここを変える
struct Texture
{
	ID3D11Texture2D            * Tex;
	ID3D11ShaderResourceView   * TexSv;
	ID3D11RenderTargetView     * TexRtv;

	void TextureSet(int LR){
		Tex = nullptr;
		TexSv = nullptr;

		HRESULT result;

		cv::Mat img;

		//画像をテクスチャに描画する
		img = img_output(LR);

		cv::cvtColor(img, img, cv::COLOR_RGB2BGR);
		cv::cvtColor(img, img, cv::COLOR_BGR2BGRA);

		//Set Texture Data
		D3D11_TEXTURE2D_DESC dsDesc;
		dsDesc.Width = img.cols;
		dsDesc.Height = img.rows;
		dsDesc.MipLevels = 1;
		dsDesc.ArraySize = 1;;
		dsDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		dsDesc.SampleDesc.Count = 1;
		dsDesc.SampleDesc.Quality = 0;
		dsDesc.Usage = D3D11_USAGE_DEFAULT;
		dsDesc.CPUAccessFlags = 0;
		dsDesc.MiscFlags = 0;
		dsDesc.CPUAccessFlags = 0;
		dsDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		//Set Subresource Data
		D3D11_SUBRESOURCE_DATA subresource_data;
		subresource_data.pSysMem = img.data;
		subresource_data.SysMemPitch = img.step[0];

		result = RENDER.pDevice->CreateTexture2D(&dsDesc, &subresource_data, &Tex);
		if (FAILED(result))
			return;

		result = RENDER.pDevice->CreateShaderResourceView(Tex, nullptr, &TexSv);
		if (FAILED(result))
			return;

		TexRtv = nullptr;
	}
	Texture(int LR) {
		TextureSet(LR);
	}
	~Texture()
	{
		TexRtv->Release();
		TexSv->Release();
		Tex->Release();
	}
};

//------------------------------------------------------------
// ovrSwapTextureSet wrapper class that also maintains the render target views
// needed for D3D11 rendering.
//OCULUSのテクスチャー
struct OculusTexture
{
	ovrHmd                   hmd;
	ovrSwapTextureSet      * TextureSet;
	static const int         TextureCount = 2;
	ID3D11RenderTargetView * TexRtv[TextureCount];

	OculusTexture() :
		hmd(nullptr),
		TextureSet(nullptr)
	{
		TexRtv[0] = TexRtv[1] = nullptr;
	}

	bool Init(ovrHmd _hmd, int sizeW, int sizeH)
	{
		hmd = _hmd;
		ovrResult result;

		D3D11_TEXTURE2D_DESC dsDesc;
		dsDesc.Width = sizeW;
		dsDesc.Height = sizeH;
		dsDesc.MipLevels = 1;
		dsDesc.ArraySize = 1;
		dsDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		dsDesc.SampleDesc.Count = 1;   // No multi-sampling allowed
		dsDesc.SampleDesc.Quality = 0;
		dsDesc.Usage = D3D11_USAGE_DEFAULT;
		dsDesc.CPUAccessFlags = 0;
		dsDesc.MiscFlags = 0;
		dsDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;


		result = ovr_CreateSwapTextureSetD3D11(hmd, RENDER.pDevice, &dsDesc, ovrSwapTextureSetD3D11_Typeless, &TextureSet);
		if (!OVR_SUCCESS(result))
			return false;

		for (int i = 0; i < TextureCount; ++i)
		{
			ovrD3D11Texture* tex = (ovrD3D11Texture*)&TextureSet->Textures[i];
			D3D11_RENDER_TARGET_VIEW_DESC rtvd = {};
			rtvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			rtvd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			RENDER.pDevice->CreateRenderTargetView(tex->D3D11.pTexture, &rtvd, &TexRtv[i]);
		}

		return true;
	}

	~OculusTexture()
	{
		for (int i = 0; i < TextureCount; ++i)
		{
			TexRtv[i]->Release();
		}
		if (TextureSet)
		{
			ovr_DestroySwapTextureSet(hmd, TextureSet);
		}
	}

	void AdvanceToNextTexture()
	{
		TextureSet->CurrentIndex = (TextureSet->CurrentIndex + 1) % TextureSet->TextureCount;
	}

};

//Create Renderer
struct Material
{
	ID3D11VertexShader		* D3DVert;
	ID3D11PixelShader		* D3DPix;
	Texture					* Tex[2]; 
	ID3D11InputLayout		* InputLayout;
	UINT					  VertexSize;

	//void Init();
	void TextureSet() {
		Tex[0]->TextureSet(LEFT);
		Tex[1]->TextureSet(RIGHT);
	}
	Material()
	{
		HRESULT result;

		//VertexShader Stage
		//頂点読み込み
		FILES file;
		file = Vertex("..\\Debug\\VertexShader.cso");


		D3D11_INPUT_ELEMENT_DESC elemdsc[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		//Create vertex shader
		result = RENDER.pDevice->CreateVertexShader(file.cso, file.size, nullptr, &D3DVert);
		//result = RENDER.pDevice->CreateVertexShader(a, b, nullptr, &D3DVert);
		if (FAILED(result))
			return;

		//Create input layout
		result = RENDER.pDevice->CreateInputLayout(elemdsc, 2, file.cso, file.size, &InputLayout);
		if (FAILED(result))
			return;
			
		//Create pixel shader
		file = Vertex("..\\Debug\\PixelShader.cso");										//ピクセル読み込み
		result = RENDER.pDevice->CreatePixelShader(file.cso, file.size, nullptr, &D3DPix);
		if (FAILED(result))
			return;

		//Create rasterizer
		//SetViewportの代わり?
		D3D11_RASTERIZER_DESC rs; memset(&rs, 0, sizeof(rs));
		rs.AntialiasedLineEnable = rs.DepthClipEnable = true;
		rs.CullMode = D3D11_CULL_BACK;
		rs.FillMode = D3D11_FILL_SOLID;
		//RENDER.pDevice->CreateRasterizerState(&rs, &Rasterizer);

		Tex[0] = new Texture(0);
		Tex[1] = new Texture(1);
	};
	~Material(){
		D3DVert->Release();
		D3DPix->Release();
		delete Tex[0]; Tex[0] = nullptr;
		delete Tex[1]; Tex[1] = nullptr;
		InputLayout->Release();

	};

};

struct Model
{
	Material  * Fill;
	DataBuffer* VertexBuffer;

	void Init(){
		Fill = new Material;
		VertexBuffer = new DataBuffer(RENDER.pDevice, D3D11_BIND_VERTEX_BUFFER, vertexL, sizeof(float) * 4 * 6); //check
	};

	void TextureSet(){
		Fill->TextureSet();
	}

	Model() : Fill(nullptr), VertexBuffer(nullptr) {
		Init();
	};
	~Model(){
		delete Fill; Fill = nullptr;
		delete VertexBuffer; VertexBuffer = nullptr;
	}

	void Render(int LR)
	{
		FLOAT RGBA[] = { 0, 0, 0, 1 };

		RENDER.pContext->IASetInputLayout(Fill->InputLayout);

		Fill->VertexSize = sizeof(float) * 4;
		UINT offset = 0;
		RENDER.pContext->IASetVertexBuffers(0, 1, &VertexBuffer->D3DBuffer, &Fill->VertexSize, &offset);
		RENDER.pContext->VSSetShader(Fill->D3DVert, nullptr, 0);
		RENDER.pContext->PSSetShader(Fill->D3DPix, nullptr, 0);
		RENDER.pContext->PSSetShaderResources(0, 1, &Fill->Tex[LR]->TexSv);
	};
};

struct Scene
{
	static const int MAX_MODELS = 100;
	Model *Models[MAX_MODELS];
	int numModels = 0;

	void Render(int eye)
	{
		for (int i = 0; i < numModels; ++i)
			Models[i]->Render(eye);
	}

	void Init(){
		Models[numModels] = new Model;
		numModels++;
	}

	void TextureSet() {
		for (int i = 0; i < numModels; ++i)
			Models[i]->TextureSet();
	}

	Scene(){
		Init();
	}
};