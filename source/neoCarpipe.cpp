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
	LOC_lightCol    = 22,

	LOC_directSpec  = 26,	// for carpipe
	LOC_reflProps   = 27,

	LOC_gloss2	= 28,
	LOC_gloss3	= 29,

	LOC_rampStart   = 36,	// for rim pipe
	LOC_rampEnd     = 37,
	LOC_rim         = 38,
	LOC_viewVec     = 41,
};


//#define DEBUGTEX

WRAPPER void CRenderer__RenderEverythingBarRoads(void) { EAXJMP(0x553AA0); }
WRAPPER void CRenderer__RenderFadingInEntities(void) { EAXJMP(0x553220); }

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

static uint32_t RenderScene_A;
WRAPPER void RenderScene(void) { VARJMP(RenderScene_A); }

void
RenderScene_hook(void)
{
	RenderScene();
	CarPipe::RenderEnvTex();
}

void
neoCarPipeInit(void)
{
	ONCE;
	carpipe.Init();
	CarPipe::SetupEnvMap();
	InterceptCall(&RenderScene_A, RenderScene_hook, 0x53EABF);
}

//
// Reflection map
//

int envMapSize;
RpWorld *&scene = *(RpWorld**)0xC17038;

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
	RpWorldAddCamera(scene, reflectionCam);

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
	CRenderer__RenderEverythingBarRoads();
	CRenderer__RenderFadingInEntities();
}

RwCamera *&gtacam = *(RwCamera**)0xC1703C;

void
CarPipe::RenderEnvTex(void)
{
	RwCameraEndUpdate(gtacam);

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
	RwMatrix *cammatrix = RwFrameGetMatrix(RwCameraGetFrame(gtacam));
	reflectionMatrix->pos = cammatrix->pos;
	RwMatrixUpdate(reflectionMatrix);
	RwFrameTransform(RwCameraGetFrame(reflectionCam), reflectionMatrix, rwCOMBINEREPLACE);
	RwRGBA color = { skyBotRed, skyBotGreen, skyBotBlue, 255 };
	RwCameraClear(reflectionCam, &color, rwCAMERACLEARIMAGE | rwCAMERACLEARZ);

	RwCameraBeginUpdate(reflectionCam);
	RwCamera *savedcam = gtacam;
	gtacam = reflectionCam;	// they do some begin/end updates with this in the called functions :/
	RenderReflectionScene();
	gtacam = savedcam;

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

	RwCameraBeginUpdate(gtacam);
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
}

void
CarPipe::CreateShaders(void)
{
	HRSRC resource;
	RwUInt32 *shader;
	resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_VEHICLEONEVS), RT_RCDATA);
	shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreateVertexShader(shader, &vertexShaderPass1);
	assert(vertexShaderPass1);
	FreeResource(shader);

	resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_VEHICLETWOVS), RT_RCDATA);
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
	if(path == NULL){
		MessageBox(NULL, "Couldn't load 'neo\\carTweakingTable.dat'", "Error", MB_ICONERROR | MB_OK);
		exit(0);
	}
	dat = fopen(path, "r");
	neoReadWeatherTimeBlock(dat, &fresnel);
	neoReadWeatherTimeBlock(dat, &power);
	neoReadWeatherTimeBlock(dat, &diffColor);
	neoReadWeatherTimeBlock(dat, &specColor);
	fclose(dat);
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
		UploadZero(loc);
}

void
CarPipe::ShaderSetup(RwMatrix *world)
{
	DirectX::XMMATRIX worldMat, viewMat, projMat, texMat;
	RwCamera *cam = (RwCamera*)RWSRCGLOBAL(curCamera);

	RwMatrix view;
	RwMatrixInvert(&view, RwFrameGetLTM(RwCameraGetFrame(cam)));

	RwToD3DMatrix(&worldMat, world);
	RwToD3DMatrix(&viewMat, &view);
	viewMat.r[0] = DirectX::XMVectorNegate(viewMat.r[0]);
	MakeProjectionMatrix(&projMat, cam);

	DirectX::XMMATRIX combined = DirectX::XMMatrixMultiply(projMat, DirectX::XMMatrixMultiply(viewMat, worldMat));
	RwD3D9SetVertexShaderConstant(LOC_combined, (void*)&combined, 4);
	RwD3D9SetVertexShaderConstant(LOC_world, (void*)&worldMat, 4);
	texMat = DirectX::XMMatrixIdentity();
	RwD3D9SetVertexShaderConstant(LOC_tex, (void*)&texMat, 4);

	RwMatrix *camfrm = RwFrameGetLTM(RwCameraGetFrame(cam));
	RwD3D9SetVertexShaderConstant(LOC_eye, (void*)RwMatrixGetPos(camfrm), 1);

	if(pAmbient)
		UploadLightColorWithSpecular(pAmbient, LOC_ambient);
	else
		UploadZero(LOC_ambient);
	if(pDirect){
		UploadLightColorWithSpecular(pDirect, LOC_directCol);
		UploadLightDirection(pDirect, LOC_directDir);
	}else{
		UploadZero(LOC_directCol);
		UploadZero(LOC_directDir);
	}
	for(int i = 0 ; i < 4; i++)
		if(pExtraDirectionals[i]){
			UploadLightDirection(pExtraDirectionals[i], LOC_lightDir+i);
			UploadLightColorWithSpecular(pExtraDirectionals[i], LOC_lightCol+i);
		}else{
			UploadZero(LOC_lightDir+i);
			UploadZero(LOC_lightCol+i);
		}
	Color spec = specColor.Get();
	spec.r *= spec.a;
	spec.g *= spec.a;
	spec.b *= spec.a;
	RwD3D9SetVertexShaderConstant(LOC_directSpec, (void*)&spec, 1);
}

void
CarPipe::DiffusePass(RxD3D9ResEntryHeader *header)
{
	RxD3D9InstanceData *inst = (RxD3D9InstanceData*)&header[1];
	CustomSpecMapPipeMaterialData *specData;

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

	for(int i = 0; i < header->numMeshes; i++){
		RwD3D9SetTexture(inst->material->texture, 0);
		// have to set these after the texture, RW sets texture stage states automatically
		// ^ still true in d3d9? i think not but can't be bothered to find out
		RwD3D9SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
		RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
		RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
		RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);

		specData = *RWPLUGINOFFSET(CustomSpecMapPipeMaterialData*, inst->material, CCustomCarEnvMapPipeline__ms_specularMapPluginOffset);

		Color c = diffColor.Get();
		Color diff(c.r*c.a, c.g*c.a, c.b*c.a, 1.0f-c.a);
		RwRGBAReal mat;
		RwRGBARealFromRwRGBA(&mat, &inst->material->color);
		mat.red = mat.red*diff.a + diff.r;
		mat.green = mat.green*diff.a + diff.g;
		mat.blue = mat.blue*diff.a + diff.b;
		RwD3D9SetVertexShaderConstant(LOC_matCol, (void*)&mat, 1);

		float reflProps[4];
		reflProps[0] = specData ? specData->specularity*config->neoSpecMult : 0.0f;
		reflProps[1] = fresnel.Get();
//		reflProps[0] = 1.0f;
		reflProps[2] = 0.6f;	// unused
		reflProps[3] = power.Get();
		RwD3D9SetVertexShaderConstant(LOC_reflProps, (void*)reflProps, 1);

		D3D9RenderVehicleDual(header, inst);
		inst++;
	}
}

void
CarPipe::SpecularPass(RxD3D9ResEntryHeader *header)
{
	RwUInt32 src, dst, fog, zwrite;
	RxD3D9InstanceData *inst = (RxD3D9InstanceData*)&header[1];
	CustomSpecMapPipeMaterialData *specData;

	RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &zwrite);
	RwRenderStateGet(rwRENDERSTATEFOGENABLE, &fog);
	RwRenderStateGet(rwRENDERSTATESRCBLEND, &src);
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &dst);

	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
	RwD3D9SetTexture(NULL, 0);
	RwD3D9SetTexture(NULL, 1);
	RwD3D9SetTexture(NULL, 2);
	RwD3D9SetTexture(NULL, 3);
	RwD3D9SetVertexShader(vertexShaderPass2);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
	for(int i = 0; i < header->numMeshes; i++){
		specData = *RWPLUGINOFFSET(CustomSpecMapPipeMaterialData*, inst->material, CCustomCarEnvMapPipeline__ms_specularMapPluginOffset);
		if(specData && specData->specularity != 0.0f)
			D3D9Render(header, inst);
		inst++;
	}
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)zwrite);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)fog);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)src);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)dst);
}

void
CarPipe::RenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	{
		static bool keystate = false;
		if(GetAsyncKeyState(VK_F7) & 0x8000){
			if(!keystate){
				keystate = true;
				LoadTweakingTable();
			}
		}else
			keystate = false;
	}

	RxD3D9ResEntryHeader *header = (RxD3D9ResEntryHeader*)&repEntry[1];
	ShaderSetup(RwFrameGetLTM(RpAtomicGetFrame((RpAtomic*)object)));

	if(header->indexBuffer != NULL)
		RwD3D9SetIndices(header->indexBuffer);
	_rwD3D9SetStreams(header->vertexStream, header->useOffsets);
	RwD3D9SetVertexDeclaration(header->vertexDeclaration);

	RwD3D9SetPixelShader(NULL);

	DiffusePass(header);
	SpecularPass(header);
	RwD3D9SetTexture(NULL, 1);
	RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D9SetTexture(NULL, 2);
	RwD3D9SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(2, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
}
