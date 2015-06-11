#include "skygfx.h"

HMODULE dllModule;

Config *config, configs[2];
bool oneGrassModel, usePCTimecyc, disableHQShadows, disableClouds, uglyWheelHack;
int original_bRadiosity = 0;

void *grassPixelShader;
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
resetRadiosityValues(void)
{
	CPostEffects__m_RadiosityFilterPasses = config->radiosityFilterPasses;
	CPostEffects__m_RadiosityRenderPasses = config->radiosityRenderPasses;
	CPostEffects__m_RadiosityIntensityLimit = config->radiosityIntensityLimit;
	CPostEffects__m_RadiosityIntensity = config->radiosityIntensity;
	CPostEffects__m_RadiosityFilterUCorrection = config->radiosityFilterUCorrection;
	CPostEffects__m_RadiosityFilterVCorrection = config->radiosityFilterVCorrection;
}

int
readhex(char *str)
{
	int n = 0;
	if(strlen(str) > 2)
		sscanf(str+2, "%X", &n);
	return n;
}

void
readIni(void)
{
	int tmpint;
	char tmp[32];
	char modulePath[MAX_PATH];

	GetModuleFileName(dllModule, modulePath, MAX_PATH);
	size_t nLen = strlen(modulePath);
	modulePath[nLen-1] = L'i';
	modulePath[nLen-2] = L'n';
	modulePath[nLen-3] = L'i';

	GetPrivateProfileString("SkyGfx", "keySwitch", "0x79", tmp, sizeof(tmp), modulePath);
	config->keys[0] = readhex(tmp);
	GetPrivateProfileString("SkyGfx", "keyReload", "0x7A", tmp, sizeof(tmp), modulePath);
	config->keys[1] = readhex(tmp);
	GetPrivateProfileString("SkyGfx", "keyReloadPlants", "0x7B", tmp, sizeof(tmp), modulePath);
	config->keys[2] = readhex(tmp);

	config->enableHotkeys = GetPrivateProfileInt("SkyGfx", "enableHotkeys", TRUE, modulePath) != FALSE;
	config->ps2Ambient = GetPrivateProfileInt("SkyGfx", "ps2Ambient", TRUE, modulePath) != FALSE;
	config->ps2ModulateWorld = GetPrivateProfileInt("SkyGfx", "ps2ModulateWorld", TRUE, modulePath) != FALSE;
	config->ps2ModulateGrass = GetPrivateProfileInt("SkyGfx", "ps2ModulateGrass", TRUE, modulePath) != FALSE;
	config->dualPassWorld = GetPrivateProfileInt("SkyGfx", "dualPassWorld", TRUE, modulePath) != FALSE;
	config->dualPassDefault = GetPrivateProfileInt("SkyGfx", "dualPassDefault", TRUE, modulePath) != FALSE;
	config->dualPassVehicle = GetPrivateProfileInt("SkyGfx", "dualPassVehicle", TRUE, modulePath) != FALSE;
	config->dualPassPed = GetPrivateProfileInt("SkyGfx", "dualPassPed", TRUE, modulePath) != FALSE;
	config->grassAddAmbient = GetPrivateProfileInt("SkyGfx", "grassAddAmbient", TRUE, modulePath) != FALSE;
	config->fixGrassPlacement = GetPrivateProfileInt("SkyGfx", "fixGrassPlacement", TRUE, modulePath) != FALSE;
	oneGrassModel = GetPrivateProfileInt("SkyGfx", "oneGrassModel", TRUE, modulePath) != FALSE;
	config->backfaceCull = GetPrivateProfileInt("SkyGfx", "backfaceCull", FALSE, modulePath) != FALSE;
	config->vehiclePipe = GetPrivateProfileInt("SkyGfx", "vehiclePipe", 0, modulePath) % 4;
	tmpint = GetPrivateProfileInt("SkyGfx", "worldPipe", 0, modulePath);
	config->worldPipe = tmpint >= 0 ? tmpint % 3 : -1;
	config->colorFilter = GetPrivateProfileInt("SkyGfx", "colorFilter", 0, modulePath) % 3;
	usePCTimecyc = GetPrivateProfileInt("SkyGfx", "usePCTimecyc", FALSE, modulePath) != FALSE;

	tmpint = GetPrivateProfileInt("SkyGfx", "blurLeft", 4000, modulePath);
	config->offLeft = tmpint == 4000 ? colourLeftVOffset : tmpint;
	tmpint = GetPrivateProfileInt("SkyGfx", "blurTop", 4000, modulePath);
	config->offTop = tmpint == 4000 ? colourTopVOffset : tmpint;
	tmpint = GetPrivateProfileInt("SkyGfx", "blurRight", 4000, modulePath);
	config->offRight = tmpint == 4000 ? colourRightVOffset : tmpint;
	tmpint = GetPrivateProfileInt("SkyGfx", "blurBottom", 4000, modulePath);
	config->offBottom = tmpint == 4000 ? colourBottomVOffset : tmpint;

	config->scaleOffsets = GetPrivateProfileInt("SkyGfx", "scaleOffsets", TRUE, modulePath) != FALSE;
	tmpint = GetPrivateProfileInt("SkyGfx", "doRadiosity", 4000, modulePath);
	config->doRadiosity = tmpint == 4000 ? original_bRadiosity : tmpint;	// saved value from stream.ini
	config->downSampleRadiosity = GetPrivateProfileInt("SkyGfx", "downSampleRadiosity", TRUE, modulePath) != FALSE;
	config->radiosityOffset = GetPrivateProfileInt("SkyGfx", "radiosityOffset", 16, modulePath);
//	GetPrivateProfileString("SkyGfx", "radiosityIntensity", "2.0", tmp, sizeof(tmp), modulePath);
//	config->radiosityIntensity = atof(tmp);

	config->radiosityFilterPasses = GetPrivateProfileInt("SkyGfx", "radiosityFilterPasses", 2, modulePath);
	config->radiosityRenderPasses = GetPrivateProfileInt("SkyGfx", "radiosityRenderPasses", 1, modulePath);
	config->radiosityIntensityLimit = GetPrivateProfileInt("SkyGfx", "radiosityIntensityLimit", 0xDC, modulePath);
	config->radiosityIntensity = GetPrivateProfileInt("SkyGfx", "radiosityIntensity", 0x23, modulePath);
	config->radiosityFilterUCorrection = GetPrivateProfileInt("SkyGfx", "radiosityFilterUCorrection", 2, modulePath);
	config->radiosityFilterVCorrection = GetPrivateProfileInt("SkyGfx", "radiosityFilterVCorrection", 2, modulePath);

	config->vcsTrails = GetPrivateProfileInt("SkyGfx", "vcsTrails", FALSE, modulePath) != FALSE;
	config->trailsLimit = GetPrivateProfileInt("SkyGfx", "trailsLimit", 80, modulePath);
	config->trailsIntensity = GetPrivateProfileInt("SkyGfx", "trailsIntensity", 38, modulePath);
	disableHQShadows = GetPrivateProfileInt("SkyGfx", "disableHQShadows", FALSE, modulePath) != FALSE;
	disableClouds = GetPrivateProfileInt("SkyGfx", "disableClouds", FALSE, modulePath) != FALSE;
	uglyWheelHack = GetPrivateProfileInt("SkyGfx", "uglyWheelHack", FALSE, modulePath) != FALSE;

	config->waterWriteZ = GetPrivateProfileInt("SkyGfx", "waterWriteZ", FALSE, modulePath) != FALSE;

	GetPrivateProfileString("SkyGfx", "farDist", "60.0", tmp, sizeof(tmp), modulePath);
	config->farDist = atof(tmp);
	GetPrivateProfileString("SkyGfx", "blendDist", "20.0", tmp, sizeof(tmp), modulePath);
	config->fadeDist = atof(tmp);
	config->fadeInvDist = 1.0f/config->fadeDist;
	GetPrivateProfileString("SkyGfx", "densityMult", "1.0", tmp, sizeof(tmp), modulePath);
	config->densityMult = atof(tmp)*0.5;

	*(float**)0x5DC281 = &config->densityMult;
	*(float**)0x5DAD98 = &config->fadeDist;
	*(float**)0x5DAE05 = &config->fadeInvDist;
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

	if(config->dualPassDefault)
		RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
	ret = AtomicDefaultRenderCallBack(atomic);

	RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)cullmode);
	gpCurrentShaderForDefaultCallbacks = NULL;

	return ret;
}

RpAtomic *myDefaultCallback(RpAtomic *atomic)
{
	static RwRect reflRect = { 0, 0, 128, 128 };
	RxPipeline *pipe;
	int zwrite, alphatest, alpharef;
	int dodual = 0;
	int detach = 0;

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
			RwRasterRenderScaled(RwCameraGetRaster(Camera), &reflRect);
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
		col[0] += CTimeCycle_GetAmbientRed()*255;
		col[1] += CTimeCycle_GetAmbientGreen()*255;
		col[2] += CTimeCycle_GetAmbientBlue()*255;
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
readIniAndCopy(void)
{
	original_bRadiosity = doRadiosity;
	doRadiosity = 1;
	readIni();
	configs[1] = configs[0];
	resetRadiosityValues();
}

// load asi ini again after having read stream ini as we need to know radiosity settings
void __declspec(naked)
afterStreamIni(void)
{
	_asm{
		call readIniAndCopy
		retn
	}
}

void __declspec(naked)
waterZwrite(void)
{
	_asm{
		mov	edx, dword ptr ds:[0xC97B24]
		mov	ecx, [config]
		push	[ecx+4]	// waterWriteZ
		push	0x6EF8C0
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

static BOOL (*IsAlreadyRunning)();

BOOL
InjectDelayedPatches()
{
	if(!IsAlreadyRunning()){
		// post ini stuff
		config = &configs[0];
		readIni();
		configs[1] = configs[0];

		if(usePCTimecyc){
			MemoryVP::Nop(0x5BBF6F, 2);
			MemoryVP::Nop(0x5BBF83, 2);
		}

		// DNPipeline
		if(config->worldPipe >= 0){
			MemoryVP::InjectHook(0x5D7100, CCustomBuildingDNPipeline__CreateCustomObjPipe_PS2);
			MemoryVP::Patch<BYTE>(0x5D7200, 0xC3);	// disable interpolation
		}

		if(uglyWheelHack)
			MemoryVP::Patch<DWORD>(0x5d5b48, (DWORD)CCarFXRenderer__CustomCarPipeClumpSetup);

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

		if(disableHQShadows){
			// disable stencil shadows
			MemoryVP::Patch<BYTE>(0x53E159, 0xC3);
			MemoryVP::Nop(0x53C1AB, 5);
			// use static ped shadows
			MemoryVP::Patch<WORD>(0x5E6789, 0xe990);
			// enable low quality car shadows
			MemoryVP::Nop(0x70BDAB, 6);
			// enable low quality pole shadows
			MemoryVP::Nop(0x70C75A, 6);
		}

		if(disableClouds){
			// jump over cloud loop
			MemoryVP::InjectHook(0x714145, 0x71422A, PATCH_JUMP);
		}
		return FALSE;
	}
	return TRUE;
}


void __declspec(naked)
CPointLights__AddLight_original(char type, float x, float y, float z, float x_dir, float y_dir, float z_dir, float radius, float r, float g, float b, char fogType, char generateExtraShadows, void *attachedTo)
{
	_asm{
		mov	ecx, dword ptr ds:[0xB6F03C]
		push	0x7000E6
		retn
	}
}

void
CPointLights__AddLight(char type, float x, float y, float z, float x_dir, float y_dir, float z_dir, float radius, float r, float g, float b, char fogType, char generateExtraShadows, void *attachedTo)
{
//	CPointLights__AddLight_original(type, x, y, z, x_dir, y_dir, z_dir, radius, 255.0f, 128.0f, 0.0f, fogType, generateExtraShadows, attachedTo);
	CPointLights__AddLight_original(type, x, y, z, x_dir, y_dir, z_dir, radius, r, g, b, fogType, generateExtraShadows, attachedTo);
}

WRAPPER void
CSprite__RenderBufferedOneXLUSprite_Rotate_Aspect_orig(float a1, float a2, float a3, float a4, float a5, unsigned __int8 r, unsigned __int8 g, unsigned __int8 b, __int16 a9, int a10, float a11, char a12) { EAXJMP(0x70E780); }


void
CSprite__RenderBufferedOneXLUSprite_Rotate_Aspect(float a1, float a2, float a3, float a4, float a5, unsigned __int8 r, unsigned __int8 g, unsigned __int8 b, __int16 a9, int a10, float a11, char a12)
{
//	printf("%d %d %d - %d > %d %d %d\n", r, g, b, a9, r*a9, g*a9, b*a9);
//	r = g = b = 255;
///	r = g = b = 14;
///	a9 = 256;
	CSprite__RenderBufferedOneXLUSprite_Rotate_Aspect_orig(a1, a2, a3, a4, a5, r, g, b, a9, a10, a11, a12);
}

WRAPPER void CPointLights__RenderFogEffect_orig(void) { EAXJMP(0x7002D0); }

/*
void *fogShader;
int doOverride;
void *overridePS;

void _rwD3D9SetPixelShader_override(void *shader)
{
	if(doOverride);
		_rwD3D9SetPixelShader(doOverride ? overridePS : shader);
}

void
CPointLights__RenderFogEffect(void)
{
	if(fogShader == NULL){
		HRSRC resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_FOGPS), RT_RCDATA);
		RwUInt32 *shader = (RwUInt32*)LoadResource(dllModule, resource);
		RwD3D9CreatePixelShader(shader, &fogShader);
		FreeResource(shader);
	}

//	doOverride = 1;
//	overridePS = fogShader;
	CPointLights__RenderFogEffect_orig();
//	doOverride = 0;
//	RwD3D9SetPixelShader(NULL);
}
*/

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
		MemoryVP::InjectHook(0x74EAA6, CPostEffects__Init, PATCH_JUMP);
		MemoryVP::InjectHook(0x704D1E, CPostEffects__ColourFilter_switch);
		MemoryVP::InjectHook(0x704D5D, CPostEffects__Radiosity_PS2);
		MemoryVP::InjectHook(0x704FB3, CPostEffects__Radiosity_PS2);
		// fix speedfx
		MemoryVP::InjectHook(0x704E8A, SpeedFX_Fix);

		MemoryVP::InjectHook(0x53DFDD, CRenderer__RenderEverythingBarRoads);
		MemoryVP::InjectHook(0x53DD27, CRenderer__RenderEverythingBarRoads); // unused?

///		MemoryVP::InjectHook(0x7000E0, CPointLights__AddLight, PATCH_JUMP);
//		MemoryVP::InjectHook(0x53E21D, CPointLights__RenderFogEffect);
//		MemoryVP::InjectHook(0x7FB824, _rwD3D9SetPixelShader_override);

//		MemoryVP::InjectHook(0x700817, CSprite__RenderBufferedOneXLUSprite_Rotate_Aspect);
///		MemoryVP::InjectHook(0x700B6B, CSprite__RenderBufferedOneXLUSprite_Rotate_Aspect);

//		MemoryVP::InjectHook(0x6EF8B9, waterZwrite, PATCH_JUMP);
//		MemoryVP::Nop(0x6EF8BE, 2);

//		MemoryVP::Nop(0x53E21D, 5);
	}

	return TRUE;
}
