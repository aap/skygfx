#include "skygfx.h"

HMODULE dllModule;

Config *config, configs[2];
bool usePCTimecyc, disableHQShadows, disableClouds;
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
	config->dualPassGrass = GetPrivateProfileInt("SkyGfx", "dualPassGrass", TRUE, modulePath) != FALSE;
	config->dualPassVehicle = GetPrivateProfileInt("SkyGfx", "dualPassVehicle", TRUE, modulePath) != FALSE;
	config->grassAddAmbient = GetPrivateProfileInt("SkyGfx", "grassAddAmbient", TRUE, modulePath) != FALSE;
	config->fixGrassPlacement = GetPrivateProfileInt("SkyGfx", "fixGrassPlacement", TRUE, modulePath) != FALSE;
	config->oneGrassModel = GetPrivateProfileInt("SkyGfx", "oneGrassModel", TRUE, modulePath) != FALSE;
	config->backfaceCull = GetPrivateProfileInt("SkyGfx", "backfaceCull", FALSE, modulePath) != FALSE;
	config->vehiclePipe = GetPrivateProfileInt("SkyGfx", "vehiclePipe", 0, modulePath) % 3;
	config->worldPipe = GetPrivateProfileInt("SkyGfx", "worldPipe", 0, modulePath) % 3;
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
	config->radiosityOffset = GetPrivateProfileInt("SkyGfx", "radiosityOffset", 16, modulePath);
	GetPrivateProfileString("SkyGfx", "radiosityIntensity", "2.0", tmp, sizeof(tmp), modulePath);
	config->radiosityIntensity = atof(tmp);
	disableHQShadows = GetPrivateProfileInt("SkyGfx", "disableHQShadows", FALSE, modulePath) != FALSE;
	disableClouds = GetPrivateProfileInt("SkyGfx", "disableClouds", FALSE, modulePath) != FALSE;

	GetPrivateProfileString("SkyGfx", "farDist", "60.0", tmp, sizeof(tmp), modulePath);
	config->farDist = atof(tmp);
	GetPrivateProfileString("SkyGfx", "blendDist", "20.0", tmp, sizeof(tmp), modulePath);
	config->fadeDist = atof(tmp);
	config->fadeInvDist = 1.0f/config->fadeDist;
	GetPrivateProfileString("SkyGfx", "densityMult", "1.0", tmp, sizeof(tmp), modulePath);
	config->densityMult = atof(tmp)*0.5;

//	MemoryVP::Patch<DWORD>(0x5DC281, (DWORD)&config->densityMult);
//	MemoryVP::Patch<DWORD>(0x5DAD98, (DWORD)&config->fadeDist);
//	MemoryVP::Patch<DWORD>(0x5DAE05, (DWORD)&config->fadeInvDist);
	*(float**)0x5DC281 = &config->densityMult;
	*(float**)0x5DAD98 = &config->fadeDist;
	*(float**)0x5DAE05 = &config->fadeInvDist;
}

RpAtomic *(*plantTab0)[4] = (RpAtomic *(*)[4])0xC039F0;
RpAtomic *(*plantTab1)[4] = (RpAtomic *(*)[4])0xC03A00;

RpAtomic*
grassRenderCallback(RpAtomic *atomic)
{
	RpAtomic *(*oldfunc)(RpAtomic*) = (RpAtomic *(*)(RpAtomic*))0x7491C0;
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

	if(config->dualPassGrass){
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)128);
		RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONGREATEREQUAL);
		ret = oldfunc(atomic);
		RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONLESS);
		ret = oldfunc(atomic);
	}else
		ret = oldfunc(atomic);

	RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)cullmode);
	gpCurrentShaderForDefaultCallbacks = NULL;

	return ret;
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
		cmp	[ecx+20], 0	// fixGrassPlacement
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
}

// load asi ini again after having read stream ini as we need no know radiosity settings
void __declspec(naked)
afterStreamIni(void)
{
	_asm{
		call readIniAndCopy
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

		if(config->oneGrassModel){
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

BOOL WINAPI
DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
	if(reason == DLL_PROCESS_ATTACH){
		// TODO: is this correct?
		if(*(DWORD*)DynBaseAddress(0x82457C) != 0x94BF &&
		   *(DWORD*)DynBaseAddress(0x8245BC) == 0x94BF)
			return FALSE;
		dllModule = hInst;

/*
		// only with /MD
		AllocConsole();
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
*/

		IsAlreadyRunning = (BOOL(*)())(*(int*)(0x74872D+1) + 0x74872D + 5);
		MemoryVP::InjectHook(0x74872D, InjectDelayedPatches);

		MemoryVP::InjectHook(0x5BCF14, afterStreamIni, PATCH_JUMP);

		MemoryVP::InjectHook(0x5BF8EA, CPlantMgr_Initialise);
		MemoryVP::InjectHook(0x756DFE, rxD3D9DefaultRenderCallback_Hook, PATCH_JUMP);
		MemoryVP::InjectHook(0x5DADB7, fixSeed, PATCH_JUMP);
		MemoryVP::InjectHook(0x5DAE61, saveIntensity, PATCH_JUMP);
		MemoryVP::InjectHook(0x560330, CTimeCycle_GetAmbientRed, PATCH_JUMP);
		MemoryVP::InjectHook(0x560340, CTimeCycle_GetAmbientGreen, PATCH_JUMP);
		MemoryVP::InjectHook(0x560350, CTimeCycle_GetAmbientBlue, PATCH_JUMP);
		MemoryVP::Patch<DWORD>(0x5DAEC8, (DWORD)setTextureAndColor);

		MemoryVP::InjectHook(0x5DDB47, SetCloseFarAlphaDist);

		// DNPipeline
		MemoryVP::InjectHook(0x5D7100, CCustomBuildingDNPipeline__CreateCustomObjPipe_PS2);
		MemoryVP::Patch<BYTE>(0x5D7200, 0xC3);	// disable interpolation

		// VehiclePipeline
		MemoryVP::InjectHook(0x5D9FE9, setVehiclePipeCB);

		// postfx
		MemoryVP::InjectHook(0x74EAA6, CPostEffects__Init, PATCH_JUMP);
		MemoryVP::InjectHook(0x704D1E, CPostEffects__ColourFilter_switch);
		MemoryVP::InjectHook(0x704D5D, CPostEffects__Radiosity_PS2);
		MemoryVP::InjectHook(0x704FB3, CPostEffects__Radiosity_PS2);
		// fix speedfx
		MemoryVP::InjectHook(0x704E8A, SpeedFX_Fix);
	}

	return TRUE;
}
