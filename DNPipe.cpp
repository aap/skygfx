#include "skygfx.h"

static void *DNPipeVS;

enum {
	LOC_World = 0,
	LOC_View = 4,
	LOC_Proj = 8,
	LOC_materialColor = 12,
	LOC_surfaceProps = 13,
	LOC_ambientLight = 14,
	LOC_shaderVars = 15,
	LOC_reflData = 16,
	LOC_envXform = 17,
	LOC_texmat = 20,
};

// PS2 callback
void
CCustomBuildingDNPipeline__CustomPipeRenderCB_PS2(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	RxD3D9ResEntryHeader *resEntryHeader;
	RxD3D9InstanceData *instancedData;
	RpMaterial *material;
	RwBool lighting;
	RwInt32	numMeshes;
	CustomEnvMapPipeMaterialData *envData;
	D3DMATRIX worldMat, viewMat, projMat;
	RwRGBAReal color;
	float surfProps[4];
	RwBool hasAlpha;
	struct DNShaderVars {
		float colorScale;
		float balance;
		float reflSwitch;
	} dnShaderVars;
	struct {
		float shininess;
		float intensity;
	} reflData;
	RwV4d envXform;

//	if(GetAsyncKeyState(VK_F8) & 0x8000)
//		return;

	RwD3D9SetPixelShader(vehiclePipePS);
	RwD3D9SetVertexShader(DNPipeVS);
	RwD3D9GetTransform(D3DTS_WORLD, &worldMat);
	RwD3D9GetTransform(D3DTS_VIEW, &viewMat);
	RwD3D9GetTransform(D3DTS_PROJECTION, &projMat);
	RwD3D9SetVertexShaderConstant(LOC_World,(void*)&worldMat,4);
	RwD3D9SetVertexShaderConstant(LOC_View,(void*)&viewMat,4);
	RwD3D9SetVertexShaderConstant(LOC_Proj,(void*)&projMat,4);
	// TODO: maybe upload normal matrix?

	RwD3D9GetRenderState(D3DRS_LIGHTING, &lighting);
	resEntryHeader = (RxD3D9ResEntryHeader*)(repEntry + 1);
	instancedData = (RxD3D9InstanceData*)(resEntryHeader + 1);;
	if(resEntryHeader->indexBuffer)
		RwD3D9SetIndices(resEntryHeader->indexBuffer);
	_rwD3D9SetStreams(resEntryHeader->vertexStream, resEntryHeader->useOffsets);
	RwD3D9SetVertexDeclaration(resEntryHeader->vertexDeclaration);

	dnShaderVars.balance = CCustomBuildingDNPipeline__m_fDNBalanceParam;
	if(dnShaderVars.balance < 0.0f) dnShaderVars.balance = 0.0f;
	if(dnShaderVars.balance > 1.0f) dnShaderVars.balance = 1.0f;

	numMeshes = resEntryHeader->numMeshes;
	while(numMeshes--){
		material = instancedData->material;

		float colorScalePS;
		colorScalePS = dnShaderVars.colorScale = 1.0f;
		if(flags & (rxGEOMETRY_TEXTURED2 | rxGEOMETRY_TEXTURED)){
			colorScalePS = dnShaderVars.colorScale = config->ps2ModulateWorld ? 255.0f/128.0f : 1.0f;
			RwD3D9SetTexture(material->texture ? material->texture : gpWhiteTexture, 0);
		}else
			RwD3D9SetTexture(gpWhiteTexture, 0);
		RwD3D9SetPixelShaderConstant(0, &colorScalePS, 1);

		int effect = RpMatFXMaterialGetEffects(material);
		if(effect == rpMATFXEFFECTUVTRANSFORM){
			RwMatrix *m1, *m2;
			RpMatFXMaterialGetUVTransformMatrices(material, &m1, &m2);
			RwD3D9SetVertexShaderConstant(LOC_texmat, (void*)m1, 4);
		}else{
			RwMatrix m;
			RwMatrixSetIdentity(&m);
			RwD3D9SetVertexShaderConstant(LOC_texmat, (void*)&m, 4);
		}

		reflData.shininess = reflData.intensity = 0.0f;
		dnShaderVars.reflSwitch = 0;
		if(*(int*)&material->surfaceProps.specular & 1){
			RwV3d transVec;

			envData = *RWPLUGINOFFSET(CustomEnvMapPipeMaterialData*, material, CCustomCarEnvMapPipeline__ms_envMapPluginOffset);
			dnShaderVars.reflSwitch = 1 + config->worldPipe;
			RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
			RwD3D9SetTexture(envData->texture, 1);
			reflData.shininess = envData->shininess/255.0f;
			reflData.intensity = pDirect->color.red*(256.0f/255.0f);

			// is this correct? taken from vehicle pipeline
			D3D9GetTransScaleVector(envData, (RpAtomic*)object, &transVec);
			envXform.x = transVec.x;
			envXform.y = transVec.y;
			envXform.z = envData->scaleX / 8.0f;
			envXform.w = envData->scaleY / 8.0f;
			RwD3D9SetVertexShaderConstant(LOC_envXform, (void*)&envXform, 1);
		}
		RwD3D9SetVertexShaderConstant(LOC_reflData, (void*)&reflData, 1);

		hasAlpha = instancedData->vertexAlpha || instancedData->material->color.alpha != 255;
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)hasAlpha);

		RwRGBARealFromRwRGBA(&color, &material->color);
		RwD3D9SetVertexShaderConstant(LOC_materialColor, (void*)&color, 1);
		// yep, not always done by the game
		if(CPostEffects::m_bInfraredVision){
			color.red = 0.0f;
			color.green = 0.0f;
			color.blue = 1.0f;
		}else
			color = pAmbient->color;
		RwD3D9SetVertexShaderConstant(LOC_ambientLight, (void*)&color, 1);
		surfProps[0] = lighting || config->worldPipe == 0 ? material->surfaceProps.ambient : 0.0f;
		RwD3D9SetVertexShaderConstant(LOC_surfaceProps, &surfProps, 1);

		RwD3D9SetVertexShaderConstant(LOC_shaderVars, (void*)&dnShaderVars, 1);
		// this takes the texture into account, somehow....
		RwD3D9GetRenderState(D3DRS_ALPHABLENDENABLE, &hasAlpha);
		if(hasAlpha && config->dualPassWorld){
			int alphafunc, alpharef;
			RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &alpharef);
			RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &alphafunc);
			RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)128);
			RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONGREATEREQUAL);
			RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
			D3D9Render(resEntryHeader, instancedData);
			RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONLESS);
			RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
			D3D9Render(resEntryHeader, instancedData);
			RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)alpharef);
			RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)alphafunc);
			RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
		}else
			D3D9Render(resEntryHeader, instancedData);
		instancedData++;
	}
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

RwBool
DNInstance_PS2(void *object, RxD3D9ResEntryHeader *resEntryHeader, RwBool reinstance)
{
	RpAtomic *atomic;
	RpGeometry *geometry;
	RpMorphTarget *morphTarget;
	RwBool isTextured, isPrelit, hasNormals;
	D3DVERTEXELEMENT9 dcl[6];
	void *vertexBuffer;
	RwUInt32 stride;
	RxD3D9InstanceData *instData;
	IDirect3DVertexBuffer9 *vertBuffer;
	int i, j;
	int numMeshes;
	void *vertexData;
	int lastLocked;

	atomic = (RpAtomic*)object;
	geometry = atomic->geometry;
	isTextured = (geometry->flags & (rpGEOMETRYTEXTURED|rpGEOMETRYTEXTURED2)) != 0;
	morphTarget = geometry->morphTarget;
	isPrelit = geometry->flags & rpGEOMETRYPRELIT;
	hasNormals = geometry->flags & rpGEOMETRYNORMALS;
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
				RwRGBA *night = *RWPLUGINOFFSET(RwRGBA*, geometry, CCustomBuildingDNPipeline__ms_extraVertColourPluginOffset);
				RwRGBA *day = *RWPLUGINOFFSET(RwRGBA*, geometry, CCustomBuildingDNPipeline__ms_extraVertColourPluginOffset + 4);
				if(isNightColorZero(night, geometry->numVertices))
					night = day;
				numMeshes = resEntryHeader->numMeshes;
				instData = (RxD3D9InstanceData*)(resEntryHeader+1);
				while(numMeshes--){
					instData->vertexAlpha = _rpD3D9VertexDeclarationInstColor(
					          (RwUInt8*)vertexData + dcl[i].Offset + resEntryHeader->vertexStream[dcl[i].Stream].stride*instData->minVert,
					          night + instData->minVert,
					          instData->numVertices,
					          resEntryHeader->vertexStream[dcl[i].Stream].stride);
					_rpD3D9VertexDeclarationInstColor(
					          (RwUInt8*)vertexData + dcl[j].Offset + resEntryHeader->vertexStream[dcl[j].Stream].stride*instData->minVert,
					          day + instData->minVert,
					          instData->numVertices,
					          resEntryHeader->vertexStream[dcl[j].Stream].stride);
					instData++;
				}
			}
		}

		if(vertexData)
			vertBuffer->Unlock();
	}
	return 1;
}

RxPipeline*
CCustomBuildingDNPipeline__CreateCustomObjPipe_PS2(void)
{
	RxPipeline *pipeline;
	RxLockedPipe *lockedpipe;
	RxPipelineNode *node;
	RxNodeDefinition *instanceNode;
	RxD3D9AllInOneInstanceCallBack instanceCB;
	RxD3D9AllInOneReinstanceCallBack reinstanceCB;

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
	if(dword_C02C20){
		instanceCB = DNInstance_PS2;
		RxD3D9AllInOneSetInstanceCallBack(node, instanceCB);
		RxD3D9AllInOneSetReinstanceCallBack(node, reinstance);
	}else{
		instanceCB = DNInstance_PS2;
		RxD3D9AllInOneSetInstanceCallBack(node, instanceCB);
		reinstanceCB = RxD3D9AllInOneGetReinstanceCallBack(node);
		RxD3D9AllInOneSetReinstanceCallBack(node, reinstanceCB);
	}
	RxD3D9AllInOneSetRenderCallBack(node, CCustomBuildingDNPipeline__CustomPipeRenderCB_PS2);

	HRSRC resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_DNPIPEVS), RT_RCDATA);
	RwUInt32 *shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreateVertexShader(shader, &DNPipeVS);
	FreeResource(shader);

	if(vehiclePipePS == NULL){
		resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_VEHICLEPS), RT_RCDATA);
		shader = (RwUInt32*)LoadResource(dllModule, resource);
		RwD3D9CreatePixelShader(shader, &vehiclePipePS);
		FreeResource(shader);
	}

	pipeline->pluginId = 0x53F20098;
	pipeline->pluginData = 0x53F20098;
	return pipeline;
}
