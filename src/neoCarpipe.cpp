#include "skygfx.h"
#include "neo.h"
#include <DirectXMath.h>

enum {
	LOC_combined    = 0,
	LOC_world       = 4,
	LOC_tex         = 8,
	LOC_eye         = 12,
	LOC_directDir   = 13,
	LOC_ambient     = 15,
	LOC_matCol      = 16,
	LOC_directCol   = 17,
	LOC_lightDir    = 18,
	LOC_lightCol    = 24,

	LOC_directSpec  = 30,	// for carpipe
	LOC_reflProps   = 31,
	LOC_surfProps	= 32,
};


//#define DEBUGTEX

WRAPPER void CRenderer__RenderRoads(void) { EAXJMP(0x553A10); }
WRAPPER void CRenderer__RenderEverythingBarRoads(void) { EAXJMP(0x553AA0); }
WRAPPER void CRenderer__RenderFadingInEntities(void) { EAXJMP(0x5531E0); }
WRAPPER void CRenderer__RenderFadingInUnderwaterEntities(void) { EAXJMP(0x553220); }

short &skyTopRed = *(short*)0xB7C4C4;
short &skyTopGreen = *(short*)0xB7C4C6;
short &skyTopBlue = *(short*)0xB7C4C8;
short &skyBotRed = *(short*)0xB7C4CA;
short &skyBotGreen = *(short*)0xB7C4CC;
short &skyBotBlue = *(short*)0xB7C4CE;


InterpolatedFloat CarPipe::fresnel(0.4f);
InterpolatedFloat CarPipe::power(18.0f);
InterpolatedLight CarPipe::diffColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
InterpolatedLight CarPipe::specColor(Color(0.7f, 0.7f, 0.7f, 1.0f));
void *CarPipe::vertexShaderPass1;
void *CarPipe::vertexShaderPass2;
// reflection map
RwCamera *CarPipe::reflectionCam;
RwTexture *CarPipe::reflectionMask;
RwTexture *CarPipe::reflectionTex;
RwIm2DVertex CarPipe::screenQuad[4];
RwImVertexIndex CarPipe::screenindices[6] = { 0, 1, 2, 0, 2, 3 };

CarPipe carpipe;

void
neoCarPipeInit(void)
{
	ONCE;
	carpipe.Init();
	CarPipe::SetupEnvMap();
}

//
// Reflection map
//

int envMapSize;

void
CarPipe::SetupEnvMap(void)
{
	reflectionMask = RwTextureRead("CarReflectionMask", NULL);

	RwRaster *envFB = RwRasterCreate(envMapSize, envMapSize, 0, rwRASTERTYPECAMERATEXTURE);
	RwRaster *envZB = RwRasterCreate(envMapSize, envMapSize, 0, rwRASTERTYPEZBUFFER);
	reflectionCam = RwCameraCreate();
	RwCameraSetRaster(reflectionCam, envFB);
	RwCameraSetZRaster(reflectionCam, envZB);
	RwCameraSetFrame(reflectionCam, RwFrameCreate());
	RwCameraSetNearClipPlane(reflectionCam, 0.1f);
	RwCameraSetFarClipPlane(reflectionCam, 250.0f);
	RwV2d vw;
	vw.x = vw.y = 0.4f;
	RwCameraSetViewWindow(reflectionCam, &vw);
	RpWorldAddCamera(Scene.world, reflectionCam);

	reflectionTex = RwTextureCreate(envFB);
	RwTextureSetFilterMode(reflectionTex, rwFILTERLINEAR);

	MakeScreenQuad();
}

void
CarPipe::MakeQuadTexCoords(bool textureSpace)
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

void
CarPipe::MakeScreenQuad(void)
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

void
CarPipe::RenderReflectionScene(void)
{
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, 0);
	CRenderer__RenderRoads();
	CRenderer__RenderEverythingBarRoads();
	CRenderer__RenderFadingInEntities();
}

void
CarPipe::RenderEnvTex(void)
{
	RwCameraEndUpdate(Scene.camera);

	RwV2d oldvw, vw = { 2.0f, 2.0f };
	oldvw = reflectionCam->viewWindow;
	RwCameraSetViewWindow(reflectionCam, &vw);

	static RwMatrix *reflectionMatrix;
	if(reflectionMatrix == NULL){
		reflectionMatrix = RwMatrixCreate();
		reflectionMatrix->right.x = -1.0f;
		reflectionMatrix->right.y = 0.0f;
		reflectionMatrix->right.z = 0.0f;
		reflectionMatrix->up.x = 0.0f;
		reflectionMatrix->up.y = -1.0f;
		reflectionMatrix->up.z = 0.0f;
		reflectionMatrix->at.x = 0.0f;
		reflectionMatrix->at.y = 0.0f;
		reflectionMatrix->at.z = 1.0f;
	}
	RwMatrix *cammatrix = RwFrameGetMatrix(RwCameraGetFrame(Scene.camera));
	reflectionMatrix->pos = cammatrix->pos;
	RwMatrixUpdate(reflectionMatrix);
	RwFrameTransform(RwCameraGetFrame(reflectionCam), reflectionMatrix, rwCOMBINEREPLACE);
	RwRGBA color = { skyBotRed, skyBotGreen, skyBotBlue, 255 };
	// blend a bit of white into the sky color, otherwise it tends to be very blue
	color.red = color.red*0.6f + 255*0.4f;
	color.green = color.green*0.6f + 255*0.4f;
	color.blue = color.blue*0.6f + 255*0.4f;
	RwCameraClear(reflectionCam, &color, rwCAMERACLEARIMAGE | rwCAMERACLEARZ);

	RwCameraBeginUpdate(reflectionCam);
	RwCamera *savedcam = Scene.camera;
	Scene.camera = reflectionCam;	// they do some begin/end updates with this in the called functions :/
	RenderReflectionScene();
	Scene.camera = savedcam;

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDZERO);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDSRCCOLOR);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, reflectionMask->raster);
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, screenQuad, 4, screenindices, 6);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, 0);

	RwCameraEndUpdate(reflectionCam);
	RwCameraSetViewWindow(reflectionCam, &oldvw);

	RwCameraBeginUpdate(Scene.camera);
#ifdef DEBUGTEX
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, reflectionTex->raster);
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, screenQuad, 4, screenindices, 6);
#endif
}

//
//
//

CarPipe::CarPipe(void)
{
//	rwPipeline = NULL;
}

void
CarPipe::Init(void)
{
//	CreateRwPipeline();
//	SetRenderCallback(RenderCallback);
	CreateShaders();
	LoadTweakingTable();
	// some camera begin/end stuff in barroads with the rw cam
//	Nop(0x553C68, 0x553C9F-0x553C68);
//	Nop(0x553CA4, 3);
//	Nop(0x553CCA, 0x553CF4-0x553CCA);
}

void
CarPipe::CreateShaders(void)
{
	HRSRC resource;
	RwUInt32 *shader;
	resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_NEOVEHICLEONEVS), RT_RCDATA);
	shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreateVertexShader(shader, &vertexShaderPass1);
	assert(vertexShaderPass1);
	FreeResource(shader);

	resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_NEOVEHICLETWOVS), RT_RCDATA);
	shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreateVertexShader(shader, &vertexShaderPass2);
	assert(vertexShaderPass2);
	FreeResource(shader);
}

void
CarPipe::LoadTweakingTable(void)
{
	char *path;
	FILE *dat;
	path = getpath("neo\\carTweakingTable.dat");
	if(path == NULL)
		return;
	dat = fopen(path, "r");
	neoReadWeatherTimeBlock(dat, &fresnel);
	neoReadWeatherTimeBlock(dat, &power);
	neoReadWeatherTimeBlock(dat, &diffColor);
	neoReadWeatherTimeBlock(dat, &specColor);
	fclose(dat);
	iCanHasNeoCar = 1;
}

//
// Rendering
//

void
UploadLightColorWithSpecular(RpLight *light, int loc)
{
	float c[4];
	if(RpLightGetFlags(light) & rpLIGHTLIGHTATOMICS){
		Color s = CarPipe::specColor.Get();
		c[0] = light->color.red * s.a;
		c[1] = light->color.green * s.a;
		c[2] = light->color.blue * s.a;
		c[3] = 1.0f;
		RwD3D9SetVertexShaderConstant(loc, (void*)c, 1);
	}else
		pipeUploadZero(loc);
}

void
CarPipe::ShaderSetup(RpAtomic *atomic)
{
	float worldMat[16], combined[16];
	DirectX::XMMATRIX texMat;
	RwCamera *cam = (RwCamera*)RWSRCGLOBAL(curCamera);

	pipeGetComposedTransformMatrix(atomic, combined);
	RwToD3DMatrix(&worldMat, RwFrameGetLTM(RpAtomicGetFrame(atomic)));
	RwD3D9SetVertexShaderConstant(LOC_combined, (void*)&combined, 4);
	RwD3D9SetVertexShaderConstant(LOC_world, (void*)&worldMat, 4);
	texMat = DirectX::XMMatrixIdentity();
	RwD3D9SetVertexShaderConstant(LOC_tex, (void*)&texMat, 4);

	RwMatrix *camfrm = RwFrameGetLTM(RwCameraGetFrame(cam));
	RwD3D9SetVertexShaderConstant(LOC_eye, (void*)RwMatrixGetPos(camfrm), 1);

	UploadLightColorWithSpecular(pAmbient, LOC_ambient);
	UploadLightColorWithSpecular(pDirect, LOC_directCol);
	pipeUploadLightDirection(pDirect, LOC_directDir);
	for(int i = 0 ; i < 6; i++)
		if(i < NumExtraDirLightsInWorld && RpLightGetType(pExtraDirectionals[i]) == rpLIGHTDIRECTIONAL){
			pipeUploadLightDirection(pExtraDirectionals[i], LOC_lightDir+i);
			UploadLightColorWithSpecular(pExtraDirectionals[i], LOC_lightCol+i);
		}else{
			pipeUploadZero(LOC_lightDir+i);
			pipeUploadZero(LOC_lightCol+i);
		}

	Color spec = specColor.Get();
	spec.r *= spec.a;
	spec.g *= spec.a;
	spec.b *= spec.a;
	RwD3D9SetVertexShaderConstant(LOC_directSpec, (void*)&spec, 1);
}

void
CarPipe::DiffusePass(RxD3D9ResEntryHeader *header, RpAtomic *atomic)
{
	RxD3D9InstanceData *inst = (RxD3D9InstanceData*)&header[1];
	CustomEnvMapPipeMaterialData *envData;
	int noRefl;

	RwD3D9SetTexture(reflectionTex, 1);
	RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_LERP);
	RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
	RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	RwD3D9SetTextureStageState(1, D3DTSS_COLORARG0, D3DTA_SPECULAR);
	RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_CURRENT);

	RwD3D9SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(2, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(3, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(3, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);

	RwD3D9SetVertexShader(vertexShaderPass1);

	noRefl = CVisibilityPlugins__GetAtomicId(atomic) & 0x6000;

	for(uint i = 0; i < header->numMeshes; i++){
		RpMaterial *material = inst->material;
		RwD3D9SetTexture(material->texture, 0);
		// have to set these after the texture, RW sets texture stage states automatically
		// ^ still true in d3d9? i think not but can't be bothered to find out
		RwD3D9SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
		RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
		RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
		RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);

		envData = *RWPLUGINOFFSET(CustomEnvMapPipeMaterialData*, material, CCustomCarEnvMapPipeline__ms_envMapPluginOffset);

		RwUInt32 materialFlags = *(RwUInt32*)&material->surfaceProps.specular;
		bool hasEnv  = !!(materialFlags & 3);
		int matfx = RpMatFXMaterialGetEffects(material);
		if(matfx != rpMATFXEFFECTENVMAP)
			hasEnv = false;

		Color c = diffColor.Get();
		Color diff(c.r*c.a, c.g*c.a, c.b*c.a, 1.0f-c.a);
		RwRGBAReal mat;
		RwRGBARealFromRwRGBA(&mat, &inst->material->color);
		mat.red = mat.red*diff.a + diff.r;
		mat.green = mat.green*diff.a + diff.g;
		mat.blue = mat.blue*diff.a + diff.b;
		RwD3D9SetVertexShaderConstant(LOC_matCol, (void*)&mat, 1);

		RwSurfaceProperties surfprops = material->surfaceProps;
		// if ambient light is too dark reflections don't look too good, so bump it
		if(surfprops.ambient > 0.1f && surfprops.ambient < 0.8f)
			surfprops.ambient = max(surfprops.ambient, 0.8f);
		RwD3D9SetVertexShaderConstant(LOC_surfProps, &surfprops, 1);

		float reflProps[4];
		reflProps[0] = hasEnv && !noRefl ? envData->shininess/255.0f * 8.0f * config->neoShininessMult : 0.0f;
		reflProps[1] = fresnel.Get();
		reflProps[3] = power.Get();
		RwD3D9SetVertexShaderConstant(LOC_reflProps, (void*)reflProps, 1);

		D3D9RenderDual(config->dualPassVehicle, header, inst);
		inst++;
	}
}

void
CarPipe::SpecularPass(RxD3D9ResEntryHeader *header, RpAtomic *atomic)
{
	RwUInt32 src, dst, fog, zwrite, alphatest;
	RwBool lighting;
	RxD3D9InstanceData *inst = (RxD3D9InstanceData*)&header[1];
	CustomSpecMapPipeMaterialData *specData;
	int noRefl;

	RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &zwrite);
	RwRenderStateGet(rwRENDERSTATEFOGENABLE, &fog);
	RwRenderStateGet(rwRENDERSTATESRCBLEND, &src);
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &dst);
	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &alphatest);

	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONALWAYS);
	RwD3D9SetTexture(NULL, 0);
	RwD3D9SetTexture(NULL, 1);
	RwD3D9SetTexture(NULL, 2);
	RwD3D9SetTexture(NULL, 3);
	RwD3D9SetVertexShader(vertexShaderPass2);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);

	RwD3D9GetRenderState(D3DRS_LIGHTING, &lighting);
	noRefl = CVisibilityPlugins__GetAtomicId(atomic) & 0x6000;

	float lightmult = 1.85f * CCustomCarEnvMapPipeline__m_EnvMapLightingMult;
	for(uint i = 0; i < header->numMeshes; i++){
		RwUInt32 materialFlags = *(RwUInt32*)&inst->material->surfaceProps.specular;
		bool hasSpec  = !((materialFlags & 4) == 0 || !lighting);
		int matfx = RpMatFXMaterialGetEffects(inst->material);
		if(matfx != rpMATFXEFFECTENVMAP)
			hasSpec = false;

		specData = *RWPLUGINOFFSET(CustomSpecMapPipeMaterialData*, inst->material, CCustomCarEnvMapPipeline__ms_specularMapPluginOffset);
		if(hasSpec && !noRefl){
			RwSurfaceProperties surfprops;
			surfprops.specular = specData->specularity*5.0f*config->neoSpecularityMult*lightmult;
			if(surfprops.specular > 1.0f) surfprops.specular = 1.0f;
			RwD3D9SetVertexShaderConstant(LOC_surfProps, &surfprops, 1);
			D3D9Render(header, inst);
		}
		inst++;
	}
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)zwrite);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)fog);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)src);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)dst);
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)alphatest);
}

void
CarPipe::RenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
//		{
//			static bool keystate = false;
//			if(GetAsyncKeyState(VK_F7) & 0x8000){
//				if(!keystate){
//					keystate = true;
//					LoadTweakingTable();
//				}
//			}else
//				keystate = false;
//		}
	if(!iCanHasNeoCar)
		return;

	RxD3D9ResEntryHeader *header = (RxD3D9ResEntryHeader*)&repEntry[1];
	ShaderSetup((RpAtomic*)object);

	if(header->indexBuffer != NULL)
		RwD3D9SetIndices(header->indexBuffer);
	_rwD3D9SetStreams(header->vertexStream, header->useOffsets);
	RwD3D9SetVertexDeclaration(header->vertexDeclaration);

	RwD3D9SetPixelShader(NULL);

	DiffusePass(header, (RpAtomic*)object);
	SpecularPass(header, (RpAtomic*)object);
	RwD3D9SetTexture(NULL, 1);
	RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D9SetTexture(NULL, 2);
	RwD3D9SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(2, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
}
