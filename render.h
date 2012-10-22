#ifndef RENDER_H
#define RENDER_H

#include <d3d9.h>
#include <d3dx9.h>

extern IDirect3DDevice9* d3dDevice;

const DWORD D3DFVF_TLVERTEX = D3DFVF_XYZRHW | D3DFVF_TEX1;

struct TLVERTEX
{
    float x;
    float y;
    float z;
    float rhw;
    float u;
    float v;
};

bool InitD3D(HWND hWnd);
bool CloseD3D();
void OnWindowSizeChange(HWND hWnd);
void FillVertexBuffer(int width, int height);
IDirect3DTexture9 *CreateBackbuffer(UINT width, UINT height);
void *TextureLock(IDirect3DTexture9 *texture);
void TextureUnlock(IDirect3DTexture9 *texture);
void Render(IDirect3DTexture9 *texture);

#endif