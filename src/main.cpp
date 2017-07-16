#include "skygfx.h"
#include "neo.h"
#include "ini_parser.hpp"

HMODULE dllModule;
char asipath[MAX_PATH];

int numConfigs;
Config *config, configs[10];
bool ps2grassFiles, usePCTimecyc, disableClouds, disableGamma, neoWaterDrops, fixPcCarLight;
int transparentLockon;
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

extern "C" {
__declspec(dllexport) Config*
GetConfig(void)
{
	return config;
}
}

char*
getpath(char *path)
{
	static char tmppath[MAX_PATH];
	FILE *f;

	f = fopen(path, "r");
	if (f) {
		fclose(f);
		return path;
	}
	extern char asipath[];
	strncpy(tmppath, asipath, MAX_PATH);
	strcat(tmppath, path);
	f = fopen(tmppath, "r");
	if (f) {
		fclose(f);
		return tmppath;
	}
	return NULL;
}



void
D3D9Render(RxD3D9ResEntryHeader *resEntryHeader, RxD3D9InstanceData *instanceData)
{
	if(resEntryHeader->indexBuffer)
		RwD3D9DrawIndexedPrimitive(resEntryHeader->primType, instanceData->baseIndex, 0, instanceData->numVertices, instanceData->startIndex, instanceData->numPrimitives);
	else
		RwD3D9DrawPrimitive(resEntryHeader->primType, instanceData->baseIndex, instanceData->numPrimitives);
}

void
D3D9RenderDual(int dual, RxD3D9ResEntryHeader *resEntryHeader, RxD3D9InstanceData *instanceData)
{
	RwBool hasAlpha;
	int alphafunc, alpharef;
	int zwrite;
	// this also takes texture alpha into account
	RwD3D9GetRenderState(D3DRS_ALPHABLENDENABLE, &hasAlpha);
	RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &zwrite);
	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &alphafunc);
	if(dual && hasAlpha && zwrite){
		RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &alpharef);
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)128);
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONGREATEREQUAL);
		RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
		D3D9Render(resEntryHeader, instanceData);
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONLESS);
		RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
		D3D9Render(resEntryHeader, instanceData);
		RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)zwrite);
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)alpharef);
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)alphafunc);
	}else if(!zwrite){
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONALWAYS);
		D3D9Render(resEntryHeader, instanceData);
		RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)alphafunc);
	}else
		D3D9Render(resEntryHeader, instanceData);
}

// Add a dual pass to the PC pipeline, the lazy way
void
D3D9RenderNotLit_DUAL(RxD3D9ResEntryHeader *resEntryHeader, RxD3D9InstanceData *instanceData)
{
	RwD3D9SetPixelShader(NULL);
	RwD3D9SetVertexShader(instanceData->vertexShader);
	D3D9RenderDual(config->dualPassVehicle, resEntryHeader, instanceData);
}

void
D3D9RenderPreLit_DUAL(RxD3D9ResEntryHeader *resEntryHeader, RxD3D9InstanceData *instanceData, RwUInt8 flags, RwTexture *texture)
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
	D3D9RenderDual(config->dualPassVehicle, resEntryHeader, instanceData);
}

WRAPPER double CTimeCycle_GetAmbientRed(void) { EAXJMP(0x560330); }
WRAPPER double CTimeCycle_GetAmbientGreen(void) { EAXJMP(0x560340); }
WRAPPER double CTimeCycle_GetAmbientBlue(void) { EAXJMP(0x560350); }

void
resetValues(void)
{
	CPostEffects__m_RadiosityFilterPasses = config->radiosityFilterPasses;
	CPostEffects__m_RadiosityRenderPasses = config->radiosityRenderPasses;
//	CPostEffects__m_RadiosityIntensityLimit = config->radiosityIntensityLimit;	// only used for timecycle override
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
	//if(config->nightVision == 0)
	//	Patch<float>(0x735F8B, 0.4f);
	//else
	//	Patch<float>(0x735F8B, 1.0f);

	//*(int*)0x8D37D0 = config->detailedWaterDist;
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

RpAtomic*
myDefaultCallback(RpAtomic *atomic)
{
	RxPipeline *pipe;
	int zwrite, alphatest, alpharef;
	int dodual = 0;
	int detach = 0;

	pipe = atomic->pipeline;
	if(pipe == NULL){
		pipe = *(RxPipeline**)(*(DWORD*)0xC97B24+0x3C+dword_C9BC60);
		RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, (void*)&zwrite);
		if(zwrite && config->dualPassDefault)
			dodual = 1;
	}else if(pipe == skinPipe && config->dualPassPed)
		dodual = 1;
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
		cmp	[ecx+4], 0	// fixGrassPlacement
		jle	dontfix
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

/*
WRAPPER void CRenderer__RenderEverythingBarRoads_orig(void) { EAXJMP(0x553AA0); }

void CRenderer__RenderEverythingBarRoads(void)
{
	reflTexDone = FALSE;
	CRenderer__RenderEverythingBarRoads_orig();
	reflTexDone = TRUE;
}
*/

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

unsigned __int64 rand_seed = 1;
float ps2randnormalize = 1.0f/0x7FFFFFFF;

int ps2rand()
{
	rand_seed = 0x5851F42D4C957F2D * rand_seed + 1;
	return ((rand_seed >> 32) & 0x7FFFFFFF);
}

void ps2srand(unsigned int seed)
{
	rand_seed = seed;
}

void __declspec(naked) floatbitpattern(void)
{
	_asm {
		fstp [esp-4]
		mov eax, [esp-4]
		ret
	}
}

WRAPPER void gtasrand(unsigned int seed) { EAXJMP(0x821B11); }

void
mysrand(unsigned int seed)
{
	gtasrand(ps2rand());
//	gtasrand(seed);
}

WRAPPER void CVehicle__DoSunGlare(void *this_) { EAXJMP(0x6DD6F0); }

void __declspec(naked) doglare(void)
{
	_asm {
		mov	ecx, [config]
		cmp	[ecx+8], 0	// doglare
		jle	noglare
		mov	ecx,esi
		call	CVehicle__DoSunGlare
	noglare:
		mov     [esp+0D4h], edi
		push	6ABD04h
		retn
	}
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

/*
struct CVector { float x, y, z; };
WRAPPER void CWaterLevel__CalculateWavesForCoordinate(int x, int y, float a3, float a4, float *z, float *colorMult, float *a7, CVector *vecnormal){ EAXJMP(0x6E6EF0); }
void
CWaterLevel__CalculateWavesForCoordinate_hook(int x, int y, float a3, float a4, float *z, float *colorMult, float *a7, CVector *vecnormal)
{
	CWaterLevel__CalculateWavesForCoordinate(x, y, a3, a4, z, colorMult, a7, vecnormal);
	*colorMult = 0.577f;
}
*/

void (*CSkidmarks__Render_orig)(void);
void CSkidmarks__Render(void)
{
	int alphafunc;
	RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &alphafunc);
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONALWAYS);
	CSkidmarks__Render_orig();
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)alphafunc);
}

void (*InitialiseGame)(void);
void
InitialiseGame_hook(void)
{
	ONCE;
	neoInit();
	InitialiseGame();
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

struct StrAssoc
{
	const char *key;
	int val;

	static int get(StrAssoc *desc, const char *key);
};
int
StrAssoc::get(StrAssoc *desc, const char *key)
{
	for(; desc->key[0] != '\0'; desc++)
		if(strcmp(desc->key, key) == 0)
			return desc->val;
	return desc->val;
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

int
readhex(const char *str)
{
	int n = 0;
	if(strlen(str) > 2)
		sscanf(str+2, "%X", &n);
	return n;
}

int
readint(const std::string &s, int default = 0)
{
	try{
		return std::stoi(s);
	}catch(...){
		return default;
	}
}

float
readfloat(const std::string &s, float default = 0)
{
	try{
		return std::stof(s);
	}catch(...){
		return default;
	}
}

void
readIni(int n)
{
	int tmpint;
	char modulePath[MAX_PATH];
	GetModuleFileName(dllModule, modulePath, MAX_PATH);
	strncpy(asipath, modulePath, MAX_PATH);
	char *p = strrchr(asipath, '\\');
	if (p) p[1] = '\0';

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
	linb::ini cfg;
	cfg.load_file(modulePath);

	c->keys[0] = readhex(cfg.get("SkyGfx", "keySwitch", "0x0").c_str());
	c->keys[1] = readhex(cfg.get("SkyGfx", "keyReload", "0x0").c_str());

	int ps2Modulate, dualPass;
	ps2Modulate = readint(cfg.get("SkyGfx", "ps2Modulate", ""), 0);
	dualPass = readint(cfg.get("SkyGfx", "dualPass", ""), 0);

	static StrAssoc buildPipeMap[] = {
		{"PS2",     0},
		{"PC",      1},
		{"",       -1},
	};
	c->buildingPipe = StrAssoc::get(buildPipeMap, cfg.get("SkyGfx", "buildingPipe", "").c_str());
	c->ps2ModulateBuilding = readint(cfg.get("SkyGfx", "ps2ModulateBuilding", ""), ps2Modulate);
	c->dualPassBuilding = readint(cfg.get("SkyGfx", "dualPassBuilding", ""), dualPass);

	static StrAssoc vehPipeMap[] = {
		{"PS2",     0},
		{"PC",      1},
		{"Xbox",    2},
		{"Spec",    3},
		{"Neo",     4},
		{"",       -1},
	};
	c->vehiclePipe = StrAssoc::get(vehPipeMap, cfg.get("SkyGfx", "vehiclePipe", "").c_str());
	c->dualPassVehicle = readint(cfg.get("SkyGfx", "dualPassVehicle", ""), dualPass);
	c->neoShininess = readfloat(cfg.get("SkyGfx", "neoShininess", ""), 0.75);
	c->neoSpecularity = readfloat(cfg.get("SkyGfx", "neoSpecularity", ""), 0.75);
	envMapSize = readint(cfg.get("SkyGfx", "neoEnvMapSize", ""), 128);
	int i = 1;
	while(i < envMapSize) i *= 2;
	envMapSize = i;
	c->doglare = readint(cfg.get("SkyGfx", "sunGlare", ""), -1);

	c->ps2ModulateGrass = readint(cfg.get("SkyGfx", "ps2ModulateGrass", ""), ps2Modulate);
	c->dualPassGrass = readint(cfg.get("SkyGfx", "dualPassGrass", ""), dualPass);
	c->grassAddAmbient = readint(cfg.get("SkyGfx", "grassAddAmbient", ""), 0);
	ps2grassFiles = readint(cfg.get("SkyGfx", "ps2grassFiles", ""), 0);
	c->fixGrassPlacement = readint(cfg.get("SkyGfx", "grassFixPlacement", ""), 0);
	c->backfaceCull = readint(cfg.get("SkyGfx", "grassBackfaceCull", ""), 1);

	static StrAssoc boolMap[] = {
		{"0",       0},
		{"false",   0},
		{"1",       1},
		{"true",    1},
		{"",       -1},
	};
	c->dualPassDefault = readint(cfg.get("SkyGfx", "dualPassDefault", ""), dualPass);
	c->dualPassPed = readint(cfg.get("SkyGfx", "dualPassPed", ""), dualPass);
	c->pedShadows = StrAssoc::get(boolMap, cfg.get("SkyGfx", "pedShadows", "").c_str());
	c->stencilShadows = StrAssoc::get(boolMap, cfg.get("SkyGfx", "stencilShadows", "").c_str());
	disableClouds = readint(cfg.get("SkyGfx", "disableClouds", ""), 0);
	disableGamma = readint(cfg.get("SkyGfx", "disableGamma", ""), 0);
	transparentLockon = readint(cfg.get("SkyGfx", "transparentLockon", ""), 0);
	c->lightningIlluminatesWorld = readint(cfg.get("SkyGfx", "lightningIlluminatesWorld", ""), 0);

	static StrAssoc colorFilterMap[] = {
		{"PS2",     0},
		{"PC",      1},
		{"Mobile",  2},
		{"III",     3},
		{"VC",      4},
		{"None",    5},
		{"",        1},
	};
	static StrAssoc ps2pcMap[] = {
		{"PS2",     0},
		{"PC",      1},
		{"",        1},
	};
	c->colorFilter = StrAssoc::get(colorFilterMap, cfg.get("SkyGfx", "colorFilter", "").c_str());
	ps2pcMap[2].val = c->colorFilter == 0 ? 0 : 1;
	c->infraredVision = StrAssoc::get(ps2pcMap, cfg.get("SkyGfx", "infraredVision", "").c_str());
	c->nightVision = StrAssoc::get(ps2pcMap, cfg.get("SkyGfx", "nightVision", "").c_str());
	c->grainFilter = StrAssoc::get(ps2pcMap, cfg.get("SkyGfx", "grainFilter", "").c_str());
	usePCTimecyc = readint(cfg.get("SkyGfx", "usePCTimecyc", ""), 0);

	tmpint = readint(cfg.get("SkyGfx", "blurLeft", ""), 4000);
	c->offLeft = tmpint == 4000 ? colourLeftVOffset : tmpint;
	tmpint = readint(cfg.get("SkyGfx", "blurTop", ""), 4000);
	c->offTop = tmpint == 4000 ? colourTopVOffset : tmpint;
	tmpint = readint(cfg.get("SkyGfx", "blurRight", ""), 4000);
	c->offRight = tmpint == 4000 ? colourRightVOffset : tmpint;
	tmpint = readint(cfg.get("SkyGfx", "blurBottom", ""), 4000);
	c->offBottom = tmpint == 4000 ? colourBottomVOffset : tmpint;

	tmpint = readint(cfg.get("SkyGfx", "doRadiosity", ""), 4000);
	c->doRadiosity = tmpint == 4000 ? original_bRadiosity : tmpint;	// saved value from stream.ini
	c->vcsTrails = readint(cfg.get("SkyGfx", "vcsTrails", ""), 0);

	c->radiosityFilterPasses = readint(cfg.get("SkyGfx", "radiosityFilterPasses", ""), 2);
	c->radiosityRenderPasses = readint(cfg.get("SkyGfx", "radiosityRenderPasses", ""), 1);
//	c->radiosityIntensityLimit = readint(cfg.get("SkyGfx", "radiosityIntensityLimit", ""), 0xDC);
	c->radiosityIntensity = readint(cfg.get("SkyGfx", "radiosityIntensity", ""), 0x23);
	c->radiosityFilterUCorrection = readint(cfg.get("SkyGfx", "radiosityFilterUCorrection", ""), 2);
	c->radiosityFilterVCorrection = readint(cfg.get("SkyGfx", "radiosityFilterVCorrection", ""), 2);

	c->trailsLimit = readint(cfg.get("SkyGfx", "trailsLimit", ""), 80);
	c->trailsIntensity = readint(cfg.get("SkyGfx", "trailsIntensity", ""), 38);

	neoWaterDrops = readint(cfg.get("SkyGfx", "neoWaterDrops", ""), 0);
	c->neoBloodDrops = readint(cfg.get("SkyGfx", "neoBloodDrops", ""), 0);
	fixPcCarLight = readint(cfg.get("SkyGfx", "fixPcCarLight", ""), 0);
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

		InterceptCall(&InitialiseGame, InitialiseGame_hook, 0x748CFB);

		if(usePCTimecyc){
			Nop(0x5BBF6F, 2);
			Nop(0x5BBF83, 2);
		}

		// custom building pipeline
		if(config->buildingPipe >= 0)
			hookBuildingPipe();

		// custom vehicle pipeline
		if(config->vehiclePipe >= 0)
			hookVehiclePipe();

		if(ps2grassFiles){
			Patch<const char*>(0x5DDA87, "grass2_1.dff");
			Patch<const char*>(0x5DDA8F, "grass2_2.dff");
			Patch<const char*>(0x5DDA97, "grass2_3.dff");
			Patch<const char*>(0x5DDA9F, "grass2_4.dff");

			Patch<const char*>(0x5DDAC3, "grass3_1.dff");
			Patch<const char*>(0x5DDACB, "grass3_2.dff");
			Patch<const char*>(0x5DDAD3, "grass3_3.dff");
			Patch<const char*>(0x5DDADB, "grass3_4.dff");
			Patch<uint>(0x5DDB14 + 1, 0xC03A30+4);
			Patch<uint>(0x5DDB21 + 2, 0xC03A30+8);
			Patch<uint>(0x5DDB2F + 2, 0xC03A30+12);
		}

		// use static ped shadows
		InjectHook(0x5E675E, &FX::GetFxQuality_ped);
		InjectHook(0x5E676D, &FX::GetFxQuality_ped);
		InjectHook(0x706BC4, &FX::GetFxQuality_ped);
		InjectHook(0x706BD3, &FX::GetFxQuality_ped);
		// stencil???
		InjectHook(0x7113B8, &FX::GetFxQuality_stencil);
		InjectHook(0x711D95, &FX::GetFxQuality_stencil);
		// vehicle, pole
		InjectHook(0x70F9B8, &FX::GetFxQuality_stencil);

		if(fixPcCarLight){
			// carenv light diffuse
			Patch<uint>(0x5D88D1 +6, 0);
			Patch<uint>(0x5D88DB +6, 0);
			Patch<uint>(0x5D88E5 +6, 0);
			// carenv light ambient
			Patch<uint>(0x5D88F9 +6, 0);
			Patch<uint>(0x5D8903 +6, 0);
			Patch<uint>(0x5D890D +6, 0);
			// use local viewer for spec light
			// ...or not... (PS2 doesn't)
			// Patch<uchar>(0x5D9AD0 +1, 1);
		}

		if(disableClouds)
			// jump over cloud loop
			InjectHook(0x714145, 0x71422A, PATCH_JUMP);

		if(disableGamma)
			InjectHook(0x74721C, 0x7472F3, PATCH_JUMP);

		if(neoWaterDrops)
			hookWaterDrops();

		// sun glare on cars
		if(config->doglare >= 0)
			InjectHook(0x6ABCFD, doglare, PATCH_JUMP);

		// remove black background of lockon siphon
		if(transparentLockon > 0){
			InjectHook(0x742E33, 0x742EC1, PATCH_JUMP);
			InjectHook(0x742FE0, 0x743085, PATCH_JUMP);
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

		if(GetAsyncKeyState(VK_F8) & 0x8000){
			AllocConsole();
			freopen("CONIN$", "r", stdin);
			freopen("CONOUT$", "w", stdout);
			freopen("CONOUT$", "w", stderr);
		}

		for(int i = 0; i < 10; i++)
			configs[i].version = VERSION;

		IsAlreadyRunning = (BOOL(*)())(*(int*)(0x74872D+1) + 0x74872D + 5);
		InjectHook(0x74872D, InjectDelayedPatches);

		InjectHook(0x5BCF14, afterStreamIni, PATCH_JUMP);

		InjectHook(0x7491C0, myDefaultCallback, PATCH_JUMP);

		InjectHook(0x5BF8EA, CPlantMgr_Initialise);
		InjectHook(0x756DFE, rxD3D9DefaultRenderCallback_Hook, PATCH_JUMP);
		InjectHook(0x5DADB7, fixSeed, PATCH_JUMP);
		InjectHook(0x5DAE61, saveIntensity, PATCH_JUMP);
		Patch<DWORD>(0x5DAEC8, (DWORD)setTextureAndColor);

		// add dual pass for PC pipeline
		InjectHook(0x5D9EEB, D3D9RenderPreLit_DUAL);
		InjectHook(0x5D9EFB, D3D9RenderNotLit_DUAL);
		// give vehicle pipe to upgrade parts
		//InjectHook(0x4C88F0, 0x5DA610, PATCH_JUMP);

		// jump over code that sets alpha ref to 140 (not on PS2).
		// This caused skidmarks to disappear when rendering the neo reflection scene
		InjectHook(0x553AD1, 0x553AE5, PATCH_JUMP);
		// ... was not enough. disable alpha test for skidmarks
		InterceptCall(&CSkidmarks__Render_orig, CSkidmarks__Render, 0x53E175);

		// postfx
		InjectHook(0x5BD7AE, CPostEffects::Init, PATCH_JUMP); // ??? why not CPostEffects::Initialise? address changed to CGame::InitialiseRenderWare, partially overwrites ReadPlayerCoordsFile()
		InjectHook(0x704D1E, CPostEffects::ColourFilter_switch);
		InjectHook(0x704D5D, CPostEffects::Radiosity_PS2);
		InjectHook(0x704FB3, CPostEffects::Radiosity_PS2);
		// fix speedfx
		InjectHook(0x704E8A, CPostEffects::SpeedFX_Fix);

		// infrared vision
		InjectHook(0x704F4B, CPostEffects::InfraredVision_PS2);
		InjectHook(0x704F59, CPostEffects::Grain_PS2);
		// night vision
		InjectHook(0x704EDA, CPostEffects::NightVision_PS2);
		InjectHook(0x704EE8, CPostEffects::Grain_PS2);
		// rain
		InjectHook(0x705078, CPostEffects::Grain_PS2);
		// unused
		InjectHook(0x705091, CPostEffects::Grain_PS2);

		// fix pointlight fog
		InjectHook(0x700B6B, CSprite__RenderBufferedOneXLUSprite_Rotate_Aspect);

		InjectHook(0x44E82E, ps2rand);
		InjectHook(0x44ECEE, ps2rand);
		InjectHook(0x42453B, ps2rand);
		InjectHook(0x42454D, ps2rand);

		InjectHook(0x53D903, myPluginAttach);

		//InjectHook(0x5A3C7D, ps2srand);
		//InjectHook(0x5A3DFB, ps2srand);
		//InjectHook(0x5A3C75, ps2rand);
		//InjectHook(0x5A3CB9, ps2rand);
		//InjectHook(0x5A3CDB, ps2rand);
		//InjectHook(0x5A3CF2, ps2rand);
		//Patch<float*>(0x5A3CC8, (float*)&ps2randnormalize);
		//Patch<float*>(0x5A3CEA, (float*)&ps2randnormalize);
		//Patch<float*>(0x5A3D05, (float*)&ps2randnormalize);
		InjectHook(0x5A3C6E, floatbitpattern);

		// increase multipass distance
		static float multipassMultiplier = 1000.0f;	// default 45.0
		Patch<float*>(0x73290A+2, &multipassMultiplier);

		// Get rid of the annoying dotproduct check in visibility renderCBs
		// Does it make any sense to compare the dot product against a distance?
		Nop(0x733313, 2);	// VehicleHiDetailCB
		Nop(0x73405A, 2);	// VehicleHiDetailAlphaCB
		Nop(0x733403, 2);	// TrainHiDetailCB
		Nop(0x73431A, 2);	// TrainHiDetailAlphaCB
		Nop(0x73444A, 2);	// VehicleHiDetailAlphaCB_BigVehicle

		// change grass close far to ps2 values...but they appear to be handled differently?
		Patch<float>(0x5DDB3D+1, 78.0f);
//		Patch<float>(0x5DDB42+1, 5.0f);	// this is too high, grass disappears o_O

		// High detail water color multiplier is multiplied by 0.65 and added to 0.27, why?
		// Removing this silly calculation seems to work better.
		Nop(0x6E716B, 6);
		Nop(0x6E7176, 6);

		//void dumpMenu(void);
		//dumpMenu();
		
		//InjectHook(0x5A3C7D, mysrand);
		//InjectHook(0x5A3DFB, mysrand);
		//InjectHook(0x5A3C75, ps2rand);

//		Nop(0x748054, 10);
///		Nop(0x748063, 5);
//		Nop(0x747FB0, 10);
//		Nop(0x748A87, 12);
//		InjectHook(0x747F98, 0x748446, PATCH_JUMP);



		// Water tests
//		Nop(0x6EFC9B, 5);	// disable CWaterLevel::RenderWaterTriangle
		//Nop(0x6EFE4C, 5);	// disable CWaterLevel::RenderWaterRectangle
//		Nop(0x6F004E, 5);	// disable CWaterLevel::RenderWaterRectangle outside
	//	Nop(0x6ECED8, 5);	// disable CWaterLevel::RenderFlatWaterRectangle
	//	Nop(0x6ECE15, 5);	// disable CWaterLevel::RenderFlatWaterRectangle
	//	Nop(0x6ECD69, 5);	// disable CWaterLevel::RenderHighDetailWaterRectangle

	//	Nop(0x6EC509, 5);	// disable CWaterLevel::RenderFlatWaterRectangle_OneLayer
//		Nop(0x6EC5B8, 5);	// disable CWaterLevel::RenderFlatWaterRectangle_OneLayer
	//	Nop(0x6EBA1D, 5);	// disable CWaterLevel::RenderHighDetailWaterRectangle_OneLayer
//		Nop(0x6EBAF8, 5);	// disable CWaterLevel::RenderHighDetailWaterRectangle_OneLayer

	//	InjectHook(0x6E9760, CWaterLevel__CalculateWavesForCoordinate_hook);
	//	InjectHook(0x6E8CB8, CWaterLevel__CalculateWavesForCoordinate_hook);

		// Cloud tests
		//Nop(0x53E1AF, 5);	// CClouds::MovingFogRender
		//Nop(0x53E1B4, 5);	// CClouds::VolumetricCloudsRender
		//Nop(0x53E121, 5);	// CClouds::RenderBottomFromHeight
	}

	return TRUE;
}
