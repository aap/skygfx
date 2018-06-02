#include "skygfx.h"

#include <map>


static TexInfo faketexinfo;

RwTexture *detailTextures[100];
std::map<std::string, TexInfo*> texdb;

int32 texdbOffset;

RwTexDictionary *detailTxd;

static void
strtolower(std::string &s)
{
	int i, len;
	len = s.size();
	for(i = 0; i < len; i++)
		s[i] = tolower(s[i]);
}

static void
readTxt(void)
{
	char buf[200], *p;

	char *path = getpath("models\\texdb.txt");
	if(path == nil)
		return;
	FILE *f = fopen(path, "r");
	if(f == nil)
		return;

	while(fgets(buf, 200, f)){
		p = strtok(buf, " \t");
		char *filename = nil;
		int alphamode = 0;
		int isdetail = 0;
		int hasdetail = 0;
		int detailtile = 0;
		while(p){
			if(filename == nil){
				if(p[0] != '"')
					break;
				filename = p+1;
				filename[strlen(filename)-1] = '\0';
			}else if(strncmp(p, "alphamode", 9) == 0)
				alphamode = atoi(p+10);
			else if(strncmp(p, "isdetail", 8) == 0)
				isdetail = atoi(p+9);
			else if(strncmp(p, "hasdetail", 9) == 0)
				hasdetail = atoi(p+10);
			else if(strncmp(p, "detailtile", 10) == 0)
				detailtile = atoi(p+11);
			p = strtok(nil, " \t");
		}
		if(filename){
//			printf("%s %d %d %d\n", filename, alphamode, isdetail, hasdetail);
			if(isdetail)
				detailTextures[isdetail] = RwTextureRead(filename, nil);
			if(hasdetail || detailtile || alphamode){
				std::string s = filename;
				strtolower(s);
				TexInfo *info = new TexInfo;
				memset(info, 0, sizeof(TexInfo));
				if(hasdetail)
					info->detail = &detailTextures[hasdetail];
				if(detailtile)
					info->detailtile = detailtile;
				if(alphamode)
					info->alphamode = alphamode;
				texdb[s] = info;
			}
		}
	}
	fclose(f);
}

void
initTexDB(void)
{
	char *path = getpath("models\\Mobile_details.txd");
	if(path == nil)
		return;
	RwStream *stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, path);
	if(RwStreamFindChunk(stream, rwID_TEXDICTIONARY, nil, nil))
		detailTxd = RwTexDictionaryStreamRead(stream);
	RwStreamClose(stream, nil);
	if(detailTxd == nil){
		MessageBox(nil, "Couldn't find Tex Dictionary inside 'models\\Mobile_details.txd'", "Error", MB_ICONERROR | MB_OK);
		exit(0);
	}
	// we can just set this to current because we're executing before CGame::Initialise
	// which sets up "generic" as the current TXD
	RwTexDictionarySetCurrent(detailTxd);

	readTxt();
}

TexInfo*
RwTextureGetTexDBInfo(RwTexture *tex)
{
	if(tex == nil)
		return &faketexinfo;
	TexInfo **plg = RWPLUGINOFFSET(TexInfo*, tex, texdbOffset);
	if(*plg)
		return *plg;
	// no info, look up in db
	std::string s = tex->name;
	strtolower(s);
	auto info = texdb.find(s);
	if(info != texdb.end())
		*plg = info->second;
	else
		*plg = &faketexinfo;
	return *plg;
}

static void*
texdbCtor(void *object, RwInt32 offsetInObject, RwInt32)
{
	*RWPLUGINOFFSET(TexInfo*, object, offsetInObject) = nil;
	return object;
}

WRAPPER RwInt32 RwTextureRegisterPlugin(RwInt32, RwUInt32, RwPluginObjectConstructor, RwPluginObjectDestructor, RwPluginObjectCopy) { EAXJMP(0x7F3BB0); }
int
TexDBPluginAttach(void)
{
	texdbOffset = RwTextureRegisterPlugin(sizeof(TexInfo*), 0xbadeaffe, texdbCtor, nil, nil);
	return 1;
}
