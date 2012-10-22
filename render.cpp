#include "render.h"

IDirect3DDevice9* d3dDevice;
IDirect3D9* d3d;
D3DCAPS9 d3dCaps;
D3DPRESENT_PARAMETERS d3dPresent;
IDirect3DVertexBuffer9* vertexBuffer;

bool InitD3D(HWND hWnd)
{
    HRESULT hr;

    d3d = Direct3DCreate9(D3D_SDK_VERSION);

    if (!d3d)
	{
        return true;
	}

    ZeroMemory (&d3dCaps, sizeof(d3dCaps));
    if (FAILED(d3d->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &d3dCaps)))
	{
        return true;
	}

	D3DDISPLAYMODE d3ddm;
	RECT rWindow;

	d3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);

	GetClientRect(hWnd, &rWindow);

	int resWidth = rWindow.right - rWindow.left;
	int resHeight = rWindow.bottom - rWindow.top;

	ZeroMemory(&d3dPresent, sizeof(d3dPresent));
    d3dPresent.SwapEffect = D3DSWAPEFFECT_FLIP;
    d3dPresent.hDeviceWindow = hWnd;
    d3dPresent.BackBufferCount = 1;
	d3dPresent.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	d3dPresent.Windowed = true;
	d3dPresent.BackBufferFormat = d3ddm.Format;
	d3dPresent.BackBufferWidth = resWidth;
	d3dPresent.BackBufferHeight = resHeight;

    if (d3dCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
    {    
        hr = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dPresent, &d3dDevice);        
    }
    else
    {
        hr = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dPresent, &d3dDevice);
    }

    if (FAILED(hr))
	{
        return true;
	}
	
    d3dDevice->SetVertexShader(NULL);
    d3dDevice->SetFVF(D3DFVF_TLVERTEX);

    d3dDevice->CreateVertexBuffer(sizeof(TLVERTEX) * 4, NULL, D3DFVF_TLVERTEX, D3DPOOL_MANAGED, &vertexBuffer, NULL);
    d3dDevice->SetStreamSource(0, vertexBuffer, 0, sizeof(TLVERTEX));

    d3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    d3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    d3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

	FillVertexBuffer(d3dPresent.BackBufferWidth, d3dPresent.BackBufferHeight);

    return false;
}

bool CloseD3D()
{
    if (vertexBuffer)
	{
        vertexBuffer->Release ();
	}

    if (d3dDevice)
	{
        d3dDevice->Release();
	}

    if (d3d)
	{
        d3d->Release();
	}

    return false;
}

void OnWindowSizeChange(HWND hWnd)
{
	RECT rWindow;
	GetClientRect(hWnd, &rWindow);

	int resWidth = rWindow.right - rWindow.left;
	int resHeight = rWindow.bottom - rWindow.top;

	D3DDISPLAYMODE d3ddm;
	d3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);

	ZeroMemory(&d3dPresent, sizeof(d3dPresent));
	d3dPresent.SwapEffect = D3DSWAPEFFECT_FLIP;
    d3dPresent.hDeviceWindow = hWnd;
    d3dPresent.BackBufferCount = 1;
	d3dPresent.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	d3dPresent.Windowed = true;
	d3dPresent.BackBufferFormat = d3ddm.Format;
	d3dPresent.BackBufferWidth = resWidth;
	d3dPresent.BackBufferHeight = resHeight;

	HRESULT hr = d3dDevice->Reset(&d3dPresent);

	d3dDevice->SetVertexShader(NULL);
    d3dDevice->SetFVF(D3DFVF_TLVERTEX);

    d3dDevice->SetStreamSource(0, vertexBuffer, 0, sizeof(TLVERTEX));

    d3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    d3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    d3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

	FillVertexBuffer(resWidth, resHeight);
}

void FillVertexBuffer(int width, int height)
{
	float ratio = 160.0f / 144.0f;

	float x = 0, y = 0, frameWidth = 0, frameHeight = 0;

	if (width / 160.0f > height / 144.0f)
	{
		frameWidth = height * ratio;
		frameHeight = (float)height;
		x = (width - frameWidth) / 2.0f;
	}
	else
	{
		frameWidth = (float)width;
		frameHeight = width / ratio;
		y = (height - frameHeight) / 2.0f;
	}

	TLVERTEX* vertices;

	vertexBuffer->Lock(0, 0, (void**)&vertices, NULL);

	vertices[0].x = x - 0.5f;
	vertices[0].y = y - 0.5f;
	vertices[0].z = 0.0f;
	vertices[0].rhw = 1.0f;
	vertices[0].u = 0.0f;
	vertices[0].v = 0.0f;

	vertices[1].x = x + frameWidth - 0.5f;
	vertices[1].y = y - 0.5f;
	vertices[1].z = 0.0f;
	vertices[1].rhw = 1.0f;
	vertices[1].u = 1.0f;
	vertices[1].v = 0.0f;

	vertices[2].x = x + frameWidth - 0.5f;
	vertices[2].y = y + frameHeight - 0.5f;
	vertices[2].z = 0.0f;
	vertices[2].rhw = 1.0f;
	vertices[2].u = 1.0f;
	vertices[2].v = 1.0f;

	vertices[3].x = x - 0.5f;
	vertices[3].y = y + frameHeight - 0.5f;
	vertices[3].z = 0.0f;
	vertices[3].rhw = 1.0f;
	vertices[3].u = 0.0f;
	vertices[3].v = 1.0f;

	vertexBuffer->Unlock();
}

IDirect3DTexture9 *CreateBackbuffer(UINT width, UINT height)
{
	IDirect3DTexture9 *backbuffer;

	if (FAILED(D3DXCreateTexture(d3dDevice, 160, 144, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &backbuffer)))
	{
		return 0;
	}

	return backbuffer;
}

void *TextureLock(IDirect3DTexture9 *texture)
{
	D3DLOCKED_RECT rect;

	if (FAILED(texture->LockRect(0, &rect, NULL, 0)))
	{
		return 0;
	}

	return rect.pBits;
}

void TextureUnlock(IDirect3DTexture9 *texture)
{
	texture->UnlockRect(0);
}

void Render(IDirect3DTexture9 *texture)
{
	d3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0xFF000000, 0.0f, 0);
	d3dDevice->BeginScene();
	
	d3dDevice->SetTexture(0, texture);
	d3dDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 2);

	d3dDevice->EndScene();
	d3dDevice->Present(NULL, NULL, NULL, NULL);
}