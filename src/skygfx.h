#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4244)	// int to float
#pragma warning(disable: 4800)	// int to bool
#pragma warning(disable: 4838)  // narrowing conversion
#pragma warning(disable: 4996)  // strcmpi

#define _USE_MATH_DEFINES

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
#include "LinkList.h"

typedef uint8_t uint8, uchar;
typedef uint16_t uint16, ushort;
typedef uint32_t uint32, uint;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;

extern HMODULE dllModule;

#define nil NULL
#define VERSION 0x370

#include "gta.h"


enum CarPipeline
{
	CAR_PS2,
	CAR_PC,
	CAR_XBOX,
	CAR_SPEC,
	CAR_MOBILE,
	CAR_NEO,
	CAR_LCS,
	CAR_VCS,

	NUMCARPIPES
};

enum BuildingPipeline
{
	BUILDING_PS2,
	BUILDING_XBOX,

	NUMBUILDINGPIPES
};

struct Config {
	// these are at fixed offsets
	int version;			// for other modules
	RwBool fixGrassPlacement;	// fixed for fixSeed in main.cpp
	RwBool doglare;			// fixed for doglare in main.cpp

	int buildingPipe;
	RwBool ps2ModulateBuilding;
	RwBool dualPassBuilding;

	RwBool usePCTimecyc;
	RwBool ps2ModulateGrass;
	RwBool grassAddAmbient;
	RwBool backfaceCull;
	RwBool dualPassDefault, dualPassGrass, dualPassVehicle, dualPassPed;
	int vehiclePipe;
	float neoShininessMult, neoSpecularityMult;
	int colorFilter;
	int infraredVision, nightVision, grainFilter;
	RwBool doRadiosity;
	int radiosityFilterPasses, radiosityRenderPasses;
	int radiosityIntensity;
	int offLeft, offRight, offTop, offBottom;
	RwBool vcsTrails;
	int trailsLimit, trailsIntensity;
	int pedShadows, stencilShadows;
	int lightningIlluminatesWorld;
	RwBool neoWaterDrops;
	RwBool neoBloodDrops;

	RwBool ps2ModulateGlobal, dualPassGlobal;

	int keys[2];

	bool bYCbCrFilter;
	float lumaScale, lumaOffset;
	float cbScale, cbOffset;
	float crScale, crOffset;
	int zwriteThreshold;

	float leedsShininessMult;
	RwBool detailMaps;
	int envMapSize;

	int radiosity;
	int coronaZtest;
};
extern int numConfigs;
extern int currentConfig;
extern Config *config, configs[10];
void readIni(int n);
void resetValues(void);
void refreshIni(void);
void reloadAllInis(void);
void setConfig(void);

extern bool iCanHasbuildingPipe;
extern int explicitBuildingPipe;

/* Env map */
extern RwCamera *reflectionCam;
extern RwRaster *envFB, *envZB;
extern RwTexture *reflectionTex;
void MakeEnvmapRasters(void);
void MakeEnvmapCam(void);

enum {
	COLORFILTER_NONE   = 0,
	COLORFILTER_PS2    = 1,
	COLORFILTER_PC     = 2,
	COLORFILTER_MOBILE = 3,
	COLORFILTER_III    = 4,
	COLORFILTER_VC     = 5,
#ifdef DEBUG
	COLORFILTER_VCS    = 6,
#endif
};

struct CPostEffects
{
	// effects:
	//          III ColourFilter/Blur
	//          VC  ColourFilter/Blur
	//          SA  ColourFilter/Blur
	//          SA  Radiosity
	//          VCS Radiosity
	//          VCS Blur
	static void Radiosity_VCS_init(void);
	static void Radiosity_VCS(int limit, int intensity);
	static void Blur_VCS(void);

	static void Radiosity(int intensityLimit, int filterPasses, int renderPasses, int intensity);
	static void Radiosity_shader(int intensityLimit, int filterPasses, int renderPasses, int intensity);
	static void DarknessFilter(uint8 alpha);
	static void DarknessFilter_fix(uint8 alpha);
	static void InfraredVision(RwRGBA c1, RwRGBA c2);
	static void InfraredVision_PS2(RwRGBA c1, RwRGBA c2);
	static void NightVision(RwRGBA color);
	static void NightVision_PS2(RwRGBA color);
	static void Grain(int strength, bool generate);
	static void Grain_PS2(int strength, bool generate);
	static void ColourFilter(RwRGBA rgb1, RwRGBA rgb2);
	static void ColourFilter_Mobile(RwRGBA rgb1, RwRGBA rgb2);
	static void ColourFilter_PS2(RwRGBA rgb1, RwRGBA rgb2);
	static void ColourFilter_Generic(RwRGBA rgb1, RwRGBA rgb2, void *ps);
	static void ColourFilter_switch(RwRGBA rgb1, RwRGBA rgb2);
	static void SetFilterMainColour_PS2(RwRaster *raster, RwRGBA color);
	static void (*Initialise_orig)(void);
	static void Initialise(void);
	static void ImmediateModeRenderStatesStore(void);
	static void ImmediateModeRenderStatesSet(void);
	static void ImmediateModeRenderStatesReStore(void);
	static void SetFilterMainColour(RwRaster *raster, RwRGBA color);
	static void DrawQuad(float x1, float y1, float x2, float y2, uchar r, uchar g, uchar b, uchar alpha, RwRaster *ras);
	static void DrawQuadSetUVs(float utl, float vtl, float utr, float vtr, float ubr, float vbr, float ubl, float vbl);
	static void DrawQuadSetDefaultUVs(void);
	static void SpeedFX(float);
	static void DrawFinalEffects(void);

	static Imf &ms_imf;

	static RwRaster *&pRasterFrontBuffer;
	static float &m_fInfraredVisionFilterRadius;;
	static RwRaster *&m_pGrainRaster;
	static int &m_InfraredVisionGrainStrength;
	static int &m_NightVisionGrainStrength;
	static float &m_fNightVisionSwitchOnFXCount;
	static bool &m_bInfraredVision;

	static bool &m_bDisableAllPostEffect;

	static bool &m_bColorEnable;
	static int &m_colourLeftUOffset;
	static int &m_colourRightUOffset;
	static int &m_colourTopVOffset;
	static int &m_colourBottomVOffset;
	static float &m_colour1Multiplier;
	static float &m_colour2Multiplier;
	static float &SCREEN_EXTRA_MULT_CHANGE_RATE;
	static float &SCREEN_EXTRA_MULT_BASE_CAP;
	static float &SCREEN_EXTRA_MULT_BASE_MULT;

	static bool &m_bRadiosity;
	static bool &m_bRadiosityDebug;
	static int &m_RadiosityFilterPasses;
	static int &m_RadiosityRenderPasses;
	static int &m_RadiosityIntensityLimit;
	static int &m_RadiosityIntensity;
	static bool &m_bRadiosityBypassTimeCycleIntensityLimit;
	static int &m_RadiosityFilterUCorrection;
	static int &m_RadiosityFilterVCorrection;

	static bool &m_bDarknessFilter;
	static int &m_DarknessFilterAlpha;
	static int &m_DarknessFilterAlphaDefault;
	static int &m_DarknessFilterRadiosityIntensityLimit;

	static bool &m_bCCTV;
	static bool &m_bFog;
	static bool &m_bNightVision;
	static bool &m_bHeatHazeFX;
	static bool &m_bHeatHazeMaskModeTest;
	static bool &m_bGrainEnable;
	static bool &m_waterEnable;

	static bool &m_bSpeedFX;
	static bool &m_bSpeedFXTestMode;
	static uint8 &m_SpeedFXAlpha;

	/* My own */
	static bool m_bBlurColourFilter;
	// YCbCr color filter
	static bool m_bYCbCrFilter;
	static float m_lumaScale, m_lumaOffset;
	static float m_cbScale, m_cbOffset;
	static float m_crScale, m_crOffset;

	static void UpdateFrontBuffer(void);
};

char *getpath(char *path);


// Tex DB
struct TexInfo
{
	char *name;	// not strictly needed
	char *affiliate;
	TexInfo *affiliateTex;
	int detailnum;
	RwTexture *detail;
	int detailtile;
	int alphamode;
	int hassibling;
};
TexInfo *RwTextureGetTexDBInfo(RwTexture *tex);
int TexDBPluginAttach(void);
void initTexDB(void);

extern bool gRenderingSpheremap;
extern CVector reflectionCamPos;

extern RxPipeline *&CCustomBuildingDNPipeline__ObjPipeline;
extern RxPipeline *&CCustomBuildingPipeline__ObjPipeline;
void TagRenderCB(RpAtomic *atomic, RxD3D9ResEntryHeader *resEntryHeader, RxD3D9InstanceData *instanceData);
RxPipeline *CCustomBuildingPipeline__CreateCustomObjPipe_PS2(void);
RxPipeline *CCustomBuildingDNPipeline__CreateCustomObjPipe_PS2(void);
int PDSPipePluginAttach(void);
void hookVehiclePipe(void);
void hookBuildingPipe(void);
void D3D9Render(RxD3D9ResEntryHeader *resEntryHeader, RxD3D9InstanceData *instanceData);
void D3D9RenderDual(int dual, RxD3D9ResEntryHeader *resEntryHeader, RxD3D9InstanceData *instancedData);

void fixSAMP(void);

extern RwInt32 pdsOffset;

//////// Pipelines
///// Shaders
// misc
extern void *simplePS;
// vehicles
extern void *vehiclePipeVS, *ps2CarFxVS;
extern void *ps2EnvSpecFxPS;	// also used by the building pipeline
extern void *specCarFxVS, *specCarFxPS;
extern void *xboxCarVS;
extern void *leedsCarFxVS;
extern void *mobileVehiclePipeVS, *mobileVehiclePipePS;
// postfx
extern void *iiiTrailsPS, *vcTrailsPS;
extern void *gradingPS, *contrastPS;
extern void *blurPS, *radiosityPS;
// building
extern void *ps2BuildingVS, *ps2BuildingFxVS;
extern void *xboxBuildingVS, *xboxBuildingPS;
extern void *simpleDetailPS;
extern void *simpleFogPS;
extern void *sphereBuildingVS;
void CreateShaders(void);
void RwToD3DMatrix(void *d3d, RwMatrix *rw);
void MakeProjectionMatrix(void *d3d, RwCamera *cam, float nbias = 0.0f, float fbias = 0.0f);
void pipeGetComposedTransformMatrix(RpAtomic *atomic, float *out);
void pipeGetCameraTransformMatrix(float *out);
void pipeGetLeedsEnvMapMatrix(RpAtomic *atomic, float *out);
void pipeUploadMatCol(int flags, RpMaterial *m, int loc);
void pipeUploadZero(int loc);
void pipeUploadLightColor(RpLight *light, int loc);
void pipeUploadLightDirection(RpLight *light, int loc);
void pipeUploadLightDirectionLocal(RpLight *light, RwMatrix *m, int loc);
void pipeUploadLightDirectionInv(RpLight *light, int loc);
inline void pipeSetTexture(RwTexture *t, int n) { RwD3D9SetTexture(t ? t : gpWhiteTexture, n); };


extern int &dword_C02C20, &dword_C9BC60;
extern RxPipeline *&skinPipe, *&CCustomCarEnvMapPipeline__ObjPipeline;

// reversed
void D3D9RenderNotLit(RxD3D9ResEntryHeader *resEntryHeader, RxD3D9InstanceData *instanceData);
void D3D9RenderPreLit(RxD3D9ResEntryHeader *resEntryHeader, RxD3D9InstanceData *instanceData, RwUInt8 flags, RwTexture *texture);
RwBool DNInstance_default(void *object, RxD3D9ResEntryHeader *resEntryHeader, RwBool reinstance);
void CCustomCarEnvMapPipeline__CustomPipeRenderCB(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags);

void CCustomCarEnvMapPipeline__Env1Xform_PC(RpAtomic *atomic,
	CustomEnvMapPipeMaterialData *envData, float *envXform);
void CCustomCarEnvMapPipeline__Env2Xform_PC(RpAtomic *atomic,
	CustomEnvMapPipeMaterialData *envData, CustomEnvMapPipeAtomicData *atmEnvData, float *envXform);

// from the exe
RwBool DNInstance(void *object, RxD3D9ResEntryHeader *resEntryHeader, RwBool reinstance);
RwBool D3D9SetRenderMaterialProperties(RwSurfaceProperties*, RwRGBA *color, RwUInt32 flags, RwReal specularLighting, RwReal specularPower);
RwBool D3D9RestoreSurfaceProperties(void);
RwUInt16 CVisibilityPlugins__GetAtomicId(RpAtomic *atomic);
RpAtomic *CCustomCarEnvMapPipeline__CustomPipeAtomicSetup(RpAtomic *atomic);
char *GetFrameNodeName(RwFrame *frame);
RpAtomic *AtomicDefaultRenderCallBack(RpAtomic*);
void CCustomCarEnvMapPipeline__CustomPipeRenderCB_exe(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags);
void GTAfree(void *data);
