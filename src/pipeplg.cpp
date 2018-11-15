#include "skygfx.h"

// GTA pipeline

RwInt32 &gtaPipeOffset = *(RwInt32*)0x8D6080;

uint32
GetPipelineID(RpAtomic *atomic)
{
	return *RWPLUGINOFFSET(uint32, atomic, gtaPipeOffset);
}

void
SetPipelineID(RpAtomic *atomic, uint32 id)
{
	*RWPLUGINOFFSET(uint32, atomic, gtaPipeOffset) = id;
}

// PDS pipeline hack

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
	if(extraData == PDS_PS2_CustomBuilding_AtmPipeID ||
	   extraData == PDS_PS2_CustomBuildingEnvMap_AtmPipeID ||
	   extraData == PDS_PS2_CustomBuildingUVA_AtmPipeID)
		if(CCustomBuildingPipeline__ObjPipeline){
			atomic->pipeline = CCustomBuildingPipeline__ObjPipeline;
			SetPipelineID(atomic, RSPIPE_PC_CustomBuilding_PipeID);
		}
	if(extraData == PDS_PS2_CustomBuildingDN_AtmPipeID ||
	   extraData == PDS_PS2_CustomBuildingDNEnvMap_AtmPipeID ||
	   extraData == PDS_PS2_CustomBuildingDNUVA_AtmPipeID)
		if(CCustomBuildingDNPipeline__ObjPipeline){
			atomic->pipeline = CCustomBuildingDNPipeline__ObjPipeline;
			SetPipelineID(atomic, RSPIPE_PC_CustomBuildingDN_PipeID);
		}
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
