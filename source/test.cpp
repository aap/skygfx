#include "skygfx.h"

struct CEntryData
{
	char action;
	char description[8];
	char specialDescFlag;
	short targetMenu;
	short posX;
	short posY;
	short align;
};

struct CMenuItem
{
	char name[8];
	char field_8;
	char field_9;
	CEntryData entryList[12];
};

CMenuItem *screens = (CMenuItem*)0x8CE008;	// 44

void
dumpMenu(void)
{
	FILE *f;
	if(f = fopen("menudump.txt", "wb"), f == NULL)
		return;
	printf("dumping menu\n");
	CMenuItem *sc = screens;
	CEntryData *e;
	for(int i = 0; i < 44; i++){
		if(sc->name[0] == '\0')
			break;
		fprintf(f, "%d %8s %x %x\n", i, sc->name, sc->field_8, sc->field_9);
		e = sc->entryList;
		for(int j = 0; j < 12; j++){
			if(e->description[0] == '\0')
				break;
			fprintf(f, "  %2x %8s %x %x %d %d %x\n",
				e->action, e->description, e->specialDescFlag, e->targetMenu, e->posX, e->posY, e->align);
			e++;
		}
		sc++;
	}
	fclose(f);
}