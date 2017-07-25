#include "skygfx.h"

WRAPPER void CPostEffects::Grain(int strengh, bool generate) { EAXJMP(0x7037C0); }
WRAPPER void CPostEffects::SpeedFX(float) { EAXJMP(0x7030A0); }
RwRaster *&CPostEffects::pRasterFrontBuffer = *(RwRaster**)0xC402D8;
float &CPostEffects::m_fInfraredVisionFilterRadius = *(float*)0x8D50B8;
RwRaster *&CPostEffects::m_pGrainRaster = *(RwRaster**)0xC402B0;
WRAPPER void CPostEffects::InfraredVision(RwRGBA color1, RwRGBA color2) { EAXJMP(0x703F80); }
WRAPPER void CPostEffects::ImmediateModeRenderStatesStore(void) { EAXJMP(0x700CC0); }
WRAPPER void CPostEffects::ImmediateModeRenderStatesSet(void) { EAXJMP(0x700D70); }
WRAPPER void CPostEffects::ImmediateModeRenderStatesReStore(void) { EAXJMP(0x700E00); }
WRAPPER void CPostEffects::SetFilterMainColour(RwRaster *raster, RwRGBA color) { EAXJMP(0x703520); }
WRAPPER void CPostEffects::DrawQuad(float x1, float y1, float x2, float y2, uchar r, uchar g, uchar b, uchar alpha, RwRaster *ras) { EAXJMP(0x700EC0); }
WRAPPER void CPostEffects::NightVision(RwRGBA color) { EAXJMP(0x7011C0); }
WRAPPER void CPostEffects::ColourFilter(RwRGBA rgb1, RwRGBA rgb2) { EAXJMP(0x703650); }
float &CPostEffects::m_fNightVisionSwitchOnFXCount = *(float*)0xC40300;
int &CPostEffects::m_InfraredVisionGrainStrength = *(int*)0x8D50B4;
int &CPostEffects::m_NightVisionGrainStrength = *(int*)0x8D50A8;
bool &CPostEffects::m_bInfraredVision = *(bool*)0xC402B9;

// Credits: much of the code in this file was originally written by NTAuthority

struct QuadVertex
{
	RwReal      x, y, z;
	RwReal      rhw;
	RwUInt32    emissiveColor;
	RwReal      u, v;
	RwReal      u1, v1;
};

struct ScreenVertex
{
	RwReal      x, y, z;
	RwReal      rhw;
	RwReal      u, v;
};

void *postfxVS, *colorFilterPS, *radiosityPS, *grainPS;
void *iiiTrailsPS, *vcTrailsPS;
void *gradingPS;
void *quadVertexDecl, *screenVertexDecl;
RwRect smallRect;
static QuadVertex quadVertices[4];
static ScreenVertex screenVertices[4];
RwRaster *target1, *target2;

RwRaster *vcs_radiosity_target1, *vcs_radiosity_target2;
static QuadVertex vcsVertices[24];
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

RwImVertexIndex radiosityIndices[] = {
	0, 1, 2, 1, 2, 3
};

static RwImVertexIndex* quadIndices = (RwImVertexIndex*)0x8D5174;

void
CPostEffects::SpeedFX_Fix(float fStrength)
{
	// So we don't do useless work if no colour postfx was performed
	if(config->colorFilter < 2){
		RwCameraEndUpdate(Camera);
		RwRasterPushContext(CPostEffects::pRasterFrontBuffer);
		RwRasterRenderFast(RwCameraGetRaster(Camera), 0, 0);
		RwRasterPopContext();
		RwCameraBeginUpdate(Camera);
	}
	CPostEffects::SpeedFX(fStrength);
}

RwD3D9Vertex radiosity_vcs_vertices[44];

#define LIMIT (config->trailsLimit)
#define INTENSITY (config->trailsIntensity)

// vertex decl: C980F4

void
makequad(RwD3D9Vertex *v, int width, int height, int texwidth = 0, int texheight = 0)
{
	float w, h, tw, th;
	w = width;
	h = height;
	tw = texwidth > 0 ? texwidth : w;
	th = texheight > 0 ? texheight : h;
	v[0].x = 0;
	v[0].y = 0;
	v[0].z = 0.0f;
	v[0].rhw = 1.0f;
	v[0].u = 0.5f / tw;
	v[0].v = 0.5f / th;
	v[0].emissiveColor = 0xFFFFFFFF;
	v[1].x = 0;
	v[1].y = h;
	v[1].z = 0.0f;
	v[1].rhw = 1.0f;
	v[1].u = 0.5f / tw;
	v[1].v = (h + 0.5f) / th;
	v[1].emissiveColor = 0xFFFFFFFF;
	v[2].x = w;
	v[2].y = 0;
	v[2].z = 0.0f;
	v[2].rhw = 1.0f;
	v[2].u = (w + 0.5f) / tw;
	v[2].v = 0.5f / th;
	v[2].emissiveColor = 0xFFFFFFFF;
	v[3].x = w;
	v[3].y = h;
	v[3].z = 0.0f;
	v[3].rhw = 1.0f;
	v[3].u = (w + 0.5f) / tw;
	v[3].v = (h + 0.5f) / th;
	v[3].emissiveColor = 0xFFFFFFFF;
}

void
CPostEffects::Radiosity_VCS_init(void)
{
	static float uOffsets[] = { -1.0f, 1.0f, 0.0f, 0.0f,   -1.0f, 1.0f, -1.0f, 1.0f };
	static float vOffsets[] = { 0.0f, 0.0f, -1.0f, 1.0f,   -1.0f, -1.0f, 1.0f, 1.0f };
	int i;
	RwUInt32 c;
	float w, h;
	if(vcs_radiosity_target1)
		RwRasterDestroy(vcs_radiosity_target1);
	vcs_radiosity_target1 = RwRasterCreate(256, 128, RwCameraGetRaster(Camera)->depth, rwRASTERTYPECAMERATEXTURE);
	if(vcs_radiosity_target2)
		RwRasterDestroy(vcs_radiosity_target2);
	vcs_radiosity_target2 = RwRasterCreate(256, 128, RwCameraGetRaster(Camera)->depth, rwRASTERTYPECAMERATEXTURE);
//	RwD3D9CreateVertexBuffer(stride, size, &vbuf, &offset);

	w = 256;
	h = 128;

	// TODO: tex coords correct?
	makequad(radiosity_vcs_vertices, 256, 128);
	makequad(radiosity_vcs_vertices+4, RwCameraGetRaster(Camera)->width, RwCameraGetRaster(Camera)->height);

	// black vertices; at 8
	for(i = 0; i < 4; i++){
		radiosity_vcs_vertices[i+8] = radiosity_vcs_vertices[i];
		radiosity_vcs_vertices[i+8].emissiveColor = 0;
	}

	// two sets blur vertices; at 12
	c = D3DCOLOR_ARGB(0xFF, 36, 36, 36);
	for(i = 0; i < 2*4*4; i++){
		radiosity_vcs_vertices[i+12] = radiosity_vcs_vertices[i%4];
		radiosity_vcs_vertices[i+12].emissiveColor = c;
		switch(i%4){
		case 0:
			radiosity_vcs_vertices[i+12].u = (uOffsets[i/4] + 0.5f) / w;
			radiosity_vcs_vertices[i+12].v = (vOffsets[i/4] + 0.5f) / h;
			break;
		case 1:
			radiosity_vcs_vertices[i+12].u = (uOffsets[i/4] + 0.5f) / w;
			radiosity_vcs_vertices[i+12].v = (h + vOffsets[i/4] + 0.5f) / h;
			break;
		case 2:
			radiosity_vcs_vertices[i+12].u = (w + uOffsets[i/4] + 0.5f) / w;
			radiosity_vcs_vertices[i+12].v = (vOffsets[i/4] + 0.5f) / h;
			break;
		case 3:
			radiosity_vcs_vertices[i+12].u = (w + uOffsets[i/4] + 0.5f) / w;
			radiosity_vcs_vertices[i+12].v = (h + vOffsets[i/4] + 0.5f) / h;
			break;
		}
	}
}

void
CPostEffects::Radiosity_VCS_new(int limit, int intensity)
{
	static int lastWidth, lastHeight;
	int i;
	RwRaster *fb;
	RwRaster *fb1, *fb2, *tmp;

	fb = RwCameraGetRaster(Camera);
	if(lastWidth != fb->width || lastHeight != fb->height){
		Radiosity_VCS_init();
		lastWidth = fb->width;
		lastHeight = fb->height;
	}

	RwRect r;
	r.x = 0;
	r.y = 0;
	r.w = 256;
	r.h = 128;

	CPostEffects::ImmediateModeRenderStatesStore();
	CPostEffects::ImmediateModeRenderStatesSet();
	RwD3D9SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
	RwD3D9SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	RwCameraEndUpdate(Camera);

	RwRasterPushContext(vcs_radiosity_target2);
	RwRasterRenderScaled(fb, &r);
	RwRasterPopContext();

	RwCameraSetRaster(Camera, vcs_radiosity_target2);
	RwCameraBeginUpdate(Camera);

	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
	RwD3D9SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_REVSUBTRACT);
	RwD3D9SetRenderState(D3DRS_SRCBLEND, D3DBLEND_BLENDFACTOR);
	RwD3D9SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
	RwD3D9SetRenderState(D3DRS_BLENDFACTOR, D3DCOLOR_ARGB(0xFF, limit/2, limit/2, limit/2));
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, radiosity_vcs_vertices, 4, radiosityIndices, 6);

	fb1 = vcs_radiosity_target1;
	fb2 = vcs_radiosity_target2;
	for(i = 0; i < 4; i++){
		RwD3D9SetRenderTarget(0, fb1);

		RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
		RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, radiosity_vcs_vertices+8, 4, radiosityIndices, 6);

		RwRenderStateSet(rwRENDERSTATETEXTURERASTER, fb2);
		RwRenderStateSet(rwRENDERSTATETEXTUREADDRESSU, (void*)rwTEXTUREADDRESSCLAMP);
		RwRenderStateSet(rwRENDERSTATETEXTUREADDRESSV, (void*)rwTEXTUREADDRESSCLAMP);
		RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		RwD3D9SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		RwD3D9SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		RwD3D9SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
		if((i % 2) == 0)
			RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, radiosity_vcs_vertices+12, 4*4, vcsIndices1, 6*7);
		else
			RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, radiosity_vcs_vertices+28, 4*4, vcsIndices1, 6*7);

		tmp = fb1;
		fb1 = fb2;
		fb2 = tmp;
	}

	RwCameraEndUpdate(Camera);
	RwCameraSetRaster(Camera, fb);
	RwCameraBeginUpdate(Camera);

	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, fb2);
	RwRenderStateSet(rwRENDERSTATETEXTUREADDRESSU, (void*)rwTEXTUREADDRESSCLAMP);
	RwRenderStateSet(rwRENDERSTATETEXTUREADDRESSV, (void*)rwTEXTUREADDRESSCLAMP);
	RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	RwD3D9SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	RwD3D9SetRenderState(D3DRS_SRCBLEND, D3DBLEND_BLENDFACTOR);
	RwD3D9SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
	RwD3D9SetRenderState(D3DRS_BLENDFACTOR, D3DCOLOR_ARGB(0xFF, intensity*4, intensity*4, intensity*4));
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, radiosity_vcs_vertices+4, 4, radiosityIndices, 6);
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, radiosity_vcs_vertices+4, 4, radiosityIndices, 6);

	RwD3D9SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
	CPostEffects::ImmediateModeRenderStatesReStore();
}

RwD3D9Vertex blur_vcs_vertices[24];
RwImVertexIndex blur_vcs_Indices[] = {
	0, 1, 2, 2, 1, 3,
	4, 5, 6, 6, 5, 7,
	8, 9, 10, 10, 9, 11,
};
RwRaster *lastFrameBuffer;
RwRGBA vcsblurrgb;

#define BLUROFFSET (2.1f)
#define BLURINTENSITY (39.0f)

void
CPostEffects::Blur_VCS(void)
{
	static int lastWidth, lastHeight;
	static int justInitialized;
	int i;
	int bufw, bufh;
	int screenw, screenh;
	int intensity;
	bufw = CPostEffects::pRasterFrontBuffer->width;
	bufh = CPostEffects::pRasterFrontBuffer->height;

	if(GetAsyncKeyState(VK_F7) & 0x8000){
		justInitialized = 1;
		return;
	}

	if(lastWidth != bufw || lastHeight != bufh){
		if(lastFrameBuffer)
			RwRasterDestroy(lastFrameBuffer);
		lastFrameBuffer = RwRasterCreate(bufw, bufh, CPostEffects::pRasterFrontBuffer->depth, rwRASTERTYPECAMERATEXTURE);
		justInitialized = 1;
		lastWidth = bufw;
		lastHeight = bufh;
	}

	screenw = RwCameraGetRaster(Camera)->width;
	screenh = RwCameraGetRaster(Camera)->height;

	makequad(blur_vcs_vertices, screenw, screenh, bufw, bufh);
	for(i = 0; i < 4; i++)
		blur_vcs_vertices[i].x += BLUROFFSET;
	makequad(blur_vcs_vertices+4, screenw, screenh, bufw, bufh);
	for(i = 4; i < 8; i++){
		blur_vcs_vertices[i].x += BLUROFFSET;
		blur_vcs_vertices[i].y += BLUROFFSET;
	}
	makequad(blur_vcs_vertices+8, screenw, screenh, bufw, bufh);
	for(i = 8; i < 12; i++)
		blur_vcs_vertices[i].y += BLUROFFSET;
	makequad(blur_vcs_vertices+12, screenw, screenh, bufw, bufh);
	for(i = 12; i < 16; i++)
		blur_vcs_vertices[i].emissiveColor = D3DCOLOR_ARGB(0xff, vcsblurrgb.red, vcsblurrgb.green, vcsblurrgb.blue);
	makequad(blur_vcs_vertices+16, screenw, screenh, bufw, bufh);
	makequad(blur_vcs_vertices+20, screenw, screenh, bufw, bufh);
	for(i = 20; i < 24; i++)
		blur_vcs_vertices[i].emissiveColor = 0;

	CPostEffects::ImmediateModeRenderStatesStore();
	CPostEffects::ImmediateModeRenderStatesSet();
	RwD3D9SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);

	// get current frame
	RwCameraEndUpdate(Camera);
	RwRasterPushContext(CPostEffects::pRasterFrontBuffer);
	RwRasterRenderFast(RwCameraGetRaster(Camera), 0, 0);
	RwRasterPopContext();
	RwCameraBeginUpdate(Camera);

	// blur frame
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, CPostEffects::pRasterFrontBuffer);
	RwD3D9SetRenderState(D3DRS_SRCBLEND, D3DBLEND_BLENDFACTOR);
	RwD3D9SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVBLENDFACTOR);
	intensity = BLURINTENSITY*0.8f;
	RwD3D9SetRenderState(D3DRS_BLENDFACTOR, D3DCOLOR_ARGB(0xFF, intensity, intensity, intensity));
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, blur_vcs_vertices, 12, blur_vcs_Indices, 3*6);

	// add colour filter color
	RwD3D9SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	RwD3D9SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, blur_vcs_vertices+12, 4, blur_vcs_Indices, 6);

	// blend with last frame
	if(justInitialized)
		justInitialized = 0;
	else{
		RwRenderStateSet(rwRENDERSTATETEXTURERASTER, lastFrameBuffer);
		RwD3D9SetRenderState(D3DRS_SRCBLEND, D3DBLEND_BLENDFACTOR);
		RwD3D9SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVBLENDFACTOR);
		RwD3D9SetRenderState(D3DRS_BLENDFACTOR, D3DCOLOR_ARGB(0xFF, 32, 32, 32));
		RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, blur_vcs_vertices+16, 4, blur_vcs_Indices, 6);
	}

	// blend with black. Is this real?
if(0){
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
	RwD3D9SetRenderState(D3DRS_SRCBLEND, D3DBLEND_BLENDFACTOR);
	RwD3D9SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVBLENDFACTOR);
	RwD3D9SetRenderState(D3DRS_BLENDFACTOR, D3DCOLOR_ARGB(0xFF, 32, 32, 32));
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, blur_vcs_vertices+20, 4, blur_vcs_Indices, 6);
}

	RwCameraEndUpdate(Camera);
	RwRasterPushContext(lastFrameBuffer);
	RwRasterRenderFast(RwCameraGetRaster(Camera), 0, 0);
	RwRasterPopContext();
	RwCameraBeginUpdate(Camera);

	RwD3D9SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
	CPostEffects::ImmediateModeRenderStatesReStore();
}

void
CPostEffects::Radiosity_VCS(int intensityLimit, int filterPasses, int renderPasses, int intensity)
{
	RwRaster *vcsBuffer1, *vcsBuffer2;
	float radiosityColors[4];
	static RwTexture *tempTexture = NULL;
	RwRaster *camraster;
	int width, height;
	float uOffsets[] = { -1.0f, 1.0f, 0.0f, 0.0f };
	float vOffsets[] = { 0.0f, 0.0f, -1.0f, 1.0f };

	width = vcsRect.w = filterPasses ? RsGlobal->MaximumWidth*256/640 : RsGlobal->MaximumWidth;
	height = vcsRect.h = filterPasses ? RsGlobal->MaximumHeight*128/480 : RsGlobal->MaximumHeight;

	if(target1->width < width || target2->width < width ||
	   target1->height < height || target2->height < height || 
	   target1->width != target2->width || target1->height != target2->height){
		int width2 = 1, height2 = 1;
		while(width2 < width) width2 <<= 1;
		while(height2 < height) height2 <<= 1;
		RwRasterDestroy(target1);
		target1 = RwRasterCreate(width2, height2, CPostEffects::pRasterFrontBuffer->depth, 5);
		RwRasterDestroy(target2);
		target2 = RwRasterCreate(width2, height2, CPostEffects::pRasterFrontBuffer->depth, 5);
	}
	vcsBuffer1 = target1;
	vcsBuffer2 = target2;

	float rasterWidth = RwRasterGetWidth(target1);
	float rasterHeight = RwRasterGetHeight(target1);
	float halfU = 0.5 / rasterWidth;
	float halfV = 0.5 / rasterHeight;
	float uMax = width / rasterWidth;
	float vMax = height / rasterHeight;
	float xMax = -1.0f + 2*uMax;
	float yMax = 1.0f - 2*vMax;
	float xOffsetScale = width/256.0f;
	float yOffsetScale = height/128.0f;
	if(config->radiosityFilterUCorrection == 0.0f)
		xOffsetScale = yOffsetScale = 0.0f;

	vcsVertices[0].x = -1.0f;
	vcsVertices[0].y = 1.0f;
	vcsVertices[0].z = 0.0f;
	vcsVertices[0].rhw = 1.0f;
	vcsVertices[0].u = 0.0f + halfU;
	vcsVertices[0].v = 0.0f + halfV;
	vcsVertices[0].emissiveColor = 0xFFFFFFFF;

	vcsVertices[1].x = -1.0f;
	vcsVertices[1].y = yMax;
	vcsVertices[1].z = 0.0f;
	vcsVertices[1].rhw = 1.0f;
	vcsVertices[1].u = 0.0f + halfU;
	vcsVertices[1].v = vMax + halfV;
	vcsVertices[1].emissiveColor = 0xFFFFFFFF;

	vcsVertices[2].x = xMax;
	vcsVertices[2].y = 1.0f;
	vcsVertices[2].z = 0.0f;
	vcsVertices[2].rhw = 1.0f;
	vcsVertices[2].u = uMax + halfU;
	vcsVertices[2].v = 0.0f + halfV;
	vcsVertices[2].emissiveColor = 0xFFFFFFFF;

	vcsVertices[3].x = xMax;
	vcsVertices[3].y = yMax;
	vcsVertices[3].z = 0.0f;
	vcsVertices[3].rhw = 1.0f;
	vcsVertices[3].u = uMax + halfU;
	vcsVertices[3].v = vMax + halfV;
	vcsVertices[3].emissiveColor = 0xFFFFFFFF;

	vcsVertices[4].x = -1.0f;
	vcsVertices[4].y = 1.0f;
	vcsVertices[4].z = 0.0f;
	vcsVertices[4].rhw = 1.0f;
	vcsVertices[4].u = 0.0f + halfU;
	vcsVertices[4].v = 0.0f + halfV;
	vcsVertices[4].emissiveColor = 0xFFFFFFFF;

	vcsVertices[5].x = -1.0f;
	vcsVertices[5].y = -1.0f;
	vcsVertices[5].z = 0.0f;
	vcsVertices[5].rhw = 1.0f;
	vcsVertices[5].u = 0.0f + halfU;
	vcsVertices[5].v = vMax + halfV;
	vcsVertices[5].emissiveColor = 0xFFFFFFFF;

	vcsVertices[6].x = 1.0f;
	vcsVertices[6].y = 1.0f;
	vcsVertices[6].z = 0.0f;
	vcsVertices[6].rhw = 1.0f;
	vcsVertices[6].u = uMax + halfU;
	vcsVertices[6].v = 0.0f + halfV;
	vcsVertices[6].emissiveColor = 0xFFFFFFFF;

	vcsVertices[7].x = 1.0f;
	vcsVertices[7].y = -1.0f;
	vcsVertices[7].z = 0.0f;
	vcsVertices[7].rhw = 1.0f;
	vcsVertices[7].u = uMax + halfU;
	vcsVertices[7].v = vMax + halfV;
	vcsVertices[7].emissiveColor = 0xFFFFFFFF;

	RwUInt32 c = 0xFF000000 | 0x010101*config->trailsIntensity;
	for(int i = 8; i < 24; i++){
		int idx = (i-8)/4;
		vcsVertices[i].x = vcsVertices[i%4].x;
		vcsVertices[i].y = vcsVertices[i%4].y;
		vcsVertices[i].z = 0.0f;
		vcsVertices[i].rhw = 1.0f;
		switch(i%4){
		case 0:
			vcsVertices[i].u = 0.0f + xOffsetScale*uOffsets[idx]/rasterWidth + halfU;
			vcsVertices[i].v = 0.0f + yOffsetScale*vOffsets[idx]/rasterHeight + halfV;
			break;
		case 1:
			vcsVertices[i].u = 0.0f + xOffsetScale*uOffsets[idx]/rasterWidth + halfU;
			vcsVertices[i].v = vMax + yOffsetScale*vOffsets[idx]/rasterHeight + halfV;
			break;
		case 2:
			vcsVertices[i].u = uMax + xOffsetScale*uOffsets[idx]/rasterWidth + halfU;
			vcsVertices[i].v = 0.0f + yOffsetScale*vOffsets[idx]/rasterHeight + halfV;
			break;
		case 3:
			vcsVertices[i].u = uMax + xOffsetScale*uOffsets[idx]/rasterWidth + halfU;
			vcsVertices[i].v = vMax + yOffsetScale*vOffsets[idx]/rasterHeight + halfV;
			break;
		}
		vcsVertices[i].emissiveColor = c;
	}

	// First pass - render downsampled and darkened frame to buffer2

	RwCameraEndUpdate(Camera);
	RwRasterPushContext(vcsBuffer1);
	camraster = RwCameraGetRaster(Camera);
	RwRasterRenderScaled(camraster, &vcsRect);
	RwRasterPopContext();
	RwCameraSetRaster(Camera, vcsBuffer2);
	RwCameraBeginUpdate(Camera);

	if(tempTexture == NULL){
		tempTexture = RwTextureCreate(NULL);
		tempTexture->filterAddressing = 0x1102;
	}

	tempTexture->raster = vcsBuffer1;
	RwD3D9SetTexture(tempTexture, 0);

	RwD3D9SetVertexDeclaration(quadVertexDecl);

	RwD3D9SetVertexShader(postfxVS);
	RwD3D9SetPixelShader(radiosityPS);

	// TODO: this may well be controlled by the timecycle and passed as an argument here
	radiosityColors[0] = config->trailsLimit/255.0f;
	radiosityColors[1] = 1.0f;
	radiosityColors[2] = 1.0f;
	RwD3D9SetPixelShaderConstant(0, radiosityColors, 1);

	int blend, srcblend, destblend, ztest, zwrite;
	RwRenderStateGet(rwRENDERSTATEZTESTENABLE, &ztest);
	RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &zwrite);
	RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &blend);
	RwRenderStateGet(rwRENDERSTATESRCBLEND, &srcblend);
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &destblend);

	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)0);
	RwD3D9SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
	RwD3D9DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 6, 2, vcsIndices1, vcsVertices, sizeof(QuadVertex));

	// Second pass - render brightened buffer2 to buffer1 (then swap buffers)
	RwD3D9SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	for(int i = 0; i < 4; i++){
		RwCameraEndUpdate(Camera);
		RwCameraSetRaster(Camera, vcsBuffer1);
		RwCameraBeginUpdate(Camera);

		RwD3D9SetPixelShader(radiosityPS);
		RwD3D9SetTexture(NULL, 0);
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)0);
		RwD3D9DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 6, 2, vcsIndices1, vcsVertices, sizeof(QuadVertex));

		RwD3D9SetPixelShader(NULL);
		tempTexture->raster = vcsBuffer2;
		RwD3D9SetTexture(tempTexture, 0);
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
		RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
		RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
		RwD3D9DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 6*7, 2*7, vcsIndices1, vcsVertices+8, sizeof(QuadVertex));
		RwRaster *tmp = vcsBuffer1;
		vcsBuffer1 = vcsBuffer2;
		vcsBuffer2 = tmp;
	}

	RwCameraEndUpdate(Camera);
	RwCameraSetRaster(Camera, camraster);
	RwCameraBeginUpdate(Camera);

	tempTexture->raster = vcsBuffer2;
	RwD3D9SetTexture(tempTexture, 0);
	RwD3D9DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 6, 2, vcsIndices1, vcsVertices+4, sizeof(QuadVertex));

	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)ztest);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)zwrite);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)blend);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)srcblend);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)destblend);
	RwD3D9SetVertexShader(NULL);
}

void
CPostEffects::Radiosity_PS2(int intensityLimit, int filterPasses, int renderPasses, int intensity)
{
	int width, height;
	RwRaster *buffer1, *buffer2, *camraster;
	float radiosityColors[4];
	static RwTexture *tempTexture = NULL;

	if(!config->doRadiosity)
		return;

	if(config->vcsTrails){
		if(!(GetAsyncKeyState(VK_F8) & 0x8000))
			CPostEffects::Radiosity_VCS_new(intensityLimit, intensity);
		if(config->colorFilter == 5)
			CPostEffects::Blur_VCS();
		return;
	}
	if(tempTexture == NULL){
		tempTexture = RwTextureCreate(NULL);
		tempTexture->filterAddressing = 0x1102;
	}


	width = RsGlobal->MaximumWidth;
	height = RsGlobal->MaximumHeight;

	if(target1->width < width || target2->width < width ||
	   target1->height < height || target2->height < height || 
	   target1->width != target2->width || target1->height != target2->height){
		int width2 = 1, height2 = 1;
		while(width2 < width) width2 <<= 1;
		while(height2 < height) height2 <<= 1;
		RwRasterDestroy(target1);
		target1 = RwRasterCreate(width2, height2, CPostEffects::pRasterFrontBuffer->depth, 5);
		RwRasterDestroy(target2);
		target2 = RwRasterCreate(width2, height2, CPostEffects::pRasterFrontBuffer->depth, 5);
	}
	buffer1 = target1;
	buffer2 = target2;

	RwCameraEndUpdate(Camera);
	RwRasterPushContext(buffer1);
	camraster = RwCameraGetRaster(Camera);
	RwRasterRenderFast(RwCameraGetRaster(Camera), 0, 0);
	RwRasterPopContext();
	RwCameraBeginUpdate(Camera);

	float rasterWidth = RwRasterGetWidth(target1);
	float rasterHeight = RwRasterGetHeight(target1);
	float xOffsetScale = width/640.0f;
	float yOffsetScale = height/480.0f;

	screenVertices[0].z = 0.0f;
	screenVertices[0].rhw = 1.0f / Camera->nearPlane;
	screenVertices[1].z = 0.0f;
	screenVertices[1].rhw = 1.0f / Camera->nearPlane;
	screenVertices[2].z = 0.0f;
	screenVertices[2].rhw = 1.0f / Camera->nearPlane;
	screenVertices[3].z = 0.0f;
	screenVertices[3].rhw = 1.0f / Camera->nearPlane;

	RwD3D9SetVertexDeclaration(screenVertexDecl);
	RwD3D9SetVertexShader(NULL);
	RwD3D9SetPixelShader(NULL);

	int blend, srcblend, destblend, ztest, zwrite;
	RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &blend);
	RwRenderStateGet(rwRENDERSTATESRCBLEND, &srcblend);
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &destblend);
	RwRenderStateGet(rwRENDERSTATEZTESTENABLE, &ztest);
	RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &zwrite);

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)0);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)0);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)0);
	RwD3D9SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	RwD3D9SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

	screenVertices[0].x = 0.0f;
	screenVertices[0].y = 0.0f;
	screenVertices[0].u = (config->radiosityFilterUCorrection*xOffsetScale + 0.5f) / rasterWidth;
	screenVertices[0].v = (config->radiosityFilterVCorrection*yOffsetScale + 0.5f) / rasterHeight;
	screenVertices[2].x = screenVertices[0].x;
	screenVertices[1].y = screenVertices[0].y;
	screenVertices[2].u = screenVertices[0].u;
	screenVertices[1].v = screenVertices[0].v;

	for(int i = 0; i < filterPasses; i++){
		screenVertices[3].x = width/2 + 1*xOffsetScale;
		screenVertices[3].y = height/2 + 1*yOffsetScale;
		screenVertices[3].u = (width + 0.5f) / rasterWidth;
		screenVertices[3].v = (height + 0.5f) / rasterHeight;

		screenVertices[1].x = screenVertices[3].x;
		screenVertices[2].y = screenVertices[3].y;
		screenVertices[1].u = screenVertices[3].u;
		screenVertices[2].v = screenVertices[3].v;

		width /= 2;
		height /= 2;

//		RwD3D9SetRenderTarget(0, buffer2);
		RwCameraEndUpdate(Camera);
		RwCameraSetRaster(Camera, buffer2);
		RwCameraBeginUpdate(Camera);

		tempTexture->raster = buffer1;
		RwD3D9SetTexture(tempTexture, 0);
		RwD3D9DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 6, 2, radiosityIndices, screenVertices, sizeof(ScreenVertex));
		RwRaster *tmp = buffer1;
		buffer1 = buffer2;
		buffer2 = tmp;
	}

	screenVertices[0].x = 0.0f;
	screenVertices[0].y = 0.0f;
	screenVertices[0].u = 0.5f / rasterWidth;
	screenVertices[0].v = 0.5f / rasterHeight;

	screenVertices[3].x = RsGlobal->MaximumWidth;
	screenVertices[3].y = RsGlobal->MaximumHeight;
	screenVertices[3].u = (width+0.5f) / rasterWidth;
	screenVertices[3].v = (height+0.5f) / rasterHeight;

	screenVertices[1].x = screenVertices[3].x;
	screenVertices[2].y = screenVertices[3].y;
	screenVertices[1].u = screenVertices[3].u;
	screenVertices[2].v = screenVertices[3].v;
	screenVertices[2].x = screenVertices[0].x;
	screenVertices[1].y = screenVertices[0].y;
	screenVertices[2].u = screenVertices[0].u;
	screenVertices[1].v = screenVertices[0].v;

	RwCameraEndUpdate(Camera);
	RwCameraSetRaster(Camera, camraster);
	RwCameraBeginUpdate(Camera);

	tempTexture->raster = buffer1;
	RwD3D9SetTexture(tempTexture, 0);
	RwD3D9SetPixelShader(radiosityPS);

	radiosityColors[0] = intensityLimit/255.0f;
	radiosityColors[1] = (intensity/255.0f)*renderPasses;
	RwD3D9SetPixelShaderConstant(0, radiosityColors, 1);

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);

	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);

	RwD3D9DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 6, 2, radiosityIndices, screenVertices, sizeof(ScreenVertex));

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)blend);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)srcblend);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)destblend);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)ztest);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)zwrite);

	RwD3D9SetVertexShader(NULL);
	RwD3D9SetPixelShader(NULL);
}

void
CPostEffects::ColourFilter_Generic(RwRGBA rgb1, RwRGBA rgb2, void *ps)
{
	float rasterWidth = RwRasterGetWidth(CPostEffects::pRasterFrontBuffer);
	float rasterHeight = RwRasterGetHeight(CPostEffects::pRasterFrontBuffer);
	float halfU = 0.5 / rasterWidth;
	float halfV = 0.5 / rasterHeight;
	float uMax = RsGlobal->MaximumWidth / rasterWidth;
	float vMax = RsGlobal->MaximumHeight / rasterHeight;
	int i = 0;

	float leftOff, rightOff, topOff, bottomOff;
	float scale = RsGlobal->MaximumWidth/640.0f;
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
	
	static RwTexture *tempTexture = NULL;
	if(tempTexture == NULL){
		tempTexture = RwTextureCreate(NULL);
		tempTexture->filterAddressing = 0x1102;
	}
	tempTexture->raster = CPostEffects::pRasterFrontBuffer;
	RwD3D9SetTexture(tempTexture, 0);

	RwD3D9SetVertexDeclaration(quadVertexDecl);

	RwD3D9SetVertexShader(postfxVS);
	RwD3D9SetPixelShader(ps);

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

void *overrideIm2dPixelShader;

void
Im2dSetPixelShader_hook(void*)
{
	if(overrideIm2dPixelShader)
		RwD3D9SetPixelShader(overrideIm2dPixelShader);
	else
		RwD3D9SetPixelShader(NULL);
}


RwIm2DVertex *postfxQuad = (RwIm2DVertex*)0xC401D8;
RwRaster *&visionRaster = *(RwRaster**)0xC40158;
RwRaster *grainRaster;

void
CPostEffects::SetFilterMainColour_PS2(RwRaster *raster, RwRGBA color)
{
	static float colorScale = 255.0f/128.0f;
	CPostEffects::ImmediateModeRenderStatesStore();
	CPostEffects::ImmediateModeRenderStatesSet();
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, 0);
	RwD3D9SetPixelShaderConstant(0, &colorScale, 1);
	overrideIm2dPixelShader = simplePS;
	CPostEffects::DrawQuad(0, 0, RwRasterGetWidth(raster), RwRasterGetHeight(raster),
	                       color.red, color.green, color.blue, color.alpha, raster);
	overrideIm2dPixelShader = NULL;
	CPostEffects::ImmediateModeRenderStatesReStore();
}

void
CPostEffects::InfraredVision_PS2(RwRGBA c1, RwRGBA c2)
{
	if(config->infraredVision != 0){
		CPostEffects::InfraredVision(c1, c2);
		return;
	}

	CPostEffects::ImmediateModeRenderStatesStore();
	CPostEffects::ImmediateModeRenderStatesSet();

	float r = CPostEffects::m_fInfraredVisionFilterRadius;
	// not sure this scales correctly, but it looks ok (need better brain)
	float ru = r * RsGlobal->MaximumWidth  / RwRasterGetWidth(visionRaster)  * 1024.0f / 640.0f;
	float rv = r * RsGlobal->MaximumHeight / RwRasterGetHeight(visionRaster) * 512.0f  / 448.0f;
	float uoff[4] = { -ru, ru, ru, -ru };
	float voff[4] = { -rv, -rv, rv, rv };

	float umin, vmin, umax, vmax;
	umin = 0.0f;
	vmin = 0.0f;
	umax = 2.0f;
	vmax = 2.0f;
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
	for(int i = 0; i < 4; i++){
		RwCameraEndUpdate(Camera);
		RwRasterPushContext(visionRaster);
		RwRasterRenderFast(RwCameraGetRaster(Camera), 0, 0);
		RwRasterPopContext();
		RwCameraBeginUpdate(Camera);

		postfxQuad[0].u = umin + uoff[i];
		postfxQuad[0].v = vmin + voff[i];
		postfxQuad[3].u = umax + uoff[i];
		postfxQuad[3].v = vmax + voff[i];
		postfxQuad[1].u = postfxQuad[3].u;
		postfxQuad[1].v = postfxQuad[0].v;
		postfxQuad[2].u = postfxQuad[0].u;
		postfxQuad[2].v = postfxQuad[3].v;
		CPostEffects::DrawQuad(0, 0, RwRasterGetWidth(visionRaster)*2, RwRasterGetHeight(visionRaster)*2,
		                       c1.red, c1.green, c1.blue, 0xFFu, visionRaster);
	}
	postfxQuad[0].u = 0.0f;
	postfxQuad[0].v = 0.0f;
	postfxQuad[1].u = 1.0f;
	postfxQuad[1].v = 0.0f;
	postfxQuad[2].u = 0.0f;
	postfxQuad[2].v = 1.0f;
	postfxQuad[3].u = 1.0f;
	postfxQuad[3].v = 1.0f;
	CPostEffects::ImmediateModeRenderStatesReStore();

	RwCameraEndUpdate(Camera);
	RwRasterPushContext(visionRaster);
	RwRasterRenderFast(RwCameraGetRaster(Camera), 0, 0);
	RwRasterPopContext();
	RwCameraBeginUpdate(Camera);
	CPostEffects::SetFilterMainColour_PS2(visionRaster, c2);
}

float &CTimer__ms_fTimeStep = *(float*)0xB7CB5C;

void
CPostEffects::NightVision_PS2(RwRGBA color)
{
	if(config->nightVision != 0){
		CPostEffects::NightVision(color);
		return;
	}

	if(CPostEffects::m_fNightVisionSwitchOnFXCount > 0.0f){
		CPostEffects::m_fNightVisionSwitchOnFXCount -= CTimer__ms_fTimeStep;
		if(CPostEffects::m_fNightVisionSwitchOnFXCount <= 0.0f)
			CPostEffects::m_fNightVisionSwitchOnFXCount = 0.0f;
		CPostEffects::ImmediateModeRenderStatesStore();
		CPostEffects::ImmediateModeRenderStatesSet();
		RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
		RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
		int n = CPostEffects::m_fNightVisionSwitchOnFXCount;
		while(n--){
		        float w = RwRasterGetWidth(visionRaster);
		        float h = RwRasterGetHeight(visionRaster);
		        CPostEffects::DrawQuad(0.0f, 0.0f, w, h, 8, 8, 8, 0xFF, visionRaster);
		}
		CPostEffects::ImmediateModeRenderStatesReStore();
	}

	RwCameraEndUpdate(Camera);
	RwRasterPushContext(visionRaster);
	RwRasterRenderFast(RwCameraGetRaster(Camera), 0, 0);
	RwRasterPopContext();
	RwCameraBeginUpdate(Camera);
	CPostEffects::SetFilterMainColour_PS2(visionRaster, color);
}


// VU style random number generator -- taken from pcsx2
uint R;
void vrinit(uint x){ R = 0x3F800000 | x & 0x007FFFFF; }
void vradvance(void){
	int x = (R >> 4) & 1;
	int y = (R >> 22) & 1;
	R <<= 1;
	R ^= x ^ y;
	R = (R&0x7fffff)|0x3f800000;
}
inline uint vrget(void){ return R; }
inline uint vrnext(void){ vradvance(); return R; }

void
CPostEffects::Grain_PS2(int strength, bool generate)
{
	if(config->grainFilter != 0){
		CPostEffects::Grain(strength, generate);
		return;
	}

	if(generate){
		RwUInt8 *pixels = RwRasterLock(grainRaster, 0, 1);
		vrinit(rand());
		int x = vrget();
		for(int i = 0; i < 64*64; i++){
			*pixels++ = x;
			*pixels++ = x;
			*pixels++ = x;
			*pixels++ = x & strength;
			x = vrnext();
		}
		RwRasterUnlock(grainRaster);
	}

	CPostEffects::ImmediateModeRenderStatesStore();
	CPostEffects::ImmediateModeRenderStatesSet();
	RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
//	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERNEAREST);
	float umax = 5.0f * RsGlobal->MaximumWidth/640.0f;
	float vmax = 7.0f * RsGlobal->MaximumHeight/448.0f;
	postfxQuad[0].u = 0.0f;
	postfxQuad[0].v = 0.0f;
	postfxQuad[1].u = umax;
	postfxQuad[1].v = 0.0f;
	postfxQuad[2].u = 0.0f;
	postfxQuad[2].v = vmax;
	postfxQuad[3].u = umax;
	postfxQuad[3].v = vmax;

	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)D3DBLEND_DESTCOLOR);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)D3DBLEND_SRCALPHA);

	// colors unused
	overrideIm2dPixelShader = grainPS;
	CPostEffects::DrawQuad(0.0, 0.0, RsGlobal->MaximumWidth, RsGlobal->MaximumHeight,
	                       0xFFu, 0xFFu, 0xFFu, 0xFF, grainRaster);
	overrideIm2dPixelShader = NULL;
	postfxQuad[0].u = 0.0f;
	postfxQuad[0].v = 0.0f;
	postfxQuad[1].u = 1.0f;
	postfxQuad[1].v = 0.0f;
	postfxQuad[2].u = 0.0f;
	postfxQuad[2].v = 1.0f;
	postfxQuad[3].u = 1.0f;
	postfxQuad[3].v = 1.0f;
	CPostEffects::ImmediateModeRenderStatesReStore();
}

struct Grade
{
	float r, g, b, a;
};
void interpolateColorcycle(Grade *red, Grade *green, Grade *blue);
int colorcycleInitialized = 0;

void
renderMobile(void)
{
	float rasterWidth = RwRasterGetWidth(CPostEffects::pRasterFrontBuffer);
	float rasterHeight = RwRasterGetHeight(CPostEffects::pRasterFrontBuffer);
	float halfU = 0.5 / rasterWidth;
	float halfV = 0.5 / rasterHeight;
	float uMax = RsGlobal->MaximumWidth / rasterWidth;
	float vMax = RsGlobal->MaximumHeight / rasterHeight;
	int i = 0;

	if(!colorcycleInitialized)
		loadColorcycle();

	RwCameraEndUpdate(Camera);
	RwRasterPushContext(CPostEffects::pRasterFrontBuffer);
	RwRasterRenderFast(RwCameraGetRaster(Camera), 0, 0);
	RwRasterPopContext();
	RwCameraBeginUpdate(Camera);

	quadVertices[i].x = 1.0f;
	quadVertices[i].y = -1.0f;
	quadVertices[i].z = 0.0f;
	quadVertices[i].rhw = 1.0f;
	quadVertices[i].u = uMax + halfU;
	quadVertices[i].v = vMax + halfV;
	i++;

	quadVertices[i].x = -1.0f;
	quadVertices[i].y = -1.0f;
	quadVertices[i].z = 0.0f;
	quadVertices[i].rhw = 1.0f;
	quadVertices[i].u = 0.0f + halfU;
	quadVertices[i].v = vMax + halfV;
	i++;

	quadVertices[i].x = -1.0f;
	quadVertices[i].y = 1.0f;
	quadVertices[i].z = 0.0f;
	quadVertices[i].rhw = 1.0f;
	quadVertices[i].u = 0.0f + halfU;
	quadVertices[i].v = 0.0f + halfV;
	i++;

	quadVertices[i].x = 1.0f;
	quadVertices[i].y = 1.0f;
	quadVertices[i].z = 0.0f;
	quadVertices[i].rhw = 1.0f;
	quadVertices[i].u = uMax + halfU;
	quadVertices[i].v = 0.0f + halfV;
	
	static RwTexture *tempTexture = NULL;
	if(tempTexture == NULL){
		tempTexture = RwTextureCreate(NULL);
		tempTexture->filterAddressing = 0x1102;
	}
	tempTexture->raster = CPostEffects::pRasterFrontBuffer;
	RwD3D9SetTexture(tempTexture, 0);

	RwD3D9SetVertexDeclaration(quadVertexDecl);

	RwD3D9SetVertexShader(postfxVS);
	RwD3D9SetPixelShader(gradingPS);

	Grade red, green, blue;
	interpolateColorcycle(&red, &green, &blue);
	float mult[3], add[3];

	mult[0] = red.r + red.g + red.b;
	mult[1] = green.r + green.g + blue.b;
	mult[2] = blue.r + blue.g + blue.b;
	add[0] = red.a;
	add[1] = green.a;
	add[2] = blue.a;
	RwD3D9SetPixelShaderConstant(0, &red, 1);
	RwD3D9SetPixelShaderConstant(1, &green, 1);
	RwD3D9SetPixelShaderConstant(2, &blue, 1);
	RwD3D9SetPixelShaderConstant(3, &mult, 1);
	RwD3D9SetPixelShaderConstant(4, &add, 1);

	RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
	RwD3D9SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	RwD3D9DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 6, 2, quadIndices, quadVertices, sizeof(QuadVertex));

	RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, 1);

	RwD3D9SetVertexShader(NULL);
	RwD3D9SetPixelShader(NULL);
}

WRAPPER char ReloadPlantConfig(void) { EAXJMP(0x5DD780); }

void
CPostEffects::ColourFilter_switch(RwRGBA rgb1, RwRGBA rgb2)
{
	{
		static bool keystate = false;
		if(GetAsyncKeyState(config->keys[0]) & 0x8000){
			if(!keystate){
				keystate = true;
				if(numConfigs == 0 || config == &configs[numConfigs-1])
					config = &configs[0];
				else
					config++;
				resetValues();
				//SetCloseFarAlphaDist(3.0, 1234.0); // second value overriden
			}
		}else
			keystate = false;
	}

	{
		static bool keystate = false;
		if(GetAsyncKeyState(config->keys[1]) & 0x8000){
			if(!keystate){
				keystate = true;
				if(numConfigs == 0)
					readIni(0);
				else
					for(int i = 1; i <= numConfigs; i++)
						readIni(i);
				resetValues();
				//SetCloseFarAlphaDist(3.0, 1234.0); // second value overriden
			}
		}else
			keystate = false;
	}

	vcsblurrgb = rgb2;
	switch(config->colorFilter){
	case 0:	CPostEffects::ColourFilter_Generic(rgb1, rgb2, colorFilterPS);
		break;
	case 1:	CPostEffects::ColourFilter(rgb1, rgb2);
		break;
	case 2:	renderMobile();
		break;
	case 3:	CPostEffects::ColourFilter_Generic(rgb1, rgb2, iiiTrailsPS);
		break;
	case 4:	CPostEffects::ColourFilter_Generic(rgb1, rgb2, vcTrailsPS);
		break;
	}

	//static int doramp = 0;
	//{
	//	static bool keystate = false;
	//	if(GetAsyncKeyState(VK_F4) & 0x8000){
	//		if(!keystate){
	//			doramp = !doramp;
	//			keystate = true;
	//		}
	//	}else
	//		keystate = false;
	//}
	//if(doramp)
	//	renderRamp();
}

void (*CPostEffects::Initialise_orig)(void);
void
CPostEffects::Initialise(void)
{
	Initialise_orig();
	InjectHook(0x7FB824, Im2dSetPixelShader_hook);

	CreateShaders();

	grainRaster = RwRasterCreate(64, 64, 32, 0x504);

	static const D3DVERTEXELEMENT9 vertexElements[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 16, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
		{ 0, 20, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		{ 0, 28, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
		D3DDECL_END()
	};
	RwD3D9CreateVertexDeclaration(vertexElements, &quadVertexDecl);

	static const D3DVERTEXELEMENT9 screenVertexElements[] =
	{
		{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
		{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};
	RwD3D9CreateVertexDeclaration(screenVertexElements, &screenVertexDecl);

	target1 = RwRasterCreate(4,
	                         4,
	                         CPostEffects::pRasterFrontBuffer->depth, 5);
	target2 = RwRasterCreate(4,
	                         4,
	                         CPostEffects::pRasterFrontBuffer->depth, 5);
	smallRect.x = 0;
	smallRect.y = 0;
	smallRect.w = Camera->frameBuffer->width/4;
	smallRect.h = Camera->frameBuffer->height/4;

////	reflTex = RwRasterCreate(128, 128, 0, 5);
//	reflTex = RwRasterCreate(CPostEffects::pRasterFrontBuffer->width, CPostEffects::pRasterFrontBuffer->height, 0, 5);
//	vcsRect.x = 0;
//	vcsRect.y = 0;
}






// Mobile stuff, partly taken from NTAuthority

short &clockSecond = *(short*)0xB70150;
byte &clockMinute = *(byte*)0xB70152;
byte &clockHour = *(byte*)0xB70153;
short &oldWeather = *(short*)0xC81320;
short &newWeather = *(short*)0xC8131C;
float &weatherInterp = *(float*)0xC8130C;

class CFileMgr
{
public:
	static void* OpenFile(const char* filename, const char* mode);

	static void  CloseFile(void* file);
};

class CFileLoader
{
public:
	static char* LoadLine(void* file);
};

WRAPPER void* CFileMgr::OpenFile(const char* filename, const char* mode) { EAXJMP(0x538900); }
WRAPPER void  CFileMgr::CloseFile(void* file) { EAXJMP(0x5389D0); }
WRAPPER char* CFileLoader::LoadLine(void* file) { EAXJMP(0x536F80); }


Grade redGrade[8][23];
Grade greenGrade[8][23];
Grade blueGrade[8][23];

float blerp[4];

int
houridx(int hour)
{
	switch(hour){
	default:
	case 0: case 1: case 2: case 3: case 4:
		return 0;
	case 5: return 1;
	case 6: return 2;
	case 7: case 8: case 9: case 10: case 11:
		return 3;
	case 12: case 13: case 14: case 15: case 16: case 17: case 18:
		return 4;
	case 19: return 5;
	case 20: case 21:
		return 6;
	case 22: case 23:
		return 7;
	}
}

void
interpolateGrade(Grade *out, Grade *a, Grade *b)
{
	out->r = a[oldWeather].r*blerp[0] +
	         b[oldWeather].r*blerp[1] +
	         a[newWeather].r*blerp[2] +
	         b[newWeather].r*blerp[3];
	out->g = a[oldWeather].g*blerp[0] +
	         b[oldWeather].g*blerp[1] +
	         a[newWeather].g*blerp[2] +
	         b[newWeather].g*blerp[3];
	out->b  = a[oldWeather].b*blerp[0] +
	          b[oldWeather].b*blerp[1] +
	          a[newWeather].b*blerp[2] +
	          b[newWeather].b*blerp[3];
	out->a = a[oldWeather].a*blerp[0] +
	         b[oldWeather].a*blerp[1] +
	         a[newWeather].a*blerp[2] +
	         b[newWeather].a*blerp[3];
}

void
interpolateColorcycle(Grade *red, Grade *green, Grade *blue)
{
	int nextHour = (clockHour+1)%24;
	float timeInterp = (clockSecond/60.0f + clockMinute)/60.0f;
	blerp[0] = (1.0f-timeInterp)*(1.0f-weatherInterp);
	blerp[1] = timeInterp*(1.0f-weatherInterp);
	blerp[2] = (1.0f-timeInterp)*weatherInterp;
	blerp[3] = timeInterp*weatherInterp;
	nextHour = houridx(nextHour);
	int thisHour = houridx(clockHour);
	//printf("%d %d %f %f %f %f\n", thisHour, nextHour, blerp[0], blerp[1], blerp[2], blerp[3]);
	interpolateGrade(red, redGrade[thisHour], redGrade[nextHour]);
	interpolateGrade(green, greenGrade[thisHour], greenGrade[nextHour]);
	interpolateGrade(blue, blueGrade[thisHour], blueGrade[nextHour]);
}

void
loadColorcycle(void)
{
	void *f = CFileMgr::OpenFile("data/colorcycle.dat", "r");
	char *line;
	for(int i = 0; i < 23; i++){
		for(int j = 0; j < 8; j++){
			line = CFileLoader::LoadLine(f);
			sscanf(line, "%f %f %f %f %f %f %f %f %f %f %f %f",
			       &redGrade[j][i].r, &redGrade[j][i].g,
			       &redGrade[j][i].b, &redGrade[j][i].a,
			       &greenGrade[j][i].r, &greenGrade[j][i].g,
			       &greenGrade[j][i].b, &greenGrade[j][i].a,
			       &blueGrade[j][i].r, &blueGrade[j][i].g,
			       &blueGrade[j][i].b, &blueGrade[j][i].a);
			float sum;
			sum = redGrade[j][i].r + redGrade[j][i].g + redGrade[j][i].b;
			if(sum > 1.7f)
				redGrade[j][i].a -= (sum - 1.7f)*0.13f;
			sum = greenGrade[j][i].r + greenGrade[j][i].g + greenGrade[j][i].b;
			if(sum > 1.7f)
				greenGrade[j][i].a -= (sum - 1.7f)*0.13f;
			sum = blueGrade[j][i].r + blueGrade[j][i].g + blueGrade[j][i].b;
			if(sum > 1.7f)
				blueGrade[j][i].a -= (sum - 1.7f)*0.13f;
			redGrade[j][i].r *= 0.67f;
			redGrade[j][i].g *= 0.67f;
			redGrade[j][i].b *= 0.67f;
			redGrade[j][i].a *= 0.67f;
			greenGrade[j][i].r *= 0.67f;
			greenGrade[j][i].g *= 0.67f;
			greenGrade[j][i].b *= 0.67f;
			greenGrade[j][i].a *= 0.67f;
			blueGrade[j][i].r *= 0.67f;
			blueGrade[j][i].g *= 0.67f;
			blueGrade[j][i].b *= 0.67f;
			blueGrade[j][i].a *= 0.67f;
			//printf("%f %f %f %f X %f %f %f %f X %f %f %f %f\n",
			//	redGrade[j][i].r, redGrade[j][i].g, redGrade[j][i].b, redGrade[j][i].a,
			//	greenGrade[j][i].r, greenGrade[j][i].g, greenGrade[j][i].b, greenGrade[j][i].a,
			//	blueGrade[j][i].r, blueGrade[j][i].g, blueGrade[j][i].b, blueGrade[j][i].a);
		}
	}
	CFileMgr::CloseFile(f);
	colorcycleInitialized = 1;
}
