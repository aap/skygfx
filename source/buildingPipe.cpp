#include "skygfx.h"

void *ps2BuildingVS, *ps2BuildingFxVS;
void *pcBuildingVS, *pcBuildingPS;
RxPipeline *buildingPipeline, *buildingDNPipeline;

float &CWeather__WetRoads = *(float*)0xC81308;

enum {
	// common
	REG_transform	= 0,
	REG_ambient	= 4,
	REG_directCol	= 5,	// 7 lights (main + 6 extra)
	REG_directDir	= 12,	//
	REG_matCol	= 19,
	REG_surfProps	= 20,

	REG_shaderParams= 29,
	// DN and UVA
	REG_dayparam	= 30,
	REG_nightparam	= 31,
	REG_texmat	= 32,
	// Env
	REG_fxParams	= 36,
	REG_envXform	= 37,
	REG_envmat	= 38,

};

bool &CWeather__LightningFlash = *(bool*)0xC812CC;
WRAPPER bool CPostEffects__IsVisionFXActive(void) { EAXJMP(0x7034F0); }

RwRGBAReal buildingAmbient;

void (*CustomBuildingPipeline__Update_orig)(void);
void
CustomBuildingPipeline__Update(void)
{
	CustomBuildingPipeline__Update_orig();

	// do *not* use pAmbient light. It causes so many problems
	buildingAmbient.red = CTimeCycle_GetAmbientRed();
	buildingAmbient.green = CTimeCycle_GetAmbientGreen();
	buildingAmbient.blue = CTimeCycle_GetAmbientBlue();

	if(config->lightningIlluminatesWorld && CWeather__LightningFlash && !CPostEffects__IsVisionFXActive())
		buildingAmbient = { 1.0, 1.0, 1.0, 0.0 };
}

void
CustomBuildingEnvMapPipeline__SetupEnv(RpAtomic *atomic, RwFrame *envframe, RwMatrix *envmat)
{
	static RwMatrix lastmat;
	static void *lastobject;
	static RwFrame *lastfrm;
	static RwUInt16 lastrenderframe;
	RwMatrix inv;
	RpClump *clump;
	RwFrame *frame;

	if(envframe == NULL)
		envframe = RwCameraGetFrame(RWSRCGLOBAL(curCamera));

	clump = RpAtomicGetClump(atomic);

	if(lastobject != (clump ? (void*)clump : (void*)atomic) ||
	   lastfrm != envframe ||
	   lastrenderframe != RWSRCGLOBAL(renderFrame)){
		RwMatrixInvert(&inv, RwFrameGetLTM(envframe));
		frame = clump ? RpClumpGetFrame(clump) : RpAtomicGetFrame(atomic);
		RwMatrixMultiply(&lastmat, RwFrameGetLTM(frame), &inv);
		if((rwMatrixGetFlags(&lastmat) & rwMATRIXTYPEMASK) != rwMATRIXTYPEORTHONORMAL)
			RwMatrixOrthoNormalize(&lastmat, &lastmat);

		lastobject = (clump ? (void*)clump : (void*)atomic);
		lastfrm = envframe;
		lastrenderframe = RWSRCGLOBAL(renderFrame);
	}
	*envmat = lastmat;
}

void
CCustomBuildingDNPipeline__CustomPipeRenderCB_PS2(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	RpAtomic *atomic;
	RxD3D9ResEntryHeader *resEntryHeader;
	RxD3D9InstanceData *instancedData;
	RpMaterial *material;
	RwBool lighting;
	RwInt32	numMeshes;
	CustomEnvMapPipeMaterialData *envData;
	RwRGBAReal color;
	float surfProps[4];
	RwBool hasAlpha;
	float colorScale;
	struct {
		float shininess;
		float lightmult;
	} fxParams;
	RwV4d envXform;
	RwMatrix envmat;
	float balance;
	float dayparam[4], nightparam[4];
	float transform[16];

	atomic = (RpAtomic*)object;

	pipeGetComposedTransformMatrix(atomic, transform);
	RwD3D9SetVertexShaderConstant(0, transform, 4);

	RwD3D9GetRenderState(D3DRS_LIGHTING, &lighting);
	resEntryHeader = (RxD3D9ResEntryHeader*)(repEntry + 1);
	instancedData = (RxD3D9InstanceData*)(resEntryHeader + 1);;
	if(resEntryHeader->indexBuffer)
		RwD3D9SetIndices(resEntryHeader->indexBuffer);
	_rwD3D9SetStreams(resEntryHeader->vertexStream, resEntryHeader->useOffsets);
	RwD3D9SetVertexDeclaration(resEntryHeader->vertexDeclaration);

	balance = CCustomBuildingDNPipeline__m_fDNBalanceParam;
	if(balance < 0.0f) balance = 0.0f;
	if(balance > 1.0f) balance = 1.0f;
	dayparam[0] = dayparam[1] = dayparam[2] = 1.0f - balance;
	dayparam[3] = CWeather__WetRoads;
	nightparam[0] = nightparam[1] = nightparam[2] = balance;
	nightparam[3] = 1.0f - CWeather__WetRoads;

	int noExtraColors = atomic->pipeline->pluginData == 0x53F2009C;

	// If no extra colors, force the one we have (night, unintuitively).
	// Night colors are guaranteed to be present by the instance callback.
	if(noExtraColors){
		dayparam[0] = dayparam[1] = dayparam[2] = dayparam[3] = 0.0f;
		nightparam[0] = nightparam[1] = nightparam[2] = nightparam[3] = 1.0f;
	}

	RwD3D9SetVertexShaderConstant(REG_dayparam, dayparam, 1);
	RwD3D9SetVertexShaderConstant(REG_nightparam, nightparam, 1);

	CustomBuildingEnvMapPipeline__SetupEnv(atomic, NULL, &envmat);
	RwD3D9SetVertexShaderConstant(REG_envmat, &envmat, 3);

	int alphafunc, alpharef;
	int src, dst;
	int fog;
	int zwrite;
	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &alpharef);
	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &alphafunc);
	RwRenderStateGet(rwRENDERSTATESRCBLEND, &src);
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &dst);
	RwRenderStateGet(rwRENDERSTATEFOGENABLE, &fog);
	RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &zwrite);

	numMeshes = resEntryHeader->numMeshes;
	while(numMeshes--){
		material = instancedData->material;

		colorScale = 1.0f;
		if(flags & (rxGEOMETRY_TEXTURED2 | rxGEOMETRY_TEXTURED)){
			colorScale = config->ps2ModulateBuilding ? 255.0f/128.0f : 1.0f;
			RwD3D9SetTexture(material->texture ? material->texture : gpWhiteTexture, 0);
		}else
			RwD3D9SetTexture(gpWhiteTexture, 0);
		RwD3D9SetPixelShaderConstant(0, &colorScale, 1);
		RwD3D9SetVertexShaderConstant(REG_shaderParams, &colorScale, 1);

		int effect = RpMatFXMaterialGetEffects(material);
		if(effect == rpMATFXEFFECTUVTRANSFORM){
			RwMatrix *m1, *m2;
			RpMatFXMaterialGetUVTransformMatrices(material, &m1, &m2);
			RwD3D9SetVertexShaderConstant(REG_texmat, m1, 4);
		}else{
			RwMatrix m;
			RwMatrixSetIdentity(&m);
			RwD3D9SetVertexShaderConstant(REG_texmat, &m, 4);
		}

		hasAlpha = instancedData->vertexAlpha || instancedData->material->color.alpha != 255;
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)hasAlpha);

		RwRGBARealFromRwRGBA(&color, &material->color);
		RwD3D9SetVertexShaderConstant(REG_matCol, &color, 1);

		RwD3D9SetVertexShaderConstant(REG_ambient, &buildingAmbient, 1);
		surfProps[0] = lighting || config->buildingPipe == 0 ? material->surfaceProps.ambient : 0.0f;
		RwD3D9SetVertexShaderConstant(REG_surfProps, &surfProps, 1);

		RwD3D9SetVertexShader(ps2BuildingVS);
		RwD3D9SetPixelShader(simplePS);

		D3D9RenderDual(config->dualPassBuilding, resEntryHeader, instancedData);

		// Reflection

		if(*(int*)&material->surfaceProps.specular & 1){
			envData = *RWPLUGINOFFSET(CustomEnvMapPipeMaterialData*, material, CCustomCarEnvMapPipeline__ms_envMapPluginOffset);
			RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
			RwD3D9SetTexture(envData->texture, 1);
			fxParams.shininess = envData->shininess/255.0f;
			fxParams.lightmult = 1.0;
			envXform.x = 0.0;
			envXform.y = 0.0;
			envXform.z = envData->scaleX / 8.0f;
			envXform.w = envData->scaleY / 8.0f;
			RwD3D9SetVertexShaderConstant(REG_envXform, &envXform, 1);
			RwD3D9SetVertexShaderConstant(REG_fxParams, &fxParams, 1);

			RwD3D9SetVertexShader(ps2BuildingFxVS);
			RwD3D9SetPixelShader(ps2EnvSpecFxPS);

			RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONALWAYS);
			RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
			RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
			RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
			RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
			RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)0);
			D3D9Render(resEntryHeader, instancedData);
			RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)fog);
			RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)zwrite);
			RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)src);
			RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)dst);
			RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)alphafunc);
		}

		instancedData++;
	}
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)alpharef);
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)alphafunc);
	RwD3D9SetTexture(NULL, 1);
	RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
}

void
CCustomBuildingDNPipeline__CustomPipeRenderCB_PC(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	RpAtomic *atomic;
	RxD3D9ResEntryHeader *resEntryHeader;
	RxD3D9InstanceData *instancedData;
	RpMaterial *material;
	RwBool lighting;
	RwInt32	numMeshes;
	CustomEnvMapPipeMaterialData *envData;
	RwRGBAReal color;
	float surfProps[4];
	RwBool hasAlpha;
	float colorScale;
	RwMatrix envmat;
	float balance;
	float dayparam[4], nightparam[4];
	float transform[16];

	atomic = (RpAtomic*)object;

	colorScale = 1.0f;
	RwD3D9SetPixelShaderConstant(0, &colorScale, 1);

	pipeGetComposedTransformMatrix(atomic, transform);
	RwD3D9SetVertexShaderConstant(0, transform, 4);

	RwD3D9GetRenderState(D3DRS_LIGHTING, &lighting);
	resEntryHeader = (RxD3D9ResEntryHeader*)(repEntry + 1);
	instancedData = (RxD3D9InstanceData*)(resEntryHeader + 1);;
	if(resEntryHeader->indexBuffer)
		RwD3D9SetIndices(resEntryHeader->indexBuffer);
	_rwD3D9SetStreams(resEntryHeader->vertexStream, resEntryHeader->useOffsets);
	RwD3D9SetVertexDeclaration(resEntryHeader->vertexDeclaration);

	balance = CCustomBuildingDNPipeline__m_fDNBalanceParam;
	if(balance < 0.0f) balance = 0.0f;
	if(balance > 1.0f) balance = 1.0f;
	dayparam[0] = dayparam[1] = dayparam[2] = 1.0f - balance;
	dayparam[3] = CWeather__WetRoads;
	nightparam[0] = nightparam[1] = nightparam[2] = balance;
	nightparam[3] = 1.0f - CWeather__WetRoads;

	int noExtraColors = atomic->pipeline->pluginData == 0x53F2009C;

	// If no extra colors, force the one we have (night, unintuitively).
	// Night colors are guaranteed to be present by the instance callback.
	if(noExtraColors){
		dayparam[0] = dayparam[1] = dayparam[2] = dayparam[3] = 0.0f;
		nightparam[0] = nightparam[1] = nightparam[2] = nightparam[3] = 1.0f;
	}

	RwD3D9SetVertexShaderConstant(REG_dayparam, dayparam, 1);
	RwD3D9SetVertexShaderConstant(REG_nightparam, nightparam, 1);

	CustomBuildingEnvMapPipeline__SetupEnv(atomic, NULL, &envmat);
	RwD3D9SetVertexShaderConstant(REG_envmat, &envmat, 3);

	int alphafunc, alpharef;
	int src, dst;
	int fog;
	int zwrite;
	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &alpharef);
	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &alphafunc);
	RwRenderStateGet(rwRENDERSTATESRCBLEND, &src);
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &dst);
	RwRenderStateGet(rwRENDERSTATEFOGENABLE, &fog);
	RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &zwrite);

	numMeshes = resEntryHeader->numMeshes;
	while(numMeshes--){
		material = instancedData->material;

		if(flags & (rxGEOMETRY_TEXTURED2 | rxGEOMETRY_TEXTURED))
			RwD3D9SetTexture(material->texture ? material->texture : gpWhiteTexture, 0);
		else
			RwD3D9SetTexture(gpWhiteTexture, 0);

		int effect = RpMatFXMaterialGetEffects(material);
		if(effect == rpMATFXEFFECTUVTRANSFORM){
			RwMatrix *m1, *m2;
			RpMatFXMaterialGetUVTransformMatrices(material, &m1, &m2);
			RwD3D9SetVertexShaderConstant(REG_texmat, m1, 4);
		}else{
			RwMatrix m;
			RwMatrixSetIdentity(&m);
			RwD3D9SetVertexShaderConstant(REG_texmat, &m, 4);
		}

		hasAlpha = instancedData->vertexAlpha || instancedData->material->color.alpha != 255;
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)hasAlpha);

		RwRGBARealFromRwRGBA(&color, &material->color);
		RwD3D9SetVertexShaderConstant(REG_matCol, &color, 1);

		RwD3D9SetVertexShaderConstant(REG_ambient, &buildingAmbient, 1);
		surfProps[0] = lighting ? material->surfaceProps.ambient : 0.0f;
		RwD3D9SetVertexShaderConstant(REG_surfProps, &surfProps, 1);

		if(*(int*)&material->surfaceProps.specular & 1){
			envData = *RWPLUGINOFFSET(CustomEnvMapPipeMaterialData*, material, CCustomCarEnvMapPipeline__ms_envMapPluginOffset);
			RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
			RwD3D9SetTexture(envData->texture, 1);
			/* set by default PC callback but not used:
				fxParams.shininess = envData->shininess/255.0f;
				fxParams.lightmult = 1.0f;
				envXform.x = 0.0f;
				envXform.y = 0.0f;
				envXform.z = envData->scaleX / 8.0f;
				envXform.w = envData->scaleY / 8.0f;
				RwD3D9SetVertexShaderConstant(REG_envXform, &envXform, 1);
			*/
			RwD3D9SetPixelShader(pcBuildingPS);
		}else
			RwD3D9SetPixelShader(simplePS);

		RwD3D9SetVertexShader(pcBuildingVS);

		D3D9RenderDual(config->dualPassBuilding, resEntryHeader, instancedData);
/*

		RwD3D9SetVertexShader(pcBuildingVS);
		RwD3D9SetPixelShader(simplePS);

		D3D9RenderDual(config->dualPassBuilding, resEntryHeader, instancedData);

		// Reflection

		if(*(int*)&material->surfaceProps.specular & 1){
			envData = *RWPLUGINOFFSET(CustomEnvMapPipeMaterialData*, material, CCustomCarEnvMapPipeline__ms_envMapPluginOffset);
			RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
			RwD3D9SetTexture(envData->texture, 1);
			fxParams.shininess = envData->shininess/255.0f;
			fxParams.lightmult = 1.0;
			envXform.x = 0.0;
			envXform.y = 0.0;
			envXform.z = envData->scaleX / 8.0f;
			envXform.w = envData->scaleY / 8.0f;
			RwD3D9SetVertexShaderConstant(REG_envXform, &envXform, 1);
			RwD3D9SetVertexShaderConstant(REG_fxParams, &fxParams, 1);

			RwD3D9SetVertexShader(ps2BuildingFxVS);
			RwD3D9SetPixelShader(ps2EnvSpecFxPS);

			RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONALWAYS);
			RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
			RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
			RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
			RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
			RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)0);
			D3D9Render(resEntryHeader, instancedData);
			RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)fog);
			RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)zwrite);
			RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)src);
			RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)dst);
			RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)alphafunc);
		}
*/

		instancedData++;
	}
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)alpharef);
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)alphafunc);
	RwD3D9SetTexture(NULL, 1);
	RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
}

void
CCustomBuildingDNPipeline__CustomPipeRenderCB_Switch(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
//	if(!noExtraColors && GetAsyncKeyState(VK_F8) & 0x8000)
//	if(GetAsyncKeyState(VK_F8) & 0x8000)
//		return;

	if(config->buildingPipe == 1)
		CCustomBuildingDNPipeline__CustomPipeRenderCB_PC(repEntry, object, type, flags);
	else
		CCustomBuildingDNPipeline__CustomPipeRenderCB_PS2(repEntry, object, type, flags);
}

static RwBool
reinstance(void *object, RwResEntry *resEntry, RxD3D9AllInOneInstanceCallBack instanceCallback)
{
	return (instanceCallback == NULL ||
	        instanceCallback(object, (RxD3D9ResEntryHeader*)(resEntry + 1), true) != 0);
}

bool
isNightColorZero(RwRGBA *rgbac, int nv)
{
	RwUInt32 *c = (RwUInt32*)rgbac;
	while(nv--)
		if(*c++)
			return false;
	return true;
}

RwRGBA*
getVertexColors(RpGeometry *geometry)
{
	RwRGBA *day = *RWPLUGINOFFSET(RwRGBA*, geometry, CCustomBuildingDNPipeline__ms_extraVertColourPluginOffset + 4);
	if(day == NULL)
		return geometry->preLitLum;
	return day;
}

RwRGBA*
getExtraColors(RpGeometry *geometry)
{
	RwRGBA *night = *RWPLUGINOFFSET(RwRGBA*, geometry, CCustomBuildingDNPipeline__ms_extraVertColourPluginOffset);
	if(night && isNightColorZero(night, geometry->numVertices))
		return NULL;
	return night;
//	if(night == NULL || isNightColorZero(night, geometry->numVertices))
//		night = day;
}

void
instWhite(RwUInt8 *mem, RwInt32 numVerts, RwUInt32 stride)
{
	while(numVerts--){
		*(D3DCOLOR*)mem = 0xFFFFFFFF;
		mem += stride;
	}
}

RwBool
DNInstance_PS2(void *object, RxD3D9ResEntryHeader *resEntryHeader, RwBool reinstance)
{
	RpAtomic *atomic;
	RpGeometry *geometry;
	RpMorphTarget *morphTarget;
	RwBool isTextured, isPrelit, hasNormals;
	D3DVERTEXELEMENT9 dcl[6];
	void *vertexBuffer;
	RwUInt16 stride;
	RxD3D9InstanceData *instData;
	IDirect3DVertexBuffer9 *vertBuffer;
	int i, j;
	int numMeshes;
	void *vertexData;
	int lastLocked;
	RwRGBA *day, *night;

	atomic = (RpAtomic*)object;
	geometry = atomic->geometry;
	isTextured = (geometry->flags & (rpGEOMETRYTEXTURED|rpGEOMETRYTEXTURED2)) != 0;
	morphTarget = geometry->morphTarget;
	isPrelit = geometry->flags & rpGEOMETRYPRELIT;
	hasNormals = geometry->flags & rpGEOMETRYNORMALS;
	day = getVertexColors(geometry);
	night = getExtraColors(geometry);
	if(reinstance){
		IDirect3DVertexDeclaration9 *vertDecl = (IDirect3DVertexDeclaration9*)resEntryHeader->vertexDeclaration;
		vertDecl->GetDeclaration(dcl, (UINT*)&i);
	}else{
		resEntryHeader->totalNumVertex = geometry->numVertices;
		vertexBuffer = resEntryHeader->vertexStream[0].vertexBuffer;
		if(vertexBuffer){
			RwD3D9DestroyVertexBuffer(resEntryHeader->vertexStream[0].stride,
			                          geometry->numVertices * resEntryHeader->vertexStream[0].stride,
			                          vertexBuffer,
			                          resEntryHeader->vertexStream[0].offset);
			resEntryHeader->vertexStream[0].vertexBuffer = NULL;
		}
		resEntryHeader->vertexStream[0].offset = 0;
		resEntryHeader->vertexStream[0].managed = 0;
		resEntryHeader->vertexStream[0].dynamicLock = 0;
		dcl[0] = {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0};
		i = 1;
		stride = 12;
		resEntryHeader->vertexStream[0].stride = stride;
		resEntryHeader->vertexStream[0].geometryFlags = rpGEOMETRYLOCKVERTICES;
		if(isTextured){
			dcl[i++] = {0, stride, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0};
			stride += 8;
			resEntryHeader->vertexStream[0].stride = stride;
			resEntryHeader->vertexStream[0].geometryFlags |= rpGEOMETRYLOCKTEXCOORDS;
		}
		if(hasNormals){
			dcl[i++] = {0, stride, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0};
			stride += 12;
			resEntryHeader->vertexStream[0].stride = stride;
			resEntryHeader->vertexStream[0].geometryFlags |= rpGEOMETRYLOCKNORMALS;
		}
		if(isPrelit){
			dcl[i++] = {0, stride, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0};
			stride += 4;
			dcl[i++] = {0, stride, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 1};
			stride += 4;
			resEntryHeader->vertexStream[0].stride = stride;
			resEntryHeader->vertexStream[0].geometryFlags |= rpGEOMETRYLOCKPRELIGHT;
		}else{
			// we need vertex colors in the shader so force white like on PS2, one set is enough
			dcl[i++] = {0, stride, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0};
			stride += 4;
			resEntryHeader->vertexStream[0].stride = stride;
			resEntryHeader->vertexStream[0].geometryFlags |= rpGEOMETRYLOCKPRELIGHT;
		}
		dcl[i] = D3DDECL_END();

		if(!RwD3D9CreateVertexDeclaration(dcl, &resEntryHeader->vertexDeclaration))
			return 0;
		resEntryHeader->vertexStream[0].managed = 1;
		if(!RwD3D9CreateVertexBuffer(resEntryHeader->vertexStream[0].stride,
		                             resEntryHeader->vertexStream[0].stride * resEntryHeader->totalNumVertex,
		                             (void**)resEntryHeader->vertexStream,
		                             &resEntryHeader->vertexStream[0].offset))
			return 0;

		numMeshes = resEntryHeader->numMeshes;
		instData = (RxD3D9InstanceData*)(resEntryHeader+1);
		while(numMeshes--){
			instData->baseIndex += instData->minVert + resEntryHeader->vertexStream[0].offset/resEntryHeader->vertexStream[0].stride;
			instData++;
		}
	}

	vertBuffer = (IDirect3DVertexBuffer9*)resEntryHeader->vertexStream[0].vertexBuffer;
	lastLocked = reinstance ? geometry->lockedSinceLastInst : rpGEOMETRYLOCKALL;
	if(lastLocked == rpGEOMETRYLOCKALL || resEntryHeader->vertexStream[0].geometryFlags & lastLocked){
		vertBuffer->Lock(resEntryHeader->vertexStream[0].offset,
		                 resEntryHeader->totalNumVertex * resEntryHeader->vertexStream[0].stride,
		                 &vertexData, D3DLOCK_NOSYSLOCK);
		if(vertexData){
			if(lastLocked & rpGEOMETRYLOCKVERTICES){
				for(i = 0; dcl[i].Usage != D3DDECLUSAGE_POSITION || dcl[i].UsageIndex != 0; ++i)
					;
				_rpD3D9VertexDeclarationInstV3d(dcl[i].Type, (RwUInt8*)vertexData + dcl[i].Offset,
					morphTarget->verts,
					resEntryHeader->totalNumVertex,
					resEntryHeader->vertexStream[dcl[i].Stream].stride);
			}
			if(isTextured && (lastLocked & rpGEOMETRYLOCKTEXCOORDS)){
				for(i = 0; dcl[i].Usage != D3DDECLUSAGE_TEXCOORD || dcl[i].UsageIndex != 0; ++i)
					;
				_rpD3D9VertexDeclarationInstV2d(dcl[i].Type, (RwUInt8*)vertexData + dcl[i].Offset,
					(RwV2d*)geometry->texCoords[0],
					resEntryHeader->totalNumVertex,
					resEntryHeader->vertexStream[dcl[i].Stream].stride);
			}
			if(hasNormals && (lastLocked & rpGEOMETRYLOCKNORMALS)){
				for(i = 0; dcl[i].Usage != D3DDECLUSAGE_NORMAL || dcl[i].UsageIndex != 0; ++i)
					    ;
				_rpD3D9VertexDeclarationInstV3d(dcl[i].Type, (RwUInt8*)vertexData + dcl[i].Offset,
					morphTarget->normals,
					resEntryHeader->totalNumVertex,
					resEntryHeader->vertexStream[dcl[i].Stream].stride);
			}
			if(isPrelit && (lastLocked & rpGEOMETRYLOCKPRELIGHT)){
				for(i = 0; dcl[i].Usage != D3DDECLUSAGE_COLOR || dcl[i].UsageIndex != 0; ++i)
					;
				for(j = 0; dcl[j].Usage != D3DDECLUSAGE_COLOR || dcl[j].UsageIndex != 1; ++j)
					;
//				RwRGBA *night = *RWPLUGINOFFSET(RwRGBA*, geometry, CCustomBuildingDNPipeline__ms_extraVertColourPluginOffset);
//				RwRGBA *day = *RWPLUGINOFFSET(RwRGBA*, geometry, CCustomBuildingDNPipeline__ms_extraVertColourPluginOffset + 4);
//				if(night == NULL || isNightColorZero(night, geometry->numVertices))
//					night = day;
				// If no extra colors, use regular colors
				// TODO: only instance one set in this case
				if(night == NULL)
					night = day;
				assert(day);
				assert(night);
				numMeshes = resEntryHeader->numMeshes;
				instData = (RxD3D9InstanceData*)(resEntryHeader+1);
				while(numMeshes--){
					instData->vertexAlpha = _rpD3D9VertexDeclarationInstColor(
					          (RwUInt8*)vertexData + dcl[i].Offset + resEntryHeader->vertexStream[dcl[i].Stream].stride*instData->minVert,
					          night + instData->minVert,
					          instData->numVertices,
					          resEntryHeader->vertexStream[dcl[i].Stream].stride);
					instData->vertexAlpha |= _rpD3D9VertexDeclarationInstColor(
					          (RwUInt8*)vertexData + dcl[j].Offset + resEntryHeader->vertexStream[dcl[j].Stream].stride*instData->minVert,
					          day + instData->minVert,
					          instData->numVertices,
					          resEntryHeader->vertexStream[dcl[j].Stream].stride);
					instData++;
				}
			}else if(lastLocked & rpGEOMETRYLOCKPRELIGHT){
				for(i = 0; dcl[i].Usage != D3DDECLUSAGE_COLOR || dcl[i].UsageIndex != 0; ++i)
					;
				numMeshes = resEntryHeader->numMeshes;
				instData = (RxD3D9InstanceData*)(resEntryHeader+1);
				while(numMeshes--){
					instData->vertexAlpha = 0;
					instWhite((RwUInt8*)vertexData + dcl[i].Offset + resEntryHeader->vertexStream[dcl[i].Stream].stride*instData->minVert,
					          instData->numVertices,
					          resEntryHeader->vertexStream[dcl[i].Stream].stride);
					instData++;
				}
			}
		}

		if(vertexData)
			vertBuffer->Unlock();

		RwRGBA **night = RWPLUGINOFFSET(RwRGBA*, geometry, CCustomBuildingDNPipeline__ms_extraVertColourPluginOffset);
		RwRGBA **day = RWPLUGINOFFSET(RwRGBA*, geometry, CCustomBuildingDNPipeline__ms_extraVertColourPluginOffset + 4);
		if(*night != nullptr){
			GTAfree(*night);
			*night = nullptr;
		}
		if(*day != nullptr){
			GTAfree(*day);
			*day = nullptr;
		}
	}
	return 1;
}

RxPipeline*
CCustomBuildingPipeline__CreateCustomObjPipe_PS2(void)
{
	RxPipeline *pipeline;
	RxLockedPipe *lockedpipe;
	RxPipelineNode *node;
	RxNodeDefinition *instanceNode;

	pipeline = RxPipelineCreate();
	instanceNode = RxNodeDefinitionGetD3D9AtomicAllInOne();
	if(pipeline == NULL)
		return NULL;
	lockedpipe = RxPipelineLock(pipeline);
	if(lockedpipe == NULL ||
	   RxLockedPipeAddFragment(lockedpipe, NULL, instanceNode, NULL) == NULL ||
	   RxLockedPipeUnlock(lockedpipe) == NULL){
		RxPipelineDestroy(pipeline);
		return NULL;
	}
	node = RxPipelineFindNodeByName(pipeline, instanceNode->name, NULL, NULL);
	RxD3D9AllInOneSetInstanceCallBack(node, DNInstance_PS2);
	RxD3D9AllInOneSetReinstanceCallBack(node, reinstance);
	RxD3D9AllInOneSetRenderCallBack(node, CCustomBuildingDNPipeline__CustomPipeRenderCB_Switch);

	CreateShaders();

	pipeline->pluginId = 0x53F2009C;
	pipeline->pluginData = 0x53F2009C;
	buildingPipeline = pipeline;
	return pipeline;
}

RxPipeline*
CCustomBuildingDNPipeline__CreateCustomObjPipe_PS2(void)
{
	RxPipeline *pipeline;
	RxLockedPipe *lockedpipe;
	RxPipelineNode *node;
	RxNodeDefinition *instanceNode;

	pipeline = RxPipelineCreate();
	instanceNode = RxNodeDefinitionGetD3D9AtomicAllInOne();
	if(pipeline == NULL)
		return NULL;
	lockedpipe = RxPipelineLock(pipeline);
	if(lockedpipe == NULL ||
	   RxLockedPipeAddFragment(lockedpipe, NULL, instanceNode, NULL) == NULL ||
	   RxLockedPipeUnlock(lockedpipe) == NULL){
		RxPipelineDestroy(pipeline);
		return NULL;
	}
	node = RxPipelineFindNodeByName(pipeline, instanceNode->name, NULL, NULL);
	RxD3D9AllInOneSetInstanceCallBack(node, DNInstance_PS2);
	RxD3D9AllInOneSetReinstanceCallBack(node, reinstance);
	RxD3D9AllInOneSetRenderCallBack(node, CCustomBuildingDNPipeline__CustomPipeRenderCB_Switch);

	CreateShaders();

	pipeline->pluginId = 0x53F20098;
	pipeline->pluginData = 0x53F20098;
	buildingDNPipeline = pipeline;
	return pipeline;
}

void
hookBuildingPipe(void)
{
	InterceptCall(&CustomBuildingPipeline__Update_orig, CustomBuildingPipeline__Update, 0x53C15E);
	InjectHook(0x5D7100, CCustomBuildingDNPipeline__CreateCustomObjPipe_PS2);
	InjectHook(0x5D7D90, CCustomBuildingPipeline__CreateCustomObjPipe_PS2);
	Patch<BYTE>(0x5D7200, 0xC3);	// disable interpolation
}
