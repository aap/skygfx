#include "skygfx.h"

RwInt32 pdsMatOffset;
RwInt32 pdsAtmOffset;

void*
pdsConstruct(void *object, RwInt32 offsetInObject, RwInt32)
{
	*RWPLUGINOFFSET(RwUInt32, object, offsetInObject) = 0;
	return object;
}

void*
pdsCopy(void *dstObject, const void *srcObject, RwInt32 offsetInObject, RwInt32)
{
	*RWPLUGINOFFSET(RwUInt32, dstObject, offsetInObject) = *RWPLUGINOFFSET(RwUInt32, srcObject, offsetInObject);
	return dstObject;
}

RwBool
pdsMatCB(void *object, RwInt32 offsetInObject, RwInt32, RwUInt32 extraData)
{
	*RWPLUGINOFFSET(RwUInt32, object, offsetInObject) = extraData;
	return TRUE;
}

RwBool
pdsAtmCB(void *object, RwInt32, RwInt32, RwUInt32 extraData)
{
	RpAtomic *atomic = (RpAtomic*)object;
	if(extraData == 0x53f20080)
		if(buildingPipeline)
			atomic->pipeline = buildingPipeline;
	if(extraData == 0x53f20082)
		if(buildingDNPipeline)
			atomic->pipeline = buildingDNPipeline;
	return TRUE;
}

int
PDSPipePluginAttach(void)
{
	//pdsMatOffset = RpMaterialRegisterPlugin(4, 0x131, pdsConstruct, NULL, pdsCopy);
	pdsAtmOffset = RpAtomicRegisterPlugin(0, 0x131, NULL, NULL, NULL);
	//RpMaterialSetStreamRightsCallBack(0x131, pdsMatCB);
	RpAtomicSetStreamRightsCallBack(0x131, pdsAtmCB);
	return 1;
}
