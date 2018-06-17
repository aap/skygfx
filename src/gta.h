
class CGeneral
{
public:
	static float GetATanOfXY(float x, float y);
};

struct CVector
{
	float x, y, z;
};

struct CMatrix
{
	RwMatrix matrix;
	RwMatrix *pMatrix;
	int haveRwMatrix;
};

struct CSimpleTransform
{
	CVector pos;
	float angle;
};

struct CPlaceable
{
	void *vtable;
	CSimpleTransform placement;
	CMatrix *m_pCoords;
	CVector GetPosition(void){
		if(m_pCoords)
			return *(CVector*)&m_pCoords->matrix.pos;
		else
			return placement.pos;
	}
};
struct CCamera : CPlaceable
{
};

CPlaceable *FindPlayerPed(int);

extern float &CTimer__ms_fTimeStep;
extern CCamera &TheCamera;
extern float &CWeather__Rain;
extern float &CWeather__UnderWaterness;
extern bool &CCutsceneMgr__ms_running;
extern int* CGame__currArea;
extern int* CEntryExitManager__ms_exitEnterState;

struct CustomEnvMapPipeMaterialData
{
	int8 scaleX;
	int8 scaleY;
	int8 transScaleX;
	int8 transScaleY;
	int8 shininess;
	uint8 pad3;
	uint16 renderFrameCounter;
	RwTexture *texture;

	float GetScaleX(void) { return scaleX/8.0f; }
	float GetScaleY(void) { return scaleY/8.0f; }
	float GetTransScaleX(void) { return transScaleX/8.0f; }
	float GetTransScaleY(void) { return transScaleY/8.0f; }
	float GetShininess(void) { return shininess/255.0f; }
};

struct CustomEnvMapPipeAtomicData
{
	float trans;
	float posx;
	float posy;

	void *operator new(size_t size);
};
CustomEnvMapPipeAtomicData *CCustomCarEnvMapPipeline__AllocEnvMapPipeAtomicData(RpAtomic *atomic);

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

struct GlobalScene
{
	RpWorld *world;
	RwCamera *camera;
};
extern GlobalScene &Scene;

struct Imf
{
	float zScreenNear;
	float recipZ;
	RwRaster *frontBuffer;
	int width;
	int height;
	float umin;
	float vmin;
	float umax;
	float vmax;
	RwD3D9Vertex triangle_verts[3];
	float tri_umin;
	float tri_umax;
	float tri_vmin;
	float tri_vmax;
	RwD3D9Vertex quad_verts[4];
};

void DefinedState(void);

double CTimeCycle_GetAmbientRed(void);
double CTimeCycle_GetAmbientGreen(void);
double CTimeCycle_GetAmbientBlue(void);

#define RwEngineInstance (*rwengine)
extern RsGlobalType *RsGlobal;
extern IDirect3DDevice9 *&d3d9device;
extern RpLight *&pAmbient;
extern RpLight *&pDirect;
extern RpLight **pExtraDirectionals;
extern int &NumExtraDirLightsInWorld;
extern D3DLIGHT9 &gCarEnvMapLight;


extern int16 &CClock__ms_nGameClockSeconds;
extern uint8 &CClock__ms_nGameClockMinutes;
extern uint8 &CClock__ms_nGameClockHours;
extern int16 &CWeather__OldWeatherType;
extern int16 &CWeather__NewWeatherType;
extern float &CWeather__InterpolationValue;

extern void **rwengine;
extern RwInt32 &CCustomCarEnvMapPipeline__ms_envMapPluginOffset;
extern RwInt32 &CCustomCarEnvMapPipeline__ms_envMapAtmPluginOffset;
extern RwInt32 &CCustomCarEnvMapPipeline__ms_specularMapPluginOffset;
extern RwReal &CCustomCarEnvMapPipeline__m_EnvMapLightingMult;
extern RwInt32 &CCustomBuildingDNPipeline__ms_extraVertColourPluginOffset;
extern RwReal &CCustomBuildingDNPipeline__m_fDNBalanceParam;

void CRenderer__RenderRoads(void);
void CRenderer__RenderEverythingBarRoads(void);
void CRenderer__RenderFadingInEntities(void);
void CRenderer__RenderFadingInUnderwaterEntities(void);
extern short &skyTopRed, &skyTopGreen, &skyTopBlue;
extern short &skyBotRed, &skyBotGreen, &skyBotBlue;

extern RwTexture *&gpWhiteTexture;


#define rpPDS_MAKEPIPEID(vendorID, pipeID)              \
    (((vendorID & 0xFFFF) << 16) | (pipeID & 0xFFFF))

enum RockstarPipelineIDs
{
	VEND_ROCKSTAR     = 0x0253F2,

	// PS2 pipes
	// building
	PDS_PS2_CustomBuilding_AtmPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x80),
	PDS_PS2_CustomBuilding_MatPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x81),
	PDS_PS2_CustomBuildingDN_AtmPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x82),
	PDS_PS2_CustomBuildingDN_MatPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x83),
	PDS_PS2_CustomBuildingEnvMap_AtmPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x8C),
	PDS_PS2_CustomBuildingEnvMap_MatPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x8D),
	PDS_PS2_CustomBuildingDNEnvMap_AtmPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x8E),
	PDS_PS2_CustomBuildingDNEnvMap_MatPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x8F),
	PDS_PS2_CustomBuildingUVA_AtmPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x90),
	PDS_PS2_CustomBuildingUVA_MatPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x91),
	PDS_PS2_CustomBuildingDNUVA_AtmPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x92),
	PDS_PS2_CustomBuildingDNUVA_MatPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x93),
	// car
	PDS_PS2_CustomCar_AtmPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x84),
	PDS_PS2_CustomCar_MatPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x85),
	PDS_PS2_CustomCarEnvMap_AtmPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x86),
	PDS_PS2_CustomCarEnvMap_MatPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x87),
//	PDS_PS2_CustomCarEnvMapUV2_AtmPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x8A),	// this does not exist
	PDS_PS2_CustomCarEnvMapUV2_MatPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x8B),
	// skin
	PDS_PS2_CustomSkinPed_AtmPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x88),
	PDS_PS2_CustomSkinPed_MatPipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x89),

	// PC pipes
	RSPIPE_PC_CustomBuilding_PipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x9C),
	RSPIPE_PC_CustomBuildingDN_PipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x98),
	RSPIPE_PC_CustomCarEnvMap_PipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x9A),	// same as XBOX!

	// Xbox pipes
	RSPIPE_XBOX_CustomBuilding_PipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x9E),
	RSPIPE_XBOX_CustomBuildingDN_PipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x96),
	RSPIPE_XBOX_CustomBuildingEnvMap_PipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0xA0),
	RSPIPE_XBOX_CustomBuildingDNEnvMap_PipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0xA2),
	RSPIPE_XBOX_CustomCarEnvMap_PipeID = rpPDS_MAKEPIPEID(VEND_ROCKSTAR, 0x9A),	// same as PC!
};

uint32 GetPipelineID(RpAtomic *atomic);
void SetPipelineID(RpAtomic *atomic, uint32 id);
