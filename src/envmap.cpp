#include "skygfx.h"

#ifdef DEBUG
//#define DEBUGENVTEX
#endif


WRAPPER void DeActivateDirectional(void) { EAXJMP(0x735C70); }
WRAPPER void SetAmbientColours(void) { EAXJMP(0x735D30); }

// We'll use these for all env maps now
RwCamera *reflectionCam;
RwRaster *envFB, *envZB;
RwTexture *reflectionTex;

/* Create envmap rasters as we need them and attach them to cam */
void
MakeEnvmapRasters(void)
{
	if(envFB && envFB->width == config->envMapSize)
		return;
	if(envFB) RwRasterDestroy(envFB);
	if(envZB) RwRasterDestroy(envZB);
	envFB = RwRasterCreate(config->envMapSize, config->envMapSize, 0, rwRASTERTYPECAMERATEXTURE);
	envZB = RwRasterCreate(config->envMapSize, config->envMapSize, 0, rwRASTERTYPEZBUFFER);
	RwCameraSetRaster(reflectionCam, envFB);
	RwCameraSetZRaster(reflectionCam, envZB);
	RwTextureSetRaster(reflectionTex, envFB);
}

void
MakeEnvmapCam(void)
{
	reflectionCam = RwCameraCreate();
	RwCameraSetFrame(reflectionCam, RwFrameCreate());
	RwCameraSetNearClipPlane(reflectionCam, 0.1f);
	RwCameraSetFarClipPlane(reflectionCam, 250.0f);
	RwV2d vw;
	vw.x = vw.y = 0.4f;
	RwCameraSetViewWindow(reflectionCam, &vw);
	RpWorldAddCamera(Scene.world, reflectionCam);
}

#ifdef DEBUGENVTEX
static RwIm2DVertex screenQuad[4];
static RwImVertexIndex screenindices[6] = { 0, 1, 2, 0, 2, 3 };

static void
MakeQuadTexCoords(bool textureSpace)
{
	float minU, minV, maxU, maxV;
	if(textureSpace){
		minU = minV = 0.0f;
		maxU = maxV = 1.0f;
	}else{
		assert(0 && "not implemented");
	}
	screenQuad[0].u = minU;
	screenQuad[0].v = minV;
	screenQuad[1].u = minU;
	screenQuad[1].v = maxV;
	screenQuad[2].u = maxU;
	screenQuad[2].v = maxV;
	screenQuad[3].u = maxU;
	screenQuad[3].v = minV;
}

static void
MakeScreenQuad(void)
{
	int width = reflectionTex->raster->width;
	int height = reflectionTex->raster->height;
	screenQuad[0].x = 0.0f;
	screenQuad[0].y = 0.0f;
	screenQuad[0].z = RwIm2DGetNearScreenZ();
	screenQuad[0].rhw = 1.0f / reflectionCam->nearPlane;
	screenQuad[0].emissiveColor = 0xFFFFFFFF;
	screenQuad[1].x = 0.0f;
	screenQuad[1].y = height;
	screenQuad[1].z = screenQuad[0].z;
	screenQuad[1].rhw = screenQuad[0].rhw;
	screenQuad[1].emissiveColor = 0xFFFFFFFF;
	screenQuad[2].x = width;
	screenQuad[2].y = height;
	screenQuad[2].z = screenQuad[0].z;
	screenQuad[2].rhw = screenQuad[0].rhw;
	screenQuad[2].emissiveColor = 0xFFFFFFFF;
	screenQuad[3].x = width;
	screenQuad[3].y = 0;
	screenQuad[3].z = screenQuad[0].z;
	screenQuad[3].rhw = screenQuad[0].rhw;
	screenQuad[3].emissiveColor = 0xFFFFFFFF;
	MakeQuadTexCoords(true);
}

#endif

class CAtomicModelInfo : public CBaseModelInfo
{
public:
	uint16 GetWetRoadReflection(void) { return m_flags & (1<<8); }
	uint16 GetIsPlantFriendly(void) { return m_flags & (1<<9); }
	uint16 GetDontCollideWithFlyer(void) { return m_flags & (1<<10); }
	// more...
};


WRAPPER void CEntity::GetBoundCentre(CVector *v) { EAXJMP(0x534290); }
WRAPPER bool CEntity::GetIsOnScreen_orig(void) { EAXJMP(0x534540); }

// TODO: FLAify this?
CBaseModelInfo **CModelInfo__ms_modelInfoPtrs = (CBaseModelInfo**)0xA9B0C8;
CBaseModelInfo*
GetModelInfo(CEntity *e)
{
	return CModelInfo__ms_modelInfoPtrs[e->m_nModelIndex];
}

class CRenderer
{
public:
	static CVector &ms_vecCameraPosition;
	static CEntity **&ms_aVisibleEntityPtrs;
	static CEntity **&ms_aVisibleLodPtrs;
	static int &ms_nNoOfVisibleEntities;
	static int &ms_nNoOfVisibleLods;

	static void RenderOneRoad(CEntity *e);
	static void RenderOneNonRoad(CEntity *e);
	static void RenderRoadsAndBuildings(void);
	static void RenderFadingInBuildings(void);

	static void RenderRoads(void);
	static void RenderEverythingBarRoads(void);
	static void RenderFadingInEntities(void);
	static void RenderFadingInUnderwaterEntities(void);
};

CVector &CRenderer::ms_vecCameraPosition = *(CVector*)0xB76870;
CEntity **&CRenderer::ms_aVisibleEntityPtrs = *(CEntity***)(0x553526 + 3); //0xB75898;	// [1000]
CEntity **&CRenderer::ms_aVisibleLodPtrs = *(CEntity***)(0x5534F2 + 3); //0xB748F8;		// [1000]
int &CRenderer::ms_nNoOfVisibleEntities = *(int*)0xB76844;
int &CRenderer::ms_nNoOfVisibleLods = *(int*)0xB76840;

WRAPPER void CRenderer::RenderOneRoad(CEntity *e) { EAXJMP(0x553230); }
WRAPPER void CRenderer::RenderOneNonRoad(CEntity *e) { EAXJMP(0x553260); }
WRAPPER void CRenderer::RenderRoads(void) { EAXJMP(0x553A10); }
WRAPPER void CRenderer::RenderEverythingBarRoads(void) { EAXJMP(0x553AA0); }
WRAPPER void CRenderer::RenderFadingInEntities(void) { EAXJMP(0x5531E0); }
WRAPPER void CRenderer::RenderFadingInUnderwaterEntities(void) { EAXJMP(0x553220); }


CLinkList<CVisibilityPlugins::AlphaObjectInfo> &CVisibilityPlugins::m_alphaList = *(CLinkList<CVisibilityPlugins::AlphaObjectInfo>*)0xC88070;
CLinkList<CVisibilityPlugins::AlphaObjectInfo> &CVisibilityPlugins::m_alphaBoatAtomicList = *(CLinkList<CVisibilityPlugins::AlphaObjectInfo>*)0xC880C8;
CLinkList<CVisibilityPlugins::AlphaObjectInfo> &CVisibilityPlugins::m_alphaEntityList = *(CLinkList<CVisibilityPlugins::AlphaObjectInfo>*)0xC88120;
CLinkList<CVisibilityPlugins::AlphaObjectInfo> &CVisibilityPlugins::m_alphaUnderwaterEntityList = *(CLinkList<CVisibilityPlugins::AlphaObjectInfo>*)0xC88178;
CLinkList<CVisibilityPlugins::AlphaObjectInfo> &CVisibilityPlugins::m_alphaReallyDrawLastList = *(CLinkList<CVisibilityPlugins::AlphaObjectInfo>*)0xC881D0;
WRAPPER uint16 CVisibilityPlugins::GetUserValue(RpAtomic *atm) { EAXJMP(0x7323A0); }



inline float sq(float f) { return f*f; }
float sphereRadius;

/* This is unfortunately not enough to render sphere maps.
 * The game has more logic to cull objects, but I don't quite know where.
 * In the worst case whole sectors will be culled. */
bool
CEntity::GetIsOnScreen(void)
{
	if(sphereRadius != 0.0f){
		float r;
		CVector c;
		GetBoundCentre(&c);
		r = GetBoundRadius();
		return sq(r) + sq(sphereRadius) >
			sq(reflectionCamPos.x - c.x) +
			sq(reflectionCamPos.y - c.y) +
			sq(reflectionCamPos.z - c.z);
		return false;
	}
	return GetIsOnScreen_orig();
}

//bool
//CEntity::IsEntityOccluded(void)
//{
//	return false;
//}

void
CVisibilityPlugins::RenderFadingBuildings(void)
{
	for(auto i = m_alphaEntityList.m_lnListTail.m_pPrev; i != &m_alphaEntityList.m_lnListHead; i = i->m_pPrev)
		if(((CEntity*)i->V().pObject)->nType == ENTITY_TYPE_BUILDING)
			i->V().callback(i->V().pObject, i->V().fCompareValue);
}


void
CRenderer::RenderRoadsAndBuildings(void)
{
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)TRUE);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
	RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLBACK);
	if(CGame__currArea == 0)
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)140);
	for(int i = 0; i < ms_nNoOfVisibleEntities; i++){
		CEntity *e = ms_aVisibleEntityPtrs[i];
		int type = e->nType;
		if(type == ENTITY_TYPE_BUILDING){
			if(((CAtomicModelInfo*)GetModelInfo(e))->GetWetRoadReflection())
				RenderOneRoad(e);
			else
				RenderOneNonRoad(e);
		}
	}
	// SA does something with the camera's z values here but that has no effect on d3d
	for(int i = 0; i < ms_nNoOfVisibleLods; i++)
		RenderOneNonRoad(ms_aVisibleLodPtrs[i]);
}

void
CRenderer::RenderFadingInBuildings(void)
{
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)TRUE);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
	RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLBACK);
	DeActivateDirectional();
	SetAmbientColours();
	CVisibilityPlugins::RenderFadingBuildings();
}



void
RenderReflectionScene(void)
{
	if(CCutsceneMgr__ms_running)
		reflectionCamPos = TheCamera.GetPosition();
	else
		reflectionCamPos = FindPlayerPed(-1)->GetPosition();

	DefinedState();
	/* We do have fog for the sphere map but we have to calculate it ourselves  */
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, 0);

	// Render the opaque world
	CRenderer::RenderRoadsAndBuildings();
	// and the transparent world
	CRenderer::RenderFadingInBuildings();
}

RwTexture **gpCoronaTexture = (RwTexture**)0xC3E000;

WRAPPER void CClouds__RenderSkyPolys(void) { EAXJMP(0x714650); }

static RwIm2DVertex coronaVerts[4*4];
static RwImVertexIndex coronaIndices[6*4];
static int numCoronaVerts, numCoronaIndices;

static void
AddCorona(float x, float y, float sz)
{
	float nearz, recipz;
	RwIm2DVertex *v;
	nearz = RwIm2DGetNearScreenZ();
	recipz = 1.0f / RwCameraGetNearClipPlane((RwCamera*)RWSRCGLOBAL(curCamera));

	v = &coronaVerts[numCoronaVerts];
	RwIm2DVertexSetScreenX(&v[0], x);
	RwIm2DVertexSetScreenY(&v[0], y);
	RwIm2DVertexSetScreenZ(&v[0], nearz);
	RwIm2DVertexSetRecipCameraZ(&v[0], recipz);
	RwIm2DVertexSetU(&v[0], 0.0f, recipz);
	RwIm2DVertexSetV(&v[0], 0.0f, recipz);
	RwIm2DVertexSetIntRGBA(&v[0], 0xFF, 0xFF, 0xFF, 0xFF);

	RwIm2DVertexSetScreenX(&v[1], x);
	RwIm2DVertexSetScreenY(&v[1], y + sz);
	RwIm2DVertexSetScreenZ(&v[1], nearz);
	RwIm2DVertexSetRecipCameraZ(&v[1], recipz);
	RwIm2DVertexSetU(&v[1], 0.0f, recipz);
	RwIm2DVertexSetV(&v[1], 1.0f, recipz);
	RwIm2DVertexSetIntRGBA(&v[1], 0xFF, 0xFF, 0xFF, 0xFF);

	RwIm2DVertexSetScreenX(&v[2], x + sz);
	RwIm2DVertexSetScreenY(&v[2], y + sz);
	RwIm2DVertexSetScreenZ(&v[2], nearz);
	RwIm2DVertexSetRecipCameraZ(&v[2], recipz);
	RwIm2DVertexSetU(&v[2], 1.0f, recipz);
	RwIm2DVertexSetV(&v[2], 1.0f, recipz);
	RwIm2DVertexSetIntRGBA(&v[2], 0xFF, 0xFF, 0xFF, 0xFF);

	RwIm2DVertexSetScreenX(&v[3], x + sz);
	RwIm2DVertexSetScreenY(&v[3], y);
	RwIm2DVertexSetScreenZ(&v[3], nearz);
	RwIm2DVertexSetRecipCameraZ(&v[3], recipz);
	RwIm2DVertexSetU(&v[3], 1.0f, recipz);
	RwIm2DVertexSetV(&v[3], 0.0f, recipz);
	RwIm2DVertexSetIntRGBA(&v[3], 0xFF, 0xFF, 0xFF, 0xFF);


	coronaIndices[numCoronaIndices++] = numCoronaVerts;
	coronaIndices[numCoronaIndices++] = numCoronaVerts + 1;
	coronaIndices[numCoronaIndices++] = numCoronaVerts + 2;
	coronaIndices[numCoronaIndices++] = numCoronaVerts;
	coronaIndices[numCoronaIndices++] = numCoronaVerts + 2;
	coronaIndices[numCoronaIndices++] = numCoronaVerts + 3;
	numCoronaVerts += 4;
}

void
DrawEnvMapCoronas(RwV3d at)
{
	const float BIG = 89.0f * reflectionTex->raster->width/128.0f;
	const float SMALL = 38.0f * reflectionTex->raster->height/128.0f;

	float x;
	numCoronaVerts = 0;
	numCoronaIndices = 0;
	x = CGeneral::GetATanOfXY(-at.y, at.x)/(2*M_PI) - 1.0f;
	x *= BIG+SMALL;
	AddCorona(x, 12.0f, SMALL);	x += SMALL;
	AddCorona(x, 0.0f, BIG);	x += BIG;
	AddCorona(x, 12.0f, SMALL);	x += SMALL;
	AddCorona(x, 0.0f, BIG);	x += BIG;

	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, gpCoronaTexture[0]->raster);
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, coronaVerts, numCoronaVerts, coronaIndices, numCoronaIndices);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);
}

void
DrawDebugEnvMap(void)
{
	if(GetAsyncKeyState(VK_F3) & 0x8000)
		return;
#ifdef DEBUGENVTEX
	MakeScreenQuad();
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, reflectionTex->raster);
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, screenQuad, 4, screenindices, 6);
#endif
}

void
RenderReflectionMap_leeds(void)
{
	RwCamera *cam = Scene.camera;
	RwCameraEndUpdate(cam);

	MakeEnvmapRasters();

	RwCameraSetViewWindow(reflectionCam, &cam->viewWindow);

	RwFrameTransform(RwCameraGetFrame(reflectionCam), &RwCameraGetFrame(cam)->ltm, rwCOMBINEREPLACE);

	RwRGBA color = { skyTopRed, skyTopGreen, skyTopBlue, 255 };
//	RwRGBA color = { skyBotRed, skyBotGreen, skyBotBlue, 255 };

	// blend a bit of white into the sky color, otherwise it tends to be very blue
//	color.red = color.red*0.6f + 255*0.4f;
//	color.green = color.green*0.6f + 255*0.4f;
//	color.blue = color.blue*0.6f + 255*0.4f;
	RwCameraClear(reflectionCam, &color, rwCAMERACLEARIMAGE | rwCAMERACLEARZ);

	RwCameraBeginUpdate(reflectionCam);
	RwCamera *savedcam = Scene.camera;
	Scene.camera = reflectionCam;	// they do some begin/end updates with this in the called functions :/
	CClouds__RenderSkyPolys();
	RenderReflectionScene();
	DrawEnvMapCoronas(RwFrameGetLTM(RwCameraGetFrame(reflectionCam))->at);
	Scene.camera = savedcam;
	RwCameraEndUpdate(reflectionCam);

	RwCameraBeginUpdate(cam);
}

void (*CRenderer__ConstructRenderList)(void);

int &CMirrors__TypeOfMirror = *(int*)0xC7C724;
bool &bFudgeNow = *(bool*)0xC7C72A;

RwRGBAReal spheremapfog;
void
RenderSphereReflections(void)
{
	if(iCanHasbuildingPipe && (config->vehiclePipe == CAR_MOBILE || config->vehiclePipe == CAR_ENV)){
		MakeEnvmapRasters();

		RwCamera *cam = Scene.camera;
		float farplane, fog;
		RwRaster *fb, *zb;

		sphereRadius = 60.0f;	// TODO? ini option?

		fb = RwCameraGetRaster(cam);
		zb = RwCameraGetZRaster(cam);
		farplane = RwCameraGetFarClipPlane(cam);
		fog = RwCameraGetFogDistance(cam);
		RwCameraSetRaster(cam, RwCameraGetRaster(reflectionCam));
		RwCameraSetZRaster(cam, RwCameraGetZRaster(reflectionCam));
		RwCameraSetFarClipPlane(cam, sphereRadius);
		RwCameraSetFogDistance(cam, sphereRadius*0.75f);

		RwRGBA color;
		if(config->vehiclePipe == CAR_ENV){
			// like Neo
			static RwRGBA skyBot = { skyBotRed, skyBotGreen, skyBotBlue, 255 };
			color = skyBot;
			// blend a bit of white into the sky color, otherwise it tends to be very blue
			color.red = color.red*0.6f + 255*0.4f;
			color.green = color.green*0.6f + 255*0.4f;
			color.blue = color.blue*0.6f + 255*0.4f;
		}else{
			static RwRGBA skyTop = { skyTopRed, skyTopGreen, skyTopBlue, 255 };
			color = skyTop;
			if(color.red < 64) color.red = 64;
			if(color.green < 64) color.green = 64;
			if(color.blue < 64) color.blue = 64;
		}
		RwCameraClear(cam, &color, rwCAMERACLEARIMAGE | rwCAMERACLEARZ);
		RwRGBARealFromRwRGBA(&spheremapfog, &color);

		bFudgeNow = true;	/* Don't know if this actually fudges, but it might help a bit */
		gRenderingSpheremap = true;
		CRenderer__ConstructRenderList();
		RwCameraBeginUpdate(cam);
		RenderReflectionScene();
		RwCameraEndUpdate(cam);
		gRenderingSpheremap = false;
		bFudgeNow = false;
		sphereRadius = 0.0f;

		RwCameraSetRaster(cam, fb);
		RwCameraSetZRaster(cam, zb);
		RwCameraSetFarClipPlane(cam, farplane);
		RwCameraSetFogDistance(cam, fog);
	}
	CRenderer__ConstructRenderList();
}

void
envmaphooks(void)
{
	InjectHook(0x536BCE, &CEntity::GetIsOnScreen);	// CEntity::IsVisible
	InjectHook(0x5540AD, &CEntity::GetIsOnScreen);	// CRenderer::SetupMapEntityVisibility
	InjectHook(0x554174, &CEntity::GetIsOnScreen);	// CRenderer::SetupMapEntityVisibility
	InjectHook(0x5543C2, &CEntity::GetIsOnScreen);	// CRenderer::SetupEntityVisibility
	InjectHook(0x554504, &CEntity::GetIsOnScreen);	// CRenderer::SetupEntityVisibility
	InjectHook(0x55495E, &CEntity::GetIsOnScreen);	// CRenderer::ScanSectorList
	InjectHook(0x409870, &CEntity::GetIsOnScreen);	// CStreaming::DeleteLeastUsedEntityRwObject
//	InjectHook(0x71FAE0, &CEntity::IsEntityOccluded, PATCH_JUMP);
//	InjectHook(0x420C40, madness, PATCH_JUMP);
	InterceptCall(&CRenderer__ConstructRenderList, RenderSphereReflections, 0x53E9F9);
}
