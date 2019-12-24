#include "skygfx.h"
#include "neo.h"

enum {
	// common
	REG_transform	= 0,
	REG_ambient	= 4,
	REG_directCol	= 5,	// 7 lights (main + 6 extra)
	REG_directDir	= 12,	//
	REG_matCol	= 19,
	REG_surfProps	= 20,

	// env
	REG_fxParams	= 30,
	REG_envXform	= 31,
	REG_envmat	= 32,

	// spec tex
	REG_specmat	= 35,
	REG_lightdir	= 38,

	// spec light
	REG_eye		= 35,
};

void *vehiclePipeVS, *ps2CarFxVS;
void *ps2EnvSpecFxPS;	// also used by the building pipeline
void *specCarFxVS, *specCarFxPS;
void *envCarVS, *envCarPS;
void *xboxCarVS;
void *leedsCarFxVS;
void *mobileVehiclePipeVS, *mobileVehiclePipePS;
int renderingWheel;

float black4f[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
float white4f[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

int betaEnvmaptest = 0;
float envmap1tweak = 1.0f;
float envmap2tweak = 1.0f;

CPool<CustomEnvMapPipeAtomicData> *&gEnvMapPipeAtmDataPool = *(CPool<CustomEnvMapPipeAtomicData>**)0xC02D2C;

void*
CustomEnvMapPipeAtomicData::operator new(size_t size)
{
	return gEnvMapPipeAtmDataPool->New();
}

static RwMatrix carfx_view, carfx_env1Inv, carfx_env2Inv;
static RwV3d carfx_lightdir;	// view space
static RwFrame *carfx_env1Frame, *carfx_env2Frame;

// Leeds reflections
static D3DMATRIX envtexmat;

CustomEnvMapPipeAtomicData*
CCustomCarEnvMapPipeline__AllocEnvMapPipeAtomicData(RpAtomic *atomic)
{
	CustomEnvMapPipeAtomicData *atmEnvData = *GETENVMAPATM(atomic);
	if(atmEnvData == NULL){
		atmEnvData = new CustomEnvMapPipeAtomicData;
		// TODO: do we always have this?
		atmEnvData->trans = 0;
		atmEnvData->posx = 0;
		atmEnvData->posy = 0;
		*GETENVMAPATM(atomic) = atmEnvData;
	}
	return atmEnvData;
}

void
CCustomCarEnvMapPipeline__Init(void)
{
	static RwV3d axis_X = { 1.0, 0.0, 0.0 };
	static RwV3d axis_Y = { 0.0, 1.0, 0.0 };
	static RwV3d axis_Z = { 0.0, 0.0, 1.0 };

	reflectionTex = RwTextureCreate(nil);
	RwTextureSetFilterMode(reflectionTex, rwFILTERLINEAR);

	MakeEnvmapCam();
	MakeEnvmapRasters();

	CreateShaders();

	if(carfx_env1Frame == NULL){
		carfx_env1Frame = RwFrameCreate();
if(betaEnvmaptest)
		RwMatrixRotate(RwFrameGetMatrix(carfx_env1Frame), &axis_X, 60.0f, rwCOMBINEREPLACE);
else{
		RwMatrixRotate(RwFrameGetMatrix(carfx_env1Frame), &axis_X, 33.0, rwCOMBINEREPLACE);
		RwMatrixRotate(RwFrameGetMatrix(carfx_env1Frame), &axis_Y, -33.0, rwCOMBINEPOSTCONCAT);
		RwMatrixRotate(RwFrameGetMatrix(carfx_env1Frame), &axis_Z, 50.0, rwCOMBINEPOSTCONCAT);
}
		RwFrameUpdateObjects(carfx_env1Frame);
		RwFrameGetLTM(carfx_env1Frame);
	}
	if(carfx_env2Frame == NULL){
		carfx_env2Frame = RwFrameCreate();
		RwFrameSetIdentity(carfx_env2Frame);
		RwFrameUpdateObjects(carfx_env2Frame);
	}

	envtexmat.m[0][0] = 0.5f;
	envtexmat.m[0][1] = 0.0f;
	envtexmat.m[0][2] = 0.0f;
	envtexmat.m[0][3] = 0.0f;

	envtexmat.m[1][0] = 0.0f;
	envtexmat.m[1][1] = -0.5f;
	envtexmat.m[1][2] = 0.0f;
	envtexmat.m[1][3] = 0.0f;

	envtexmat.m[2][0] = 0.0f;
	envtexmat.m[2][1] = 0.0f;
	envtexmat.m[2][2] = 0.0f;
	envtexmat.m[2][3] = 0.0f;

	envtexmat.m[3][0] = 0.5f;
	envtexmat.m[3][1] = 0.5f;
	envtexmat.m[3][2] = 0.0f;
	envtexmat.m[3][3] = 1.0f;
}

void (*CCustomCarEnvMapPipeline__PreRenderUpdate_orig)(void);
void
CCustomCarEnvMapPipeline__PreRenderUpdate(void)
{
	RwV3d l;
	RwMatrixInvert(&carfx_view, RwFrameGetLTM(RwCameraGetFrame(RWSRCGLOBAL(curCamera))));
	l = RwFrameGetMatrix(RpLightGetFrame(pDirect))->at;
	RwV3dTransformVector(&carfx_lightdir, &l, &carfx_view);
	RwV3dNormalize(&carfx_lightdir, &carfx_lightdir);

	RwMatrixInvert(&carfx_env1Inv, RwFrameGetLTM(carfx_env1Frame));
	RwMatrixInvert(&carfx_env2Inv, RwFrameGetLTM(carfx_env2Frame));

	CCustomCarEnvMapPipeline__PreRenderUpdate_orig();
}

void
CCustomCarEnvMapPipeline__Env1Xform(RwMatrix *envmat,
	CustomEnvMapPipeMaterialData *envData, float *envXform)
{
	float sclx, scly;
	sclx = envData->GetTransScaleX()*50.0f;
	scly = envData->GetTransScaleY()*50.0f;
if(betaEnvmaptest){
sclx *= envmap1tweak;
scly *= envmap1tweak;
}
	// fractional parts of pos/scl
	envXform[0] = (envmat->pos.x - ((float)(int)(envmat->pos.x/sclx))*sclx)/sclx;
	envXform[1] = (envmat->pos.y - ((float)(int)(envmat->pos.y/scly))*scly)/scly;
}

// get absolute fractional part of pos/scl, or 1 - fractional part
inline float scaledfract(float pos, float scl){
	int i;
	float f;
	i = pos/scl;
	f = fabs((i*scl - pos)/scl);
	return i & 1 ? f : 1.0 - f;
}

void
CCustomCarEnvMapPipeline__Env2Xform(RpAtomic *atomic, RwMatrix *envmat,
	CustomEnvMapPipeMaterialData *envData, CustomEnvMapPipeAtomicData *atmEnvData, float *envXform)
{
	static void *lastobject;
	static CustomEnvMapPipeMaterialData *lastenvdata;
	static RwUInt16 lastrenderframe;
	static float lastTransX, lastTransY;
	static float lastx, lasty;

	RwV3d diff, upnorm;
	float trans;
	float sclx, scly;
	float val1, val2;
	int sub;

	sclx = envData->GetTransScaleX()*50.0f;
	scly = envData->GetTransScaleY()*50.0f;

	if(lastrenderframe != RWSRCGLOBAL(renderFrame) ||
	   lastobject != atomic ||
	   lastenvdata != envData){
		envData->renderFrameCounter = RWSRCGLOBAL(renderFrame);
		lastrenderframe = RWSRCGLOBAL(renderFrame);
		lastobject = atomic;
		lastenvdata = envData;

		val1 = scaledfract(envmat->pos.x, sclx) + scaledfract(envmat->pos.y, scly);
		val2 = scaledfract(atmEnvData->posx, sclx) + scaledfract(atmEnvData->posy, scly);

		diff = { envmat->pos.x - atmEnvData->posx, envmat->pos.y - atmEnvData->posy, 0.0 };

		sub = 0;
		if(RwV3dDotProduct(&diff, &diff) > 0.0f ||
		   RwV3dNormalize(&diff, &diff), RwV3dNormalize(&upnorm, &envmat->up), RwV3dDotProduct(&diff, &upnorm) < 0.0f){
			trans = atmEnvData->trans - fabs(val2-val1);
			if(trans < 0.0f)
				trans += 1.0f;
		}else{
			trans = atmEnvData->trans + fabs(val2-val1);
			if(trans >= 1.0f)
				trans -= 1.0f;
		}

		lastTransX = atmEnvData->trans = trans;
		lastTransY = envmat->at.x + envmat->at.y;
		if(lastTransY > 0.1) lastTransY = 0.1f;
		if(lastTransY < 0.0) lastTransY = 0.0f;
		if(envmat->at.z < 0.0f)
			lastTransY = 1.0 - lastTransY;
		lastx = atmEnvData->posx = envmat->pos.x;
		lasty = atmEnvData->posy = envmat->pos.y;
	}else{
		atmEnvData->trans = lastTransX;
		atmEnvData->posx = lastx;
		atmEnvData->posy = lasty;
	}
	envXform[0] = lastTransX;
	envXform[1] = lastTransY;
}

void
CCustomCarEnvMapPipeline__Env1Xform_PC(RpAtomic *atomic,
	CustomEnvMapPipeMaterialData *envData, float *envXform)
{
	float sclx, scly;
	RwMatrix *envmat;
	envmat = RwFrameGetLTM(RpAtomicGetClump(atomic) ? RpClumpGetFrame(RpAtomicGetClump(atomic)) : RpAtomicGetFrame(atomic));
	sclx = envData->GetTransScaleX()*50.0f;
	scly = envData->GetTransScaleY()*50.0f;
	// fractional parts of pos/scl
	envXform[0] = -(envmat->pos.x - ((float)(int)(envmat->pos.x/sclx))*sclx)/sclx;
	envXform[1] = -(envmat->pos.y - ((float)(int)(envmat->pos.y/scly))*scly)/scly;
}

void
CCustomCarEnvMapPipeline__Env2Xform_PC(RpAtomic *atomic,
	CustomEnvMapPipeMaterialData *envData, CustomEnvMapPipeAtomicData *atmEnvData, float *envXform)
{
	static void *lastobject;
	static CustomEnvMapPipeMaterialData *lastenvdata;
	static RwUInt16 lastrenderframe;
	static float lastTransX, lastTransY;
	static float lastx, lasty;

	RwV3d diff, upnorm;
	float trans;
	float sclx, scly;
	float val1, val2;
	int sub;
	RwMatrix *envmat;

	sclx = envData->GetTransScaleX()*50.0f;
	scly = envData->GetTransScaleY()*50.0f;
	envmat = RwFrameGetLTM(RpAtomicGetClump(atomic) ? RpClumpGetFrame(RpAtomicGetClump(atomic)) : RpAtomicGetFrame(atomic));

	if(lastrenderframe != RWSRCGLOBAL(renderFrame) ||
	   lastobject != atomic ||
	   lastenvdata != envData){
		envData->renderFrameCounter = RWSRCGLOBAL(renderFrame);
		lastrenderframe = RWSRCGLOBAL(renderFrame);
		lastobject = atomic;
		lastenvdata = envData;

		val1 = scaledfract(envmat->pos.x, sclx) + scaledfract(envmat->pos.y, scly);
		val2 = scaledfract(atmEnvData->posx, sclx) + scaledfract(atmEnvData->posy, scly);

		diff = { envmat->pos.x - atmEnvData->posx, envmat->pos.y - atmEnvData->posy, 0.0 };

		sub = 0;
		if(RwV3dDotProduct(&diff, &diff) > 0.0f ||
		   RwV3dNormalize(&diff, &diff), RwV3dNormalize(&upnorm, &envmat->up), RwV3dDotProduct(&diff, &upnorm) < 0.0f){
			trans = atmEnvData->trans - fabs(val2-val1);
			if(trans < 0.0f)
				trans += 1.0f;
		}else{
			trans = atmEnvData->trans + fabs(val2-val1);
			if(trans >= 1.0f)
				trans -= 1.0f;
		}

		lastTransX = atmEnvData->trans = trans;
		lastTransY = envmat->at.x + envmat->at.y;
		if(lastTransY > 0.1) lastTransY = 0.1f;
		if(lastTransY < 0.0) lastTransY = 0.0f;
		if(envmat->at.z < 0.0f)
			lastTransY = 1.0 - lastTransY;
		lastx = atmEnvData->posx = envmat->pos.x;
		lasty = atmEnvData->posy = envmat->pos.y;
	}else{
		atmEnvData->trans = lastTransX;
		atmEnvData->posx = lastx;
		atmEnvData->posy = lasty;
	}
	envXform[0] = -lastTransX;
	envXform[1] = lastTransY;
}

void
CCustomCarEnvMapPipeline__SetupEnv(RpAtomic *atomic, RwFrame *envframe, RwMatrix *frminv, RwMatrix *envmat)
{
	static RwMatrix lastmat;
	static void *lastobject;
	static RwFrame *lastfrm;
	static RwUInt16 lastrenderframe;
	RpClump *clump;
	RwFrame *frame;

	clump = RpAtomicGetClump(atomic);
	if(lastobject != (clump ? (void*)clump : (void*)atomic) ||
	   lastfrm != envframe ||
	   lastrenderframe != RWSRCGLOBAL(renderFrame)){
		frame = clump ? RpClumpGetFrame(clump) : RpAtomicGetFrame(atomic);
		RwMatrixMultiply(&lastmat, RwFrameGetLTM(frame), frminv);

		lastobject = (clump ? (void*)clump : (void*)atomic);
		lastfrm = envframe;
		lastrenderframe = RWSRCGLOBAL(renderFrame);
	}
	*envmat = lastmat;
}

void
CCustomCarEnvMapPipeline__SetupSpec(RpAtomic *atomic, RwMatrix *specmat, RwV3d *specdir)
{
	RwMatrixMultiply(specmat, RwFrameGetLTM(RpAtomicGetFrame(atomic)), &carfx_view);
	*specdir = carfx_lightdir;
}

void
uploadLightCol(RwUInt32 registerAddress, RpLight *l)
{
	if(RpLightGetFlags(l) & rpLIGHTLIGHTATOMICS)
		RwD3D9SetVertexShaderConstant(registerAddress,(void*)&l->color,1);
	else
		RwD3D9SetVertexShaderConstant(registerAddress,(void*)black4f,1);
}

void
uploadLights(RwMatrix *lightmat)
{
	pipeUploadLightColor(pAmbient, REG_ambient);
	pipeUploadLightColor(pDirect, REG_directCol);
	pipeUploadLightDirectionLocal(pDirect, lightmat, REG_directDir);
	for(int i = 0; i < 6; i++)
		if(i < NumExtraDirLightsInWorld && RpLightGetType(pExtraDirectionals[i]) == rpLIGHTDIRECTIONAL){
			pipeUploadLightColor(pExtraDirectionals[i], REG_directCol+i+1);
			pipeUploadLightDirectionLocal(pExtraDirectionals[i], lightmat, REG_directDir+i+1);
		}else{
			pipeUploadZero(REG_directCol+i+1);
			pipeUploadZero(REG_directDir+i+1);
		}
}

void
uploadNoLights(void)
{
	static float black[4*(1+2*7)] = { 0.0f };
	RwD3D9SetVertexShaderConstant(REG_ambient, black, 1+2*7);
}

void
CCustomCarEnvMapPipeline__CustomPipeRenderCB_PS2(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	RxD3D9ResEntryHeader *resEntryHeader;
	RxD3D9InstanceData *instancedData;
	RpAtomic *atomic;
	RwInt32	numMeshes;
	RwBool noFx;
	CustomEnvMapPipeMaterialData *envData;
	CustomEnvMapPipeAtomicData *atmEnvData;
	CustomSpecMapPipeMaterialData *specData;
	RpMaterial *material;
	RwUInt32 materialFlags;
	RwBool hasEnv1, hasEnv2, hasSpec, hasAlpha;
	struct {
		float fxSwitch;
		float shininess;
		float specularity;
		float lightmult;
	} fxParams;
	RwMatrix envmat;
	RwV4d envXform;
	RwMatrix specmat;
	RwV3d specdir;
	RwMatrix lightmat;
	float transform[16];

	atomic = (RpAtomic*)object;

	_rwD3D9EnableClippingIfNeeded(object, type);

	float colorscale = 1.0f;
	RwD3D9SetPixelShaderConstant(0, &colorscale, 1);

	pipeGetComposedTransformMatrix(atomic, transform);
	RwD3D9SetVertexShaderConstant(REG_transform, transform, 4);

	if(flags & rpGEOMETRYLIGHT){
		RwMatrixInvert(&lightmat, RwFrameGetLTM(RpAtomicGetFrame(atomic)));
		uploadLights(&lightmat);
	}else
		uploadNoLights();

	resEntryHeader = (RxD3D9ResEntryHeader *)(repEntry + 1);
	instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);
	if(resEntryHeader->indexBuffer != NULL)
		RwD3D9SetIndices(resEntryHeader->indexBuffer);
	_rwD3D9SetStreams(resEntryHeader->vertexStream,resEntryHeader->useOffsets);
	RwD3D9SetVertexDeclaration(resEntryHeader->vertexDeclaration);
	numMeshes = resEntryHeader->numMeshes;

	int alphafunc;
	int src, dst;
	int fog;
	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &alphafunc);
	RwRenderStateGet(rwRENDERSTATESRCBLEND, &src);
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &dst);
	RwRenderStateGet(rwRENDERSTATEFOGCOLOR, &fog);

	noFx = CVisibilityPlugins__GetAtomicId(atomic) & 0x6000;
	fxParams.lightmult = CCustomCarEnvMapPipeline__m_EnvMapLightingMult;
	CCustomCarEnvMapPipeline__SetupSpec(atomic, &specmat, &specdir);
	RwD3D9SetVertexShaderConstant(REG_specmat, &specmat, 3);
	RwD3D9SetVertexShaderConstant(REG_lightdir, &specdir, 1);

	for(; numMeshes--; instancedData++){
		material = instancedData->material;

		if(instancedData->material->color.alpha == 0)
			continue;

		pipeSetTexture(material->texture, 0);

		hasAlpha = instancedData->vertexAlpha || instancedData->material->color.alpha != 255;
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)hasAlpha);

		pipeUploadMatCol(flags, material, REG_matCol);
		float surfProps[4];
		surfProps[0] = material->surfaceProps.ambient;
		surfProps[2] = material->surfaceProps.diffuse;
		surfProps[3] = flags & rpGEOMETRYPRELIT ? 1.0f : 0.0f;
		RwD3D9SetVertexShaderConstant(REG_surfProps, surfProps, 1);

		RwD3D9SetVertexShader(vehiclePipeVS);
		RwD3D9SetPixelShader(simplePS);

		D3D9RenderDual(config->dualPassVehicle, resEntryHeader, instancedData);

		//
		// FX pass
		//
		if(noFx)
			continue;

		materialFlags = *(RwUInt32*)&material->surfaceProps.specular;
		hasEnv1  = !!(materialFlags & 1);
		hasEnv2  = !!(materialFlags & 2);
		hasSpec  = !!(materialFlags & 4) && !renderingWheel;
if(betaEnvmaptest) hasSpec = false;
		// TODO: is this even needed?
		if(RpMatFXMaterialGetEffects(material) != rpMATFXEFFECTENVMAP){
			hasEnv1 = false;
			hasEnv2 = false;
			hasSpec = false;
		}

		int fxpass = 0;
		fxParams.shininess = 0.0f;
		fxParams.specularity = 0.0f;
		envData = *GETENVMAP(material);
		specData = *GETSPECMAP(material);
		if(hasEnv1){
			fxParams.fxSwitch = 1;
			fxParams.shininess = envData->GetShininess();
			CCustomCarEnvMapPipeline__SetupEnv(atomic, carfx_env1Frame, &carfx_env1Inv, &envmat);
			RwD3D9SetVertexShaderConstant(REG_envmat, &envmat, 3);
			RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);	// is this needed?
			RwD3D9SetTexture(envData->texture, 1);
			CCustomCarEnvMapPipeline__Env1Xform(&envmat, envData, &envXform.x);
			envXform.z = envData->GetScaleX();
			envXform.w = envData->GetScaleY();
if(betaEnvmaptest){
	envXform.z *= envmap2tweak;
	envXform.w *= envmap2tweak;
}
			fxpass = 1;
		}else if(hasEnv2){
			fxParams.fxSwitch = 2;
			fxParams.shininess = envData->GetShininess();
			CCustomCarEnvMapPipeline__SetupEnv(atomic, carfx_env2Frame, &carfx_env2Inv, &envmat);
			RwD3D9SetVertexShaderConstant(REG_envmat, &envmat, 3);
			RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);	// is this needed?
			RwD3D9SetTexture(envData->texture, 1);
			atmEnvData = CCustomCarEnvMapPipeline__AllocEnvMapPipeAtomicData(atomic);
			CCustomCarEnvMapPipeline__Env2Xform(atomic, &envmat, envData, atmEnvData, &envXform.x);
			envXform.z = envData->GetScaleX();
			envXform.w = envData->GetScaleY();
			fxpass = 1;
		}

		if(hasSpec){
			fxParams.specularity = specData->specularity;
			RwD3D9SetTexture(specData->texture, 2);
			fxpass = 1;
		}

		if(fxpass){
			RwD3D9SetVertexShader(ps2CarFxVS);
			RwD3D9SetPixelShader(ps2EnvSpecFxPS);

			RwD3D9SetVertexShaderConstant(REG_fxParams, &fxParams, 1);
			RwD3D9SetVertexShaderConstant(REG_envXform, &envXform, 1);

			RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONALWAYS);
			RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
			RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
			RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
			RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
			RwRenderStateSet(rwRENDERSTATEFOGCOLOR, (void*)0);
			D3D9Render(resEntryHeader, instancedData);
			RwRenderStateSet(rwRENDERSTATEFOGCOLOR, (void*)fog);
			RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
			RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)src);
			RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)dst);
			RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)alphafunc);
		}
	}
	RwD3D9SetVertexShader(NULL);
	RwD3D9SetPixelShader(NULL);
	RwD3D9SetTexture(NULL, 1);
	RwD3D9SetTexture(NULL, 2);
	RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
}

void
CCustomCarEnvMapPipeline__CustomPipeRenderCB_Specular(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	RxD3D9ResEntryHeader *resEntryHeader;
	RxD3D9InstanceData *instancedData;
	RpAtomic *atomic;
	RwInt32	numMeshes;
	RwBool noFx;
	CustomEnvMapPipeMaterialData *envData;
	CustomEnvMapPipeAtomicData *atmEnvData;
	CustomSpecMapPipeMaterialData *specData;
	RpMaterial *material;
	RwUInt32 materialFlags;
	RwBool hasEnv1, hasEnv2, hasSpec, hasAlpha;
	struct {
		float fxSwitch;
		float shininess;
		float specularity;
		float lightmult;
	} fxParams;
	RwMatrix envmat;
	RwV4d envXform;
	RwV3d eye;
	RwMatrix lightmat;
	float transform[16];

	atomic = (RpAtomic*)object;

	_rwD3D9EnableClippingIfNeeded(object, type);

	float colorscale = 1.0f;
	RwD3D9SetPixelShaderConstant(0, &colorscale, 1);

	pipeGetComposedTransformMatrix(atomic, transform);
	RwD3D9SetVertexShaderConstant(0, transform, 4);
	RwMatrixInvert(&lightmat, RwFrameGetLTM(RpAtomicGetFrame(atomic)));
	if(flags & rpGEOMETRYLIGHT)
		uploadLights(&lightmat);
	else
		uploadNoLights();

	resEntryHeader = (RxD3D9ResEntryHeader *)(repEntry + 1);
	instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);
	if(resEntryHeader->indexBuffer != NULL)
		RwD3D9SetIndices(resEntryHeader->indexBuffer);
	_rwD3D9SetStreams(resEntryHeader->vertexStream,resEntryHeader->useOffsets);
	RwD3D9SetVertexDeclaration(resEntryHeader->vertexDeclaration);
	numMeshes = resEntryHeader->numMeshes;

	int alphafunc;
	int src, dst;
	int fog;
	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &alphafunc);
	RwRenderStateGet(rwRENDERSTATESRCBLEND, &src);
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &dst);
	RwRenderStateGet(rwRENDERSTATEFOGCOLOR, &fog);

	noFx = CVisibilityPlugins__GetAtomicId(atomic) & 0x6000;
	fxParams.lightmult = CCustomCarEnvMapPipeline__m_EnvMapLightingMult;
	RwMatrix *camfrm = RwFrameGetLTM(RwCameraGetFrame(Scene.camera));
	RwV3dTransformPoint(&eye, RwMatrixGetPos(camfrm), &lightmat);
	RwD3D9SetVertexShaderConstant(REG_eye, &eye, 1);

	for(; numMeshes--; instancedData++){
		material = instancedData->material;

		if(instancedData->material->color.alpha == 0)
			continue;

		pipeSetTexture(material->texture, 0);

		hasAlpha = instancedData->vertexAlpha || instancedData->material->color.alpha != 255;
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)hasAlpha);

		pipeUploadMatCol(flags, material, REG_matCol);
		float surfProps[4];
		surfProps[0] = material->surfaceProps.ambient;
		surfProps[2] = material->surfaceProps.diffuse;
		surfProps[3] = !!(flags & rpGEOMETRYPRELIT);
		RwD3D9SetVertexShaderConstant(REG_surfProps, surfProps, 1);

		RwD3D9SetVertexShader(vehiclePipeVS);
		RwD3D9SetPixelShader(simplePS);

		D3D9RenderDual(config->dualPassVehicle, resEntryHeader, instancedData);

		//
		// FX pass
		//
		if(noFx)
			continue;

		materialFlags = *(RwUInt32*)&material->surfaceProps.specular;
		hasEnv1  = !!(materialFlags & 1);
		hasEnv2  = !!(materialFlags & 2);
		hasSpec  = !!(materialFlags & 4) && !renderingWheel;
		if(RpMatFXMaterialGetEffects(material) != rpMATFXEFFECTENVMAP){
			hasEnv1 = false;
			hasEnv2 = false;
			hasSpec = false;
		}

		int fxpass = 0;
		fxParams.shininess = 0.0f;
		fxParams.specularity = 0.0f;
		envData = *GETENVMAP(material);
		specData = *GETSPECMAP(material);
		if(hasEnv1){
			fxParams.fxSwitch = 1;
			fxParams.shininess = envData->GetShininess();
			CCustomCarEnvMapPipeline__SetupEnv(atomic, carfx_env1Frame, &carfx_env1Inv, &envmat);
			RwD3D9SetVertexShaderConstant(REG_envmat, &envmat, 3);
			RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);	// is this needed?
			RwD3D9SetTexture(envData->texture, 1);
			CCustomCarEnvMapPipeline__Env1Xform(&envmat, envData, &envXform.x);
			envXform.z = envData->GetScaleX();
			envXform.w = envData->GetScaleY();
			fxpass = 1;
		}else if(hasEnv2){
			fxParams.fxSwitch = 2;
			fxParams.shininess = envData->GetShininess();
			CCustomCarEnvMapPipeline__SetupEnv(atomic, carfx_env2Frame, &carfx_env2Inv, &envmat);
			RwD3D9SetVertexShaderConstant(REG_envmat, &envmat, 3);
			RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);	// is this needed?
			RwD3D9SetTexture(envData->texture, 1);
			atmEnvData = CCustomCarEnvMapPipeline__AllocEnvMapPipeAtomicData(atomic);
			CCustomCarEnvMapPipeline__Env2Xform(atomic, &envmat, envData, atmEnvData, &envXform.x);
			envXform.z = envData->GetScaleX();
			envXform.w = envData->GetScaleY();
			fxpass = 1;
		}

		if(hasSpec){
			fxParams.specularity = specData->specularity;
			fxpass = 1;
		}

		if(fxpass){
			RwD3D9SetVertexShader(specCarFxVS);
			RwD3D9SetPixelShader(specCarFxPS);

			RwD3D9SetVertexShaderConstant(REG_fxParams, &fxParams, 1);
			RwD3D9SetVertexShaderConstant(REG_envXform, &envXform, 1);

			RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONALWAYS);
			RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
			RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
			RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
			RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
			RwRenderStateSet(rwRENDERSTATEFOGCOLOR, (void*)0);
			D3D9Render(resEntryHeader, instancedData);
			RwRenderStateSet(rwRENDERSTATEFOGCOLOR, (void*)fog);
			RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
			RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)src);
			RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)dst);
			RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)alphafunc);
		}
	}
	RwD3D9SetVertexShader(NULL);
	RwD3D9SetPixelShader(NULL);
	RwD3D9SetTexture(NULL, 1);
	RwD3D9SetTexture(NULL, 2);
	RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
}

// OLD
enum {
	LOC_World = 0,
	LOC_View = 4,
	LOC_Proj = 8,
	LOC_WorldIT = 12,
	LOC_Texture = 16,
	LOC_matCol = 20,
	LOC_reflData = 21,
	LOC_envXForm = 22,
	LOC_sunDir = 23,
	LOC_sunDiff = 24,
	LOC_sunAmb = 25,
	LOC_surfProps = 26,
	LOC_lights = 27,
	LOC_envSwitch = 39,
	LOC_eye = 40,
};

// This re-implementation of the original code is perhaps a bit too slavish
void
SetupMaterial_Xbox(RpMaterial *material, RwBool lighting, RwUInt32 flags, RwBool blownUp, float specularity)
{
	static RwRGBA white = { 255, 255, 255, 255 };
	RwRGBA matColor;
	RwSurfaceProperties surf = material->surfaceProps;

	surf.specular = specularity;
	if(lighting && flags & rpGEOMETRYLIGHT){
		if(flags & rpGEOMETRYMODULATEMATERIALCOLOR)
			matColor = material->color;
		else
			matColor = white;
	}else{
		matColor = white;
		surf.ambient = 1.0f;
		surf.diffuse = 1.0f;
		surf.specular = 0.0f;
	}

	if(blownUp){
		surf.diffuse *= 0.1f;
		surf.ambient *= 0.1f;
		surf.specular = 0.0f;
	}

	RwRGBAReal color;
	RwRGBARealFromRwRGBA(&color, &matColor);
	RwD3D9SetVertexShaderConstant(LOC_matCol, (void*)&color, 1);
	RwD3D9SetVertexShaderConstant(LOC_surfProps, (void*)&surf, 1);
}

void
CCustomCarEnvMapPipeline__CustomPipeRenderCB_Xbox(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	RxD3D9ResEntryHeader *resEntryHeader;
	RxD3D9InstanceData *instancedData;
	RpAtomic *atomic;
	RwBool lighting, blownUp;
	RwInt32	numMeshes;
	RwBool noFx;
	CustomEnvMapPipeMaterialData *envData;
	CustomEnvMapPipeAtomicData *atmEnvData;
	RpMaterial *material;
	RwUInt32 materialFlags;
	RwBool hasEnv1, hasEnv2, hasSpec, hasAlpha;
	D3DMATRIX worldMat, worldITMat, viewMat, projMat;
	float envSwitch;
	float specularity;

	_rwD3D9EnableClippingIfNeeded(object, type);

	atomic = (RpAtomic*)object;
	noFx = !!(CVisibilityPlugins__GetAtomicId(atomic) & 0x6000);
	blownUp = !((RpLightGetFlags(pDirect) & rpLIGHTLIGHTATOMICS) == 0 ||
	           (CVisibilityPlugins__GetAtomicId(atomic) & 0x4000) == 0);

	RwD3D9SetPixelShader(NULL);
	RwD3D9SetVertexShader(xboxCarVS);

	RwD3D9GetTransform(D3DTS_WORLD, &worldMat);
	RwD3D9GetTransform(D3DTS_VIEW, &viewMat);
	RwD3D9GetTransform(D3DTS_PROJECTION, &projMat);
	RwD3D9SetVertexShaderConstant(LOC_World,(void*)&worldMat,4);
	RwD3D9SetVertexShaderConstant(LOC_View,(void*)&viewMat,4);
	RwD3D9SetVertexShaderConstant(LOC_Proj,(void*)&projMat,4);
	_rwD3D9VSSetActiveWorldMatrix(RwFrameGetLTM(RpAtomicGetFrame(atomic)));
	_rwD3D9VSGetInverseWorldMatrix((void *)&worldITMat);
	RwD3D9SetVertexShaderConstant(LOC_WorldIT,(void*)&worldITMat,4);

	RwMatrix *camfrm = RwFrameGetLTM(RwCameraGetFrame(Scene.camera));
	RwD3D9SetVertexShaderConstant(LOC_eye, (void*)RwMatrixGetPos(camfrm), 1);

	RwD3D9SetVertexShaderConstant(LOC_sunDir,(void*)RwMatrixGetAt(RwFrameGetLTM(RpLightGetFrame(pDirect))),1);
	RwD3D9SetVertexShaderConstant(LOC_sunDiff,(void*)&pDirect->color,1);
	RwD3D9SetVertexShaderConstant(LOC_sunAmb,(void*)&pAmbient->color,1);

	RwD3D9GetRenderState(D3DRS_LIGHTING, &lighting);

	resEntryHeader = (RxD3D9ResEntryHeader *)(repEntry + 1);
	instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);
	if(resEntryHeader->indexBuffer != NULL)
		RwD3D9SetIndices(resEntryHeader->indexBuffer);
	_rwD3D9SetStreams(resEntryHeader->vertexStream,resEntryHeader->useOffsets);
	RwD3D9SetVertexDeclaration(resEntryHeader->vertexDeclaration);
	numMeshes = resEntryHeader->numMeshes;

	for(; numMeshes--; instancedData++){
		material = instancedData->material;

		if(instancedData->material->color.alpha == 0)
			continue;

		envSwitch = 1;
		materialFlags = *(RwUInt32*)&material->surfaceProps.specular;

		hasEnv1  = (materialFlags & 1) && !noFx;
		hasEnv2  = (materialFlags & 2) && !noFx && (flags & rpGEOMETRYTEXTURED2);
		hasSpec  = (materialFlags & 4) && !noFx && !renderingWheel;

		hasAlpha = instancedData->vertexAlpha || instancedData->material->color.alpha != 255;
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)hasAlpha);

		RwD3D9SetTexture(NULL, 1);

		specularity = 0.0f;
		if(hasSpec && !blownUp){
			envSwitch = 4;
			specularity = CCustomCarEnvMapPipeline__m_EnvMapLightingMult * 1.8f;
			if(specularity > 1.0f) specularity = 1.0f;
		}

		envData = *GETENVMAP(material);
		if(blownUp){
			if(hasEnv2)
				envSwitch = 3;
		}else if(hasEnv1){
			envSwitch = 5;
			if(!hasSpec) envSwitch = 2;
			static D3DMATRIX texMat;
			float trans[2];
			RwInt32 tfactor;

			RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
			texMat._11 = envData->GetScaleX();
			texMat._22 = envData->GetScaleY();
			texMat._33 = 1.0f;
			texMat._44 = 1.0f;
			CCustomCarEnvMapPipeline__Env1Xform_PC(atomic, envData, trans);
			texMat._31 = trans[0];
			texMat._32 = trans[1];
			RwD3D9SetVertexShaderConstant(LOC_Texture, (void*)&texMat, 4);
			RwD3D9SetTexture(envData->texture, 1);
			tfactor = CCustomCarEnvMapPipeline__m_EnvMapLightingMult * 96.0f;
			if(tfactor > 255)
				tfactor = 255;
			RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_XRGB(tfactor,tfactor,tfactor));
			RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MULTIPLYADD);
			RwD3D9SetTextureStageState(1, D3DTSS_COLORARG0, D3DTA_CURRENT);
			RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_TFACTOR);
			RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
			RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
			RwD3D9SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
		}else if(hasEnv2){
			envSwitch = 6;
			if(!hasSpec) envSwitch = 3;
			static D3DMATRIX texMat;
			float trans[2] = { 0, 0 };
			RwInt32 tfactor;

			RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
			atmEnvData = CCustomCarEnvMapPipeline__AllocEnvMapPipeAtomicData(atomic);
			CCustomCarEnvMapPipeline__Env2Xform_PC(atomic, envData, atmEnvData, trans);
			texMat._11 = 1.0f;
			texMat._22 = 1.0f;
			texMat._33 = 1.0f;
			texMat._44 = 1.0f;
			texMat._31 = trans[0];
			texMat._32 = trans[1];
			RwD3D9SetVertexShaderConstant(LOC_Texture, (void*)&texMat, 4);
			RwD3D9SetTexture(envData->texture, 1);
			tfactor = CCustomCarEnvMapPipeline__m_EnvMapLightingMult * 24.0f;
			if(tfactor > 255) tfactor = 255;
			RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(tfactor,255,255,255));
			RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_BLENDFACTORALPHA);
			RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
			RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
			RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
			RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
			RwD3D9SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
		}

		RwD3D9SetVertexShaderConstant(LOC_envSwitch, (void*)&envSwitch, 1);

		SetupMaterial_Xbox(material, lighting, flags, blownUp, specularity);

		if(flags & (rxGEOMETRY_TEXTURED2 | rxGEOMETRY_TEXTURED)){
			RwD3D9SetTexture(material->texture, 0);
			RwD3D9SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
			RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
			RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
			RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
		}else{
			RwD3D9SetTexture(NULL, 0);
			RwD3D9SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
			RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
			RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
			RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
		}
		D3D9RenderDual(config->dualPassVehicle, resEntryHeader, instancedData);
	}
	RwD3D9SetVertexShader(NULL);
	RwD3D9SetPixelShader(NULL);
	RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
}



void
CCustomCarEnvMapPipeline__CustomPipeRenderCB_leeds(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	RxD3D9ResEntryHeader *resEntryHeader;
	RxD3D9InstanceData *instancedData;
	RpAtomic *atomic;
	RwInt32	numMeshes;
	RwBool noFx;
	CustomEnvMapPipeMaterialData *envData;
	RpMaterial *material;
	RwUInt32 materialFlags;
	RwBool hasEnv, hasAlpha;
	struct {
		float fxSwitch;
		float shininess;
		float specularity;
		float lightmult;
	} fxParams;
	float envmat[16];
	RwV3d eye;
	RwMatrix lightmat;
	float transform[16];

	atomic = (RpAtomic*)object;

	_rwD3D9EnableClippingIfNeeded(object, type);

	float colorscale = 1.0f;
	RwD3D9SetPixelShaderConstant(0, &colorscale, 1);

	pipeGetComposedTransformMatrix(atomic, transform);
	RwD3D9SetVertexShaderConstant(0, transform, 4);
	RwMatrixInvert(&lightmat, RwFrameGetLTM(RpAtomicGetFrame(atomic)));
	if(flags & rpGEOMETRYLIGHT)
		uploadLights(&lightmat);
	else
		uploadNoLights();

	resEntryHeader = (RxD3D9ResEntryHeader *)(repEntry + 1);
	instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);
	if(resEntryHeader->indexBuffer != NULL)
		RwD3D9SetIndices(resEntryHeader->indexBuffer);
	_rwD3D9SetStreams(resEntryHeader->vertexStream,resEntryHeader->useOffsets);
	RwD3D9SetVertexDeclaration(resEntryHeader->vertexDeclaration);
	numMeshes = resEntryHeader->numMeshes;

	int alphafunc;
	int src, dst;
	int fog;
	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &alphafunc);
	RwRenderStateGet(rwRENDERSTATESRCBLEND, &src);
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &dst);
	RwRenderStateGet(rwRENDERSTATEFOGENABLE, &fog);

	noFx = CVisibilityPlugins__GetAtomicId(atomic) & 0x6000;
	fxParams.lightmult = CCustomCarEnvMapPipeline__m_EnvMapLightingMult;
	RwMatrix *camfrm = RwFrameGetLTM(RwCameraGetFrame(Scene.camera));
	RwV3dTransformPoint(&eye, RwMatrixGetPos(camfrm), &lightmat);
	RwD3D9SetVertexShaderConstant(REG_eye, &eye, 1);

	pipeGetLeedsEnvMapMatrix(atomic, envmat);
	RwD3D9SetVertexShaderConstant(REG_envmat, &envmat, 3);
	RwD3D9SetVertexShaderConstant(36, &envtexmat, 4);

	for(; numMeshes--; instancedData++){
		material = instancedData->material;

		if(instancedData->material->color.alpha == 0)
			continue;

		pipeSetTexture(material->texture, 0);

		hasAlpha = instancedData->vertexAlpha || instancedData->material->color.alpha != 255;
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)hasAlpha);

		pipeUploadMatCol(flags, material, REG_matCol);
		float surfProps[4];
		surfProps[0] = material->surfaceProps.ambient;
		if(surfProps[0] > 0.1f && surfProps[0] < 1.0f)
			surfProps[0] = 1.0f;
		surfProps[2] = material->surfaceProps.diffuse;
		surfProps[3] = !!(flags & rpGEOMETRYPRELIT);
		RwD3D9SetVertexShaderConstant(REG_surfProps, surfProps, 1);

		RwD3D9SetVertexShader(vehiclePipeVS);
		RwD3D9SetPixelShader(simplePS);

		D3D9RenderDual(config->dualPassVehicle, resEntryHeader, instancedData);

		//
		// FX pass
		//
		materialFlags = *(RwUInt32*)&material->surfaceProps.specular;
		hasEnv  = !!(materialFlags & 3);
		if(RpMatFXMaterialGetEffects(material) != rpMATFXEFFECTENVMAP)
			hasEnv = false;
		if(noFx || !hasEnv)
			continue;

		RwD3D9SetVertexShader(leedsCarFxVS);

		envData = *GETENVMAP(material);

		fxParams.lightmult = 1.0f;
		fxParams.shininess = envData ? envData->GetShininess() * 3.0f * config->leedsShininessMult : 0.0f;
		if(fxParams.shininess > 1.0f)
			fxParams.shininess = 1.0f;
		fxParams.shininess *= 0.5f;

		RwD3D9SetVertexShaderConstant(REG_fxParams, &fxParams, 1);

		RwD3D9SetTexture(reflectionTex, 0);
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONALWAYS);
		RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
		RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
		RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
		if(config->vehiclePipe == 6)	// VCS
			RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
		else
			RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);

		D3D9Render(resEntryHeader, instancedData);

		RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);

		RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)fog);
		RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)alphafunc);
	}
	RwD3D9SetVertexShader(NULL);
	RwD3D9SetPixelShader(NULL);
	RwD3D9SetTexture(NULL, 1);
	RwD3D9SetTexture(NULL, 2);
	RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
}

void
CCustomCarEnvMapPipeline__CustomPipeRenderCB_mobile(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	RxD3D9ResEntryHeader *resEntryHeader;
	RxD3D9InstanceData *instancedData;
	RpAtomic *atomic;
	RwInt32	numMeshes;
	RwBool noFx;
	CustomEnvMapPipeMaterialData *envData;
	RpMaterial *material;
	RwUInt32 materialFlags;
	RwBool hasEnv1, hasEnv2, hasSpec, hasAlpha;
	struct {
		float fxSwitch;
		float shininess;
		float specularity;
		float lightmult;
	} fxParams;
	RwV3d eye;
	RwMatrix lightmat;
	float transform[16];

	atomic = (RpAtomic*)object;

	_rwD3D9EnableClippingIfNeeded(object, type);

	float colorscale = 1.0f;
	RwD3D9SetPixelShaderConstant(0, &colorscale, 1);

	pipeGetComposedTransformMatrix(atomic, transform);
	RwD3D9SetVertexShaderConstant(0, transform, 4);
	RwMatrixInvert(&lightmat, RwFrameGetLTM(RpAtomicGetFrame(atomic)));
	if(flags & rpGEOMETRYLIGHT)
		uploadLights(&lightmat);
	else
		uploadNoLights();

	resEntryHeader = (RxD3D9ResEntryHeader *)(repEntry + 1);
	instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);
	if(resEntryHeader->indexBuffer != NULL)
		RwD3D9SetIndices(resEntryHeader->indexBuffer);
	_rwD3D9SetStreams(resEntryHeader->vertexStream,resEntryHeader->useOffsets);
	RwD3D9SetVertexDeclaration(resEntryHeader->vertexDeclaration);
	numMeshes = resEntryHeader->numMeshes;

	noFx = CVisibilityPlugins__GetAtomicId(atomic) & 0x6000;
	fxParams.lightmult = CCustomCarEnvMapPipeline__m_EnvMapLightingMult;

	eye = RwFrameGetLTM(RwCameraGetFrame((RwCamera*)RWSRCGLOBAL(curCamera)))->pos;
	RwD3D9SetVertexShaderConstant(REG_eye, &eye, 1);
	float worldmat[16];
	RwToD3DMatrix(worldmat, RwFrameGetLTM(RpAtomicGetFrame(atomic)));
	RwD3D9SetVertexShaderConstant(31, worldmat, 4);	// world mat


	// Try to get mobile direct color
	float c[4];
	c[0] = pDirect->color.red * 1.28f * 1.5f;// *1.6;
	c[1] = pDirect->color.green * 1.28f * 1.5f;// *1.6;
	c[2] = pDirect->color.blue * 1.28f * 1.5f;// *1.6;
	c[3] = 1.0f;
	RwD3D9SetVertexShaderConstant(REG_directCol, (void*)c, 1);


	for(; numMeshes--; instancedData++){
		material = instancedData->material;

		if(instancedData->material->color.alpha == 0)
			continue;

		bool lighttex = material->texture && strstr(material->texture->name, "vehiclelights");
		pipeSetTexture(material->texture, 0);

		hasAlpha = instancedData->vertexAlpha || instancedData->material->color.alpha != 255;
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)hasAlpha);

		pipeUploadMatCol(flags, material, REG_matCol);
		float surfProps[4];
		surfProps[0] = material->surfaceProps.ambient;
		surfProps[2] = material->surfaceProps.diffuse;
		surfProps[3] = !!(flags & rpGEOMETRYPRELIT);
		RwD3D9SetVertexShaderConstant(REG_surfProps, surfProps, 1);

		materialFlags = *(RwUInt32*)&material->surfaceProps.specular;
		hasEnv1  = !!(materialFlags & 1);
		hasEnv2  = !!(materialFlags & 2);
		hasSpec  = !!(materialFlags & 4) && !renderingWheel;
		if(noFx || RpMatFXMaterialGetEffects(material) != rpMATFXEFFECTENVMAP){
			hasEnv1 = false;
			hasEnv2 = false;
			hasSpec = false;
		}
		// Try to get rid of reflections on light textures, not perfect but they don't look too nice
		if(lighttex){
			hasEnv1 = false;
			hasEnv2 = false;
		}
		envData = *GETENVMAP(material);

		if(hasEnv1 || hasEnv2 || hasSpec){
			float shininess = envData ? envData->GetShininess() : 0.0f;

			if(!hasAlpha){
				float sum = material->color.red + material->color.green + material->color.blue;
				float envmult = (255.0f - sum)*2.0f/255.0f;
				if(envmult <= 1.0f){
					if(material->color.red == 255)
						envmult = 1.0f;
					else{
						envmult = (sum/600.0f) * (sum/600.0f);
						if(envmult <= 1.0f)
							envmult = 1.0f;
					}
				}
				shininess *= envmult;
			}

			//shininess *= 1.5f;	// this only happens for the LQ env map

			// the window effect seems to be a bit overblown in the mobile version
			// on ps3 it looks a bit more pleasant
			if(hasAlpha)
			//	shininess *= 5.0f;	// mobile
				shininess *= 2.5f*fxParams.lightmult;	// see if this works out

			if(shininess > 0.32f) shininess = 0.32f;
			if(shininess < 0.01f) shininess = 0.01f;
			shininess *= 1.4f;

			// shininess *= 0.5f;	// not sure if this is done or not, i think not

			fxParams.shininess = (hasEnv1 || hasEnv2) ? shininess : 0.0f;
			// lets have some more specularity, our light seems to be a bit
			// darker so compensate for that
			fxParams.specularity = hasSpec ? shininess*1.5f : 0.0f;
			RwD3D9SetVertexShaderConstant(REG_fxParams, &fxParams, 1);
			RwD3D9SetPixelShaderConstant(REG_fxParams, &fxParams, 1);
			pipeSetTexture(reflectionTex, 1);
			RwD3D9SetVertexShader(mobileVehiclePipeVS);
			RwD3D9SetPixelShader(mobileVehiclePipePS);
		}else{
			RwD3D9SetVertexShader(vehiclePipeVS);
			RwD3D9SetPixelShader(simplePS);
		}

		D3D9RenderDual(config->dualPassVehicle, resEntryHeader, instancedData);
	}
	RwD3D9SetVertexShader(NULL);
	RwD3D9SetPixelShader(NULL);
	RwD3D9SetTexture(NULL, 1);
	RwD3D9SetTexture(NULL, 2);
	RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
}

void
CCustomCarEnvMapPipeline__CustomPipeRenderCB_Env(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	RxD3D9ResEntryHeader *resEntryHeader;
	RxD3D9InstanceData *instancedData;
	RpAtomic *atomic;
	RwInt32	numMeshes;
	RwBool noFx;
	CustomEnvMapPipeMaterialData *envData;
	CustomSpecMapPipeMaterialData *specData;
	RpMaterial *material;
	RwUInt32 materialFlags;
	RwBool hasEnv1, hasEnv2, hasSpec, hasAlpha;
	struct {
		float fresnel;
		float power;
		float lightmult;
	} fxParams;
	struct {
		float ambient;
		float diffuse;
		float specular;
		float reflection;
	} surfProps;
	RwV3d eye;
	RwMatrix lightmat;
	float transform[16];

	atomic = (RpAtomic*)object;

	_rwD3D9EnableClippingIfNeeded(object, type);

	pipeGetComposedTransformMatrix(atomic, transform);
	RwD3D9SetVertexShaderConstant(0, transform, 4);
	pipeGetWorldMatrix(transform);
	RwD3D9SetVertexShaderConstant(30, transform, 4);
	eye = RwFrameGetLTM(RwCameraGetFrame((RwCamera*)RWSRCGLOBAL(curCamera)))->pos;
	RwD3D9SetVertexShaderConstant(34, &eye, 1);
	RwMatrixInvert(&lightmat, RwFrameGetLTM(RpAtomicGetFrame(atomic)));
	if(flags & rpGEOMETRYLIGHT)
		uploadLights(&lightmat);
	else
		uploadNoLights();

	resEntryHeader = (RxD3D9ResEntryHeader *)(repEntry + 1);
	instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);
	if(resEntryHeader->indexBuffer != NULL)
		RwD3D9SetIndices(resEntryHeader->indexBuffer);
	_rwD3D9SetStreams(resEntryHeader->vertexStream,resEntryHeader->useOffsets);
	RwD3D9SetVertexDeclaration(resEntryHeader->vertexDeclaration);
	numMeshes = resEntryHeader->numMeshes;

	int alphafunc;
	int src, dst;
	int fog;
	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &alphafunc);
	RwRenderStateGet(rwRENDERSTATESRCBLEND, &src);
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &dst);
	RwRenderStateGet(rwRENDERSTATEFOGCOLOR, &fog);

	noFx = CVisibilityPlugins__GetAtomicId(atomic) & 0x6000;
	fxParams.fresnel = 0.3f;
	fxParams.power = 10.0f;
	fxParams.lightmult = CCustomCarEnvMapPipeline__m_EnvMapLightingMult;
	RwD3D9SetVertexShaderConstant(21, &fxParams, 1);

	pipeSetTexture(reflectionTex, 1);
	pipeSetTexture(CarPipe::reflectionMask, 2);

	for(; numMeshes--; instancedData++){
		material = instancedData->material;

		if(instancedData->material->color.alpha == 0)
			continue;

		pipeSetTexture(material->texture, 0);

		hasAlpha = instancedData->vertexAlpha || instancedData->material->color.alpha != 255;
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)hasAlpha);

		envData = *GETENVMAP(material);
		specData = *GETSPECMAP(material);

		surfProps.reflection = 0.0f;
		surfProps.specular = 0.0f;
		if(!noFx){
			materialFlags = *(RwUInt32*)&material->surfaceProps.specular;
			hasEnv1 = !!(materialFlags & 1);
			hasEnv2 = !!(materialFlags & 2);
			hasSpec = !!(materialFlags & 4) && !renderingWheel;
			if(RpMatFXMaterialGetEffects(material) != rpMATFXEFFECTENVMAP){
				hasEnv1 = false;
				hasEnv2 = false;
				hasSpec = false;
			}

			if(hasEnv1 || hasEnv2){
				surfProps.reflection = envData->GetShininess();
				surfProps.reflection *= 15.0f * config->neoShininessMult;
			}

			if(hasSpec){
				surfProps.specular = specData->specularity;
				surfProps.specular *= 3.0f * config->neoSpecularityMult;
			}
		}

		pipeUploadMatCol(flags, material, REG_matCol);
		surfProps.ambient = material->surfaceProps.ambient;
		surfProps.diffuse = material->surfaceProps.diffuse;
		if(surfProps.ambient > 0.1f && surfProps.ambient < 0.8f)
			surfProps.ambient = max(surfProps.ambient, 0.8f);
		RwD3D9SetVertexShaderConstant(REG_surfProps, &surfProps, 1);
		RwD3D9SetVertexShaderConstant(21, &fxParams, 1);

		RwD3D9SetVertexShader(envCarVS);
		RwD3D9SetPixelShader(envCarPS);

		D3D9RenderDual(config->dualPassVehicle, resEntryHeader, instancedData);
	}
	RwD3D9SetVertexShader(NULL);
	RwD3D9SetPixelShader(NULL);
	RwD3D9SetTexture(NULL, 1);
	RwD3D9SetTexture(NULL, 2);
	RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
}

void
CCustomCarEnvMapPipeline__CustomPipeRenderCB_Switch(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	switch(config->vehiclePipe){
	case CAR_PS2:
		CCustomCarEnvMapPipeline__CustomPipeRenderCB_PS2(repEntry, object, type, flags);
		break;
	case CAR_PC:
		CCustomCarEnvMapPipeline__CustomPipeRenderCB_exe(repEntry, object, type, flags);
		break;
	case CAR_XBOX:
		CCustomCarEnvMapPipeline__CustomPipeRenderCB_Xbox(repEntry, object, type, flags);
		break;
	case CAR_SPEC:
		CCustomCarEnvMapPipeline__CustomPipeRenderCB_Specular(repEntry, object, type, flags);
		break;
	case CAR_NEO:
		if(iCanHasNeoCar)
			CarPipe::RenderCallback(repEntry, object, type, flags);
		break;
	case CAR_LCS:
	case CAR_VCS:
		CCustomCarEnvMapPipeline__CustomPipeRenderCB_leeds(repEntry, object, type, flags);
		break;
	case CAR_MOBILE:
		/* Need hooked building pipe for sphere maps */
		if(iCanHasbuildingPipe)
			CCustomCarEnvMapPipeline__CustomPipeRenderCB_mobile(repEntry, object, type, flags);
		break;
	case CAR_ENV:
		if(iCanHasbuildingPipe && iCanHasNeoCar)
			CCustomCarEnvMapPipeline__CustomPipeRenderCB_Env(repEntry, object, type, flags);
		break;
	}
	fixSAMP();
}

void
setVehiclePipeCB(RxPipelineNode *node, RxD3D9AllInOneRenderCallBack callback)
{
	CCustomCarEnvMapPipeline__Init();
	RxD3D9AllInOneSetRenderCallBack(node, CCustomCarEnvMapPipeline__CustomPipeRenderCB_Switch);
}

RpAtomic*
CVisibilityPlugins__RenderWheelAtomicCB(RpAtomic *atomic)
{
	renderingWheel = 1;
	AtomicDefaultRenderCallBack(atomic);
	renderingWheel = 0;
	return atomic;
}

int CCarFXRenderer__IsCCPCPipelineAttached(RpAtomic *atomic)
{
	// Temporary to disable car pipeline and fall back to MatFX
	return false;
//	return GetPipelineID(atomic) == RSPIPE_PC_CustomCarEnvMap_PipeID;
}

#include "debugmenu_public.h"
RwTexture *RwTextureRead_HACK(const RwChar * name, const RwChar * maskName)
{
	if(strcmp(name, "xvehicleenv128") == 0)
		return RwTextureRead("vehicleenvmap128", maskName);
	return RwTextureRead(name, maskName);
}

void
hookVehiclePipe(void)
{
	InjectHook(0x5D9FE9, setVehiclePipeCB);
	InterceptCall(&CCustomCarEnvMapPipeline__PreRenderUpdate_orig, CCustomCarEnvMapPipeline__PreRenderUpdate, 0x5D5B10);
	InjectHook(0x7323C0, CVisibilityPlugins__RenderWheelAtomicCB, PATCH_JUMP);

	// TEMP - disable car pipe
#if 0
	InjectHook(0x5D5B80, CCarFXRenderer__IsCCPCPipelineAttached, PATCH_JUMP);
	Nop(0x4C8907, 5);	// don't auto-assign on vehicles
	*(float*)0x8A7780 = 1.0f;	// hardcoded coefficient
//	Nop(0x4C8899, 5);	// don't set coefficient
	Nop(0x4C982F, 5);	// don't update coefficient for lighting
#endif
	// temp hack RwTextureRead to read other env map
	if(betaEnvmaptest){
		InjectHook(0x80483C, RwTextureRead_HACK);
		if(DebugMenuLoad()){
			DebugMenuAddVarBool32("SkyGFX", "Beta Env map test", &betaEnvmaptest, nil);
			DebugMenuAddVar("SkyGFX", "envmap tweak 1", &envmap1tweak, nil, 0.1f, 0.0f, 10.0f);
			DebugMenuAddVar("SkyGFX", "envmap tweak 2", &envmap2tweak, nil, 0.1f, 0.0f, 10.0f);
		}
	}
}
