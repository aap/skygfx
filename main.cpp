#include "skygfx.h"

HMODULE dllModule;

int numConfigs;
Config *config, configs[10];
bool oneGrassModel, usePCTimecyc, disableClouds;//, uglyWheelHack;
int original_bRadiosity = 0;

void *grassPixelShader;
void *simplePS;
void *gpCurrentShaderForDefaultCallbacks;
//void (*TimecycInit)(void) = (void (*)(void))0x5BBAC0;

int &colourLeftVOffset = *(int*)0x8D5150;
int &colourRightVOffset = *(int*)0x8D5154;
int &colourTopVOffset = *(int*)0x8D5158;
int &colourBottomVOffset = *(int*)0x8D515C;

float *gfLaRiotsLightMult = (float*)0x8CD060;
float *ambientColors = (float*)0xB7C4A0;

void
D3D9Render(RxD3D9ResEntryHeader *resEntryHeader, RxD3D9InstanceData *instanceData)
{
	if(resEntryHeader->indexBuffer)
		RwD3D9DrawIndexedPrimitive(resEntryHeader->primType, instanceData->baseIndex, 0, instanceData->numVertices, instanceData->startIndex, instanceData->numPrimitives);
	else
		RwD3D9DrawPrimitive(resEntryHeader->primType, instanceData->baseIndex, instanceData->numPrimitives);
}

double
CTimeCycle_GetAmbientRed(void)
{
	float c = 0.0;
	if(config->ps2Ambient){
		for(int i = 0; i < 3; i++)
			if(ambientColors[i] > c)
				c = ambientColors[i];
		c += ambientColors[0];
		if(c > 1.0f) c = 1.0f;
	}else
		c = ambientColors[0];
	return *gfLaRiotsLightMult * c;
}

double
CTimeCycle_GetAmbientGreen(void)
{
	float c = 0.0;
	if(config->ps2Ambient){
		for(int i = 0; i < 3; i++)
			if(ambientColors[i] > c)
				c = ambientColors[i];
		c += ambientColors[1];
		if(c > 1.0f) c = 1.0f;
	}else
		c = ambientColors[1];
	return *gfLaRiotsLightMult * c;
}

double
CTimeCycle_GetAmbientBlue(void)
{
	float c = 0.0;
	if(config->ps2Ambient){
		for(int i = 0; i < 3; i++)
			if(ambientColors[i] > c)
				c = ambientColors[i];
		c += ambientColors[2];
		if(c > 1.0f) c = 1.0f;
	}else
		c = ambientColors[2];
	return *gfLaRiotsLightMult * c;
}

void
SetCloseFarAlphaDist(float close, float far)
{
	*(float*)0xC02DBC = close;
	*(float*)0x8D132C = config->farDist;
}

void
resetValues(void)
{
	CPostEffects__m_RadiosityFilterPasses = config->radiosityFilterPasses;
	CPostEffects__m_RadiosityRenderPasses = config->radiosityRenderPasses;
	CPostEffects__m_RadiosityIntensityLimit = config->radiosityIntensityLimit;
	CPostEffects__m_RadiosityIntensity = config->radiosityIntensity;
	CPostEffects__m_RadiosityFilterUCorrection = config->radiosityFilterUCorrection;
	CPostEffects__m_RadiosityFilterVCorrection = config->radiosityFilterVCorrection;

	if(config->grainFilter == 0){
		CPostEffects::m_InfraredVisionGrainStrength = 0x18;
		CPostEffects::m_NightVisionGrainStrength = 0x10;
	}else{
		CPostEffects::m_InfraredVisionGrainStrength = 0x40;
		CPostEffects::m_NightVisionGrainStrength = 0x30;
	}
	// night vision ambient green
	// not if this is the correct switch, maybe ps2ModulateWorld?
	if(config->nightVision == 0)
		MemoryVP::Patch<float>(0x735F8B, 0.4f);
	else
		MemoryVP::Patch<float>(0x735F8B, 1.0f);

	*(int*)0x8D37D0 = config->detailedWaterDist;
}

int
readhex(char *str)
{
	int n = 0;
	if(strlen(str) > 2)
		sscanf(str+2, "%X", &n);
	return n;
}

BOOL FileExists(LPCTSTR szPath)
{
	DWORD dwAttrib = GetFileAttributes(szPath);
	return dwAttrib != INVALID_FILE_ATTRIBUTES && 
	       !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}

void
findInis(void)
{
	char modulePath[MAX_PATH];
	GetModuleFileName(dllModule, modulePath, MAX_PATH);
	size_t nLen = strlen(modulePath);
	modulePath[nLen+1] = L'\0';
	modulePath[nLen] = L'i';
	modulePath[nLen-1] = L'n';
	modulePath[nLen-2] = L'i';
	modulePath[nLen-3] = L'.';
	modulePath[nLen-4] = '1';

	numConfigs = 0;
	while(numConfigs < 9 && FileExists(modulePath)){
		modulePath[nLen-4]++;
		numConfigs++;
	}
}

void
readIni(int n)
{
	int tmpint;
	char tmp[32];
	char modulePath[MAX_PATH];

	GetModuleFileName(dllModule, modulePath, MAX_PATH);
	size_t nLen = strlen(modulePath);
	Config *c;
	if(n > 0){
		modulePath[nLen+1] = L'\0';
		modulePath[nLen] = L'i';
		modulePath[nLen-1] = L'n';
		modulePath[nLen-2] = L'i';
		modulePath[nLen-3] = L'.';
		modulePath[nLen-4] = n+'0';
		c = &configs[n-1];
	}else{
		modulePath[nLen-1] = L'i';
		modulePath[nLen-2] = L'n';
		modulePath[nLen-3] = L'i';
		c = &configs[n];
	}

	GetPrivateProfileString("SkyGfx", "keySwitch", "0x79", tmp, sizeof(tmp), modulePath);
	c->keys[0] = readhex(tmp);
	GetPrivateProfileString("SkyGfx", "keyReload", "0x7A", tmp, sizeof(tmp), modulePath);
	c->keys[1] = readhex(tmp);
	GetPrivateProfileString("SkyGfx", "keyReloadPlants", "0x7B", tmp, sizeof(tmp), modulePath);
	c->keys[2] = readhex(tmp);

	c->enableHotkeys = GetPrivateProfileInt("SkyGfx", "enableHotkeys", TRUE, modulePath) != FALSE;
	c->ps2Ambient = GetPrivateProfileInt("SkyGfx", "ps2Ambient", TRUE, modulePath) != FALSE;
	c->ps2ModulateWorld = GetPrivateProfileInt("SkyGfx", "ps2ModulateWorld", TRUE, modulePath) != FALSE;
	c->ps2ModulateGrass = GetPrivateProfileInt("SkyGfx", "ps2ModulateGrass", TRUE, modulePath) != FALSE;
	c->dualPassWorld = GetPrivateProfileInt("SkyGfx", "dualPassWorld", TRUE, modulePath) != FALSE;
	c->dualPassDefault = GetPrivateProfileInt("SkyGfx", "dualPassDefault", TRUE, modulePath) != FALSE;
	c->dualPassGrass = GetPrivateProfileInt("SkyGfx", "dualPassGrass", TRUE, modulePath) != FALSE;
	c->dualPassVehicle = GetPrivateProfileInt("SkyGfx", "dualPassVehicle", TRUE, modulePath) != FALSE;
	c->dualPassPed = GetPrivateProfileInt("SkyGfx", "dualPassPed", TRUE, modulePath) != FALSE;
	c->grassAddAmbient = GetPrivateProfileInt("SkyGfx", "grassAddAmbient", TRUE, modulePath) != FALSE;
	c->fixGrassPlacement = GetPrivateProfileInt("SkyGfx", "fixGrassPlacement", TRUE, modulePath) != FALSE;
	oneGrassModel = GetPrivateProfileInt("SkyGfx", "oneGrassModel", TRUE, modulePath) != FALSE;
	c->backfaceCull = GetPrivateProfileInt("SkyGfx", "backfaceCull", FALSE, modulePath) != FALSE;
	c->vehiclePipe = GetPrivateProfileInt("SkyGfx", "vehiclePipe", 0, modulePath) % 5;
	tmpint = GetPrivateProfileInt("SkyGfx", "worldPipe", 0, modulePath);
	c->worldPipe = tmpint >= 0 ? tmpint % 3 : -1;
	c->colorFilter = GetPrivateProfileInt("SkyGfx", "colorFilter", 0, modulePath) % 4;
	c->infraredVision = GetPrivateProfileInt("SkyGfx", "infraredVision", 0, modulePath) % 2;
	c->nightVision = GetPrivateProfileInt("SkyGfx", "nightVision", 0, modulePath) % 2;
	c->grainFilter = GetPrivateProfileInt("SkyGfx", "grainFilter", 0, modulePath) % 2;
	usePCTimecyc = GetPrivateProfileInt("SkyGfx", "usePCTimecyc", FALSE, modulePath) != FALSE;

	tmpint = GetPrivateProfileInt("SkyGfx", "blurLeft", 4000, modulePath);
	c->offLeft = tmpint == 4000 ? colourLeftVOffset : tmpint;
	tmpint = GetPrivateProfileInt("SkyGfx", "blurTop", 4000, modulePath);
	c->offTop = tmpint == 4000 ? colourTopVOffset : tmpint;
	tmpint = GetPrivateProfileInt("SkyGfx", "blurRight", 4000, modulePath);
	c->offRight = tmpint == 4000 ? colourRightVOffset : tmpint;
	tmpint = GetPrivateProfileInt("SkyGfx", "blurBottom", 4000, modulePath);
	c->offBottom = tmpint == 4000 ? colourBottomVOffset : tmpint;

	c->scaleOffsets = GetPrivateProfileInt("SkyGfx", "scaleOffsets", TRUE, modulePath) != FALSE;
	tmpint = GetPrivateProfileInt("SkyGfx", "doRadiosity", 4000, modulePath);
	c->doRadiosity = tmpint == 4000 ? original_bRadiosity : tmpint;	// saved value from stream.ini

	c->radiosityFilterPasses = GetPrivateProfileInt("SkyGfx", "radiosityFilterPasses", 2, modulePath);
	c->radiosityRenderPasses = GetPrivateProfileInt("SkyGfx", "radiosityRenderPasses", 1, modulePath);
	c->radiosityIntensityLimit = GetPrivateProfileInt("SkyGfx", "radiosityIntensityLimit", 0xDC, modulePath);
	c->radiosityIntensity = GetPrivateProfileInt("SkyGfx", "radiosityIntensity", 0x23, modulePath);
	c->radiosityFilterUCorrection = GetPrivateProfileInt("SkyGfx", "radiosityFilterUCorrection", 2, modulePath);
	c->radiosityFilterVCorrection = GetPrivateProfileInt("SkyGfx", "radiosityFilterVCorrection", 2, modulePath);

	c->vcsTrails = GetPrivateProfileInt("SkyGfx", "vcsTrails", FALSE, modulePath) != FALSE;
	c->trailsLimit = GetPrivateProfileInt("SkyGfx", "trailsLimit", 80, modulePath);
	c->trailsIntensity = GetPrivateProfileInt("SkyGfx", "trailsIntensity", 38, modulePath);
	c->pedShadows = GetPrivateProfileInt("SkyGfx", "pedShadows", 0, modulePath);
	c->stencilShadows = GetPrivateProfileInt("SkyGfx", "stencilShadows", 0, modulePath);
	disableClouds = GetPrivateProfileInt("SkyGfx", "disableClouds", FALSE, modulePath) != FALSE;
	//uglyWheelHack = GetPrivateProfileInt("SkyGfx", "uglyWheelHack", FALSE, modulePath) != FALSE;

	c->detailedWaterDist = GetPrivateProfileInt("SkyGfx", "detailedWaterDist", 48, modulePath);

	GetPrivateProfileString("SkyGfx", "farDist", "60.0", tmp, sizeof(tmp), modulePath);
	c->farDist = atof(tmp);
	GetPrivateProfileString("SkyGfx", "blendDist", "20.0", tmp, sizeof(tmp), modulePath);
	c->fadeDist = atof(tmp);
	c->fadeInvDist = 1.0f/c->fadeDist;
	GetPrivateProfileString("SkyGfx", "densityMult", "1.0", tmp, sizeof(tmp), modulePath);
	c->densityMult = atof(tmp)*0.5;

	*(float**)0x5DC281 = &c->densityMult;
	*(float**)0x5DAD98 = &c->fadeDist;
	*(float**)0x5DAE05 = &c->fadeInvDist;
}

RpAtomic *(*plantTab0)[4] = (RpAtomic *(*)[4])0xC039F0;
RpAtomic *(*plantTab1)[4] = (RpAtomic *(*)[4])0xC03A00;

RpAtomic*
grassRenderCallback(RpAtomic *atomic)
{
	RpAtomic *ret;
	int cullmode;
	RwRGBAReal color = { 0.0f, 0.0, 1.0f, 1.0f };

	if(config->ps2ModulateGrass){
		gpCurrentShaderForDefaultCallbacks = grassPixelShader;
		RwRGBARealFromRwRGBA(&color, &atomic->geometry->matList.materials[0]->color);
		RwD3D9SetPixelShaderConstant(0, &color, 1);
	}

	RwRenderStateGet(rwRENDERSTATECULLMODE, &cullmode);
	if(!config->backfaceCull)
		RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);


	RxPipeline *pipe;
	int alphatest, alpharef;
	int dodual = 0;
	int detach = 0;
	pipe = atomic->pipeline;
	if(pipe == NULL)
		pipe = *(RxPipeline**)(*(DWORD*)0xC97B24+0x3C+dword_C9BC60);
	if(config->dualPassGrass){
		RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
		RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, (void*)&alphatest);
		RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)&alpharef);
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)128);
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONGREATEREQUAL);
		RxPipelineExecute(pipe, atomic, 1);
		RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONLESS);
		pipe = RxPipelineExecute(pipe, atomic, 1);
		RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)alphatest);
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)alpharef);
	}else
		pipe = RxPipelineExecute(pipe, atomic, 1);
	ret = pipe ? atomic : NULL;


	RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)cullmode);
	gpCurrentShaderForDefaultCallbacks = NULL;

	return ret;
}

/*
WRAPPER RwStream *RwStreamOpen(RwStreamType, RwStreamAccessType, const void*) { EAXJMP(0x7ECEF0); }
WRAPPER RwBool RwStreamClose(RwStream*, void*) { EAXJMP(0x7ECE20); }
WRAPPER RwBool RwStreamFindChunk(RwStream*, RwUInt32, RwUInt32*, RwUInt32*) { EAXJMP(0x7ED2D0); }
WRAPPER RpClump *RpClumpStreamRead(RwStream*) { EAXJMP(0x74B420); }
WRAPPER RpClump *RpClumpStreamWrite(RpClump*, RwStream*) { EAXJMP(0x74AA10); }
WRAPPER RwBool RpClumpDestroy(RpClump*) { EAXJMP(0x74A310); }
WRAPPER RpClump *RpClumpClone(RpClump*) { EAXJMP(0x749F70); }
WRAPPER RpClump *RpClumpForAllAtomics(RpClump*, RpAtomicCallBack, void*) { EAXJMP(0x749B70); }

WRAPPER RwError *RwErrorGet(RwError *code) { EAXJMP(0x808880); }

void __declspec(naked)
nullsize(void)
{
	_asm{
		xor	eax, eax
		retn
	}
}

void
writeclump(RpClump *clump, char *path)
{
	RwStream *stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMWRITE, path);
	RpClumpStreamWrite(clump, stream);
	RwStreamClose(stream, NULL);
}

RpClump*
readclump(char *path)
{
	RwStream *stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, path);
	RwBool found = RwStreamFindChunk(stream, rwID_CLUMP, NULL, NULL);
	RpClump *clump = RpClumpStreamRead(stream);
	RwStreamClose(stream, NULL);
	return clump;
}

WRAPPER RpAtomic *AtomicInstanceCB(RpAtomic*, void*) { EAXJMP(0x5A4380); }

void
instanceClump(RpClump *clump)
{
	RpClumpForAllAtomics(clump, AtomicInstanceCB, 0);
}

void
writedffs(void)
{
	static int done = 0;
	if(done)
		return;

	MemoryVP::InjectHook(0x5D6DC0, nullsize, PATCH_JUMP);
	MemoryVP::InjectHook(0x59D0F0, nullsize, PATCH_JUMP);
	MemoryVP::InjectHook(0x72FBC0, nullsize, PATCH_JUMP);

	done++;
	RpClump *clump = readclump("TEST\\player.dff");
	instanceClump(clump);
	writeclump(clump, "TEST\\player_out.dff");

	RpClump *clone = RpClumpClone(clump);
	writeclump(clone, "TEST\\clone1.dff");
	RpClump *clone2 = RpClumpClone(clone);
	writeclump(clone2, "TEST\\clone2.dff");
	RpClumpDestroy(clump);
	RpClumpDestroy(clone);
	RpClumpDestroy(clone2);

	clump = readclump("TEST\\rccam.dff");
	instanceClump(clump);
	writeclump(clump, "TEST\\rccam_out.dff");
	RpClumpDestroy(clump);

	clump = readclump("TEST\\cesar.dff");
	instanceClump(clump);
	writeclump(clump, "TEST\\cesar_out.dff");
	RpClumpDestroy(clump);

	clump = readclump("TEST\\hanger.dff");
	instanceClump(clump);
	writeclump(clump, "TEST\\hanger_out.dff");
	RpClumpDestroy(clump);

	clump = readclump("TEST\\bullet_.dff");
	instanceClump(clump);
	writeclump(clump, "TEST\\bullet_out.dff");
	RpClumpDestroy(clump);

	clump = readclump("TEST\\lae2_roads89.dff");
	instanceClump(clump);
	writeclump(clump, "TEST\\lae2_roads89_out.dff");
	RpClumpDestroy(clump);
}
*/

RpAtomic*
myDefaultCallback(RpAtomic *atomic)
{
	RxPipeline *pipe;
	int zwrite, alphatest, alpharef;
	int dodual = 0;
	int detach = 0;

//	if(GetAsyncKeyState(VK_F8) & 0x8000)
//		writedffs();

	pipe = atomic->pipeline;
//	if(GetAsyncKeyState(VK_F8) & 0x8000)
//		return atomic;
	if(pipe == NULL){
		pipe = *(RxPipeline**)(*(DWORD*)0xC97B24+0x3C+dword_C9BC60);
		RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, (void*)&zwrite);
		if(zwrite && config->dualPassDefault)
			dodual = 1;
	}else if(pipe == skinPipe && config->dualPassPed)
		dodual = 1;
//	if(pipe == CCustomCarEnvMapPipeline__ObjPipeline && !reflTexDone){
	if(pipe == skinPipe && !reflTexDone){
		if(config->vehiclePipe == 3){
			RwCameraEndUpdate(Camera);
			RwRasterPushContext(reflTex);
			RwRasterRenderFast(RwCameraGetRaster(Camera), 0, 0);
			RwRasterPopContext();
			RwCameraBeginUpdate(Camera);
		}
		reflTexDone = TRUE;
	}
	if(dodual){
		RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, (void*)&alphatest);
		RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)&alpharef);
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)128);
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONGREATEREQUAL);
		RxPipelineExecute(pipe, atomic, 1);
		RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONLESS);
		pipe = RxPipelineExecute(pipe, atomic, 1);
		RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)alphatest);
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)alpharef);
	}else
		pipe = RxPipelineExecute(pipe, atomic, 1);
	return pipe ? atomic : NULL;
}

int tmpintensity;
RwTexture **tmptexture = (RwTexture**)0xc02dc0;
RwRGBA *CPlantMgr_AmbientColor = (RwRGBA*)0xC03A44;

RpMaterial*
setTextureAndColor(RpMaterial *material, RwRGBA *color)
{
	RwTexture *texture;
	RwRGBA newcolor;
	unsigned int col[3];

	texture = *tmptexture;
	col[0] = color->red;
	col[1] = color->green;
	col[2] = color->blue;
	if(config->grassAddAmbient){
		if(CPostEffects::m_bInfraredVision){
			col[0] += 0;
			col[1] += 0;
			col[2] += 255;
		}else{
			col[0] += CTimeCycle_GetAmbientRed()*255;
			col[1] += CTimeCycle_GetAmbientGreen()*255;
			col[2] += CTimeCycle_GetAmbientBlue()*255;
		}
		if(col[0] > 255) col[0] = 255;
		if(col[1] > 255) col[1] = 255;
		if(col[2] > 255) col[2] = 255;
	}
	newcolor.red = (tmpintensity * col[0]) >> 8;
	newcolor.green = (tmpintensity * col[1]) >> 8;
	newcolor.blue = (tmpintensity * col[2]) >> 8;
	newcolor.alpha = color->alpha;
	material->color = newcolor;
	if(material->texture != texture)
		RpMaterialSetTexture(material, texture);
	return material;
}

void __declspec(naked)
fixSeed(void)
{
	_asm{
	// 0x5DADB7
		mov	ecx, [config]
		cmp	[ecx], 0	// fixGrassPlacement
		jz	dontfix
		mov	ecx, [esp+54h]
		mov	ebx, [ecx+eax*4]
		mov	ebp, [ebx+4]
		lea	edi, [ebp+10h]
		mov	eax, [esi+48h]

		push	5DADCCh
		retn

	dontfix:
		fld	dword ptr [esi+48h]
		mov	ecx, [esp+54h]
		push	5DADBEh
		retn

	}
}

// copy color as is and save intensity (eax) for use in setTextureAndColor() later
void __declspec(naked)
saveIntensity(void)
{
	_asm{
	// 0x5DAE61
		movzx	eax, byte ptr [esi+44h]
		mov	[tmpintensity], eax
		mov	al, byte ptr [esi+40h]	// color
		mov	cl, byte ptr [esi+41h]
		mov	dl, byte ptr [esi+42h]
		mov     byte ptr [esp+10h], al	// local variable color
		mov     byte ptr [esp+11h], cl
		mov     byte ptr [esp+12h], dl
		mov     eax, [esi+3Ch]	// code expects texture in eax
		push	5DAEB7h
		retn
	}
}

// from Silent
void __declspec(naked)
rxD3D9DefaultRenderCallback_Hook(void)
{
	_asm
	{
		mov	ecx, [gpCurrentShaderForDefaultCallbacks]
		cmp	eax, ecx	// _rwD3D9LastPixelShaderUsed
		je	rxD3D9DefaultRenderCallback_Hook_Return
		mov	dword ptr ds:[8E244Ch], ecx
		push	ecx
		mov	eax, dword ptr ds:[0C97C28h]	// RwD3D9Device
		push	eax
		mov	ecx, [eax]
		call	dword ptr [ecx+1ACh]

	rxD3D9DefaultRenderCallback_Hook_Return:
		push	756E17h
		retn
	}
}

void
readInis(void)
{
	original_bRadiosity = doRadiosity;
	doRadiosity = 1;
	if(numConfigs == 0)
		readIni(0);
	else
		for(int i = 1; i <= numConfigs; i++)
			readIni(i);
	resetValues();
}

// load asi ini again after having read stream ini as we need to know radiosity settings
void __declspec(naked)
afterStreamIni(void)
{
	_asm{
		call readInis
		retn
	}
}

char
CPlantMgr_Initialise(void)
{
	char (*oldfunc)(void) = (char (*)(void))0x5DD910;
	char ret;
	ret = oldfunc();

	HRSRC resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_GRASSPS), RT_RCDATA);
	RwUInt32 *shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreatePixelShader(shader, &grassPixelShader);
	FreeResource(shader);

	RpAtomic *atomic;
	for(int i = 0; i < 4; i++){
		atomic = (*plantTab0)[i];
		atomic->renderCallBack = grassRenderCallback;
		atomic = (*plantTab1)[i];
		atomic->renderCallBack = grassRenderCallback;
	}
	return ret;
}

WRAPPER void CRenderer__RenderEverythingBarRoads_orig(void) { EAXJMP(0x553AA0); }

void CRenderer__RenderEverythingBarRoads(void)
{
	reflTexDone = FALSE;
	CRenderer__RenderEverythingBarRoads_orig();
	reflTexDone = TRUE;
}

struct FX
{
	char data[0x54];
	int fxQuality;
	int GetFxQuality_ped(void);
	int GetFxQuality_stencil(void);
};

int
FX::GetFxQuality_ped(void)
{
	if(config->pedShadows >= 0)
		return config->pedShadows ? 3 : 0;
	return this->fxQuality;
}

int
FX::GetFxQuality_stencil(void)
{
	if(config->stencilShadows >= 0)
		return config->stencilShadows ? 3 : 0;
	return this->fxQuality;
}

static BOOL (*IsAlreadyRunning)();

BOOL
InjectDelayedPatches()
{
	if(!IsAlreadyRunning()){
		// post init stuff
		config = &configs[0];
		findInis();
		if(numConfigs == 0)
			readIni(0);
		else
			readIni(1);
		// only load one ini for now, others are loaded later by readInis()

		if(usePCTimecyc){
			MemoryVP::Nop(0x5BBF6F, 2);
			MemoryVP::Nop(0x5BBF83, 2);
		}

		// DNPipeline
		if(config->worldPipe >= 0){
			MemoryVP::InjectHook(0x5D7100, CCustomBuildingDNPipeline__CreateCustomObjPipe_PS2);
			MemoryVP::Patch<BYTE>(0x5D7200, 0xC3);	// disable interpolation
		}

		//if(uglyWheelHack)
		//	MemoryVP::Patch<DWORD>(0x5d5b48, (DWORD)CCarFXRenderer__CustomCarPipeClumpSetup);

		if(oneGrassModel){
			char *modelname = "grass0_1.dff";
			MemoryVP::Patch<DWORD>(0x5DDA87, (DWORD)modelname);
			MemoryVP::Patch<DWORD>(0x5DDA8F, (DWORD)modelname);
			MemoryVP::Patch<DWORD>(0x5DDA97, (DWORD)modelname);
			MemoryVP::Patch<DWORD>(0x5DDA9F, (DWORD)modelname);

			MemoryVP::Patch<DWORD>(0x5DDAC3, (DWORD)modelname);
			MemoryVP::Patch<DWORD>(0x5DDACB, (DWORD)modelname);
			MemoryVP::Patch<DWORD>(0x5DDAD3, (DWORD)modelname);
			MemoryVP::Patch<DWORD>(0x5DDADB, (DWORD)modelname);
		}

		//if(disableHQShadows){
			// disable stencil shadows
			//MemoryVP::Patch<BYTE>(0x53E159, 0xC3);
			//MemoryVP::Nop(0x53C1AB, 5);
			// stencil???
			MemoryVP::InjectHook(0x7113B8, &FX::GetFxQuality_stencil);
			MemoryVP::InjectHook(0x711D95, &FX::GetFxQuality_stencil);
			// use static ped shadows
			//MemoryVP::Patch<WORD>(0x5E6789, 0xe990);
			MemoryVP::InjectHook(0x5E675E, &FX::GetFxQuality_ped);
			MemoryVP::InjectHook(0x5E676D, &FX::GetFxQuality_ped);
			
			MemoryVP::InjectHook(0x706BC4, &FX::GetFxQuality_ped);
			MemoryVP::InjectHook(0x706BD3, &FX::GetFxQuality_ped);
			// vehicle pole
			MemoryVP::InjectHook(0x70F9B8, &FX::GetFxQuality_stencil);
			// enable low quality car shadows
			//MemoryVP::Nop(0x70BDAB, 6);
			// enable low quality pole shadows
			//MemoryVP::Nop(0x70C75A, 6);
		//}

		if(disableClouds){
			// jump over cloud loop
			MemoryVP::InjectHook(0x714145, 0x71422A, PATCH_JUMP);
		}

		//loadColorcycle();
		return FALSE;
	}
	return TRUE;
}

struct PointLight
{
	RwV3d pos;
	RwV3d dir;
	float radius;
	float color[3];
	void *attachedTo;
	char type;
	char fogType;
	char generateExtraShadows;
	char pad;
};

PointLight *pointLights = (PointLight*)0xC3F0E0;

WRAPPER void
CSprite__RenderBufferedOneXLUSprite_Rotate_Aspect_orig(float x, float y, float z, float a4, float a5, RwUInt8 r, RwUInt8 g, RwUInt8 b, RwInt16 f, int a10, float a11, RwUInt8 alpha) { EAXJMP(0x70E780); }

int currentLight;
char *stkp;

unsigned __int64 rand_seed = 1;

int ps2rand()
{
	rand_seed = 0x5851F42D4C957F2D * rand_seed + 1;
	return ((rand_seed >> 32) & 0x7FFFFFFF);
}

void
CSprite__RenderBufferedOneXLUSprite_Rotate_Aspect(float x, float y, float z, float a4, float a5, RwUInt8 r, RwUInt8 g, RwUInt8 b, RwInt16 f, int a10, float a11, RwUInt8 alpha)
{
	_asm mov [currentLight], esi
	_asm mov [stkp], ebp
	float mult = *(float*)(stkp + 0x48);
	currentLight /= sizeof(PointLight);

	float add = pointLights[currentLight].fogType == 1 ? 0.0f : 16.0f;
	r = mult*pointLights[currentLight].color[0]+add;
	g = mult*pointLights[currentLight].color[1]+add;
	b = mult*pointLights[currentLight].color[2]+add;
	f = 0xFF;
	CSprite__RenderBufferedOneXLUSprite_Rotate_Aspect_orig(x, y, z, a4, a5, r, g, b, f, a10, a11, alpha);
}

WRAPPER void CPointLights__RenderFogEffect_orig(void) { EAXJMP(0x7002D0); }

BOOL WINAPI
DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
	if(reason == DLL_PROCESS_ATTACH){
		// TODO: is this correct?
		if(*(DWORD*)DynBaseAddress(0x82457C) != 0x94BF &&
		   *(DWORD*)DynBaseAddress(0x8245BC) == 0x94BF)
			return FALSE;
		dllModule = hInst;

/*		AllocConsole();
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);*/

		IsAlreadyRunning = (BOOL(*)())(*(int*)(0x74872D+1) + 0x74872D + 5);
		MemoryVP::InjectHook(0x74872D, InjectDelayedPatches);

		MemoryVP::InjectHook(0x5BCF14, afterStreamIni, PATCH_JUMP);

		MemoryVP::InjectHook(0x7491C0, myDefaultCallback, PATCH_JUMP);

		MemoryVP::InjectHook(0x5BF8EA, CPlantMgr_Initialise);
		MemoryVP::InjectHook(0x756DFE, rxD3D9DefaultRenderCallback_Hook, PATCH_JUMP);
		MemoryVP::InjectHook(0x5DADB7, fixSeed, PATCH_JUMP);
		MemoryVP::InjectHook(0x5DAE61, saveIntensity, PATCH_JUMP);
		MemoryVP::InjectHook(0x560330, CTimeCycle_GetAmbientRed, PATCH_JUMP);
		MemoryVP::InjectHook(0x560340, CTimeCycle_GetAmbientGreen, PATCH_JUMP);
		MemoryVP::InjectHook(0x560350, CTimeCycle_GetAmbientBlue, PATCH_JUMP);
		MemoryVP::Patch<DWORD>(0x5DAEC8, (DWORD)setTextureAndColor);

		MemoryVP::InjectHook(0x5DDB47, SetCloseFarAlphaDist);

		// VehiclePipeline
		MemoryVP::InjectHook(0x5D9FE9, setVehiclePipeCB);

		// postfx
		MemoryVP::InjectHook(0x74EAA6, CPostEffects::Init, PATCH_JUMP);
		MemoryVP::InjectHook(0x704D1E, CPostEffects::ColourFilter_switch);
		MemoryVP::InjectHook(0x704D5D, CPostEffects::Radiosity_PS2);
		MemoryVP::InjectHook(0x704FB3, CPostEffects::Radiosity_PS2);
		// fix speedfx
		MemoryVP::InjectHook(0x704E8A, CPostEffects::SpeedFX_Fix);

		// infrared vision
		MemoryVP::InjectHook(0x704F4B, CPostEffects::InfraredVision_PS2);
		MemoryVP::InjectHook(0x704F59, CPostEffects::Grain_PS2);
		// night vision
		MemoryVP::InjectHook(0x704EDA, CPostEffects::NightVision_PS2);
		MemoryVP::InjectHook(0x704EE8, CPostEffects::Grain_PS2);
		// TODO: rain grain @ 0x705078
		MemoryVP::InjectHook(0x705091, CPostEffects::Grain_PS2);

		MemoryVP::InjectHook(0x53DFDD, CRenderer__RenderEverythingBarRoads);
		MemoryVP::InjectHook(0x53DD27, CRenderer__RenderEverythingBarRoads); // unused?

///		MemoryVP::InjectHook(0x7000E0, CPointLights__AddLight, PATCH_JUMP);
//		MemoryVP::InjectHook(0x53E21D, CPointLights__RenderFogEffect);
//		MemoryVP::InjectHook(0x7FB824, _rwD3D9SetPixelShader_override);

//		MemoryVP::InjectHook(0x700817, CSprite__RenderBufferedOneXLUSprite_Rotate_Aspect);
		MemoryVP::InjectHook(0x700B6B, CSprite__RenderBufferedOneXLUSprite_Rotate_Aspect);

		MemoryVP::InjectHook(0x44E82E, ps2rand);
		MemoryVP::InjectHook(0x44ECEE, ps2rand);
		MemoryVP::InjectHook(0x42453B, ps2rand);
		MemoryVP::InjectHook(0x42454D, ps2rand);

		//MemoryVP::InjectHook(0x53ECA1, myPluginAttach);

//		MemoryVP::Nop(0x748054, 10);
///		MemoryVP::Nop(0x748063, 5);
//		MemoryVP::Nop(0x747FB0, 10);
//		MemoryVP::Nop(0x748A87, 12);
//		MemoryVP::InjectHook(0x747F98, 0x748446, PATCH_JUMP);

//		MemoryVP::Nop(0x6EFC9B, 5);
//		MemoryVP::Nop(0x6EFE4C, 5);
	}

	return TRUE;
}
