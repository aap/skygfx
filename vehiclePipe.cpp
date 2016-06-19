#include "skygfx.h"

static void *vehiclePipeVS, *ps2CarFxVS;
void *vehiclePipePS, *ps2CarFxPS;	// reused by the building pipeline
static void *specCarFxVS, *specCarFxPS;
static void *vcsReflVS, *vcsReflPS;
static void *xboxCarVS;

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

	LOC_activeLights = 0,
};

void CCustomCarEnvMapPipeline__CustomPipeRenderCB_Specular(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags);
void CCustomCarEnvMapPipeline__CustomPipeRenderCB_Xbox(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags);
void CCustomCarEnvMapPipeline__CustomPipeRenderCB_VCS(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags);

CPool<CustomEnvMapPipeAtomicData> *&gEnvMapPipeAtmDataPool = *(CPool<CustomEnvMapPipeAtomicData>**)0xC02D2C;

void*
CustomEnvMapPipeAtomicData::operator new(size_t size)
{
	return gEnvMapPipeAtmDataPool->New();
}

void
CCustomCarEnvMapPipeline__CustomPipeRenderCB_PS2(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	RxD3D9ResEntryHeader *resEntryHeader;
	RxD3D9InstanceData *instancedData;
	RpAtomic *atomic;
	RwBool lighting;
	RwInt32	numMeshes;
	RwBool noRefl;
	CustomEnvMapPipeMaterialData *envData;
	CustomEnvMapPipeAtomicData *atmEnvData;
	CustomSpecMapPipeMaterialData *specData;
	RpMaterial *material;
	RwUInt32 materialFlags;
	RwBool hasRefl, hasEnv, hasSpec, hasAlpha;
	int activeLights;
	struct {
		float shininess;
		float specularity;
		float intensity;
	} reflData;
	RwV4d envXform;
	float envSwitch;
	D3DMATRIX worldMat, worldITMat, viewMat, projMat;
	//RwUInt32 pluginData;

	// set multipass distance. i'm lazy, just do it here
	float *mpd = (float*)0xC88044;
	*mpd = 100000000.0f;

	if(config->vehiclePipe == 3){
		CCustomCarEnvMapPipeline__CustomPipeRenderCB_VCS(repEntry, object, type, flags);
		return;
	}
	if(config->vehiclePipe == 4){
		CCustomCarEnvMapPipeline__CustomPipeRenderCB_Xbox(repEntry, object, type, flags);
		return;
	}
	if(config->vehiclePipe == 5){
		CCustomCarEnvMapPipeline__CustomPipeRenderCB_Specular(repEntry, object, type, flags);
		return;
	}

	if(config->vehiclePipe > 1){
		CCustomCarEnvMapPipeline__CustomPipeRenderCB_exe(repEntry, object, type, flags);
		return;
	}

	atomic = (RpAtomic*)object;

	float colorscale = 1.0f;
	RwD3D9SetPixelShaderConstant(0, &colorscale, 1);

	RwD3D9GetTransform(D3DTS_WORLD, &worldMat);
	RwD3D9GetTransform(D3DTS_VIEW, &viewMat);
	RwD3D9GetTransform(D3DTS_PROJECTION, &projMat);
	RwD3D9SetVertexShaderConstant(LOC_World,(void*)&worldMat,4);
	RwD3D9SetVertexShaderConstant(LOC_View,(void*)&viewMat,4);
	RwD3D9SetVertexShaderConstant(LOC_Proj,(void*)&projMat,4);
	_rwD3D9VSSetActiveWorldMatrix(RwFrameGetLTM(RpAtomicGetFrame(atomic)));
	_rwD3D9VSGetInverseWorldMatrix((void *)&worldITMat);
	RwD3D9SetVertexShaderConstant(LOC_WorldIT,(void*)&worldITMat,4);

	noRefl = CVisibilityPlugins__GetAtomicId(atomic) & 0x6000;
	// lights
	activeLights = 0;
	if(CVisibilityPlugins__GetAtomicId(atomic) & 0x4000){	// exploded
		RwRGBAReal c = { 0.0f, 0.0f, 0.0f, 0.0f };
		RwD3D9SetVertexShaderConstant(LOC_sunDiff,(void*)&c,1);
		c = { 0.18f, 0.18f, 0.18f, 0.0f };
		RwD3D9SetVertexShaderConstant(LOC_sunAmb,(void*)&c,1);
	}else{
		RwD3D9SetVertexShaderConstant(LOC_sunDir,(void*)RwMatrixGetAt(RwFrameGetLTM(RpLightGetFrame(pDirect))),1);
		RwD3D9SetVertexShaderConstant(LOC_sunDiff,(void*)&pDirect->color,1);
		RwD3D9SetVertexShaderConstant(LOC_sunAmb,(void*)&pAmbient->color,1);
		reflData.intensity = pDirect->color.red*(256.0f/255.0f);
		for(int i = 0; i < NumExtraDirLightsInWorld; i++)
			if(RpLightGetType(pExtraDirectionals[i]) == rpLIGHTDIRECTIONAL){
				RwD3D9SetVertexShaderConstant(LOC_lights+i*2, (void*)RwMatrixGetAt(RwFrameGetLTM(RpLightGetFrame(pExtraDirectionals[i]))),1);
				RwD3D9SetVertexShaderConstant(LOC_lights+i*2+1, (void*)&pExtraDirectionals[i]->color, 1);
				activeLights++;
			}
	}
	IDirect3DDevice9_SetVertexShaderConstantI(d3d9device, LOC_activeLights, &activeLights, 1);

	RwD3D9GetRenderState(D3DRS_LIGHTING, &lighting);

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

	while(numMeshes--){
		material = instancedData->material;
		if(flags & (rxGEOMETRY_TEXTURED2 | rxGEOMETRY_TEXTURED))
			RwD3D9SetTexture(material->texture ? material->texture : gpWhiteTexture, 0);
		else
			RwD3D9SetTexture(gpWhiteTexture, 0);

		hasAlpha = instancedData->vertexAlpha || instancedData->material->color.alpha != 255;
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)hasAlpha);

		RwRGBA matColor = material->color;
		float surfProps[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		if(lighting || flags & rpGEOMETRYPRELIT){
			// WTF is this?
			int rgb;
			rgb = *(int*)&matColor & 0xFFFFFF;
			if(rgb == 0xAF00FF || rgb == 0x00FFB9 || rgb == 0x00FF3C || rgb == 0x003CFF ||
			   rgb == 0x00AFFF || rgb == 0xC8FF00 || rgb == 0xFF00FF || rgb == 0xFFFF00){
				matColor.red = 0;
				matColor.green = 0;
				matColor.blue = 0;
			}

			surfProps[0] = material->surfaceProps.ambient;
			surfProps[2] = material->surfaceProps.diffuse;
			surfProps[3] = !!(flags & rpGEOMETRYPRELIT);
		}
		RwRGBAReal color;
		RwRGBARealFromRwRGBA(&color, &matColor);
		RwD3D9SetVertexShaderConstant(LOC_matCol, (void*)&color, 1);
		RwD3D9SetVertexShaderConstant(LOC_surfProps, (void*)surfProps, 1);

		RwD3D9SetVertexShader(vehiclePipeVS);
		RwD3D9SetPixelShader(vehiclePipePS);

		D3D9RenderVehicleDual(resEntryHeader, instancedData);

		//
		// FX pass
		//

		materialFlags = *(RwUInt32*)&material->surfaceProps.specular;
		hasRefl  = !((materialFlags & 1) == 0);
		hasEnv   = !((materialFlags & 2) == 0 || (atomic->geometry->flags & rpGEOMETRYTEXTURED2) == 0);
		hasSpec  = !((materialFlags & 4) == 0 || !lighting);
		int matfx = RpMatFXMaterialGetEffects(material);
		if(matfx != rpMATFXEFFECTENVMAP){
			hasSpec = false;
			hasEnv = false;
			hasRefl = false;
		}

		int fxpass = 0;
		reflData.shininess = 0.0f;
		reflData.specularity = 0.0f;
		envData = *RWPLUGINOFFSET(CustomEnvMapPipeMaterialData*, material, CCustomCarEnvMapPipeline__ms_envMapPluginOffset);
		specData = *RWPLUGINOFFSET(CustomSpecMapPipeMaterialData*, material, CCustomCarEnvMapPipeline__ms_specularMapPluginOffset);
		if(hasRefl && !noRefl){
			static D3DMATRIX texMat;
			RwV3d transVec;

			envSwitch = 2 + config->vehiclePipe;
			reflData.shininess = envData->shininess/255.0f;
			// ?
			RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
			texMat._11 = envData->scaleX / 8.0f;
			texMat._22 = envData->scaleY / 8.0f;
			texMat._33 = 1.0f;
			texMat._44 = 1.0f;
			D3D9GetTransScaleVector(envData, atomic, &transVec);
			texMat._31 = transVec.x;
			texMat._32 = transVec.y;
			RwD3D9SetVertexShaderConstant(LOC_Texture,(void*)&texMat,4);
			RwD3D9SetTexture(envData->texture, 1);

			envXform.x = transVec.x;
			envXform.y = transVec.y;
			envXform.z = envData->scaleX / 8.0f;
			envXform.w = envData->scaleY / 8.0f;
			fxpass = 1;
		}else if(hasEnv && !noRefl){
			static D3DMATRIX texMat;
			RwV3d transVec;

			envSwitch = 0 + config->vehiclePipe;
			reflData.shininess = envData->shininess/255.0f;
			// ?
			RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
			atmEnvData = *RWPLUGINOFFSET(CustomEnvMapPipeAtomicData*, atomic,
			                             CCustomCarEnvMapPipeline__ms_envMapAtmPluginOffset);
			if(atmEnvData == NULL){
				atmEnvData = new CustomEnvMapPipeAtomicData;
				atmEnvData->pad1 = 0;
				atmEnvData->pad2 = 0;
				atmEnvData->pad3 = 0;
				*RWPLUGINOFFSET(CustomEnvMapPipeAtomicData*, atomic,
				                CCustomCarEnvMapPipeline__ms_envMapAtmPluginOffset) = atmEnvData;
			}
			D3D9GetEnvMapVector(atomic, atmEnvData, envData, &transVec);
			texMat._11 = 1.0f;
			texMat._22 = 1.0f;
			texMat._33 = 1.0f;
			texMat._44 = 1.0f;
			texMat._31 = transVec.x;
			texMat._32 = transVec.y;
			RwD3D9SetVertexShaderConstant(LOC_Texture,(void*)&texMat,4);
			RwD3D9SetTexture(envData->texture, 1);

			envXform.x = -transVec.x;
			envXform.y = transVec.y;
			envXform.z = envData->scaleX / 8.0f;
			envXform.w = envData->scaleY / 8.0f;
			fxpass = 1;
		}

		if(hasSpec && !noRefl){
			reflData.specularity = specData->specularity;
			RwD3D9SetTexture(specData->texture, 2);
			fxpass = 1;
		}

		if(fxpass){
			RwD3D9SetVertexShader(ps2CarFxVS);
			RwD3D9SetPixelShader(ps2CarFxPS);

			RwD3D9SetVertexShaderConstant(LOC_reflData, (void*)&reflData, 1);
			RwD3D9SetVertexShaderConstant(LOC_envXForm, (void*)&envXform, 1);
			RwD3D9SetVertexShaderConstant(LOC_envSwitch, (void*)&envSwitch, 1);

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

		instancedData++;
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
	RwBool lighting;
	RwInt32	numMeshes;
	RwBool noRefl;
	CustomEnvMapPipeMaterialData *envData;
	CustomEnvMapPipeAtomicData *atmEnvData;
	CustomSpecMapPipeMaterialData *specData;
	RpMaterial *material;
	RwUInt32 materialFlags;
	RwBool hasRefl, hasEnv, hasSpec, hasAlpha;
	int activeLights;
	struct {
		float shininess;
		float specularity;
		float intensity;
	} reflData;
	RwV4d envXform;
	float envSwitch;
	D3DMATRIX worldMat, worldITMat, viewMat, projMat;
	//RwUInt32 pluginData;

	atomic = (RpAtomic*)object;

	float colorscale = 1.0f;
	RwD3D9SetPixelShaderConstant(0, &colorscale, 1);

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

	noRefl = CVisibilityPlugins__GetAtomicId(atomic) & 0x6000;
	// lights
	activeLights = 0;
	if(CVisibilityPlugins__GetAtomicId(atomic) & 0x4000){	// exploded
		RwRGBAReal c = { 0.0f, 0.0f, 0.0f, 0.0f };
		RwD3D9SetVertexShaderConstant(LOC_sunDiff,(void*)&c,1);
		c = { 0.18f, 0.18f, 0.18f, 0.0f };
		RwD3D9SetVertexShaderConstant(LOC_sunAmb,(void*)&c,1);
	}else{
		RwD3D9SetVertexShaderConstant(LOC_sunDir,(void*)RwMatrixGetAt(RwFrameGetLTM(RpLightGetFrame(pDirect))),1);
		RwD3D9SetVertexShaderConstant(LOC_sunDiff,(void*)&pDirect->color,1);
		RwD3D9SetVertexShaderConstant(LOC_sunAmb,(void*)&pAmbient->color,1);
		reflData.intensity = pDirect->color.red*(256.0f/255.0f);
		for(int i = 0; i < NumExtraDirLightsInWorld; i++)
			if(RpLightGetType(pExtraDirectionals[i]) == rpLIGHTDIRECTIONAL){
				RwD3D9SetVertexShaderConstant(LOC_lights+i*2, (void*)RwMatrixGetAt(RwFrameGetLTM(RpLightGetFrame(pExtraDirectionals[i]))),1);
				RwD3D9SetVertexShaderConstant(LOC_lights+i*2+1, (void*)&pExtraDirectionals[i]->color, 1);
				activeLights++;
			}
	}
	IDirect3DDevice9_SetVertexShaderConstantI(d3d9device, LOC_activeLights, &activeLights, 1);

	RwD3D9GetRenderState(D3DRS_LIGHTING, &lighting);

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

	while(numMeshes--){
		material = instancedData->material;
		if(flags & (rxGEOMETRY_TEXTURED2 | rxGEOMETRY_TEXTURED))
			RwD3D9SetTexture(material->texture ? material->texture : gpWhiteTexture, 0);
		else
			RwD3D9SetTexture(gpWhiteTexture, 0);

		hasAlpha = instancedData->vertexAlpha || instancedData->material->color.alpha != 255;
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)hasAlpha);

		RwRGBA matColor = material->color;
		float surfProps[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		if(lighting || flags & rpGEOMETRYPRELIT){
			// WTF is this?
			int rgb;
			rgb = *(int*)&matColor & 0xFFFFFF;
			if(rgb == 0xAF00FF || rgb == 0x00FFB9 || rgb == 0x00FF3C || rgb == 0x003CFF ||
			   rgb == 0x00AFFF || rgb == 0xC8FF00 || rgb == 0xFF00FF || rgb == 0xFFFF00){
				matColor.red = 0;
				matColor.green = 0;
				matColor.blue = 0;
			}

			surfProps[0] = material->surfaceProps.ambient;
			surfProps[2] = material->surfaceProps.diffuse;
			surfProps[3] = !!(flags & rpGEOMETRYPRELIT);
		}
		RwRGBAReal color;
		RwRGBARealFromRwRGBA(&color, &matColor);
		RwD3D9SetVertexShaderConstant(LOC_matCol, (void*)&color, 1);
		RwD3D9SetVertexShaderConstant(LOC_surfProps, (void*)surfProps, 1);

		RwD3D9SetVertexShader(vehiclePipeVS);
		RwD3D9SetPixelShader(vehiclePipePS);

		D3D9RenderVehicleDual(resEntryHeader, instancedData);

		//
		// FX pass
		//

		materialFlags = *(RwUInt32*)&material->surfaceProps.specular;
		hasRefl  = !((materialFlags & 1) == 0);
		hasEnv   = !((materialFlags & 2) == 0 || (atomic->geometry->flags & rpGEOMETRYTEXTURED2) == 0);
		hasSpec  = !((materialFlags & 4) == 0 || !lighting);
		int matfx = RpMatFXMaterialGetEffects(material);
		if(matfx != rpMATFXEFFECTENVMAP){
			hasSpec = false;
			hasEnv = false;
			hasRefl = false;
		}

		int fxpass = 0;
		reflData.shininess = 0.0f;
		reflData.specularity = 0.0f;
		envData = *RWPLUGINOFFSET(CustomEnvMapPipeMaterialData*, material, CCustomCarEnvMapPipeline__ms_envMapPluginOffset);
		specData = *RWPLUGINOFFSET(CustomSpecMapPipeMaterialData*, material, CCustomCarEnvMapPipeline__ms_specularMapPluginOffset);
		if(hasRefl && !noRefl){
			static D3DMATRIX texMat;
			RwV3d transVec;

			envSwitch = 1;
			reflData.shininess = envData->shininess/255.0f;
			// ?
			RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
			texMat._11 = envData->scaleX / 8.0f;
			texMat._22 = envData->scaleY / 8.0f;
			texMat._33 = 1.0f;
			texMat._44 = 1.0f;
			D3D9GetTransScaleVector(envData, atomic, &transVec);
			texMat._31 = transVec.x;
			texMat._32 = transVec.y;
			RwD3D9SetVertexShaderConstant(LOC_Texture,(void*)&texMat,4);
			RwD3D9SetTexture(envData->texture, 1);

			envXform.x = transVec.x;
			envXform.y = transVec.y;
			envXform.z = envData->scaleX / 8.0f;
			envXform.w = envData->scaleY / 8.0f;
			fxpass = 1;
		}else if(hasEnv && !noRefl){
			static D3DMATRIX texMat;
			RwV3d transVec;

			envSwitch = 0;
			reflData.shininess = envData->shininess/255.0f;
			// ?
			RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
			atmEnvData = *RWPLUGINOFFSET(CustomEnvMapPipeAtomicData*, atomic,
			                             CCustomCarEnvMapPipeline__ms_envMapAtmPluginOffset);
			if(atmEnvData == NULL){
				atmEnvData = new CustomEnvMapPipeAtomicData;
				atmEnvData->pad1 = 0;
				atmEnvData->pad2 = 0;
				atmEnvData->pad3 = 0;
				*RWPLUGINOFFSET(CustomEnvMapPipeAtomicData*, atomic,
				                CCustomCarEnvMapPipeline__ms_envMapAtmPluginOffset) = atmEnvData;
			}
			D3D9GetEnvMapVector(atomic, atmEnvData, envData, &transVec);
			texMat._11 = 1.0f;
			texMat._22 = 1.0f;
			texMat._33 = 1.0f;
			texMat._44 = 1.0f;
			texMat._31 = transVec.x;
			texMat._32 = transVec.y;
			RwD3D9SetVertexShaderConstant(LOC_Texture,(void*)&texMat,4);
			RwD3D9SetTexture(envData->texture, 1);

			envXform.x = -transVec.x;
			envXform.y = transVec.y;
			envXform.z = envData->scaleX / 8.0f;
			envXform.w = envData->scaleY / 8.0f;
			fxpass = 1;
		}

		if(hasSpec && !noRefl){
//			reflData.specularity = CCustomCarEnvMapPipeline__m_EnvMapLightingMult * 1.8f;
			reflData.specularity = specData->specularity * 1.8f;
			if(reflData.specularity > 1.0f) reflData.specularity = 1.0f;
			fxpass = 1;
		}

		if(fxpass){
			RwD3D9SetVertexShader(specCarFxVS);
			RwD3D9SetPixelShader(specCarFxPS);

			RwD3D9SetVertexShaderConstant(LOC_reflData, (void*)&reflData, 1);
			RwD3D9SetVertexShaderConstant(LOC_envXForm, (void*)&envXform, 1);
			RwD3D9SetVertexShaderConstant(LOC_envSwitch, (void*)&envSwitch, 1);

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

		instancedData++;
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
CCustomCarEnvMapPipeline__CustomPipeRenderCB_Xbox(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	RxD3D9ResEntryHeader *resEntryHeader;
	RxD3D9InstanceData *instancedData;
	RpAtomic *atomic;
	RwBool lighting, notLit;
	RwInt32	numMeshes;
	RwBool noRefl;
	CustomEnvMapPipeMaterialData *envData;
	CustomEnvMapPipeAtomicData *atmEnvData;
	RpMaterial *material;
	RwUInt32 materialFlags;
	RwBool hasRefl, hasEnv, hasSpec, hasAlpha;
	D3DMATRIX worldMat, worldITMat, viewMat, projMat;
	float envSwitch;
	struct {
		float shininess;
		float specularity;
		float intensity;
	} reflData;

	atomic = (RpAtomic*)object;
	noRefl = CVisibilityPlugins__GetAtomicId(atomic) & 0x6000;
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

		hasRefl  = !((materialFlags & 1) == 0 || noRefl);
		hasEnv   = !((materialFlags & 2) == 0 || noRefl || (atomic->geometry->flags & rpGEOMETRYTEXTURED2) == 0);
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
			if(hasEnv)
				envSwitch = 3;
		}else if(hasRefl){
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
			D3D9GetTransScaleVector(envData, atomic, &transVec);
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
		}else if(hasEnv){
			envSwitch = 6;
			if(!hasSpec) envSwitch = 3;
			static D3DMATRIX texMat;
			RwV3d transVec;
			RwInt32 tfactor;

			RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
			atmEnvData = *RWPLUGINOFFSET(CustomEnvMapPipeAtomicData*, atomic,
			                             CCustomCarEnvMapPipeline__ms_envMapAtmPluginOffset);
			if(atmEnvData == NULL){
				atmEnvData = new CustomEnvMapPipeAtomicData;
				atmEnvData->pad1 = 0;
				atmEnvData->pad2 = 0;
				atmEnvData->pad3 = 0;
				*RWPLUGINOFFSET(CustomEnvMapPipeAtomicData*, atomic,
				                CCustomCarEnvMapPipeline__ms_envMapAtmPluginOffset) = atmEnvData;
			}
			D3D9GetEnvMapVector(atomic, atmEnvData, envData, &transVec);
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
		D3D9RenderVehicleDual(resEntryHeader, instancedData);
	//	D3D9Render(resEntryHeader, instancedData);

		instancedData++;
	}
	RwD3D9SetVertexShader(NULL);
	RwD3D9SetPixelShader(NULL);
	RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
}

RwTexture *vcsReflTex;

void
CCustomCarEnvMapPipeline__CustomPipeRenderCB_VCS(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	RxD3D9ResEntryHeader *resEntryHeader;
	RxD3D9InstanceData *instancedData;
	RpAtomic *atomic;
	RwBool lighting;
	RwInt32	numMeshes;
	RwBool noRefl;
	CustomEnvMapPipeMaterialData *envData;
	RpMaterial *material;
	RwUInt32 materialFlags;
	RwBool hasRefl, hasAlpha;
	int activeLights;
	struct {
		float shininess;
		float specularity;
		float intensity;
	} reflData;
	RwV4d envXform;
	D3DMATRIX worldMat, worldITMat, viewMat, projMat;

	// set multipass distance. i'm lazy, just do it here
	float *mpd = (float*)0xC88044;
	*mpd = 100000000.0f;

	atomic = (RpAtomic*)object;

	// matrices
	RwD3D9SetPixelShader(vcsReflPS);
	RwD3D9SetVertexShader(vcsReflVS);
	RwD3D9GetTransform(D3DTS_WORLD, &worldMat);
	RwD3D9GetTransform(D3DTS_VIEW, &viewMat);
	RwD3D9GetTransform(D3DTS_PROJECTION, &projMat);
	RwD3D9SetVertexShaderConstant(LOC_World,(void*)&worldMat,4);
	RwD3D9SetVertexShaderConstant(LOC_View,(void*)&viewMat,4);
	RwD3D9SetVertexShaderConstant(LOC_Proj,(void*)&projMat,4);
	_rwD3D9VSSetActiveWorldMatrix(RwFrameGetLTM(RpAtomicGetFrame(atomic)));
	_rwD3D9VSGetInverseWorldMatrix((void *)&worldITMat);
	RwD3D9SetVertexShaderConstant(LOC_WorldIT,(void*)&worldITMat,4);

	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
	vcsReflTex->raster = reflTex; //CPostEffects::pRasterFrontBuffer;

	noRefl = CVisibilityPlugins__GetAtomicId(atomic) & 0x6000;
	// lights
	activeLights = 0;
	if(CVisibilityPlugins__GetAtomicId(atomic) & 0x4000){	// exploded
		noRefl = 1;	// force it, probably not necessary, but be sure
		RwRGBAReal c = { 0.0f, 0.0f, 0.0f, 0.0f };
		RwD3D9SetVertexShaderConstant(LOC_sunDiff,(void*)&c,1);
		c = { 0.18f, 0.18f, 0.18f, 0.0f };
		RwD3D9SetVertexShaderConstant(LOC_sunAmb,(void*)&c,1);
	}else{
		RwD3D9SetVertexShaderConstant(LOC_sunDir,(void*)RwMatrixGetAt(RwFrameGetLTM(RpLightGetFrame(pDirect))),1);
		RwD3D9SetVertexShaderConstant(LOC_sunDiff,(void*)&pDirect->color,1);
		RwD3D9SetVertexShaderConstant(LOC_sunAmb,(void*)&pAmbient->color,1);
		reflData.intensity = pDirect->color.red*(256.0f/255.0f);
		for(int i = 0; i < NumExtraDirLightsInWorld; i++)
			if(RpLightGetType(pExtraDirectionals[i]) == rpLIGHTDIRECTIONAL){
				RwD3D9SetVertexShaderConstant(LOC_lights+i*2, (void*)RwMatrixGetAt(RwFrameGetLTM(RpLightGetFrame(pExtraDirectionals[i]))),1);
				RwD3D9SetVertexShaderConstant(LOC_lights+i*2+1, (void*)&pExtraDirectionals[i]->color, 1);
				activeLights++;
			}
	}
	IDirect3DDevice9_SetVertexShaderConstantI(d3d9device, LOC_activeLights, &activeLights, 1);

	RwD3D9GetRenderState(D3DRS_LIGHTING, &lighting);

	resEntryHeader = (RxD3D9ResEntryHeader *)(repEntry + 1);
	instancedData = (RxD3D9InstanceData *)(resEntryHeader + 1);
	if(resEntryHeader->indexBuffer != NULL)
		RwD3D9SetIndices(resEntryHeader->indexBuffer);
	_rwD3D9SetStreams(resEntryHeader->vertexStream,resEntryHeader->useOffsets);
	RwD3D9SetVertexDeclaration(resEntryHeader->vertexDeclaration);
	numMeshes = resEntryHeader->numMeshes;

	while(numMeshes--){
		material = instancedData->material;
		materialFlags = *(RwUInt32*)&material->surfaceProps.specular;

		//hasRefl = materialFlags & 4;
		hasRefl = materialFlags & 3;

		if(flags & (rxGEOMETRY_TEXTURED2 | rxGEOMETRY_TEXTURED))
			RwD3D9SetTexture(material->texture ? material->texture : gpWhiteTexture, 0);
		else
			RwD3D9SetTexture(gpWhiteTexture, 0);

		reflData.shininess = 0.0f;
		RwD3D9SetTexture(NULL, 1);
		envData = *RWPLUGINOFFSET(CustomEnvMapPipeMaterialData*, material, CCustomCarEnvMapPipeline__ms_envMapPluginOffset);
		if(hasRefl && !noRefl){
			// or maybe from matfx?
			reflData.shininess = 0x26/255.0f;
			// ?
			RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
			RwD3D9SetTexture(vcsReflTex, 1);
			envXform.x = 1.0f;
			envXform.y = 1.0f;
			envXform.z = 1.0f;
			envXform.w = 1.0f;
		}

		hasAlpha = instancedData->vertexAlpha || instancedData->material->color.alpha != 255;
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)hasAlpha);
		RwD3D9SetVertexShaderConstant(LOC_reflData, (void*)&reflData, 1);
		RwD3D9SetVertexShaderConstant(LOC_envXForm, (void*)&envXform, 1);

		RwRGBA matColor = material->color;
		float surfProps[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		if(lighting || flags & rpGEOMETRYPRELIT){
			// WTF is this?
			int rgb;
			rgb = *(int*)&matColor & 0xFFFFFF;
			if(rgb == 0xAF00FF || rgb == 0x00FFB9 || rgb == 0x00FF3C || rgb == 0x003CFF ||
			   rgb == 0x00AFFF || rgb == 0xC8FF00 || rgb == 0xFF00FF || rgb == 0xFFFF00){
				matColor.red = 0;
				matColor.green = 0;
				matColor.blue = 0;
			}

			surfProps[0] = material->surfaceProps.ambient;
			surfProps[2] = material->surfaceProps.diffuse;
			surfProps[3] = !!(flags & rpGEOMETRYPRELIT);
		}
		RwRGBAReal color;
		RwRGBARealFromRwRGBA(&color, &matColor);
		RwD3D9SetVertexShaderConstant(LOC_matCol, (void*)&color, 1);
		RwD3D9SetVertexShaderConstant(LOC_surfProps, (void*)surfProps, 1);
		D3D9RenderVehicleDual(resEntryHeader, instancedData);
		instancedData++;
	}
	RwD3D9SetVertexShader(NULL);
	RwD3D9SetPixelShader(NULL);
	RwD3D9SetTexture(NULL, 1);
	RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
}

void
setVehiclePipeCB(RxPipelineNode *node, RxD3D9AllInOneRenderCallBack callback)
{
	vcsReflTex = RwTextureCreate(reflTex);

	HRSRC resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_VEHICLEVS), RT_RCDATA);
	RwUInt32 *shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreateVertexShader(shader, &vehiclePipeVS);
	FreeResource(shader);

	if(vehiclePipePS == NULL){
		resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_VEHICLEPS), RT_RCDATA);
		shader = (RwUInt32*)LoadResource(dllModule, resource);
		RwD3D9CreatePixelShader(shader, &vehiclePipePS);
		FreeResource(shader);
	}

	resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_PS2CARFXVS), RT_RCDATA);
	shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreateVertexShader(shader, &ps2CarFxVS);
	FreeResource(shader);

	if(ps2CarFxPS == NULL){
		resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_PS2CARFXPS), RT_RCDATA);
		shader = (RwUInt32*)LoadResource(dllModule, resource);
		RwD3D9CreatePixelShader(shader, &ps2CarFxPS);
		FreeResource(shader);
	}

	resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_SPECCARFXVS), RT_RCDATA);
	shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreateVertexShader(shader, &specCarFxVS);
	FreeResource(shader);

	resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_SPECCARFXPS), RT_RCDATA);
	shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreatePixelShader(shader, &specCarFxPS);
	FreeResource(shader);

	resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_VCSREFLVS), RT_RCDATA);
	shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreateVertexShader(shader, &vcsReflVS);
	FreeResource(shader);

	resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_VCSREFLPS), RT_RCDATA);
	shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreatePixelShader(shader, &vcsReflPS);
	FreeResource(shader);

	resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_XBOXCARVS), RT_RCDATA);
	shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreateVertexShader(shader, &xboxCarVS);
	FreeResource(shader);

	RxD3D9AllInOneSetRenderCallBack(node, CCustomCarEnvMapPipeline__CustomPipeRenderCB_PS2);
}
