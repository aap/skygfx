extern RwTexDictionary *neoTxd;

/*
 * neo water drops
 */

struct VertexTex2
{
	RwReal      x;
	RwReal      y;
	RwReal      z;
	RwReal      rhw;
	RwUInt32    emissiveColor;
	RwReal      u0;
	RwReal      v0;
	RwReal      u1;
	RwReal      v1;
};

class WaterDrop
{
public:
	float x, y, time;		// shorts on xbox (short float?)
	float size, uvsize, ttl;	// "
	uchar alpha;
	bool active;
	bool fades;

	void Fade(void);
};

class WaterDropMoving
{
public:
	WaterDrop *drop;
	float dist;
};

class WaterDrops
{
public:
	enum {
		MAXDROPS = 2000,
		MAXDROPSMOVING = 700
	};

	// Logic

	static float ms_xOff, ms_yOff;	// not quite sure what these are
	static WaterDrop ms_drops[MAXDROPS];
	static int ms_numDrops;
	static WaterDropMoving ms_dropsMoving[MAXDROPSMOVING];
	static int ms_numDropsMoving;

	static bool ms_enabled;
	static bool ms_movingEnabled;

	static float ms_distMoved, ms_vecLen, ms_rainStrength;
	static RwV3d ms_vec;
	static RwV3d ms_lastAt;
	static RwV3d ms_lastPos;
	static RwV3d ms_posDelta;

	static int ms_splashDuration;
	//static CPlaceable_III *ms_splashObject;

	static void Process(void);
	static void CalculateMovement(void);
	static void SprayDrops(void);
	static void MoveDrop(WaterDropMoving *moving);
	static void ProcessMoving(void);
	static void Fade(void);

	static WaterDrop *PlaceNew(float x, float y, float size, float time, bool fades);
	static void NewTrace(WaterDropMoving*);
	static void NewDropMoving(WaterDrop*);
	// this has one more argument in VC: ttl, but it's always 2000.0
	static void FillScreenMoving(float amount);
	static void FillScreen(int n);
	static void Clear(void);
	static void Reset(void);

	//static void RegisterSplash(CPlaceable_III *plc);
	static bool NoRain(void);

	// Rendering

	static RwTexture *ms_maskTex;
	static RwTexture *ms_tex;
//	static RwRaster *ms_maskRaster;
	static RwRaster *ms_raster;
	static int ms_fbWidth, ms_fbHeight;
	static void *ms_vertexBuf;
	static void *ms_indexBuf;
	static VertexTex2 *ms_vertPtr;
	static int ms_numBatchedDrops;
	static int ms_initialised;

	static void InitialiseRender(RwCamera *cam);
	static void AddToRenderList(WaterDrop *drop);
	static void Render(void);
};
