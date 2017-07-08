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
void *xboxCarVS;

float black4f[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
float white4f[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

CPool<CustomEnvMapPipeAtomicData> *&gEnvMapPipeAtmDataPool = *(CPool<CustomEnvMapPipeAtomicData>**)0xC02D2C;

void*
CustomEnvMapPipeAtomicData::operator new(size_t size)
{
	return gEnvMapPipeAtmDataPool->New();
}

static RwMatrix carfx_view, carfx_env1Inv, carfx_env2Inv;
static RwV3d carfx_lightdir;	// view space
static RwFrame *carfx_env1Frame, *carfx_env2Frame;

CustomEnvMapPipeAtomicData*
CCustomCarEnvMapPipeline__AllocEnvMapPipeAtomicData(RpAtomic *atomic)
{
	CustomEnvMapPipeAtomicData *atmEnvData = *RWPLUGINOFFSET(CustomEnvMapPipeAtomicData*, atomic,
	                             CCustomCarEnvMapPipeline__ms_envMapAtmPluginOffset);
	if(atmEnvData == NULL){
		atmEnvData = new CustomEnvMapPipeAtomicData;
		atmEnvData->trans = 0;
		atmEnvData->posx = 0;
		atmEnvData->posy = 0;
		*RWPLUGINOFFSET(CustomEnvMapPipeAtomicData*, atomic,
		                CCustomCarEnvMapPipeline__ms_envMapAtmPluginOffset) = atmEnvData;
	}
	return atmEnvData;
}

void
CCustomCarEnvMapPipeline__Init(void)
{
	static RwV3d axis_X = { 1.0, 0.0, 0.0 };
	static RwV3d axis_Y = { 0.0, 1.0, 0.0 };
	static RwV3d axis_Z = { 0.0, 0.0, 1.0 };

	CreateShaders();

	if(carfx_env1Frame == NULL){
		carfx_env1Frame = RwFrameCreate();
		RwMatrixRotate(RwFrameGetMatrix(carfx_env1Frame), &axis_X, 33.0, rwCOMBINEREPLACE);
		RwMatrixRotate(RwFrameGetMatrix(carfx_env1Frame), &axis_Y, -33.0, rwCOMBINEPOSTCONCAT);
		RwMatrixRotate(RwFrameGetMatrix(carfx_env1Frame), &axis_Z, 50.0, rwCOMBINEPOSTCONCAT);
		RwFrameUpdateObjects(carfx_env1Frame);
		RwFrameGetLTM(carfx_env1Frame);
	}
	if(carfx_env2Frame == NULL){
		carfx_env2Frame = RwFrameCreate();
		RwFrameSetIdentity(carfx_env2Frame);
		RwFrameUpdateObjects(carfx_env2Frame);
	}
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
}

void
CCustomCarEnvMapPipeline__Env1Xform(RwMatrix *envmat,
	CustomEnvMapPipeMaterialData *envData, float *envXform)
{
	float sclx, scly;
	sclx = envData->transScaleX/8.0f*50.0f;
	scly = envData->transScaleY/8.0f*50.0f;
	envXform[0] = (envmat->pos.x - ((float)(int)(envmat->pos.x/sclx))*sclx)/sclx;
	envXform[1] = (envmat->pos.y - ((float)(int)(envmat->pos.y/scly))*scly)/scly;
}

inline float calcthing(float pos, float scl){
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

	sclx = envData->transScaleX/8.0f*50.0f;
	scly = envData->transScaleY/8.0f*50.0f;

	if(lastrenderframe != RWSRCGLOBAL(renderFrame) ||
	   lastobject != atomic ||
	   lastenvdata != envData){
		envData->renderFrameCounter = RWSRCGLOBAL(renderFrame);
		lastrenderframe = RWSRCGLOBAL(renderFrame);
		lastobject = atomic;
		lastenvdata = envData;

		val1 = calcthing(envmat->pos.x, sclx) + calcthing(envmat->pos.y, scly);
		val2 = calcthing(atmEnvData->posx, sclx) + calcthing(atmEnvData->posy, scly);

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

	int alphafunc, alpharef;
	int src, dst;
	int fog;
	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &alpharef);
	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &alphafunc);
	RwRenderStateGet(rwRENDERSTATESRCBLEND, &src);
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &dst);
	RwRenderStateGet(rwRENDERSTATEFOGENABLE, &fog);

	noFx = CVisibilityPlugins__GetAtomicId(atomic) & 0x6000;
	fxParams.lightmult = CCustomCarEnvMapPipeline__m_EnvMapLightingMult;
	CCustomCarEnvMapPipeline__SetupSpec(atomic, &specmat, &specdir);
	RwD3D9SetVertexShaderConstant(REG_specmat, &specmat, 3);
	RwD3D9SetVertexShaderConstant(REG_lightdir, &specdir, 1);

	for(; numMeshes--; instancedData++){
		material = instancedData->material;
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
		hasSpec  = !!(materialFlags & 4);
		if(RpMatFXMaterialGetEffects(material) != rpMATFXEFFECTENVMAP){
			hasEnv1 = false;
			hasEnv2 = false;
			hasSpec = false;
		}

		int fxpass = 0;
		fxParams.shininess = 0.0f;
		fxParams.specularity = 0.0f;
		envData = *RWPLUGINOFFSET(CustomEnvMapPipeMaterialData*, material, CCustomCarEnvMapPipeline__ms_envMapPluginOffset);
		specData = *RWPLUGINOFFSET(CustomSpecMapPipeMaterialData*, material, CCustomCarEnvMapPipeline__ms_specularMapPluginOffset);
		if(hasEnv1){
			fxParams.fxSwitch = 1;
			fxParams.shininess = envData->shininess/255.0f;
			CCustomCarEnvMapPipeline__SetupEnv(atomic, carfx_env1Frame, &carfx_env1Inv, &envmat);
			RwD3D9SetVertexShaderConstant(REG_envmat, &envmat, 3);
			RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);	// is this needed?
			RwD3D9SetTexture(envData->texture, 1);
			CCustomCarEnvMapPipeline__Env1Xform(&envmat, envData, &envXform.x);
			envXform.z = envData->scaleX / 8.0f;
			envXform.w = envData->scaleY / 8.0f;
			fxpass = 1;
		}else if(hasEnv2){
			fxParams.fxSwitch = 2;
			fxParams.shininess = envData->shininess/255.0f;
			CCustomCarEnvMapPipeline__SetupEnv(atomic, carfx_env2Frame, &carfx_env2Inv, &envmat);
			RwD3D9SetVertexShaderConstant(REG_envmat, &envmat, 3);
			RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);	// is this needed?
			RwD3D9SetTexture(envData->texture, 1);
			atmEnvData = CCustomCarEnvMapPipeline__AllocEnvMapPipeAtomicData(atomic);
			CCustomCarEnvMapPipeline__Env2Xform(atomic, &envmat, envData, atmEnvData, &envXform.x);
			envXform.z = envData->scaleX / 8.0f;
			envXform.w = envData->scaleY / 8.0f;
			fxpass = 1;
		}

		if(hasSpec){
			fxParams.specularity = specData->specularity;
			RwD3D9SetTexture(specData->texture, 2);
			fxpass = 1;
		}

		if(fxpass && instancedData->material->color.alpha > 0){
			RwD3D9SetVertexShader(ps2CarFxVS);
			RwD3D9SetPixelShader(ps2EnvSpecFxPS);

			RwD3D9SetVertexShaderConstant(REG_fxParams, &fxParams, 1);
			RwD3D9SetVertexShaderConstant(REG_envXform, &envXform, 1);

			RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONALWAYS);
			RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
			RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
			RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
			RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
			RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)0);
			D3D9Render(resEntryHeader, instancedData);
			RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)fog);
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

	int alphafunc, alpharef;
	int src, dst;
	int fog;
	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &alpharef);
	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &alphafunc);
	RwRenderStateGet(rwRENDERSTATESRCBLEND, &src);
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &dst);
	RwRenderStateGet(rwRENDERSTATEFOGENABLE, &fog);

	noFx = CVisibilityPlugins__GetAtomicId(atomic) & 0x6000;
	fxParams.lightmult = CCustomCarEnvMapPipeline__m_EnvMapLightingMult;
	RwMatrix *camfrm = RwFrameGetLTM(RwCameraGetFrame(Camera));
	RwV3dTransformPoint(&eye, RwMatrixGetPos(camfrm), &lightmat);
	RwD3D9SetVertexShaderConstant(REG_eye, &eye, 1);

	for(; numMeshes--; instancedData++){
		material = instancedData->material;
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
		hasSpec  = !!(materialFlags & 4);
		if(RpMatFXMaterialGetEffects(material) != rpMATFXEFFECTENVMAP){
			hasEnv1 = false;
			hasEnv2 = false;
			hasSpec = false;
		}

		int fxpass = 0;
		fxParams.shininess = 0.0f;
		fxParams.specularity = 0.0f;
		envData = *RWPLUGINOFFSET(CustomEnvMapPipeMaterialData*, material, CCustomCarEnvMapPipeline__ms_envMapPluginOffset);
		specData = *RWPLUGINOFFSET(CustomSpecMapPipeMaterialData*, material, CCustomCarEnvMapPipeline__ms_specularMapPluginOffset);
		if(hasEnv1){
			fxParams.fxSwitch = 1;
			fxParams.shininess = envData->shininess/255.0f;
			CCustomCarEnvMapPipeline__SetupEnv(atomic, carfx_env1Frame, &carfx_env1Inv, &envmat);
			RwD3D9SetVertexShaderConstant(REG_envmat, &envmat, 3);
			RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);	// is this needed?
			RwD3D9SetTexture(envData->texture, 1);
			CCustomCarEnvMapPipeline__Env1Xform(&envmat, envData, &envXform.x);
			envXform.z = envData->scaleX / 8.0f;
			envXform.w = envData->scaleY / 8.0f;
			fxpass = 1;
		}else if(hasEnv2){
			fxParams.fxSwitch = 2;
			fxParams.shininess = envData->shininess/255.0f;
			CCustomCarEnvMapPipeline__SetupEnv(atomic, carfx_env2Frame, &carfx_env2Inv, &envmat);
			RwD3D9SetVertexShaderConstant(REG_envmat, &envmat, 3);
			RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);	// is this needed?
			RwD3D9SetTexture(envData->texture, 1);
			atmEnvData = CCustomCarEnvMapPipeline__AllocEnvMapPipeAtomicData(atomic);
			CCustomCarEnvMapPipeline__Env2Xform(atomic, &envmat, envData, atmEnvData, &envXform.x);
			envXform.z = envData->scaleX / 8.0f;
			envXform.w = envData->scaleY / 8.0f;
			fxpass = 1;
		}

		if(hasSpec){
			fxParams.specularity = specData->specularity;
			fxpass = 1;
		}

		if(fxpass && instancedData->material->color.alpha > 0){
			RwD3D9SetVertexShader(specCarFxVS);
			RwD3D9SetPixelShader(specCarFxPS);

			RwD3D9SetVertexShaderConstant(REG_fxParams, &fxParams, 1);
			RwD3D9SetVertexShaderConstant(REG_envXform, &envXform, 1);

			RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONALWAYS);
			RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
			RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
			RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
			RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
			RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)0);
			D3D9Render(resEntryHeader, instancedData);
			RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)fog);
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

// TODO: reverse and implement this better

void
CCustomCarEnvMapPipeline__CustomPipeRenderCB_Xbox(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	RxD3D9ResEntryHeader *resEntryHeader;
	RxD3D9InstanceData *instancedData;
	RpAtomic *atomic;
	RwBool lighting, notLit;
	RwInt32	numMeshes;
	RwBool noFx;
	CustomEnvMapPipeMaterialData *envData;
	CustomEnvMapPipeAtomicData *atmEnvData;
	RpMaterial *material;
	RwUInt32 materialFlags;
	RwBool hasEnv1, hasEnv2, hasSpec, hasAlpha;
	D3DMATRIX worldMat, worldITMat, viewMat, projMat;
	float envSwitch;
	struct {
		float shininess;
		float specularity;
		float intensity;
	} reflData;

	atomic = (RpAtomic*)object;
	noFx = CVisibilityPlugins__GetAtomicId(atomic) & 0x6000;
	notLit = !((pDirect->object.object.flags & 1) == 0 ||
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

	RwMatrix *camfrm = RwFrameGetLTM(RwCameraGetFrame(Camera));
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

	while(numMeshes--){
		envSwitch = 1;
		material = instancedData->material;
		materialFlags = *(RwUInt32*)&material->surfaceProps.specular;

		hasEnv1  = !((materialFlags & 1) == 0 || noFx);
		hasEnv2   = !((materialFlags & 2) == 0 || noFx || (atomic->geometry->flags & rpGEOMETRYTEXTURED2) == 0);
		hasSpec  = !((materialFlags & 4) == 0 || !lighting);
		hasAlpha = instancedData->vertexAlpha || instancedData->material->color.alpha != 255;

		RwD3D9SetTexture(NULL, 1);

		reflData.specularity = 0.0f;	// R* does this before the loop :/
		if(hasSpec && !notLit){
			envSwitch = 4;
			reflData.specularity = CCustomCarEnvMapPipeline__m_EnvMapLightingMult * 1.8f;
			if(reflData.specularity > 1.0f) reflData.specularity = 1.0f;
		}

		envData = *RWPLUGINOFFSET(CustomEnvMapPipeMaterialData*, material, CCustomCarEnvMapPipeline__ms_envMapPluginOffset);

		if(notLit){
			if(hasEnv2)
				envSwitch = 3;
		}else if(hasEnv1){
			envSwitch = 5;
			if(!hasSpec) envSwitch = 2;
			static D3DMATRIX texMat;
			RwV3d transVec;
			RwInt32 tfactor;

			RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
			texMat._11 = envData->scaleX / 8.0f;
			texMat._22 = envData->scaleY / 8.0f;
			texMat._33 = 1.0f;
			texMat._44 = 1.0f;
			GetTransScaleVector(envData, atomic, &transVec);
			texMat._31 = transVec.x;
			texMat._32 = transVec.y;
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
			RwV3d transVec;
			RwInt32 tfactor;

			RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
			atmEnvData = CCustomCarEnvMapPipeline__AllocEnvMapPipeAtomicData(atomic);
			GetEnvMapVector(atomic, atmEnvData, envData, &transVec);
			texMat._11 = 1.0f;
			texMat._22 = 1.0f;
			texMat._33 = 1.0f;
			texMat._44 = 1.0f;
			texMat._31 = transVec.x;
			texMat._32 = transVec.y;
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

		RwD3D9SetVertexShaderConstant(LOC_reflData, (void*)&reflData, 1);
		RwD3D9SetVertexShaderConstant(LOC_envSwitch, (void*)&envSwitch, 1);

		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)hasAlpha);
		RwRGBA matColor = material->color;
		{
			// WTF is this?
			int rgb;
			rgb = *(int*)&matColor & 0xFFFFFF;
			if(rgb > 0xAF00FF){
				if(rgb == 0xC8FF00 || rgb == 0xFF00FF || rgb == 0xFFFF00){
					matColor.red = 0;
					matColor.green = 0;
					matColor.blue = 0;
				}
			}else if (rgb == 0xAF00FF ||
			          rgb == 0x003CFF || rgb == 0x007300 || rgb == 0x004F3D || rgb == 0x004FBA){
				matColor.red = 0;
				matColor.green = 0;
				matColor.blue = 0;
			}
		}
		RwRGBAReal color;
		RwRGBARealFromRwRGBA(&color, &matColor);
		RwD3D9SetVertexShaderConstant(LOC_matCol, (void*)&color, 1);
		RwD3D9SetVertexShaderConstant(LOC_surfProps, (void*)&material->surfaceProps, 1);
		RwD3D9GetRenderState(D3DRS_ALPHABLENDENABLE, &hasAlpha);

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

		instancedData++;
	}
	RwD3D9SetVertexShader(NULL);
	RwD3D9SetPixelShader(NULL);
	RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
}

void
CCustomCarEnvMapPipeline__CustomPipeRenderCB_Switch(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	switch(config->vehiclePipe){
	case 0:
		CCustomCarEnvMapPipeline__CustomPipeRenderCB_PS2(repEntry, object, type, flags);
		return;
	case 1:
		CCustomCarEnvMapPipeline__CustomPipeRenderCB_exe(repEntry, object, type, flags);
		return;
	case 2:
		CCustomCarEnvMapPipeline__CustomPipeRenderCB_Xbox(repEntry, object, type, flags);
		return;
	case 3:
		CCustomCarEnvMapPipeline__CustomPipeRenderCB_Specular(repEntry, object, type, flags);
		return;
	case 4:
		if(iCanHasNeoCar)
			CarPipe::RenderCallback(repEntry, object, type, flags);
		return;
	}
}

void
setVehiclePipeCB(RxPipelineNode *node, RxD3D9AllInOneRenderCallBack callback)
{
	CCustomCarEnvMapPipeline__Init();
	RxD3D9AllInOneSetRenderCallBack(node, CCustomCarEnvMapPipeline__CustomPipeRenderCB_Switch);
}

void
hookVehiclePipe(void)
{
	InjectHook(0x5D9FE9, setVehiclePipeCB);
	InterceptCall(&CCustomCarEnvMapPipeline__PreRenderUpdate_orig, CCustomCarEnvMapPipeline__PreRenderUpdate, 0x5D5B10);
}
