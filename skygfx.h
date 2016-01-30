#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <rwcore.h>
#include <rwplcore.h>
#include <rpworld.h>
#include <rpmatfx.h>
#include <d3d9.h>
#include <d3d9types.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "resource.h"
#include "MemoryMgr.h"
#include "Pools.h"

extern HMODULE dllModule;

struct Config {
	RwBool fixGrassPlacement;	// this has to stay at that offset, see fixSeed in main.cpp
	RwBool enableHotkeys;
	RwBool ps2Ambient, ps2ModulateWorld, ps2ModulateGrass;
	RwBool grassAddAmbient;
	RwBool backfaceCull;
	RwBool dualPassWorld, dualPassDefault, dualPassGrass, dualPassVehicle, dualPassPed;
	int vehiclePipe, worldPipe;
	int colorFilter;
	int infraredVision, nightVision, grainFilter;
	RwBool scaleOffsets;
	RwBool doRadiosity;
	int radiosityFilterPasses, radiosityRenderPasses, radiosityIntensityLimit;
	int radiosityIntensity, radiosityFilterUCorrection, radiosityFilterVCorrection;
	int offLeft, offRight, offTop, offBottom;
	RwBool vcsTrails;
	int trailsLimit, trailsIntensity;
	int pedShadows, stencilShadows;
	int detailedWaterDist;

	float farDist, fadeDist, fadeInvDist, densityMult;

	int keys[3];
};
extern int numConfigs;
extern Config *config, configs[10];

struct CustomEnvMapPipeMaterialData
{
	int8_t scaleX;
	int8_t scaleY;
	int8_t transScaleX;
	int8_t transScaleY;
	int8_t shininess;
	uint8_t pad3;
	uint16_t renderFrameCounter;
	RwTexture *texture;
};

struct CustomEnvMapPipeAtomicData
{
	int pad1;
	int pad2;
	int pad3;

	void *operator new(size_t size);
};

struct CustomSpecMapPipeMaterialData
{
	float specularity;
	RwTexture *texture;
};

struct PsGlobalType
{
	HWND	window;
	DWORD	instance;
	DWORD	fullscreen;
	DWORD	lastMousePos_X;
	DWORD	lastMousePos_Y;
	DWORD	unk;
	DWORD	diInterface;
	DWORD	diMouse;
	void*	diDevice1;
	void*	diDevice2;
};

struct RsGlobalType
{
	DWORD			AppName;
	DWORD			MaximumWidth;
	DWORD			MaximumHeight;
	DWORD			frameLimit;
	DWORD			quit;
	PsGlobalType*	ps;
	void*			keyboard;
	void*			mouse;
	void*			pad;
};

struct CPostEffects
{
	static void Radiosity_VCS(int col1, int nSubdivs, int unknown, int col2);
	static void Radiosity_PS2(int col1, int nSubdivs, int unknown, int col2);
	static void InfraredVision(RwRGBA c1, RwRGBA c2);
	static void InfraredVision_PS2(RwRGBA c1, RwRGBA c2);
	static void NightVision(RwRGBA color);
	static void NightVision_PS2(RwRGBA color);
	static void Grain(int strength, bool generate);
	static void Grain_PS2(int strength, bool generate);
	static void ColourFilter(RwRGBA rgb1, RwRGBA rgb2);
	static void ColourFilter_PS2(RwRGBA rgb1, RwRGBA rgb2);
	static void ColourFilter_switch(RwRGBA rgb1, RwRGBA rgb2);
	static void SetFilterMainColour_PS2(RwRaster *raster, RwRGBA color);
	static void Init(void);
	static void ImmediateModeRenderStatesStore(void);
	static void ImmediateModeRenderStatesSet(void);
	static void ImmediateModeRenderStatesReStore(void);
	static void SetFilterMainColour(RwRaster *raster, RwRGBA color);
	static void DrawQuad(float x1, float y1, float x2, float y2, char r, char g, char b, char alpha, RwRaster *ras);
	static void SpeedFX(float);
	static void SpeedFX_Fix(float fStrength);

	static RwRaster *&pRasterFrontBuffer;
	static float &m_fInfraredVisionFilterRadius;;
	static RwRaster *&m_pGrainRaster;
	static int &m_InfraredVisionGrainStrength;
	static int &m_NightVisionGrainStrength;
	static float &m_fNightVisionSwitchOnFXCount;
	static bool &m_bInfraredVision;
};

RxPipeline *CCustomBuildingDNPipeline__CreateCustomObjPipe_PS2(void);
int myPluginAttach(void);
void setVehiclePipeCB(RxPipelineNode *node, RxD3D9AllInOneRenderCallBack callback);
void loadColorcycle(void);
void readIni(int n);
void SetCloseFarAlphaDist(float close, float far);
void resetValues(void);
void D3D9Render(RxD3D9ResEntryHeader *resEntryHeader, RxD3D9InstanceData *instanceData);
//RpAtomic *CCarFXRenderer__CustomCarPipeClumpSetup(RpAtomic *atomic, void *data);

#define RwEngineInstance (*rwengine)
extern RsGlobalType *RsGlobal;
extern IDirect3DDevice9 *&d3d9device;
extern RwCamera *&Camera;
extern RpLight *&pAmbient;
extern RpLight *&pDirect;
extern RpLight **pExtraDirectionals;
extern int &NumExtraDirLightsInWorld;
extern D3DLIGHT9 &gCarEnvMapLight;

extern char &doRadiosity;

extern RwTexture *&gpWhiteTexture;
extern void *simplePS;
extern void *vehiclePipePS;
extern RwBool reflTexDone;
extern RwRaster *reflTex;
extern RwInt32 pdsOffset;

extern void **rwengine;
extern RwInt32 &CCustomCarEnvMapPipeline__ms_envMapPluginOffset;
extern RwInt32 &CCustomCarEnvMapPipeline__ms_envMapAtmPluginOffset;
extern RwInt32 &CCustomCarEnvMapPipeline__ms_specularMapPluginOffset;
extern RwReal &CCustomCarEnvMapPipeline__m_EnvMapLightingMult;
extern RwInt32 &CCustomBuildingDNPipeline__ms_extraVertColourPluginOffset;
extern RwReal &CCustomBuildingDNPipeline__m_fDNBalanceParam;

extern int &dword_C02C20, &dword_C9BC60;
extern RxPipeline *&skinPipe, *&CCustomCarEnvMapPipeline__ObjPipeline;

extern int &CPostEffects__m_RadiosityFilterPasses;
extern int &CPostEffects__m_RadiosityRenderPasses;
extern int &CPostEffects__m_RadiosityIntensityLimit;
extern int &CPostEffects__m_RadiosityIntensity;
extern int &CPostEffects__m_RadiosityFilterUCorrection;
extern int &CPostEffects__m_RadiosityFilterVCorrection;

// reversed
void D3D9RenderNotLit(RxD3D9ResEntryHeader *resEntryHeader, RxD3D9InstanceData *instanceData);
void D3D9RenderPreLit(RxD3D9ResEntryHeader *resEntryHeader, RxD3D9InstanceData *instanceData, RwUInt8 flags, RwTexture *texture);
void D3D9GetEnvMapVector(RpAtomic *atomic, CustomEnvMapPipeAtomicData *atmdata, CustomEnvMapPipeMaterialData *data, RwV3d *transScale);
void D3D9GetTransScaleVector(CustomEnvMapPipeMaterialData *data, RpAtomic *atomic, RwV3d *transScale);
RwBool DNInstance_default(void *object, RxD3D9ResEntryHeader *resEntryHeader, RwBool reinstance);
void CCustomCarEnvMapPipeline__CustomPipeRenderCB(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags);

// from the exe
RwBool DNInstance(void *object, RxD3D9ResEntryHeader *resEntryHeader, RwBool reinstance);
RwBool D3D9SetRenderMaterialProperties(RwSurfaceProperties*, RwRGBA *color, RwUInt32 flags, RwReal specularLighting, RwReal specularPower);
RwBool D3D9RestoreSurfaceProperties(void);
RwUInt16 CVisibilityPlugins__GetAtomicId(RpAtomic *atomic);
RpAtomic *CCustomCarEnvMapPipeline__CustomPipeAtomicSetup(RpAtomic *atomic);
char *GetFrameNodeName(RwFrame *frame);
void SetPipelineID(RpAtomic*, unsigned int it);
RpAtomic *AtomicDefaultRenderCallBack(RpAtomic*);
void CCustomCarEnvMapPipeline__CustomPipeRenderCB_exe(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags);
void GTAfree(void *data);
