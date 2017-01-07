#include "skygfx.h"

void
D3D9RenderNotLit(RxD3D9ResEntryHeader *resEntryHeader, RxD3D9InstanceData *instanceData)
{
	RwD3D9SetPixelShader(NULL);
	RwD3D9SetVertexShader(instanceData->vertexShader);
	if(resEntryHeader->indexBuffer)
		RwD3D9DrawIndexedPrimitive(resEntryHeader->primType, instanceData->baseIndex, 0, instanceData->numVertices, instanceData->startIndex, instanceData->numPrimitives);
	else
		RwD3D9DrawPrimitive(resEntryHeader->primType, instanceData->baseIndex, instanceData->numPrimitives);
}

void
D3D9RenderPreLit(RxD3D9ResEntryHeader *resEntryHeader, RxD3D9InstanceData *instanceData, RwUInt8 flags, RwTexture *texture)
{
	if(flags & (rxGEOMETRY_TEXTURED2 | rxGEOMETRY_TEXTURED)){
		RwD3D9SetTexture(texture, 0);
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
		RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
	}
	RwD3D9SetPixelShader(NULL);
	RwD3D9SetVertexShader(instanceData->vertexShader);
	if(resEntryHeader->indexBuffer)
		RwD3D9DrawIndexedPrimitive(resEntryHeader->primType, instanceData->baseIndex, 0, instanceData->numVertices, instanceData->startIndex, instanceData->numPrimitives);
	else
		RwD3D9DrawPrimitive(resEntryHeader->primType, instanceData->baseIndex, instanceData->numPrimitives);
}

void __declspec(naked)
D3D9GetEnvMapVector(RpAtomic *atomic, CustomEnvMapPipeAtomicData *atmdata, CustomEnvMapPipeMaterialData *data, RwV3d *transScale)
{
	_asm{
		mov ecx, [esp+4]
		mov edi, [esp+8]
		mov esi, [esp+12]
		push [esp+16]
		mov edx, 0x5D8590
		call edx
		pop eax
		retn
	}
}

void __declspec(naked)
D3D9GetTransScaleVector(CustomEnvMapPipeMaterialData *data, RpAtomic *atomic, RwV3d *transScale)
{
	_asm{
		mov eax, [esp+4]
		mov ecx, [esp+8]
		mov esi, [esp+12]
		mov edx, 0x5D84C0
		call edx
		retn
	}
}

void
CCustomBuildingDNPipeline__CustomPipeRenderCB(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	RxD3D9ResEntryHeader *resEntryHeader;
	RxD3D9InstanceData *instancedData;
	RpMaterial *material;
	RwBool lighting;
	RwInt32	numMeshes;
	RwBool notLit;
	CustomEnvMapPipeMaterialData *envData = nullptr;

	RwD3D9GetRenderState(D3DRS_LIGHTING, &lighting);
	if(lighting || flags & rpGEOMETRYPRELIT){
		notLit = 0;
	}else{
		notLit = 1;
		RwD3D9SetTexture(0, 0);
		RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, 0xFF000000);
		RwD3D9SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
		RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
		RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
		RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
	}
	resEntryHeader = (RxD3D9ResEntryHeader*)(repEntry + 1);
	instancedData = (RxD3D9InstanceData*)(resEntryHeader + 1);;
	if(resEntryHeader->indexBuffer)
		RwD3D9SetIndices(resEntryHeader->indexBuffer);
	_rwD3D9SetStreams(resEntryHeader->vertexStream, resEntryHeader->useOffsets);
	RwD3D9SetVertexDeclaration(resEntryHeader->vertexDeclaration);

	numMeshes = resEntryHeader->numMeshes;
	while(numMeshes--){
		material = instancedData->material;
		RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		RwD3D9SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
		if(*(int*)&material->surfaceProps.specular & 1){
			static D3DMATRIX texMat;	// actually unused
			RwInt32 tfactor;

			RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
			RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
			// remnants of a texture matrix
			texMat._11 = envData->scaleX * 0.125;
			texMat._22 = envData->scaleY * 0.125;
			texMat._33 = 1.0f;
			texMat._44 = 1.0f;
			RwD3D9SetTexture(envData->texture, 1);
			tfactor = envData->shininess * 254.0/255.0;
			if(tfactor > 255)
				tfactor = 255;
			RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_XRGB(tfactor,tfactor,tfactor));
			RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MULTIPLYADD);
			RwD3D9SetTextureStageState(1, D3DTSS_COLORARG0, D3DTA_CURRENT);
			RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
			RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
			RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
			RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACENORMAL | 1);
			RwD3D9SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
			RwD3D9SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
		}
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)(instancedData->vertexAlpha || instancedData->material->color.alpha != 255));
		if(notLit)
			D3D9RenderNotLit(resEntryHeader, instancedData);
		else{
			if(lighting)
				RwD3D9SetSurfaceProperties(&material->surfaceProps, &material->color, flags);
			D3D9RenderPreLit(resEntryHeader, instancedData, flags, material->texture);
		}
		instancedData++;
	}
	RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
}

static RwBool
reinstance(void *object, RwResEntry *resEntry, RxD3D9AllInOneInstanceCallBack instanceCallback)
{
	return (instanceCallback == NULL ||
	        instanceCallback(object, (RxD3D9ResEntryHeader*)(resEntry + 1), true) != 0);
}

RwBool
DNInstance_default(void *object, RxD3D9ResEntryHeader *resEntryHeader, RwBool reinstance)
{
	RpAtomic *atomic;
	RpGeometry *geometry;
	RpMorphTarget *morphTarget;
	RwBool isTextured, isPrelit, hasNormals;
	D3DVERTEXELEMENT9 dcl[5];
	void *vertexBuffer;
	RwUInt16 stride;
	RxD3D9InstanceData *instData;
	IDirect3DVertexBuffer9 *vertBuffer;
	int i;
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
				RwRGBA *DNdata = *(RwRGBA**)((RwUInt8*)geometry + CCustomBuildingDNPipeline__ms_extraVertColourPluginOffset);
				numMeshes = resEntryHeader->numMeshes;
				instData = (RxD3D9InstanceData*)(resEntryHeader+1);
				while(numMeshes--){
					instData->vertexAlpha = _rpD3D9VertexDeclarationInstColor(
					          (RwUInt8*)vertexData + dcl[i].Offset + resEntryHeader->vertexStream[dcl[i].Stream].stride*instData->minVert,
					          DNdata + instData->minVert,
					          instData->numVertices,
					          resEntryHeader->vertexStream[dcl[i].Stream].stride);
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
CCustomBuildingDNPipeline__CreateCustomObjPipe(void)
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
		instanceCB = RxD3D9AllInOneGetInstanceCallBack(node);
		RxD3D9AllInOneSetInstanceCallBack(node, instanceCB);
		RxD3D9AllInOneSetReinstanceCallBack(node, reinstance);
	}else{
		instanceCB = RxD3D9AllInOneGetInstanceCallBack(node);
		RxD3D9AllInOneSetInstanceCallBack(node, instanceCB);
		reinstanceCB = RxD3D9AllInOneGetReinstanceCallBack(node);
		RxD3D9AllInOneSetReinstanceCallBack(node, reinstanceCB);
	}
	RxD3D9AllInOneSetRenderCallBack(node, CCustomBuildingDNPipeline__CustomPipeRenderCB);

	pipeline->pluginId = 0x53F20098;
	pipeline->pluginData = 0x53F20098;
	return pipeline;
}

void
CCustomCarEnvMapPipeline__CustomPipeRenderCB(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	RxD3D9ResEntryHeader *resEntryHeader;
	RxD3D9InstanceData *instancedData;
	RpAtomic *atomic;
	RwBool lighting;
	RwInt32	numMeshes;
	RwBool notLit;
	RwBool noRefl;
	CustomEnvMapPipeMaterialData *envData;
	CustomEnvMapPipeAtomicData *atmEnvData;
	CustomSpecMapPipeMaterialData *specData;
	RpMaterial *material;
	RwUInt32 materialFlags;
	RwBool hasRefl, hasEnv, hasSpec;
	RwReal specularLighting, specularPower;
	RwReal lightMult;

	lightMult = CCustomCarEnvMapPipeline__m_EnvMapLightingMult * 1.85;
	atomic = (RpAtomic*)object;
	noRefl = CVisibilityPlugins__GetAtomicId(atomic) & 0x6000;
	RwD3D9GetRenderState(D3DRS_LIGHTING, &lighting);
	if(lighting || flags & 8){
		notLit = 0;
	}else{
		notLit = 1;
		RwD3D9SetTexture(0, 0);
		RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, 0xFF000000);
		RwD3D9SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
		RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
		RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
		RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
	}

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

		if(hasSpec && lighting){
			specData = *RWPLUGINOFFSET(CustomSpecMapPipeMaterialData*, material,
			                           CCustomCarEnvMapPipeline__ms_specularMapPluginOffset);
			RwD3D9SetLight(1, &gCarEnvMapLight);
			RwD3D9EnableLight(1, 1);
			specularLighting = lightMult*specData->specularity + lightMult*specData->specularity;
			if(specularLighting > 1.0f) specularLighting = 1.0f;
			specularPower = specData->specularity * 100.0f;
			RwD3D9SetRenderState(D3DRS_SPECULARENABLE, 1);
			RwD3D9SetRenderState(D3DRS_LOCALVIEWER, 0);
			RwD3D9SetRenderState(D3DRS_SPECULARMATERIALSOURCE, 0);
		}else{
			RwD3D9SetRenderState(D3DRS_SPECULARENABLE, 0);
			specularLighting = 0.0f;
			specularPower = 0.0f;
		}
		RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		RwD3D9SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);

		envData = *RWPLUGINOFFSET(CustomEnvMapPipeMaterialData*, material, CCustomCarEnvMapPipeline__ms_envMapPluginOffset);
		if(hasRefl){
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
			RwD3D9SetTransform(D3DTS_TEXTURE1, &texMat);
			RwD3D9SetTexture(envData->texture, 1);
			tfactor = envData->shininess * lightMult * 254.0/255.0;
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
			RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACENORMAL | 1);
			RwD3D9SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_PROJECTED | D3DTTFF_COUNT3);
			RwD3D9SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
		}

		if(hasEnv){
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
			RwD3D9SetTransform(D3DTS_TEXTURE1, &texMat);
			RwD3D9SetTexture(envData->texture, 1);
			tfactor = lightMult * 24.0f;
			if(tfactor > 255) tfactor = 255;
			RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(tfactor,255,255,255));
			RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_BLENDFACTORALPHA);
			RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
			RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
			RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
			RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
			RwD3D9SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
			RwD3D9SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
		}
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)(instancedData->vertexAlpha || instancedData->material->color.alpha != 255));
		if(notLit){
			D3D9RenderNotLit(resEntryHeader, instancedData);
		}else{
			if(lighting){
				RwRGBA matColor;
				int rgb;
				matColor = material->color;
				rgb = *(int*)&matColor & 0xFFFFFF;
				if(rgb == 0xAF00FF || rgb == 0x00FFB9 || rgb == 0x00FF3C || rgb == 0x003CFF ||
				   rgb == 0x00AFFF || rgb == 0xC8FF00 || rgb == 0xFF00FF || rgb == 0xFFFF00){
					matColor.red = 0;
					matColor.green = 0;
					matColor.blue = 0;
				}
				D3D9SetRenderMaterialProperties(&material->surfaceProps, &matColor, flags, specularLighting, specularPower);
			}
			D3D9RenderPreLit(resEntryHeader, instancedData, flags, material->texture);
		}
		if(hasSpec && lighting){
			RwD3D9SetRenderState(D3DRS_SPECULARENABLE, 0);
			RwD3D9EnableLight(1, 0);
		}
		instancedData++;
	}

	D3D9RestoreSurfaceProperties();
	RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
}
