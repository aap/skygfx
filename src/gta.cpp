#include "skygfx.h"

RsGlobalType *RsGlobal = (RsGlobalType*)0xC17040;
IDirect3DDevice9 *&d3d9device = *(IDirect3DDevice9**)0xC97C28;
RwCamera *&Camera = *(RwCamera**)0xC1703C;
RpLight *&pAmbient = *(RpLight**)0xC886E8;
RpLight *&pDirect = *(RpLight**)0xC886EC;
RpLight **pExtraDirectionals = (RpLight**)0xC886F0;
int &NumExtraDirLightsInWorld = *(int*)0xC88708;
D3DLIGHT9 &gCarEnvMapLight = *(D3DLIGHT9*)0xC02CB0;

RwTexture *&gpWhiteTexture = *(RwTexture**)0xB4E3EC;


int16 &CClock__ms_nGameClockSeconds = *(short*)0xB70150;
uint8 &CClock__ms_nGameClockMinutes = *(byte*)0xB70152;
uint8 &CClock__ms_nGameClockHours = *(byte*)0xB70153;
int16 &CWeather__OldWeatherType = *(short*)0xC81320;
int16 &CWeather__NewWeatherType = *(short*)0xC8131C;
float &CWeather__InterpolationValue = *(float*)0xC8130C;

void **rwengine = *(void***)0x58FFC0;
RwInt32 &CCustomCarEnvMapPipeline__ms_envMapPluginOffset = *(RwInt32*)0x8D12C4;
RwInt32 &CCustomCarEnvMapPipeline__ms_envMapAtmPluginOffset = *(RwInt32*)0x8D12C8;
RwInt32 &CCustomCarEnvMapPipeline__ms_specularMapPluginOffset = *(RwInt32*)0x8D12CC;
RwReal &CCustomCarEnvMapPipeline__m_EnvMapLightingMult = *(RwReal*)0x8D12D0;
RwInt32 &CCustomBuildingDNPipeline__ms_extraVertColourPluginOffset = *(RwInt32*)0x8D12BC;
RwReal &CCustomBuildingDNPipeline__m_fDNBalanceParam = *(float*)0x8D12C0;
int &dword_C02C20 = *(int*)0xC02C20;
int &dword_C9BC60 = *(int*)0xC9BC60;
RxPipeline *&skinPipe = *(RxPipeline**)0xC978C4;
RxPipeline *&CCustomCarEnvMapPipeline__ObjPipeline = *(RxPipeline**)0xC02D24;


WRAPPER RwMatrix *RwFrameGetLTM(RwFrame * frame) { EAXJMP(0x7F0990); }
WRAPPER RpMaterial *RpMaterialSetTexture(RpMaterial *mat, RwTexture *tex) { EAXJMP(0x74DBC0); }
WRAPPER RwRaster *RwRasterCreate(RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 flags) { EAXJMP(0x7FB230); }
WRAPPER RwBool RwRasterDestroy(RwRaster *raster) { EAXJMP(0x7FB020); }
WRAPPER RwRaster *RwRasterPushContext(RwRaster *raster) { EAXJMP(0x7FB060); }
WRAPPER RwRaster *RwRasterPopContext(void) { EAXJMP(0x7FB110); }
WRAPPER RwRaster *RwRasterRenderScaled(RwRaster *raster, RwRect *rect) { EAXJMP(0x7FAE80); }
WRAPPER RwRaster *RwRasterRenderFast(RwRaster *raster, RwInt32 x, RwInt32 y) { EAXJMP(0x7FAF50); }
WRAPPER RwUInt8 *RwRasterLock(RwRaster*, RwUInt8, RwInt32) { EAXJMP(0x7FB2D0); }
WRAPPER RwRaster *RwRasterUnlock(RwRaster*) { EAXJMP(0x7FAEC0); }
WRAPPER RwCamera *RwCameraBeginUpdate(RwCamera *camera) { EAXJMP(0x7EE190); }
WRAPPER RwCamera *RwCameraEndUpdate(RwCamera *camera) { EAXJMP(0x7EE180); }
WRAPPER RwTexture *RwTextureCreate(RwRaster*) { EAXJMP(0x7F37C0); }

WRAPPER RwInt32 RpMaterialRegisterPlugin(RwInt32, RwUInt32, RwPluginObjectConstructor,
	RwPluginObjectDestructor, RwPluginObjectCopy) { EAXJMP(0x74DBF0); }
WRAPPER RwInt32 RpMaterialSetStreamRightsCallBack(RwUInt32, RwPluginDataChunkRightsCallBack) { EAXJMP(0x74DC70); }
WRAPPER RwInt32 RpAtomicRegisterPlugin(RwInt32, RwUInt32, RwPluginObjectConstructor,
	RwPluginObjectDestructor, RwPluginObjectCopy) { EAXJMP(0x74BDA0); }
WRAPPER RwInt32 RpAtomicSetStreamRightsCallBack(RwUInt32, RwPluginDataChunkRightsCallBack) { EAXJMP(0x74BE50); }


WRAPPER RpMatFXMaterialFlags RpMatFXMaterialGetEffects(const RpMaterial*) { EAXJMP(0x812140); }
WRAPPER const RpMaterial *RpMatFXMaterialGetUVTransformMatrices(const RpMaterial*, RwMatrix**, RwMatrix**) { EAXJMP(0x812A50); }

WRAPPER RwBool RwD3D9CreatePixelShader(const RwUInt32 *function, void **shader) { EAXJMP(0x7FACC0); }
WRAPPER RwBool RwD3D9CreateVertexShader(const RwUInt32 *function, void **shader) { EAXJMP(0x7FAC60); }
WRAPPER void _rwD3D9SetPixelShaderConstant(RwUInt32 i, const void *data, RwUInt32 size) { EAXJMP(0x7FAD00); }
WRAPPER void _rwD3D9VSSetActiveWorldMatrix(const RwMatrix *worldMatrix) { EAXJMP(0x764650); }
WRAPPER void _rwD3D9VSGetComposedTransformMatrix(void *transformMatrix) { EAXJMP(0x7646E0); }
WRAPPER void _rwD3D9VSGetInverseWorldMatrix(void *inverseWorldMatrix) { EAXJMP(0x7647B0); }
WRAPPER void RwD3D9GetRenderState(RwUInt32 state, void *value) { EAXJMP(0x7FC320); }
WRAPPER void RwD3D9SetRenderState(RwUInt32 state, RwUInt32 value) { EAXJMP(0x7FC2D0); }
WRAPPER RwBool RwD3D9SetTexture(RwTexture *texture, RwUInt32 stage) { EAXJMP(0x7FDE70); }
WRAPPER void RwD3D9SetTextureStageState(RwUInt32 stage, RwUInt32 type, RwUInt32 value) { EAXJMP(0x7FC340); }
WRAPPER RwBool RwD3D9SetSurfaceProperties(const RwSurfaceProperties *surfaceProps, const RwRGBA *color, RwUInt32 flags) { EAXJMP(0x7FC4D0); }
WRAPPER void RwD3D9GetTransform(RwUInt32 state, void *matrix) { EAXJMP(0x7FA4F0); }
WRAPPER RwBool RwD3D9SetTransform(RwUInt32 state, const void *matrix) { EAXJMP(0x7FA390); }
WRAPPER void _rwD3D9SetVertexShaderConstant(RwUInt32 registerAddress, const void *constantData, RwUInt32  constantCount) { EAXJMP(0x7FACA0); }
WRAPPER void _rwD3D9SetVertexShader(void *shader) { EAXJMP(0x7F9FB0); }
WRAPPER void _rwD3D9SetPixelShader(void *shader) { EAXJMP(0x7F9FF0); }
WRAPPER void _rwD3D9SetIndices(void *indexBuffer) { EAXJMP(0x7FA1C0); }
WRAPPER void _rwD3D9SetStreams(const RxD3D9VertexStream *streams, RwBool useOffsets) { EAXJMP(0x7FA090); }
WRAPPER void _rwD3D9SetVertexDeclaration(void *vertexDeclaration) { EAXJMP(0x7F9F70); }
WRAPPER void _rwD3D9DrawIndexedPrimitive(RwUInt32 primitiveType, RwInt32 baseVertexIndex, RwUInt32 minIndex,
		RwUInt32 numVertices, RwUInt32 startIndex, RwUInt32 primitiveCount) { EAXJMP(0x7FA320); }
WRAPPER void _rwD3D9DrawPrimitive(RwUInt32 primitiveType, RwUInt32 startVertex, RwUInt32 primitiveCount) { EAXJMP(0x7FA360); }
WRAPPER void _rwD3D9DrawIndexedPrimitiveUP(RwUInt32 primitiveType, RwUInt32 minIndex, RwUInt32 numVertices, RwUInt32 primitiveCount,
	const void *indexData, const void *vertexStreamZeroData, RwUInt32 VertexStreamZeroStride) { EAXJMP(0x7FA1F0); }
WRAPPER RwBool RwD3D9SetLight(RwInt32 index, const void *light) { EAXJMP(0x7FA660); }
WRAPPER RwBool RwD3D9EnableLight(RwInt32 index, RwBool enable) { EAXJMP(0x7FA860); }
WRAPPER RwBool RwD3D9CreateVertexDeclaration(const void *elements, void **vertexdeclaration) { EAXJMP(0x7FAA30); }
WRAPPER void RwD3D9DestroyVertexBuffer(RwUInt32 stride, RwUInt32 size, void *vertexBuffer, RwUInt32 offset) { EAXJMP(0x7F56A0); }
WRAPPER RwBool RwD3D9CreateVertexBuffer(RwUInt32 stride, RwUInt32 size, void **vertexBuffer, RwUInt32 *offset) { EAXJMP(0x7F5500); }
WRAPPER RwUInt32 _rpD3D9VertexDeclarationInstV3d(RwUInt32 type, RwUInt8 *mem, const RwV3d *src,
		RwInt32 numVerts, RwUInt32 stride) { EAXJMP(0x752AD0) };
WRAPPER RwUInt32 _rpD3D9VertexDeclarationInstV2d(RwUInt32 type, RwUInt8 *mem, const RwV2d *src,
		RwInt32 numVerts, RwUInt32 stride) { EAXJMP(0x7544E0) };
WRAPPER RwBool _rpD3D9VertexDeclarationInstColor(RwUInt8 *mem, const RwRGBA *color, RwInt32 numVerts, RwUInt32 stride) { EAXJMP(0x754AE0); };
WRAPPER RwBool RwD3D9SetRenderTarget(RwUInt32 index, RwRaster *raster) { EAXJMP(0x7F9E90); };

WRAPPER void _rxPipelineDestroy(RxPipeline * Pipeline) { EAXJMP(0x805820); }
WRAPPER RxPipeline *RxPipelineCreate(void) { EAXJMP(0x8057B0); }
WRAPPER RxPipeline *RxPipelineExecute(RxPipeline *pipeline, void *data, RwBool heapReset) { EAXJMP(0x805710); };
WRAPPER RxLockedPipe *RxPipelineLock(RxPipeline *pipeline) { EAXJMP(0x806990); }
WRAPPER RxPipeline *RxLockedPipeUnlock(RxLockedPipe *pipeline) { EAXJMP(0x805F40); }
WRAPPER RxNodeDefinition *RxNodeDefinitionGetD3D9AtomicAllInOne(void) { EAXJMP(0x7582E0); }
WRAPPER RxPipelineNode *RxPipelineFindNodeByName(RxPipeline *pipeline, const RwChar *name, RxPipelineNode *start, RwInt32 *nodeIndex) { EAXJMP(0x806B30); }
WRAPPER RxLockedPipe *RxLockedPipeAddFragment(RxLockedPipe *pipeline, RwUInt32 *firstIndex, RxNodeDefinition *nodeDef0, ...) { EAXJMP(0x806BE0); }
WRAPPER RxD3D9AllInOneInstanceCallBack RxD3D9AllInOneGetInstanceCallBack(RxPipelineNode *node) { EAXJMP(0x757390) };
WRAPPER RxD3D9AllInOneReinstanceCallBack RxD3D9AllInOneGetReinstanceCallBack(RxPipelineNode *node) { EAXJMP(0x7573B0) };
WRAPPER void RxD3D9AllInOneSetInstanceCallBack(RxPipelineNode *node, RxD3D9AllInOneInstanceCallBack callback) { EAXJMP(0x757380) };
WRAPPER void RxD3D9AllInOneSetReinstanceCallBack(RxPipelineNode *node, RxD3D9AllInOneReinstanceCallBack callback) { EAXJMP(0x7573A0) };
WRAPPER void RxD3D9AllInOneSetRenderCallBack(RxPipelineNode *node, RxD3D9AllInOneRenderCallBack callback) { EAXJMP(0x7573E0) };

WRAPPER RwBool DNInstance(void *object, RxD3D9ResEntryHeader *resEntryHeader, RwBool reinstance) { EAXJMP(0x5D5FE0) };
WRAPPER RwBool D3D9RestoreSurfaceProperties(void) { EAXJMP(0x5DAAC0); }
WRAPPER RwBool D3D9SetRenderMaterialProperties(RwSurfaceProperties*, RwRGBA *color,
		RwUInt32 flags, RwReal specularLighting, RwReal specularPower) { EAXJMP(0x5DA790); };

WRAPPER RwUInt16 CVisibilityPlugins__GetAtomicId(RpAtomic *atomic) { EAXJMP(0x732370); }

WRAPPER RpAtomic *CCustomCarEnvMapPipeline__CustomPipeAtomicSetup(RpAtomic *atomic) { EAXJMP(0x5DA610); }
WRAPPER char *GetFrameNodeName(RwFrame *frame) { EAXJMP(0x72FB30); }
WRAPPER void SetPipelineID(RpAtomic*, unsigned int it) { EAXJMP(0x72FC50); }
WRAPPER RpAtomic *AtomicDefaultRenderCallBack(RpAtomic*) { EAXJMP(0x7491C0); };
WRAPPER void CCustomCarEnvMapPipeline__CustomPipeRenderCB_exe(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags) { EAXJMP(0x5D9900) };
WRAPPER void GTAfree(void *data) { EAXJMP(0x82413F); }

static uint32_t RwV3dLength_A = (uint32_t)(0x7EDAC0);
WRAPPER RwReal RwV3dLength(const RwV3d*) { VARJMP(RwV3dLength_A); }
static uint32_t RwD3D9SetStreamSource_A = (uint32_t)(0x7FA030);
WRAPPER void RwD3D9SetStreamSource(RwUInt32, void*, RwUInt32, RwUInt32) { VARJMP(RwD3D9SetStreamSource_A); }
WRAPPER void RwD3D9SetFVF(RwUInt32) { EAXJMP(0x7F9F30); }

WRAPPER RwMatrix *RwMatrixCreate(void) { EAXJMP(0x7F2A50); }
WRAPPER RwMatrix *RwMatrixInvert(RwMatrix*, const RwMatrix*) { EAXJMP(0x7F2070); }
WRAPPER RwMatrix *RwMatrixUpdate(RwMatrix* matrix) { EAXJMP(0x7F18A0); }
WRAPPER RwFrame *RwFrameTransform(RwFrame*, const RwMatrix*, RwOpCombineType) { EAXJMP(0x7F0F70); }
WRAPPER RwFrame *RwFrameCreate(void) { EAXJMP(0x7F0410); }
WRAPPER void _rwObjectHasFrameSetFrame(void*, RwFrame*) { EAXJMP(0x804EF0); }
WRAPPER RwCamera *RwCameraClear(RwCamera*, RwRGBA*, RwInt32) { EAXJMP(0x7EE340); }
WRAPPER RwCamera *RwCameraCreate(void) { EAXJMP(0x7EE4F0); }
WRAPPER RwCamera *RwCameraSetViewWindow(RwCamera*, const RwV2d*) { EAXJMP(0x7EE410); }
WRAPPER RwCamera *RwCameraSetNearClipPlane(RwCamera*, RwReal) { EAXJMP(0x7EE1D0); }
WRAPPER RwCamera *RwCameraSetFarClipPlane(RwCamera*, RwReal) { EAXJMP(0x7EE2A0); }
WRAPPER RpWorld *RpWorldAddCamera(RpWorld*, RwCamera*) { EAXJMP(0x750F20); }

WRAPPER RwV3d *RwV3dTransformVector(RwV3d*, const RwV3d*, const RwMatrix*) { EAXJMP(0x7EDDC0); }
WRAPPER RwV3d *RwV3dTransformPoint(RwV3d*, const RwV3d*, const RwMatrix*) { EAXJMP(0x7EDD60); }
WRAPPER RwReal RwV3dNormalize(RwV3d*, const RwV3d*) { EAXJMP(0x7ED9B0); }
WRAPPER RwMatrix *RwMatrixMultiply(RwMatrix*, const RwMatrix*, const RwMatrix*) { EAXJMP(0x7F18B0); }
WRAPPER RwMatrix *RwMatrixRotate(RwMatrix*, const RwV3d*, RwReal, RwOpCombineType) { EAXJMP(0x7F1FD0); }
WRAPPER RwFrame *RwFrameUpdateObjects(RwFrame*) { EAXJMP(0x7F0910); }
WRAPPER RwFrame *RwFrameSetIdentity(RwFrame*) { EAXJMP(0x7F10B0); }
WRAPPER RwMatrix *RwMatrixOrthoNormalize(RwMatrix*, const RwMatrix*) { EAXJMP(0x7F1920); }
WRAPPER RwMatrix *RwMatrixTransform(RwMatrix*, const RwMatrix*, RwOpCombineType) { EAXJMP(0x7F25A0); }
WRAPPER RwMatrix *RwMatrixOptimize(RwMatrix*, const RwMatrixTolerance*) { EAXJMP(0x7F17E0); }