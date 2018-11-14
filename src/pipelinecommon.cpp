#include "skygfx.h"
#include <DirectXMath.h>

void
RwToD3DMatrix(void *d3d, RwMatrix *rw)
{
	D3DMATRIX *m = (D3DMATRIX*)d3d;
	m->m[0][0] = rw->right.x;
	m->m[0][1] = rw->up.x;
	m->m[0][2] = rw->at.x;
	m->m[0][3] = rw->pos.x;
	m->m[1][0] = rw->right.y;
	m->m[1][1] = rw->up.y;
	m->m[1][2] = rw->at.y;
	m->m[1][3] = rw->pos.y;
	m->m[2][0] = rw->right.z;
	m->m[2][1] = rw->up.z;
	m->m[2][2] = rw->at.z;
	m->m[2][3] = rw->pos.z;
	m->m[3][0] = 0.0f;
	m->m[3][1] = 0.0f;
	m->m[3][2] = 0.0f;
	m->m[3][3] = 1.0f;
}


void
MakeProjectionMatrix(void *d3d, RwCamera *cam, float nbias, float fbias)
{
	float f = cam->farPlane + fbias;
	float n = cam->nearPlane + nbias;
	D3DMATRIX *m = (D3DMATRIX*)d3d;
	m->m[0][0] = cam->recipViewWindow.x;
	m->m[0][1] = 0.0f;
	m->m[0][2] = 0.0f;
	m->m[0][3] = 0.0f;
	m->m[1][0] = 0.0f;
	m->m[1][1] = cam->recipViewWindow.y;
	m->m[1][2] = 0.0f;
	m->m[1][3] = 0.0f;
	m->m[2][0] = 0.0f;
	m->m[2][1] = 0.0f;
	m->m[2][2] = f/(f-n);
	m->m[2][3] = -n*m->m[2][2];
	m->m[3][0] = 0.0f;
	m->m[3][1] = 0.0f;
	m->m[3][2] = 1.0f;
	m->m[3][3] = 0.0f;
}

enum {
	LOC_combined = 0,
	LOC_world = 1,
};

float *RwD3D9D3D9ViewTransform = (float*)0xC9BC80;
float *RwD3D9D3D9ProjTransform = (float*)0x8E2458;

void
transpose(void *dst, void *src)
{
	float (*m1)[4] = (float(*)[4])dst;
	float (*m2)[4] = (float(*)[4])src;
	m1[0][0] = m2[0][0];
	m1[0][1] = m2[1][0];
	m1[0][2] = m2[2][0];
	m1[0][3] = m2[3][0];
	m1[1][0] = m2[0][1];
	m1[1][1] = m2[1][1];
	m1[1][2] = m2[2][1];
	m1[1][3] = m2[3][1];
	m1[2][0] = m2[0][2];
	m1[2][1] = m2[1][2];
	m1[2][2] = m2[2][2];
	m1[2][3] = m2[3][2];
	m1[3][0] = m2[0][3];
	m1[3][1] = m2[1][3];
	m1[3][2] = m2[2][3];
	m1[3][3] = m2[3][3];
}

static DirectX::XMMATRIX pipeWorldMat, pipeViewMat, pipeProjMat;

void
pipeGetComposedTransformMatrix(RpAtomic *atomic, float *out)
{
	RwMatrix *world = RwFrameGetLTM(RpAtomicGetFrame(atomic));

	RwToD3DMatrix(&pipeWorldMat, world);
	transpose(&pipeViewMat, RwD3D9D3D9ViewTransform);
	transpose(&pipeProjMat, RwD3D9D3D9ProjTransform);

	DirectX::XMMATRIX combined = DirectX::XMMatrixMultiply(pipeProjMat, DirectX::XMMatrixMultiply(pipeViewMat, pipeWorldMat));
	memcpy(out, &combined, 64);
}

// Have to call the above before!
void
pipeGetCameraTransformMatrix(float *out)
{
	DirectX::XMMATRIX combined = DirectX::XMMatrixMultiply(pipeViewMat, pipeWorldMat);
	memcpy(out, &combined, 64);
}

void
pipeGetLeedsEnvMapMatrix(RpAtomic *atomic, float *out)
{
	DirectX::XMMATRIX tmp;

	RwMatrix *world = RwFrameGetLTM(RpAtomicGetFrame(atomic));
	RwCamera *cam = (RwCamera*)RWSRCGLOBAL(curCamera);

	float view[16];
	// Kill pitch in camera matrix
	RwMatrix mat = *RwFrameGetLTM(RwCameraGetFrame(cam));
	mat.pos.x = 0.0f;
	mat.pos.y = 0.0f;
	mat.pos.z = 0.0f;
	mat.at.z = 0.0f;
	RwV3dNormalize(&mat.at, &mat.at);
	mat.up.x = 0.0f;
	mat.up.y = 0.0f;
	mat.up.z = 1.0f;
	mat.right.x = -mat.at.y;
	mat.right.y = mat.at.x;
	mat.right.z = 0;
	mat.flags = rwMATRIXTYPEORTHONORMAL;

	// this can be simplified, invert is just transpose as well
	RwMatrixInvert((RwMatrix*)view, &mat);
	view[0] = -view[0];
	view[3] = 0.0f;
	view[4] = -view[4];
	view[7] = 0.0f;
	view[8] = -view[8];
	view[11] = 0.0f;
	view[12] = -view[12];
	view[15] = 1.0f;
	transpose(&tmp, view);

	tmp = DirectX::XMMatrixMultiply(tmp, pipeWorldMat);
	memcpy(out, &tmp, 64);
}

void
pipeUploadMatCol(int flags, RpMaterial *m, int loc)
{
	static float white[4] = { 1.0, 1.0, 1.0, 1.0 };
	if(flags & rpGEOMETRYMODULATEMATERIALCOLOR){
		RwRGBAReal color;
		RwRGBARealFromRwRGBA(&color, &m->color);
		RwD3D9SetVertexShaderConstant(loc, &color, 1);
	}else
		RwD3D9SetVertexShaderConstant(loc, white, 1);
}

void
pipeUploadZero(int loc)
{
	static float z[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	RwD3D9SetVertexShaderConstant(loc, (void*)z, 1);
}

void
pipeUploadLightColor(RpLight *light, int loc)
{
	float c[4];
	if(RpLightGetFlags(light) & rpLIGHTLIGHTATOMICS){
		c[0] = light->color.red;
		c[1] = light->color.green;
		c[2] = light->color.blue;
		c[3] = 1.0f;
		RwD3D9SetVertexShaderConstant(loc, (void*)c, 1);
	}else
		pipeUploadZero(loc);
}

void
pipeUploadLightDirection(RpLight *light, int loc)
{
	float c[4];
	if(RpLightGetFlags(light) & rpLIGHTLIGHTATOMICS){
		RwV3d *at = RwMatrixGetAt(RwFrameGetLTM(RpLightGetFrame(light)));
		c[0] = at->x;
		c[1] = at->y;
		c[2] = at->z;
		c[3] = 1.0f;
		RwD3D9SetVertexShaderConstant(loc, (void*)c, 1);
	}else
		pipeUploadZero(loc);
}

void
pipeUploadLightDirectionLocal(RpLight *light, RwMatrix *m, int loc)
{
	float c[4];
	if(RpLightGetFlags(light) & rpLIGHTLIGHTATOMICS){
		RwV3d *at = RwMatrixGetAt(RwFrameGetLTM(RpLightGetFrame(light)));
		RwV3dTransformVector((RwV3d*)c, at, m);
		c[3] = 1.0f;
		RwD3D9SetVertexShaderConstant(loc, (void*)c, 1);
	}else
		pipeUploadZero(loc);
}

void
pipeUploadLightDirectionInv(RpLight *light, int loc)
{
	float c[4];
	RwV3d *at = RwMatrixGetAt(RwFrameGetLTM(RpLightGetFrame(light)));
	c[0] = -at->x;
	c[1] = -at->y;
	c[2] = -at->z;
	RwD3D9SetVertexShaderConstant(loc, (void*)c, 1);
}

void
makePS(int res, void **sh)
{
	if(*sh == NULL){
		HRSRC resource = FindResource(dllModule, MAKEINTRESOURCE(res), RT_RCDATA);
		RwUInt32 *shader = (RwUInt32*)LoadResource(dllModule, resource);
		RwD3D9CreatePixelShader(shader, sh);
		FreeResource(shader);
	}
}

void
makeVS(int res, void **sh)
{
	if(*sh == NULL){
		HRSRC resource = FindResource(dllModule, MAKEINTRESOURCE(res), RT_RCDATA);
		RwUInt32 *shader = (RwUInt32*)LoadResource(dllModule, resource);
		RwD3D9CreateVertexShader(shader, sh);
		FreeResource(shader);
	}
}

void
CreateShaders(void)
{
	// postfx
	makePS(IDR_IIITRAILSPS, &iiiTrailsPS);
	makePS(IDR_VCTRAILSPS, &vcTrailsPS);
	makePS(IDR_GRADINGPS, &gradingPS);
	makePS(IDR_CONTRASTPS, &contrastPS);

	makePS(IDR_SIMPLEPS, &simplePS);

	// vehicles
	makeVS(IDR_VEHICLEVS, &vehiclePipeVS);
	makeVS(IDR_PS2CARFXVS, &ps2CarFxVS);
	makePS(IDR_PS2ENVSPECFXPS, &ps2EnvSpecFxPS);	// also building
	makeVS(IDR_SPECCARFXVS, &specCarFxVS);
	makePS(IDR_SPECCARFXPS, &specCarFxPS);
	makeVS(IDR_XBOXCARVS, &xboxCarVS);
	makeVS(IDR_LEEDSCARFXVS, &leedsCarFxVS);
	makeVS(IDR_MOBILEVEHICLEVS, &mobileVehiclePipeVS);
	makePS(IDR_MOBILEVEHICLEPS, &mobileVehiclePipePS);

	// building
	makeVS(IDR_PS2BUILDINGVS, &ps2BuildingVS);
	makeVS(IDR_PS2BUILDINGFXVS, &ps2BuildingFxVS);
	makeVS(IDR_XBOXBUILDINGVS, &xboxBuildingVS);
	makePS(IDR_XBOXBUILDINGPS, &xboxBuildingPS);
	makePS(IDR_SIMPLEDETAILPS, &simpleDetailPS);
	makeVS(IDR_SPHEREBUILDINGVS, &sphereBuildingVS);
}
