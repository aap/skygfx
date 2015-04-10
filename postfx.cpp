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

static QuadVertex vcsVertices[20];
RwRaster *vcsBuffer1, *vcsBuffer2;
RwRect vcsRect;
RwImVertexIndex vcsIndices1[] = {
	0, 1, 2, 1, 2, 3,
		4, 5, 2, 5, 2, 3,
	4, 5, 6, 5, 6, 7,
		8, 9, 6, 9, 6, 7,
	8, 9, 10, 9, 10, 11,
		12, 13, 10, 13, 10, 11,
	12, 13, 14, 13, 14, 15,
};

static RwImVertexIndex* quadIndices = (RwImVertexIndex*)0x8D5174;
RwRaster *&CPostEffects__pRasterFrontBuffer = *(RwRaster**)0xC402D8;
RwCamera *&Camera = *(RwCamera**)0xC1703C;

WRAPPER void SpeedFX(float) { EAXJMP(0x7030A0); }

void
SpeedFX_Fix(float fStrength)
{
	// So we don't do useless work if no colour postfx was performed
	if(config->colorFilter < 2){
		RwCameraEndUpdate(Camera);
		RwRasterPushContext(CPostEffects__pRasterFrontBuffer);
		RwRasterRenderFast(RwCameraGetRaster(Camera), 0, 0);
		RwRasterPopContext();
		RwCameraBeginUpdate(Camera);
	}
	SpeedFX(fStrength);
}

void
CPostEffects__Radiosity_VCS(int col1, int nSubdivs, int unknown, int col2)
{
	float radiosityColors[4];
	RwTexture tempTexture;
	RwRaster *camraster;
	float uOffsets[] = { -1.0f, 1.0f, 0.0f, 0.0f };
	float vOffsets[] = { 0.0f, 0.0f, -1.0f, 1.0f };
	float halfU = 0.5f/256.0f, halfV = 0.5f/128.0f;

	vcsVertices[0].x = -1.0f;
	vcsVertices[0].y = 1.0f;
	vcsVertices[0].z = 0.0f;
	vcsVertices[0].rhw = 1.0f;
	vcsVertices[0].u = 0.0f + halfU;
	vcsVertices[0].v = 0.0f + halfV;
	vcsVertices[0].emissiveColor = 0xFFFFFFFF;

	vcsVertices[1].x = -1.0f;
	vcsVertices[1].y = -1.0f;
	vcsVertices[1].z = 0.0f;
	vcsVertices[1].rhw = 1.0f;
	vcsVertices[1].u = 0.0f + halfU;
	vcsVertices[1].v = 1.0f + halfV;
	vcsVertices[1].emissiveColor = 0xFFFFFFFF;

	vcsVertices[2].x = 1.0f;
	vcsVertices[2].y = 1.0f;
	vcsVertices[2].z = 0.0f;
	vcsVertices[2].rhw = 1.0f;
	vcsVertices[2].u = 1.0f + halfU;
	vcsVertices[2].v = 0.0f + halfV;
	vcsVertices[2].emissiveColor = 0xFFFFFFFF;

	vcsVertices[3].x = 1.0f;
	vcsVertices[3].y = -1.0f;
	vcsVertices[3].z = 0.0f;
	vcsVertices[3].rhw = 1.0f;
	vcsVertices[3].u = 1.0f + halfU;
	vcsVertices[3].v = 1.0f + halfV;
	vcsVertices[3].emissiveColor = 0xFFFFFFFF;

	for(int i = 4; i < 20; i++){
		int idx = (i-4)/4;
		vcsVertices[i].x = vcsVertices[i%4].x;
		vcsVertices[i].y = vcsVertices[i%4].y;
		vcsVertices[i].z = vcsVertices[i%4].z;
		vcsVertices[i].rhw = 1.0f;
		switch(i%4){
		case 0:
			vcsVertices[i].u = 0.0f + uOffsets[idx]/256.0f + halfU;
			vcsVertices[i].v = 0.0f + vOffsets[idx]/128.0f + halfV;
			break;
		case 1:
			vcsVertices[i].u = 0.0f + uOffsets[idx]/256.0f + halfU;
			vcsVertices[i].v = 1.0f + vOffsets[idx]/128.0f + halfV;
			break;
		case 2:
			vcsVertices[i].u = 1.0f + uOffsets[idx]/256.0f + halfU;
			vcsVertices[i].v = 0.0f + vOffsets[idx]/128.0f + halfV;
			break;
		case 3:
			vcsVertices[i].u = 1.0f + uOffsets[idx]/256.0f + halfU;
			vcsVertices[i].v = 1.0f + vOffsets[idx]/128.0f + halfV;
			break;
		}
		vcsVertices[i].emissiveColor = 0xFF262626;
	}

	// First pass - render downsampled and darkened frame to buffer2

	RwCameraEndUpdate(Camera);
	RwRasterPushContext(vcsBuffer1);
	camraster = RwCameraGetRaster(Camera);
	RwRasterRenderScaled(camraster, &vcsRect);
	RwRasterPopContext();
	RwCameraSetRaster(Camera, vcsBuffer2);
	RwCameraBeginUpdate(Camera);

	tempTexture.raster = vcsBuffer1;
	RwD3D9SetTexture(&tempTexture, 0);

	RwD3D9SetVertexDeclaration(quadVertexDecl);

	RwD3D9SetVertexShader(postfxVS);
	RwD3D9SetPixelShader(radiosityPS);

	radiosityColors[0] = 0x50/255.0f;
	radiosityColors[1] = 1.0f;
	radiosityColors[2] = 1.0f;
	RwD3D9SetPixelShaderConstant(0, radiosityColors, 1);

	int blend, srcblend, destblend;
	RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &blend);
	RwRenderStateGet(rwRENDERSTATESRCBLEND, &srcblend);
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &destblend);

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)0);
	RwD3D9SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
	RwD3D9DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 6, 2, vcsIndices1, vcsVertices, sizeof(QuadVertex));

	// Second pass - render brightened buffer2 to buffer1 (then swap buffers)

	for(int i = 0; i < 4; i++){
		RwCameraEndUpdate(Camera);
		RwCameraSetRaster(Camera, vcsBuffer1);
		RwCameraBeginUpdate(Camera);

		RwD3D9SetPixelShader(radiosityPS);
		RwD3D9SetTexture(NULL, 0);
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)0);
		RwD3D9DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 6, 2, vcsIndices1, vcsVertices, sizeof(QuadVertex));

		RwD3D9SetPixelShader(NULL);
		tempTexture.raster = vcsBuffer2;
		RwD3D9SetTexture(&tempTexture, 0);
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
		RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
		RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
		RwD3D9DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 6*7, 2*7, vcsIndices1, vcsVertices+4, sizeof(QuadVertex));
		RwRaster *tmp = vcsBuffer1;
		vcsBuffer1 = vcsBuffer2;
		vcsBuffer2 = tmp;
	}

	RwCameraEndUpdate(Camera);
	RwCameraSetRaster(Camera, camraster);
	RwCameraBeginUpdate(Camera);

	tempTexture.raster = vcsBuffer2;
	RwD3D9SetTexture(&tempTexture, 0);
	RwD3D9DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 6, 2, vcsIndices1, vcsVertices, sizeof(QuadVertex));

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)blend);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)srcblend);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)destblend);
	RwD3D9SetVertexShader(NULL);
}

void
CPostEffects__Radiosity_PS2(int col1, int nSubdivs, int unknown, int col2)
{
	float radiosityColors[4];
	RwTexture tempTexture;

	if(!config->doRadiosity)
		return;

	printf("vcsTrails: %d (%d %d)\n", config->vcsTrails, configs[0].vcsTrails, configs[1].vcsTrails);
	if(config->vcsTrails){
		CPostEffects__Radiosity_VCS(col1, nSubdivs, unknown, col2);
		return;
	}

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

	float texScale;
	RwCameraEndUpdate(Camera);
	RwRasterPushContext(smallFrontBuffer);
	if(config->downSampleRadiosity){
		texScale = 1.0f/4.0f;
		RwRasterRenderScaled(RwCameraGetRaster(Camera), &smallRect);
	}else{
		texScale = 1.0f;
		RwRasterRenderFast(RwCameraGetRaster(Camera), 0, 0);
	}
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
	float uOff = (config->radiosityOffset/16.0f)*scale * (4.0f*texScale) / rasterWidth;
	float vOff = (config->radiosityOffset/16.0f)*scale * (4.0f*texScale) / rasterHeight;

	quadVertices[0].x = 1.0f;
	quadVertices[0].y = -1.0f;
	quadVertices[0].z = 0.0f;
	quadVertices[0].rhw = 1.0f;
	quadVertices[0].u = uMax*texScale + halfU - uOff;
	quadVertices[0].v = vMax*texScale + halfV - vOff;

	quadVertices[1].x = -1.0f;
	quadVertices[1].y = -1.0f;
	quadVertices[1].z = 0.0f;
	quadVertices[1].rhw = 1.0f;
	quadVertices[1].u = 0.0f + halfU + uOff;
	quadVertices[1].v = vMax*texScale + halfV - vOff;

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
	quadVertices[3].u = uMax*texScale + halfU - uOff;
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

	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);

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
		if(GetAsyncKeyState(config->keys[0]) & 0x8000){
			if(!keystate){
				keystate = true;
				if(config == &configs[0])
					config++;
				else
					config = &configs[0];
				SetCloseFarAlphaDist(3.0, 1234.0); // second value overriden
			}
		}else
			keystate = false;
	}

	if(config->enableHotkeys){
		static bool keystate = false;
		if(GetAsyncKeyState(config->keys[1]) & 0x8000){
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
		if(GetAsyncKeyState(config->keys[2]) & 0x8000){
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

	vcsBuffer1 = RwRasterCreate(256, 128, CPostEffects__pRasterFrontBuffer->depth, 5);
	vcsBuffer2 = RwRasterCreate(256, 128, CPostEffects__pRasterFrontBuffer->depth, 5);
	vcsRect.x = 0;
	vcsRect.y = 0;
	vcsRect.w = 256;
	vcsRect.h = 128;
}

