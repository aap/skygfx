#include "skygfx.h"
#include "neo.h"

byte &CClock__ms_nGameClockHours = *(byte*)0xB70153;
byte &CClock__ms_nGameClockMinutes = *(byte*)0xB70152;
short &CClock__ms_nGameClockSeconds = *(short*)0xB70150;
short &CWeather__OldWeatherType = *(short*)0xC81320;
short &CWeather__NewWeatherType = *(short*)0xC8131C;
float &CWeather__InterpolationValue = *(float*)0xC8130C;

RwTexDictionary *neoTxd;
int iCanHasNeoCar;

void
neoInit(void)
{
	ONCE;
//	if(xboxcarpipe >= 0 || xboxwaterdrops){
	{
		char *path = getpath("neo\\neo.txd");
		if(path == NULL)
			return;
		RwStream *stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, path);
		if(RwStreamFindChunk(stream, rwID_TEXDICTIONARY, NULL, NULL))
			neoTxd = RwTexDictionaryStreamRead(stream);
		RwStreamClose(stream, NULL);
		if(neoTxd == NULL){
			MessageBox(NULL, "Couldn't find Tex Dictionary inside 'neo\\neo.txd'", "Error", MB_ICONERROR | MB_OK);
			exit(0);
		}
		// we can just set this to current because we're executing before CGame::Initialise
		// which sets up "generic" as the current TXD
		RwTexDictionarySetCurrent(neoTxd);
	}

	WaterDrops::ms_maskTex = RwTextureRead("dropmask", NULL);

	neoCarPipeInit();
}

#define INTERP_SETUP \
		int h1 = CClock__ms_nGameClockHours;								  \
		int h2 = (h1+1)%24;										  \
		int w1 = CWeather__OldWeatherType;								  \
		int w2 = CWeather__NewWeatherType;								  \
		float timeInterp = (CClock__ms_nGameClockSeconds/60.0f + CClock__ms_nGameClockMinutes)/60.0f;	  \
		float c0 = (1.0f-timeInterp)*(1.0f-CWeather__InterpolationValue);				  \
		float c1 = timeInterp*(1.0f-CWeather__InterpolationValue);					  \
		float c2 = (1.0f-timeInterp)*CWeather__InterpolationValue;					  \
		float c3 = timeInterp*CWeather__InterpolationValue;
#define INTERP(v) v[h1][w1]*c0 + v[h2][w1]*c1 + v[h1][w2]*c2 + v[h2][w2]*c3;
#define INTERPF(v,f) v[h1][w1].f*c0 + v[h2][w1].f*c1 + v[h1][w2].f*c2 + v[h2][w2].f*c3;


InterpolatedFloat::InterpolatedFloat(float init)
{
	curInterpolator = 61;	// compared against second
	for(int h = 0; h < 24; h++)
		for(int w = 0; w < NUMWEATHERS; w++)
			data[h][w] = init;
}

void
InterpolatedFloat::Read(char *s, int line, int field)
{
	sscanf(s, "%f", &data[line][field]);
}

float
InterpolatedFloat::Get(void)
{
	if(curInterpolator != CClock__ms_nGameClockSeconds){
		INTERP_SETUP
		curVal = INTERP(data);
	}
	return curVal;
}

InterpolatedColor::InterpolatedColor(const Color &init)
{
	curInterpolator = 61;	// compared against second
	for(int h = 0; h < 24; h++)
		for(int w = 0; w < NUMWEATHERS; w++)
			data[h][w] = init;
}

void
InterpolatedColor::Read(char *s, int line, int field)
{
	int r, g, b, a;
	sscanf(s, "%i, %i, %i, %i", &r, &g, &b, &a);
	data[line][field] = Color(r/255.0f, g/255.0f, b/255.0f, a/255.0f);
}

Color
InterpolatedColor::Get(void)
{
	if(curInterpolator != CClock__ms_nGameClockSeconds){
		INTERP_SETUP
		curVal.r = INTERPF(data, r);
		curVal.g = INTERPF(data, g);
		curVal.b = INTERPF(data, b);
		curVal.a = INTERPF(data, a);
	}
	return curVal;
}

void
InterpolatedLight::Read(char *s, int line, int field)
{
	int r, g, b, a;
	sscanf(s, "%i, %i, %i, %i", &r, &g, &b, &a);
	data[line][field] = Color(r/255.0f, g/255.0f, b/255.0f, a/100.0f);
}

void
neoReadWeatherTimeBlock(FILE *file, InterpolatedValue *interp)
{
	char buf[24], *p;
	int c;
	int line, field;

	line = 0;
	c = getc(file);
	while(c != EOF && line < 24){
		field = 0;
		if(c != EOF && c != '#'){
			while(c != EOF && c != '\n' && field < NUMWEATHERS){
				p = buf;
				while(c != EOF && c == '\t')
					c = getc(file);
				*p++ = c;
				while(c = getc(file), c != EOF && c != '\t' && c != '\n')
					*p++ = c;
				*p++ = '\0';
				interp->Read(buf, line, field);
				field++;
			}
			line++;
		}
		while(c != EOF && c != '\n')
			c = getc(file);
		c = getc(file);
	}
	ungetc(c, file);
}

#if 0
RxPipeline*
neoCreatePipe(void)
{
	RxPipeline *pipe;
	RxPipelineNode *node;

	pipe = RxPipelineCreate();
	if(pipe){
		RxLockedPipe *lpipe;
		lpipe = RxPipelineLock(pipe);
		if(lpipe){
			RxNodeDefinition *nodedef= RxNodeDefinitionGetD3D8AtomicAllInOne();
			RxLockedPipeAddFragment(lpipe, 0, nodedef, NULL);
			RxLockedPipeUnlock(lpipe);

			node = RxPipelineFindNodeByName(pipe, nodedef->name, NULL, NULL);
			RxD3D8AllInOneSetInstanceCallBack(node, D3D8AtomicDefaultInstanceCallback_fixed);
			RxD3D8AllInOneSetRenderCallBack(node, rxD3D8DefaultRenderCallback_xbox);
			return pipe;
		}
		RxPipelineDestroy(pipe);
	}
	return NULL;
}

void
CustomPipe::CreateRwPipeline(void)
{
	rwPipeline = neoCreatePipe();
}

void
CustomPipe::SetRenderCallback(RxD3D8AllInOneRenderCallBack cb)
{
	RxPipelineNode *node;
	RxNodeDefinition *nodedef = RxNodeDefinitionGetD3D8AtomicAllInOne();
	node = RxPipelineFindNodeByName(rwPipeline, nodedef->name, NULL, NULL);
	originalRenderCB = RxD3D8AllInOneGetRenderCallBack(node);
	RxD3D8AllInOneSetRenderCallBack(node, cb);
}

void
CustomPipe::Attach(RpAtomic *atomic)
{
	RpAtomicSetPipeline(atomic, rwPipeline);
	/*
	 * From rw 3.7 changelog:
	 *
	 *	07/01/03 Cloning atomics with MatFX - BZ#3430
	 *	  When cloning cloning an atomic with matfx applied the matfx copy callback
	 *	  would also setup the matfx pipes after they had been setup by the atomic
	 *	  clone function. This could overwrite things like skin or patch pipes with
	 *	  generic matfx pipes when it shouldn't.
	 *
	 * Since we're not using 3.5 we have to fix this manually:
	 */
	*RWPLUGINOFFSET(int, atomic, MatFXAtomicDataOffset) = 0;
}

RpAtomic*
CustomPipe::setatomicCB(RpAtomic *atomic, void *data)
{
	((CustomPipe*)data)->Attach(atomic);
	return atomic;
}
#endif
