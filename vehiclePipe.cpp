#include "skygfx.h"

static void *vehiclePipeVS;
void *vehiclePipePS;

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

	LOC_activeLights = 0,
};

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

	// set multipass distance. i'm lazy, just do it here
	float *mpd = (float*)0xC88044;
	*mpd = 100000000.0f;

	if(config->vehiclePipe > 1){
		CCustomCarEnvMapPipeline__CustomPipeRenderCB_exe(repEntry, object, type, flags);
		return;
	}

	atomic = (RpAtomic*)object;

	// matrices
	RwD3D9SetPixelShader(vehiclePipePS);
	RwD3D9SetVertexShader(vehiclePipeVS);
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

		hasRefl = true;
		if((materialFlags & 1) == 0 || noRefl)
			hasRefl = false;
		hasEnv = true;
		if((materialFlags & 2) == 0 || noRefl || (atomic->geometry->flags & rpGEOMETRYTEXTURED2) == 0)
			hasEnv = false;
		// ignoring visualFX level
		hasSpec = materialFlags & 4;

		if(flags & (rxGEOMETRY_TEXTURED2 | rxGEOMETRY_TEXTURED))
			RwD3D9SetTexture(material->texture ? material->texture : gpWhiteTexture, 0);
		else
			RwD3D9SetTexture(gpWhiteTexture, 0);

		if(hasSpec && lighting){
			specData = *RWPLUGINOFFSET(CustomSpecMapPipeMaterialData*, material,
			                           CCustomCarEnvMapPipeline__ms_specularMapPluginOffset);
			reflData.specularity = specData->specularity;
			RwD3D9SetTexture(specData->texture, 2);
		}else{
			reflData.specularity = 0.0f;
			RwD3D9SetTexture(NULL, 2);
		}

		reflData.shininess = 0.0f;
		RwD3D9SetTexture(NULL, 1);
		envData = *RWPLUGINOFFSET(CustomEnvMapPipeMaterialData*, material, CCustomCarEnvMapPipeline__ms_envMapPluginOffset);
		if(hasRefl){
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
		}

		if(hasEnv && envData->pad3 == 0xF2){
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
			// is this correct?
			envXform.z = 1.0f;
			envXform.w = 1.0f;
		}
		hasAlpha = instancedData->vertexAlpha || instancedData->material->color.alpha != 255;
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)hasAlpha);
		RwD3D9SetVertexShaderConstant(LOC_reflData, (void*)&reflData, 1);
		RwD3D9SetVertexShaderConstant(LOC_envXForm, (void*)&envXform, 1);
		RwD3D9SetVertexShaderConstant(LOC_envSwitch, (void*)&envSwitch, 1);

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
		// this takes the texture into account, somehow....
		RwD3D9GetRenderState(D3DRS_ALPHABLENDENABLE, &hasAlpha);
		if(hasAlpha && config->dualPassVehicle){
			int alphafunc;
			RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &alphafunc);
			RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONGREATEREQUAL);
			D3D9Render(resEntryHeader, instancedData);
			RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONLESS);
			RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, FALSE);
			D3D9Render(resEntryHeader, instancedData);
			RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)alphafunc);
			RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
		}else
			D3D9Render(resEntryHeader, instancedData);
		instancedData++;
	}
	// we don't even change it, but apparently this render CB has to reset it (otherwise might still be on from MatFX)
	RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
}

RpAtomic*
CCarFXRenderer__CustomCarPipeClumpSetup(RpAtomic *atomic, void *data)
{
	RpAtomic *ret;

	ret = CCustomCarEnvMapPipeline__CustomPipeAtomicSetup(atomic);
	if(strstr(GetFrameNodeName(RpAtomicGetFrame(atomic)), "wheel")){
		ret->pipeline = NULL;
		SetPipelineID(atomic, 0);
	}
	return ret;
}

void
setVehiclePipeCB(RxPipelineNode *node, RxD3D9AllInOneRenderCallBack callback)
{
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

	RxD3D9AllInOneSetRenderCallBack(node, CCustomCarEnvMapPipeline__CustomPipeRenderCB_PS2);
}
