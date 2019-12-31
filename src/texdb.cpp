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

static TexInfo*
FindTexInfo(char *name)
{
	std::string s = name;
	strtolower(s);
	auto info = texdb.find(s);
	if(info != texdb.end())
		return info->second;
	else
		return nil;
}

static void
readTxt(void)
{
	char buf[200], *start, *end, *p;
	TexInfo *info;

	char *path = getpath("models\\texdb.txt");
	if(path == nil)
		return;
	FILE *f = fopen(path, "r");
	if(f == nil)
		return;

	while(fgets(buf, 200, f)){
		char *filename = nil;
		char *affiliate = nil;
		int alphamode = 0;
		int isdetail = 0;
		int hasdetail = 0;
		int detailtile = 80;
		int hassibling = 0;
		start = end = buf;
		while(end){
			if(start[0] == '"'){
				end = strchr(++start, '\"');
				p = end;
				end = strchr(end, ' ');
				*p = '\0';
			}else
				end = strchr(start, ' ');
			if(end)
				*end = '\0';

			if(strchr(start, '=') == nil)
				filename = start;
			else if(strncmp(start, "cat=", 4) == 0)
				goto nextline;
			else if(strncmp(start, "alphamode=", 10) == 0)
				alphamode = atoi(start+10);
			else if(strncmp(start, "isdetail=", 9) == 0)
				isdetail = atoi(start+9);
			else if(strncmp(start, "hasdetail=", 10) == 0)
				hasdetail = atoi(start+10);
			else if(strncmp(start, "detailtile=", 11) == 0)
				detailtile = atoi(start+11);
			else if(strncmp(start, "hassibling=", 11) == 0)
				hassibling = atoi(start+11) != 0;
			else if(strncmp(start, "affiliate=", 10) == 0)
				affiliate = start+10;

			start = end+1;
		}
		if(filename){
			if(isdetail)
				detailTextures[isdetail] = RwTextureRead(filename, nil);
			else if(hasdetail || alphamode || hassibling || affiliate){
				std::string s = filename;
				strtolower(s);
				info = new TexInfo;
				memset(info, 0, sizeof(TexInfo));

				info->name = strdup(filename);
				if(affiliate)
					info->affiliate = strdup(affiliate);
				if(hasdetail){
					info->detailnum = hasdetail;
					info->detailtile = detailtile;
				}
				if(hassibling)
					info->hassibling = 1;
				if(alphamode)
					info->alphamode = alphamode;

				texdb[s] = info;
			}
		}
	nextline:;
	}

	// Link up stuff
	for(auto const &e : texdb){
		info = e.second;
		if(info->affiliate)
			info->affiliateTex = FindTexInfo(info->affiliate);
		if(info->detailnum)
			info->detail = detailTextures[info->detailnum];
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
	else
		return &faketexinfo;
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

/* For the sibling mechanism we need the name of the current TXD.
 * Since CTxdStore doesn't know about the names (only their hashes)
 * we have to hook it and set the name together with the TXD */

static int txdslot;
char *txdnames[65500];	//[5000]; UG needs a lot
char currentTxdName[32];

static RwTexture *(*TxdStoreFindCB)(char *name);
static RwTexture*
TexDbFindCB(char *name)
{
	RwTexture *tex;
	tex = TxdStoreFindCB(name);
	if(tex){
		TexInfo *info = FindTexInfo(name);
		if(info){
			if(info->affiliateTex)
				info = info->affiliateTex;
			if(info->hassibling){
				static char sibling[200];
				sprintf(sibling, "%s_%s", name, currentTxdName);
				TexInfo *sib = FindTexInfo(sibling);
				if(sib)
					info = sib;
			}
			if(info->affiliateTex)
				info = info->affiliateTex;
			*RWPLUGINOFFSET(TexInfo*, tex, texdbOffset) = info;
		}
	}
	return tex;
}

static int
storeTxdName(int ret/*return address on stack*/, char *txdname)
{
	txdnames[txdslot] = strdup(txdname);
	return txdslot;
}

static void
setCurrentTxdName(void)
{
	strcpy(currentTxdName, txdnames[txdslot]);
}

static void __declspec(naked)
addTxdSlot_hook(void)
{
	_asm{
		mov	txdslot,eax
		call	storeTxdName
		retn
	}
}

static void __declspec(naked)
setCurrentTxd_hook(void)
{
	_asm{
		mov	ecx,[esp+4]
		mov	txdslot,ecx
		mov	[esp+4],edx
		push	eax	// return value, is it even used?
		call	setCurrentTxdName
		pop	eax	// we overwrote the original code but there's another jump closeby
		push	0x7319DB
		retn
	}
}

void
hooktexdb(void)
{
	InjectHook(0x731CCB, addTxdSlot_hook, PATCH_JUMP);
	InjectHook(0x7319EB, setCurrentTxd_hook, PATCH_JUMP);

	TxdStoreFindCB = *(RwTexture*(**)(char*))(0x731FD5+1);
	Patch(0x731FD5+1, TexDbFindCB);
}
