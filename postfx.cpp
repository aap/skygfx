#include "skygfx.h"

// Credits: much of the code in this file was originally written by NTAuthority

struct QuadVertex
{
	RwReal      x, y, z;
	RwReal      rhw;
	RwUInt32    emissiveColor;
	RwReal      u, v;
	RwReal      u1, v1;
};

void *postfxVS, *colorFilterPS, *radiosityPS;
void *quadVertexDecl;
RwRect smallRect;
static QuadVertex quadVertices[4];
RwRaster *smallFrontBuffer = NULL;

static RwImVertexIndex* quadIndices = (RwImVertexIndex*)0x8D5174;
RwRaster *&CPostEffects__pRasterFrontBuffer = *(RwRaster**)0xC402D8;
RwCamera *&Camera = *(RwCamera**)0xC1703C;

void
CPostEffects__Radiosity_PS2(int col1, int nSubdivs, int unknown, int col2)
{
	float radiosityColors[4];
	RwTexture tempTexture;

	if(!config->doRadiosity)
		return;

	// if size of buffer changed, create a new downsampled version
	if(CPostEffects__pRasterFrontBuffer->width != smallFrontBuffer->width ||
	   CPostEffects__pRasterFrontBuffer->height != smallFrontBuffer->height ||
	   CPostEffects__pRasterFrontBuffer->depth != smallFrontBuffer->depth) {
		RwRasterDestroy(smallFrontBuffer);
		smallFrontBuffer = RwRasterCreate(CPostEffects__pRasterFrontBuffer->width,
						  CPostEffects__pRasterFrontBuffer->height,
						  CPostEffects__pRasterFrontBuffer->depth, 5);
		smallRect.x = 0;
		smallRect.y = 0;
		smallRect.w = Camera->frameBuffer->width/4;
		smallRect.h = Camera->frameBuffer->height/4;
	}

	RwCameraEndUpdate(Camera);
	RwRasterPushContext(smallFrontBuffer);
	RwRasterRenderScaled(RwCameraGetRaster(Camera), &smallRect);
	RwRasterPopContext();
	RwCameraBeginUpdate(Camera);

	tempTexture.raster = smallFrontBuffer;
	RwD3D9SetTexture(&tempTexture, 0);

	float rasterWidth = RwRasterGetWidth(CPostEffects__pRasterFrontBuffer);
	float rasterHeight = RwRasterGetHeight(CPostEffects__pRasterFrontBuffer);
	float halfU = 0.5 / rasterWidth;
	float halfV = 0.5 / rasterHeight;
	float uMax = RsGlobal->MaximumWidth / rasterWidth;
	float vMax = RsGlobal->MaximumHeight / rasterHeight;
	float scale = config->scaleOffsets ? RsGlobal->MaximumWidth/640.0f : 1.0f;
	float uOff = config->radiosityOffset*scale / 16.0f / rasterWidth;
	float vOff = config->radiosityOffset*scale / 16.0f / rasterHeight;

	quadVertices[0].x = 1.0f;
	quadVertices[0].y = -1.0f;
	quadVertices[0].z = 0.0f;
	quadVertices[0].rhw = 1.0f;
	quadVertices[0].u = uMax/4.0f + halfU - uOff;
	quadVertices[0].v = vMax/4.0f + halfV - vOff;

	quadVertices[1].x = -1.0f;
	quadVertices[1].y = -1.0f;
	quadVertices[1].z = 0.0f;
	quadVertices[1].rhw = 1.0f;
	quadVertices[1].u = 0.0f + halfU + uOff;
	quadVertices[1].v = vMax/4.0f + halfV - vOff;

	quadVertices[2].x = -1.0f;
	quadVertices[2].y = 1.0f;
	quadVertices[2].z = 0.0f;
	quadVertices[2].rhw = 1.0f;
	quadVertices[2].u = 0.0f + halfU + uOff;
	quadVertices[2].v = 0.0f + halfV + vOff;

	quadVertices[3].x = 1.0f;
	quadVertices[3].y = 1.0f;
	quadVertices[3].z = 0.0f;
	quadVertices[3].rhw = 1.0f;
	quadVertices[3].u = uMax/4.0f + halfU - uOff;
	quadVertices[3].v = 0.0f + halfV + vOff;

	RwD3D9SetVertexDeclaration(quadVertexDecl);

	RwD3D9SetVertexShader(postfxVS);
	RwD3D9SetPixelShader(radiosityPS);

	radiosityColors[0] = col1/255.0f;
	radiosityColors[1] = col2/255.0f;
	radiosityColors[2] = config->radiosityIntensity;
	RwD3D9SetPixelShaderConstant(0, radiosityColors, 1);

	int blend, srcblend, destblend;
	RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &blend);
	RwRenderStateGet(rwRENDERSTATESRCBLEND, &srcblend);
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &destblend);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
	RwD3D9SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	RwD3D9DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 6, 2, quadIndices, quadVertices, sizeof(QuadVertex));

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)blend);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)srcblend);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)destblend);

	RwD3D9SetVertexShader(NULL);
	RwD3D9SetPixelShader(NULL);
}

void
CPostEffects__ColourFilter_PS2(RwRGBA rgb1, RwRGBA rgb2)
{
	float rasterWidth = RwRasterGetWidth(CPostEffects__pRasterFrontBuffer);
	float rasterHeight = RwRasterGetHeight(CPostEffects__pRasterFrontBuffer);
	float halfU = 0.5 / rasterWidth;
	float halfV = 0.5 / rasterHeight;
	float uMax = RsGlobal->MaximumWidth / rasterWidth;
	float vMax = RsGlobal->MaximumHeight / rasterHeight;
	int i = 0;

	float leftOff, rightOff, topOff, bottomOff;
	float scale = config->scaleOffsets ? RsGlobal->MaximumWidth/640.0f : 1.0f;
	leftOff = (float)config->offLeft*scale / 16.0f / rasterWidth;
	rightOff = (float)config->offRight*scale / 16.0f / rasterWidth;
	topOff = (float)config->offTop*scale / 16.0f / rasterHeight;
	bottomOff = (float)config->offBottom*scale / 16.0f / rasterHeight;

	quadVertices[i].x = 1.0f;
	quadVertices[i].y = -1.0f;
	quadVertices[i].z = 0.0f;
	quadVertices[i].rhw = 1.0f;
	quadVertices[i].u = uMax + halfU;
	quadVertices[i].v = vMax + halfV;
	quadVertices[i].u1 = quadVertices[i].u + rightOff;
	quadVertices[i].v1 = quadVertices[i].v + bottomOff;
	i++;

	quadVertices[i].x = -1.0f;
	quadVertices[i].y = -1.0f;
	quadVertices[i].z = 0.0f;
	quadVertices[i].rhw = 1.0f;
	quadVertices[i].u = 0.0f + halfU;
	quadVertices[i].v = vMax + halfV;
	quadVertices[i].u1 = quadVertices[i].u + leftOff;
	quadVertices[i].v1 = quadVertices[i].v + bottomOff;
	i++;

	quadVertices[i].x = -1.0f;
	quadVertices[i].y = 1.0f;
	quadVertices[i].z = 0.0f;
	quadVertices[i].rhw = 1.0f;
	quadVertices[i].u = 0.0f + halfU;
	quadVertices[i].v = 0.0f + halfV;
	quadVertices[i].u1 = quadVertices[i].u + leftOff;
	quadVertices[i].v1 = quadVertices[i].v + topOff;
	i++;

	quadVertices[i].x = 1.0f;
	quadVertices[i].y = 1.0f;
	quadVertices[i].z = 0.0f;
	quadVertices[i].rhw = 1.0f;
	quadVertices[i].u = uMax + halfU;
	quadVertices[i].v = 0.0f + halfV;
	quadVertices[i].u1 = quadVertices[i].u + rightOff;
	quadVertices[i].v1 = quadVertices[i].v + topOff;
	
	RwTexture tempTexture;
	tempTexture.raster = CPostEffects__pRasterFrontBuffer;
	RwD3D9SetTexture(&tempTexture, 0);

	RwD3D9SetVertexDeclaration(quadVertexDecl);

	RwD3D9SetVertexShader(postfxVS);
	RwD3D9SetPixelShader(colorFilterPS);

	RwRGBAReal color, color2;
	RwRGBARealFromRwRGBA(&color, &rgb1);
	RwRGBARealFromRwRGBA(&color2, &rgb2);

	RwD3D9SetPixelShaderConstant(0, &color, 1);
	RwD3D9SetPixelShaderConstant(1, &color2, 1);

	RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
	RwD3D9SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	RwD3D9DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 6, 2, quadIndices, quadVertices, sizeof(QuadVertex));

	RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, 1);

	RwD3D9SetVertexShader(NULL);
	RwD3D9SetPixelShader(NULL);
}

WRAPPER char ReloadPlantConfig(void) { EAXJMP(0x5DD780); }

void
CPostEffects__ColourFilter_switch(RwRGBA rgb1, RwRGBA rgb2)
{
	if(config->enableHotkeys){
		static bool keystate = false;
		if(GetAsyncKeyState(VK_F10) & 0x8000){
			if(!keystate){
				keystate = true;
				if(config == &configs[0])
					config++;
				else
					config = &configs[0];
			}
		}else
			keystate = false;
	}

	if(config->enableHotkeys){
		static bool keystate = false;
		if(GetAsyncKeyState(VK_F11) & 0x8000){
			if(!keystate){
				keystate = true;
				if(config == &configs[0])
					config++;
				else
					config = &configs[0];
				readIni();
				SetCloseFarAlphaDist(3.0, 1234.0); // second value overriden
			}
		}else
			keystate = false;
	}

	if(config->enableHotkeys){
		static bool keystate = false;
		if(GetAsyncKeyState(VK_F12) & 0x8000){
			if(!keystate){
				keystate = true;
				if(config == &configs[0])
					config++;
				else
					config = &configs[0];
				readIni();
				ReloadPlantConfig();
				SetCloseFarAlphaDist(3.0, 1234.0); // second value overriden
			}
		}else
			keystate = false;
	}

	if(config->colorFilter == 0)
		CPostEffects__ColourFilter_PS2(rgb1, rgb2);
	else if(config->colorFilter == 1)
		CPostEffects__ColourFilter(rgb1, rgb2);
}

void
CPostEffects__Init(void)
{
	HRSRC resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_POSTFXVS), RT_RCDATA);
	RwUInt32 *shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreateVertexShader(shader, &postfxVS);
	FreeResource(shader);

	resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_FILTERPS), RT_RCDATA);
	shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreatePixelShader(shader, &colorFilterPS);
	FreeResource(shader);

	resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_RADIOSITYPS), RT_RCDATA);
	shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreatePixelShader(shader, &radiosityPS);
	FreeResource(shader);

	static const D3DVERTEXELEMENT9 vertexElements[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 16, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
		{ 0, 20, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		{ 0, 28, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
		D3DDECL_END()
	};

	RwD3D9CreateVertexDeclaration(vertexElements, &quadVertexDecl);

	smallFrontBuffer = RwRasterCreate(CPostEffects__pRasterFrontBuffer->width,
	                                  CPostEffects__pRasterFrontBuffer->height,
	                                  CPostEffects__pRasterFrontBuffer->depth, 5);
	smallRect.x = 0;
	smallRect.y = 0;
	smallRect.w = Camera->frameBuffer->width/4;
	smallRect.h = Camera->frameBuffer->height/4;
}

