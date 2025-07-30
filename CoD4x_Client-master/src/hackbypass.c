#include "q_shared.h"
#include "cg_shared.h"
#include "cm_local.h"
#include "dobj.h"
#include "refdef.h"
#include "returnaddrcheck.h"
#include "sys_patch.h"
#include "xassets/weapondef.h"
#include "antireversing/antireversing.h"

#include "exeobfus/exeobfus.h"
#include "win_sys.h"
#include <stdbool.h>

//void __usercall CG_GetEntityBModelBounds(float *maxs@<ecx>, float *absmaxs@<edi>, centity_t *cent@<esi>, float *mins, float *absmins)

//static inline unsigned int computeCrcFromFunc(unsigned char* funstart);
//static inline unsigned int computeCrcFromDrawIndexedPrimitive();

#define Q_sqrt sqrt
static int Cmd_IsCommandPresent( const char *cmd_name);
static int Cvar_IsPresent(const char *var_name);
void BG_WeaponFireRecoil( );
void AimAssist_UpdateMouseInput_Stub();

#define ragdoll_enable getcvaradr(0xCC789F8)

cmodel_t *__cdecl CM_ClipHandleToModel(unsigned int index)
{
  if ( cm.numSubModels <= index )
    return *(cmodel_t **)(Sys_GetValue(3) + 12);
  else
    return &cm.cmodels[index];

}



/*
===================
CM_ModelBounds
===================
*/
void CM_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs ) {
	cmodel_t    *cmod;

	cmod = CM_ClipHandleToModel( model );
	VectorCopy( cmod->mins, mins );
	VectorCopy( cmod->maxs, maxs );
}

/*
void CG_GetEntityBModelBounds(centity_t *cent, float *mins, float *maxs, float *absmins, float *absmaxs)
{
  double v7;
  float v8;
  double v9;
  float a5a;
  float a5b;
  float a5c;
  float a5d;
  float a5e;

  if ( cent->nextState.solid == 0xFFFFFF )
  {
    CM_ModelBounds(cent->nextState.index, mins, maxs);
  }
  else
  {
	int q1 = cent->nextState.solid & 0xFF00;
	int q2 = cent->nextState.solid & 0xFF0000;
    a5a = (double)(unsigned __int8)cent->nextState.solid;
    v7 = a5a;
    a5b = 1.0 - a5a;
    mins[1] = a5b;
    *mins = a5b;
    a5c = v7 - 1.0;
    maxs[1] = a5c;
    *maxs = a5c;
    mins[2] = 1.0 - (double)(q1 - 1);
    maxs[2] = (double)(q2 - 32) - 1.0;
  }
  v8 = RadiusFromBounds(maxs, mins);
  a5d = Q_sqrt(v8);
  v9 = a5d;
  a5e = -a5d;
  *absmins = a5e;
  absmins[1] = a5e;
  absmins[2] = a5e;
  *absmaxs = v9;
  absmaxs[1] = v9;
  absmaxs[2] = v9;

  _VectorAdd( cent->pose.origin, absmins, absmins );
  _VectorAdd( cent->pose.origin, absmaxs, absmaxs );
}
*/


DObjAnimMat *__cdecl DObjGetRotTransArrayEx(DObj_t *obj)
{
  return obj->skel.mat;
}

//#define CRCDEBUG
#ifdef COD4XDEV
#define CRCDEBUG
#endif

static inline unsigned short icrc16(unsigned char* data_p, DWORD length) {
    unsigned char x;
    unsigned short crc = 0xFFFF;

    while (length--){
        x = crc >> 8 ^ *data_p++;
        x ^= x>>4;
        crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x <<5)) ^ ((unsigned short)x);
    }
    return crc;
}
#if 0
static inline unsigned short icrc16_masked(unsigned char* data_p, DWORD length, unsigned char* mask){
    unsigned char x;
    unsigned short crc = 0xFFFF;
    //unsigned int cnt = 0;

    while (length--)
    {
        //Com_Printf("%x%c ", *data_p, *mask);
        //if(cnt++ % 10 == 0)
        //  Com_Printf("\n");

        if(*mask == '!')
        {
          x = crc >> 8 ^ *data_p;
          x ^= x>>4;
          crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x <<5)) ^ ((unsigned short)x);
        }
        
        mask++;
        data_p++;
    }

    return crc;
}
#endif
static inline void checkImage(DWORD start, DWORD length, WORD sum, int index)
{
#ifndef COD4XDEV
	if(sum != icrc16((BYTE*)start, length))
	{
#ifdef CRCDEBUG
		Com_Printf(CON_FIRST_DEBUG_CHANNEL, "^@Invalid CRC for index %d. Expected CRC is %x Got %x\n", index, sum, icrc16((BYTE*)start, length));
#else
		Sec_SetToCrashGame();
#endif
	}
#endif // COD4XDEV
}

#define CHECKIMAGESE(start, end, sum, index) checkImage(start, end - start, sum, index)

typedef struct
{
	DWORD start;
	DWORD end;
	DWORD sum;
}imagecheckdata_t;


bool ZXIsJumpInstruction(void* adr)
{
	byte* inst = (byte*)adr;
	if(inst[0] == 0xe9)
	{
		return 1;
	}
	return 0;
}
#define ZXHOOKDETECT ZXIsJumpInstruction


unsigned int dummyD3DContext_DrawIndexedPrimitive_CRC;
// unsigned int ranonce = 0;
#define NOD3DVALIDATION

void initd3dcheck()
{
#ifndef NOD3DVALIDATION
#ifndef COD4XDEV
if(dummyD3DContext_DrawIndexedPrimitive_CRC == 0)
{
  dummyD3DContext_DrawIndexedPrimitive_CRC = computeCrcFromDrawIndexedPrimitiveDummy();
}
#endif
#endif
}

void RunImageChecks()
{
#ifndef COD4XDEV
//    unsigned short checksum;
//    int loopidx;
    
    int* pInterface = (int*)*((int*)r_dx.device);
    //0xdcb428f >

    //DISASM_MISALIGN;

	static int i = 0;
  //Notice: When start and end offset is mixed uo it crashes always!
	imagecheckdata_t data[] = {
        { 0x457C60, 0x457CA7, 0xa49b },
        { 0x457CB7, 0x457D2E, 0xeef2 },
        { 0x457D33, 0x457D83, 0x3966 },
        { 0x457D8A, 0x457EF7, 0x8f1d },
        { 0x457EFE, 0x457F40, 0xd300 },
        { 0x41A7B0, 0x41A7D7, 0x9D36 },
        { 0x41A7DE, 0x41AA10, 0xe803 },
        { 0x456D00, 0x456D95, 0x3d6e }, //Laser was here already Dom
        { 0x456D99, 0x456E43, 0xda34 }, //Laser was here already Dom
        { 0x456E48, 0x456F07, 0x6544 },
        { 0x456F0E, 0x456F21, 0x7452 },
        { 0x456F28, 0x456F58, 0xa403 },
        { 0x445370, 0x445504, 0x77e6 }, // 0x0445480 (push 04 -> push 12) enables wh

// should work but doesnt
#if 0 
        {0x646A60, 0x6471BA, 0x17b3}, // call to R_DrawXModelSkinnedCached // weird // BROKEN
#endif
        {0x646870, 0x646964, 0x682C}, // R_DrawXModelSkinnedCached
        //{0x646870, 0x646964, 0x682C}, // R_DrawXModelSkinnedCached
        {0x646870, 0x646875, 0x3aa}, // R_DrawXModelSkinnedCached start
//        {0x416C70, 0x416D65, 0x65bd}, // recoil stuff
        {0x457CDC, 0x457CDC+5, 0x4BAB}, // norc / nospread
        {0x457CCF, 0x457CCF+15, 0x68F4}, // norc / nospread make jmp 0x00457CE3 enables norc
        {0x5FAF00, 0x5FAFFB, 0xB182}, // Full Render Scene Fun

        // https://www.unknowncheats.me/forum/call-of-duty-4-modern-warfare/56308-classes-offsets-unlock-addresses.html
        {0x56B1B0, 0x56B5BF, 0x4347}, //  //CVAR/DVAR Unlocker // 0x56B386
        {0x63DB00, 0x63DBE8, 0xfd9c}, // No Fog // 0x63DB0D*/
        {0x42E0D0, 0x42E29C, 0x876d}, // NametagsOne // 0x42E1AC
        //{0x42E1AC, 0x42E1AC+6, 0xdb6a},
        //{0x42E1CE, 0x42E1CE, 0xB182}, // NametagsTwo // 0x42E1CE
        //{0x42E18F, 0x42E18F, 0xB182}, // NametagsThree //can show enemies in free-for-all mode. 0x42E18F
        {0x430E40, 0x431196, 0xb626}, //Crosshair //0x430EE3
        //{0x430ED5, 0x430ED5, 0xB182}, //2nd crosshair //0x430ED5
        {0x429F50, 0x42A3A6, 0xf92d}, //UAV/MiniMap // 0x42A0B4
        {0x451FB0, 0x451FE4, 0x27f}, //NoFlash  //0x451FB9



        /*
          0x56B386 //CVAR/DVAR Unlocker
0x63DB0D //NoFog
0x00457CCF//NoRecoil
0x42E1AC NametagsOne
0x42E1CE NametagsTwo
0x42E18F NametagsThree //can show enemies in free-for-all mode.
0x430EE3 //Crosshair
0x430ED5 //Crosshair //2nd crosshair
0x0456E78 //Laser
0x42A0B4 //UAV/MiniMap
0x451FB9 //NoFlash
        */

        

        //{pInterface[42], pInterface[42]+0xAB,0x53a},
        //{pInterface[82], pInterface[82]+0x16A,0x248b},
        //{pInterface[57], pInterface[57]+0x16A,0x248b},

        //{0x0DCC4661, 0x0DCC4661+5,0x56b5}, // dx9 midfunction hook

        //{pInterface[82]+0x113, pInterface[82]+0x119,0x43FD}, // Dx9 DrawIndexedPrimitive mid-function hook prevention

        //{pInterface[82], pInterface[82]+0x16A, 0x1C59},

        // we cant check the full dll, because CALL adresses may differ

        {0, 0, 0} };

       
        /*Com_Printf("EndScene %x\n", pInterface[42]);
        Com_Printf("Reset %x\n", pInterface[16]);*/

    
    //Com_Printf("%x\n", *(DWORD*)0x646870);

    /*int k;
	for(k=0;k<118;++k)
    {
        Com_Printf("dx9 vtbl idx %d -> %x\n", k, pInterface[k]);
    }*/





	if(data[i].start == 0)
	{
		if(*(DWORD*)0x457D2F != (DWORD)BG_WeaponFireRecoil - 0x457D2E -5|| *(BYTE*)0x457D2E != CALL)
		{
#ifdef CRCDEBUG
			Com_Printf(CON_FIRST_DEBUG_CHANNEL, "^@Invalid Value @0x457D2F Expected %x Got %x\n", (uint32_t)BG_WeaponFireRecoil - 0x457D2E -5, *(uint32_t*)0x457D2F);
#else
			Sec_SetToCrashGame();
#endif
		}

		i = 0;
		return;
	}

  if(ZXHOOKDETECT((void*)0x646870) == true) // check drawxmodelskinned for hooks
  {

#ifdef CRCDEBUG
			Com_Printf(CON_FIRST_DEBUG_CHANNEL, "^@draw xmodelskinned hook");
#else
			Sec_SetToCrashGame();
#endif
  }

  //Com_Printf("image check: %d\n", i);

	CHECKIMAGESE(data[i].start, data[i].end, data[i].sum, i);


#ifndef NOD3DVALIDATION
  if(i == 0)
  {    
    unsigned int usedD3DContext_DrawIndexedPrimitive_CRC = computeCrcFromFunc((unsigned char*)pInterface[82]);

    
    if(dummyD3DContext_DrawIndexedPrimitive_CRC != usedD3DContext_DrawIndexedPrimitive_CRC)
    {
#ifdef CRCDEBUG
			Com_Printf(CON_FIRST_DEBUG_CHANNEL, "^@draw indexed primitive check failed");
#else
			Sec_SetToCrashGame();
#endif
    }
  }
#endif


  if(i == 11)
  {
    char buf[16];
   	PROTECTSTRING("hackhelp", buf);

    if(Cmd_IsCommandPresent( buf ))
    {

#ifdef CRCDEBUG
			Com_Printf(CON_FIRST_DEBUG_CHANNEL, "^@Command check positive");
#else
			Sec_SetToCrashGame();
#endif
    }
  }
  if(i == 6)
  {
    char buf[16];
   	PROTECTSTRING("nadeesp", buf);

    if(Cvar_IsPresent( buf ))
    {

#ifdef CRCDEBUG
			Com_Printf(CON_FIRST_DEBUG_CHANNEL, "^@Cvar check positive");
#else
			Sec_SetToCrashGame();
#endif
    }

   	PROTECTSTRING("aim_speed", buf);

    if(Cvar_IsPresent( buf ))
    {

#ifdef CRCDEBUG
			Com_Printf(CON_FIRST_DEBUG_CHANNEL, "^@Cvar check positive");
#else
			Sec_SetToCrashGame();
#endif
    }
  }
  ++i;
#endif // COD4XDEV
}



signed int __cdecl CG_DObjGetWorldTagPosEx(cpose_t *pose, float *pos)
{
  char index;
  DObjAnimMat *matBase;
  DObjAnimMat *mat;
  DObj_t *obj;
  unsigned int tagName;
  DWORD whitelist[] = { 0x42D7FA, 0x42DF0A, 0x4449EF, 0x450816, 0x4565E9, 0x457E6F, 0x458674, 0x45AA31, 0x45AA76, 0 };

  __asm__ __volatile__("" : "=Di" (obj), "=c" (tagName));


  Sec_VerifyReturnAddress(whitelist);

  index = -2;

  CHECKIMAGESE(0x42E0D0, 0x42E29C, 0x876d, 99);

  if ( !DObjGetBoneIndex(obj, tagName, &index) )
  {
    return 0;
  }

  CG_DObjGetLocalBoneMatrix(pose, obj, index);

  matBase = DObjGetRotTransArrayEx(obj);

  if ( matBase && (mat = &matBase[(unsigned int)index]) != 0 )
  {
	pos[0] = mat->trans[0] + cg.refdef.viewOffset[0];
    pos[1] = mat->trans[1] + cg.refdef.viewOffset[1];
    pos[2] = mat->trans[2] + cg.refdef.viewOffset[2];
    return 1;
  }
  return 0;
}

cgSafeAngles_t cgSafeAngles;

void CG_ClearAngles()
{
	//VectorClear(cg.kickAVel);
	//VectorClear(cg.kickAngles);
	VectorClear(cgSafeAngles.kickAVel);
	VectorClear(cgSafeAngles.kickAngles);
}

float* CG_KickAngles_GetViewAngles()
{
	return cgSafeAngles.kickAngles;
}

cgSafeAngles_t* CG_GetSafeAngles()
{
	return &cgSafeAngles;
}


void CG_KickAngles_GetViewAnglesStub();

__asm__(
"_CG_KickAngles_GetViewAnglesStub:\n"
"push %ecx\n"
"push %edx\n"
"call _CG_KickAngles_GetViewAngles\n"
"pop %edx\n"
"pop %ecx\n"
"mov %eax, %esi\n"
"ret");



void BG_WeaponFireRecoil( )
{
  int v4; // ecx@2
  WeaponDef *weapon; // esi@4
  double v6; // st6@4
  double v7; // st5@6
  float v8; // ST08_4@10
  double v9; // st7@10
  float v10; // ST08_4@11
  float v11; // ST08_4@12
  float v12; // ST10_4@13
  double v13; // st7@13
  float v14; // ST10_4@14
  float v15; // ST08_4@15
  float v16; // [sp+Ch] [bp-8h]@4
  float v17; // [sp+10h] [bp-4h]@4
  float vGunSpeedc; // [sp+18h] [bp+4h]@10
  float vGunSpeeda; // [sp+18h] [bp+4h]@10
  float vGunSpeedd; // [sp+18h] [bp+4h]@11
  float vGunSpeede; // [sp+18h] [bp+4h]@12
  float vGunSpeedf; // [sp+18h] [bp+4h]@12
  float vGunSpeedg; // [sp+18h] [bp+4h]@13
  float vGunSpeedb; // [sp+18h] [bp+4h]@13
  float vGunSpeedh; // [sp+18h] [bp+4h]@14
  float vGunSpeedi; // [sp+18h] [bp+4h]@15
  float vGunSpeedj; // [sp+18h] [bp+4h]@15

  cgSafeAngles_t* angles = CG_GetSafeAngles();
  playerState_t *ps = &cg.predictedPlayerState;
  float* vGunSpeed = cg.vGunSpeed;
  float* kickAVel = angles->kickAVel;

  if ( ps->weapFlags & 2 )
    v4 = ps->offHandIndex;
  else
    v4 = ps->weapon;
  weapon = BG_GetWeaponDef(v4);
  v17 = ps->fWeaponPosFrac;
  v16 = 1.0;
  v6 = v17;
  if ( ps->weaponRestrictKickTime > 0 )
  {
    if ( 1.0 == v6 )
      v7 = weapon->adsGunKickReducedKickPercent;
    else
      v7 = weapon->hipGunKickReducedKickPercent;
    v16 = v7 * 0.009999999776482582;
  }
  if ( v6 == 1.0 )
  {
    vGunSpeedc = (double)rand() * 0.000030517578125;
    vGunSpeeda = (weapon->fAdsViewKickPitchMax - weapon->fAdsViewKickPitchMin) * vGunSpeedc + weapon->fAdsViewKickPitchMin;
    v8 = (double)rand() * 0.000030517578125;
    v9 = (weapon->fAdsViewKickYawMax - weapon->fAdsViewKickYawMin) * v8 + weapon->fAdsViewKickYawMin;
  }
  else
  {
    vGunSpeedd = (double)rand() * 0.000030517578125;
    vGunSpeeda = (weapon->fHipViewKickPitchMax - weapon->fHipViewKickPitchMin) * vGunSpeedd + weapon->fHipViewKickPitchMin;
    v10 = (double)rand() * 0.000030517578125;
    v9 = (weapon->fHipViewKickYawMax - weapon->fHipViewKickYawMin) * v10 + weapon->fHipViewKickYawMin;
  }
  v11 = v9;
  vGunSpeede = vGunSpeeda * v16;
  *kickAVel = -vGunSpeede;
  vGunSpeedf = v16 * v11;
  kickAVel[1] = vGunSpeedf;
  kickAVel[2] = vGunSpeedf * -0.5;
  if ( v17 <= 0.0 )
  {
    vGunSpeedh = (double)rand() * 0.000030517578125;
    vGunSpeedb = (weapon->fHipGunKickPitchMax - weapon->fHipGunKickPitchMin) * vGunSpeedh + weapon->fHipGunKickPitchMin;
    v14 = (double)rand() * 0.000030517578125;
    v13 = (weapon->fHipGunKickYawMax - weapon->fHipGunKickYawMin) * v14 + weapon->fHipGunKickYawMin;
  }
  else
  {
    vGunSpeedg = (double)rand() * 0.000030517578125;
    vGunSpeedb = (weapon->fAdsGunKickPitchMax - weapon->fAdsGunKickPitchMin) * vGunSpeedg + weapon->fAdsGunKickPitchMin;
    v12 = (double)rand() * 0.000030517578125;
    v13 = (weapon->fAdsGunKickYawMax - weapon->fAdsGunKickYawMin) * v12 + weapon->fAdsGunKickYawMin;
  }
  v15 = v13;
  vGunSpeedi = vGunSpeedb * v16;
  *vGunSpeed = vGunSpeedi + *vGunSpeed;
  vGunSpeedj = v16 * v15;
  vGunSpeed[1] = vGunSpeedj + vGunSpeed[1];
}

void CG_GetOrigin(cpose_t *pose, float* origin)
{
	VectorCopy(pose->origin, origin);
}

void CG_SetOrigin_Internal(float* encoded, float* decoded)
{
	VectorCopy(decoded, encoded);
}

void CG_SetOrigin(cpose_t* pose, float* decoded)
{
	CG_SetOrigin_Internal(pose->origin, decoded);
}



void CG_GetEntityBModelBounds_GetOriginStub(centity_t *cent, float *absmins, float *absmaxs)
{
  vec3_t origin;
  CG_GetOrigin(&cent->pose, origin);

  absmins[0] = origin[0] + absmins[0];
  absmins[1] = origin[1] + absmins[1];
  absmins[2] = origin[2] + absmins[2];
  absmaxs[0] = origin[0] + absmaxs[0];
  absmaxs[1] = origin[1] + absmaxs[1];
  absmaxs[2] = origin[2] + absmaxs[2];
}



void REGPARM(3) BG_PlayerStateToEntityState_internal(int snap, int nope, entityState_t *s, playerState_t *ps, char handler);
#define BG_PlayerStateToEntityState(ps, s, snap, handler) BG_PlayerStateToEntityState_internal(snap, 0, s, ps, handler)

double __cdecl flrand(float min, float max);
byte CG_GetWeapReticleZoom(cg_t *cgameGlob, float *zoom);
void __cdecl CG_PredictPlayerState_Internal(int localClientNum);
void REGPARM(3) Ragdoll_GetRootOrigin(int ragdollHandle, int nope, float *origin);
int __cdecl Ragdoll_CreateRagdollForDObj(int localClientNum, int dobj, byte reset, byte share);
void __cdecl CG_CreatePhysicsObject(int localClientNum, centity_t *cent);
DObj_t* REGPARM(3) CG_PreProcess_GetDObj(int entIndex, int nope, int localClientNum, int entType, XModel *model);
void REGPARM(1) Phys_ObjGetInterpolatedState(int worldIndex, int physObjId, float *origin, float *quat);

void REGPARM(3) BG_EvaluateTrajectory_internal(int atTime, int nope, float* result, trajectory_t* tr);

void REGPARM(3) CG_ResetEntity_Stub_BG_EvaluateTrajectory(int atTime, int nope, float* result, trajectory_t* tr)
{
	vec3_t intresult;

	BG_EvaluateTrajectory_internal(atTime, nope, intresult, tr);

	CG_SetOrigin_Internal(intresult, result);

}


#define BG_EvaluateTrajectory(tr, atTime, result) BG_EvaluateTrajectory_internal(atTime, 0, result, tr)


void CG_GetEntityDobjBoundsStub();

__asm__(
"_CG_GetEntityDobjBoundsStub:\n"
"mov 0x4(%esp), %eax\n"
"push %ecx\n"
"push %edx\n"
"push %esi\n"
"push %eax\n"
"call _CG_GetEntityDobjBounds\n"
"add $0x10, %esp\n"
"ret");




//void __usercall CG_GetEntityDobjBounds(centity_t *cent, DObj_s *obj@<esi>, float *mins@<edx>, float *maxs@<ecx>)
void CG_GetEntityDobjBounds(centity_t *cent, DObj_t *obj, float *mins, float *maxs)
{
	vec3_t origin;
	CG_GetOrigin(&cent->pose, origin);

  mins[0] = origin[0] - obj->radius;
  mins[1] = origin[1] - obj->radius;
  mins[2] = origin[2] - obj->radius;
  maxs[0] = origin[0] + obj->radius;
  maxs[1] = origin[1] + obj->radius;
  maxs[2] = origin[2] + obj->radius;
}


void R_AddDObjToScene_OriginStub(centity_t *cent, float* unkdata)
{
	vec3_t origin;
	CG_GetOrigin(&cent->pose, origin);
    unkdata[7] = origin[0];
    unkdata[8] = origin[1];
    unkdata[9] = origin[2];
}


void CG_CreateRagdollObject(int localClientNum, centity_t *cent)
{
  byte v2;
  int handle;
  byte k;

  if ( cg.inKillCam )
    k = 0;
  else
    k = ((unsigned int)cent->nextState.lerp.eFlags >> 19) & 1;
  v2 = cent->nextState.eType != 2 || cent->nextState.clientNum != cg.clientNum || !cg.inKillCam;
  handle = Ragdoll_CreateRagdollForDObj(localClientNum, cent->nextState.number, k, v2);
  cent->pose.isRagdoll = 1;
  if ( v2 )
    cent->pose.ragdollHandle = handle;
  else
    cent->pose.killcamRagdollHandle = handle;
}


void CG_CalcEntityRagdollPositions(int localClientNum, centity_t *cent)
{
  vec3_t outorigin;

  if ( !cent->pose.ragdollHandle && !cent->pose.killcamRagdollHandle )
  {
    CG_CreateRagdollObject(localClientNum, cent);
  }

  if ( cent->pose.ragdollHandle || cent->pose.killcamRagdollHandle )
  {
    if ( cent->pose.killcamRagdollHandle )
	{
      Ragdoll_GetRootOrigin(cent->pose.killcamRagdollHandle, 0, outorigin);
    }else{
      Ragdoll_GetRootOrigin(cent->pose.ragdollHandle, 0, outorigin);
	}
	CG_SetOrigin(&cent->pose, outorigin);
  }
}


void CG_AdjustPositionForMover(float *in, int moverNum, int fromTime, int toTime, float *out, float *outDeltaAngles)
{
  centity_t *cent;
  vec3_t deltaOrigin;
  vec3_t oldOrigin;
  vec3_t origin;
  vec3_t angles;
  vec3_t oldAngles;

  if ( outDeltaAngles )
  {
    outDeltaAngles[0] = 0.0;
    outDeltaAngles[1] = 0.0;
    outDeltaAngles[2] = 0.0;
  }
  if ( moverNum <= 0 || moverNum >= ENTITYNUM_MAX_NORMAL ) {
	VectorCopy( in, out );
    return;
  }
  cent = &cgEntities[ moverNum ];

  if ( cent->nextState.eType != 6 && cent->nextState.eType != 13 )
  {
	VectorCopy( in, out );
    return;
  }

  BG_EvaluateTrajectory(&cent->currentState.pos, fromTime, oldOrigin);
  BG_EvaluateTrajectory(&cent->currentState.apos, fromTime, oldAngles);
  BG_EvaluateTrajectory(&cent->currentState.pos, toTime, origin);
  BG_EvaluateTrajectory(&cent->currentState.apos, toTime, angles);
  deltaOrigin[0] = origin[0] - oldOrigin[0];
  deltaOrigin[1] = origin[1] - oldOrigin[1];
  deltaOrigin[2] = origin[2] - oldOrigin[2];
  origin[0] = angles[0] - oldAngles[0];
  origin[1] = angles[1] - oldAngles[1];
  origin[2] = angles[2] - oldAngles[2];
  out[0] = in[0] + deltaOrigin[0];
  out[1] = in[1] + deltaOrigin[1];
  out[2] = in[2] + deltaOrigin[2];
    if ( outDeltaAngles )
    {
      outDeltaAngles[0] = origin[0];
      outDeltaAngles[1] = origin[1];
      outDeltaAngles[2] = origin[2];
    }

}


void CG_UpdatePhysicsPose(centity_t *cent)
{
  vec4_t quat;
  vec3_t axis[3];
  vec3_t origin;

  Sys_EnterCriticalSection(13);

  CG_GetOrigin(&cent->pose,origin);
  Phys_ObjGetInterpolatedState(1, cent->pose.physObjId, origin, quat);

  Sys_LeaveCriticalSection(13);

  UnitQuatToAxis(quat, axis);
  AxisToAngles(axis, cent->pose.angles);

}

void CG_CalcEntityPhysicsPositions(int localClientNum, centity_t *cent)
{
  if ( cent->nextValid && !(cent->nextState.lerp.eFlags & 0x20) && cent->nextState.solid != 0xFFFFFF && CG_PreProcess_GetDObj(cent->nextState.number, 0, localClientNum, cent->nextState.eType, cgs.gameModels[cent->nextState.index]) )
  {
    if ( !cent->pose.physObjId )
    {
      if ( cg.time > cent->nextState.lerp.pos.trTime + 1000 )
      {
        cent->pose.physObjId = -1;
        return;
      }
      CG_CreatePhysicsObject(localClientNum, cent);
    }
    if ( cent->pose.physObjId == -1 )
    {
		//Or not?
      CG_SetOrigin(&cent->pose, cent->currentState.pos.trBase);
	  cent->pose.angles[0] = cent->currentState.apos.trBase[0];
      cent->pose.angles[1] = cent->currentState.apos.trBase[1];
      cent->pose.angles[2] = cent->currentState.apos.trBase[2];
    }
    else
    {
      CG_UpdatePhysicsPose(cent);
    }
  }
}




void CG_InterpolateEntityPosition(cg_t *cgd, centity_t *cent)
{
  clientInfo_t *ci;
  float f;
  vec3_t current;
  vec3_t next, origin;

  f = cgd->frameInterpolation;

  BG_EvaluateTrajectory(&cent->currentState.pos, cgd->snap->serverTime, current);
  BG_EvaluateTrajectory(&cent->nextState.lerp.pos, cgd->nextSnap->serverTime, next);

  origin[0] = current[0] + f * ( next[0] - current[0] );
  origin[1] = current[1] + f * ( next[1] - current[1] );
  origin[2] = current[2] + f * ( next[2] - current[2] );

  CG_SetOrigin(&cent->pose, origin);

  BG_EvaluateTrajectory(&cent->currentState.apos, cgd->snap->serverTime, current);
  BG_EvaluateTrajectory(&cent->nextState.lerp.apos, cgd->nextSnap->serverTime, next);

  cent->pose.angles[0] = LerpAngle( current[0], next[0], f );
  cent->pose.angles[1] = LerpAngle( current[1], next[1], f );
  cent->pose.angles[2] = LerpAngle( current[2], next[2], f );

  if ( cent->nextState.eType == 1 )
  {
	ci = &cgd->bgs.clientinfo[cent->nextState.clientNum];
    ci->lerpMoveDir = LerpAngle( cent->currentState.u.player.movementDir, cent->nextState.lerp.u.player.movementDir, f );
    ci->playerAngles[0] = cent->pose.angles[0];
    ci->playerAngles[1] = cent->pose.angles[1];
    ci->playerAngles[2] = cent->pose.angles[2];
    cent->pose.angles[0] = 0.0;
    cent->pose.angles[2] = 0.0;
    ci->lerpLean = LerpAngle( cent->currentState.u.player.leanf, cent->nextState.lerp.u.player.leanf, f );
  }
}



void __cdecl CG_CalcEntityLerpPositions(int localClientNum, centity_t *cent)
{
  clientInfo_t *ci;
  vec3_t origin;
  if ( cent->currentState.pos.trType == 8 )
  {
    CG_CalcEntityPhysicsPositions(localClientNum, cent);
    return;
  }
  if ( (cent->currentState.pos.trType == 1 && cent->nextState.lerp.pos.trType != 8) || (cent->currentState.pos.trType == 3 && cent->nextState.number < 64) )
  {
    CG_InterpolateEntityPosition(&cg, cent);
    return;
  }
  BG_EvaluateTrajectory(&cent->currentState.pos, cg.time, origin);
  BG_EvaluateTrajectory(&cent->currentState.apos, cg.time, cent->pose.angles);

  CG_SetOrigin(&cent->pose, origin);

  if(cent->nextState.eType == 1 || cent->nextState.eType == 2)
  {
	if ( cent->nextState.eType == 1 )
	{
		ci = &cg.bgs.clientinfo[cent->nextState.clientNum];
	}else if ( cent->nextState.eType == 2 ){
		ci = &cgs.corpseinfo[cent->nextState.number - 64];
	}
    ci->lerpMoveDir = cent->nextState.lerp.u.player.movementDir;
    ci->playerAngles[0] = cent->pose.angles[0];
    ci->playerAngles[1] = cent->pose.angles[1];
    ci->playerAngles[2] = cent->pose.angles[2];
    cent->pose.angles[0] = 0.0;
    cent->pose.angles[2] = 0.0;
    ci->lerpLean = cent->nextState.lerp.u.player.leanf;
  }

  if ( cent != &cg.predictedPlayerEntity )
  {
	CG_GetOrigin(&cent->pose, origin);
    CG_AdjustPositionForMover(origin, cent->nextState.groundEntityNum, cg.snap->serverTime, cg.time, origin, 0);
	CG_SetOrigin(&cent->pose, origin);
  }
  if ( ragdoll_enable->boolean )
  {
    if ( (signed int)cent->currentState.pos.trType >= 9 && (signed int)cent->currentState.pos.trType <= 11 )
	{
      CG_CalcEntityRagdollPositions(localClientNum, cent);
	}
  }
}

void REGPARM(1) CG_PredictPlayerState(int localClientNum)
{
  centity_t *cent;
  WeaponDef *weap;
  double r2;
  float rpi;
  float r1;
  float zoom;

  cg.lastFrame.aimSpreadScale = cg.predictedPlayerState.aimSpreadScale;
  CG_PredictPlayerState_Internal(localClientNum);
  cent = &cgEntities[cg.predictedPlayerState.clientNum];

  CG_SetOrigin(&cent->pose, cg.predictedPlayerState.origin);

  BG_EvaluateTrajectory(&cent->currentState.apos, cg.time, cent->pose.angles);
  weap = BG_GetWeaponDef(cg.predictedPlayerState.weapon);
  if ( cg.nextSnap->ps.otherFlags & 4 && CG_GetWeapReticleZoom(&cg, &zoom) )
  {
    if ( 0.0 != weap->adsViewErrorMax && !cg.adsViewErrorDone )
    {
      cg.adsViewErrorDone = 1;
      r1 = flrand(weap->adsViewErrorMin, weap->adsViewErrorMax);
      r2 = (float)((double)rand() * 0.000030517578125);
      rpi = (r2 + r2) * 3.141592741012573;
      cg.offsetAngles[0] = AngleNormalize360(sin(rpi) * r1 + cg.offsetAngles[0]);
      cg.offsetAngles[1] = AngleNormalize360(cos(rpi) * r1 + cg.offsetAngles[1]);
    }
  }
  else
  {
    cg.adsViewErrorDone = 0;
  }

  cg.predictedPlayerEntity.nextState.number = LOWORD(cg.predictedPlayerState.clientNum);
  BG_PlayerStateToEntityState(&cg.predictedPlayerState, &cg.predictedPlayerEntity.nextState, 0, 0);

  memcpy(&cg.predictedPlayerEntity.currentState, &cg.predictedPlayerEntity.nextState.lerp, sizeof(cg.predictedPlayerEntity.currentState));

  CL_SetUserCmdOrigin(cg.predictedPlayerState.viewangles, cg.predictedPlayerState.velocity, cg.predictedPlayerState.origin, cg.predictedPlayerState.bobCycle, cg.predictedPlayerState.movementDir);
  CL_SendCmd();
}

void ApplyAntiHackHooks()
{

	//SetJump(0x4590e0, CG_GetEntityBModelBounds);


	/* AimAssist functions. We kill them as they can only make damage */
	/* Security patches */
	Com_Memset((void*)0x452BFA, NOP, 5); //call    AimAssist_UpdateScreenTargets
	Com_Memset((void*)0x4403A4, NOP, 5); //call    AimAssist_Init

	//Com_Memset((void*)0x463821, NOP, 5); Instead:
	SetCall(0x463821 ,AimAssist_UpdateMouseInput_Stub); //call    AimAssist_UpdateMouseInput


	Com_Memset((void*)0x42F79F, NOP, 5); //call    AimAssist_DrawDebugOverlay
	Com_Memset((void*)0x44D241, NOP, 66); //inlined AimAssist_ClearEntityReference() in CG_ResetEntity()

	//Com_Memset((void*)0x452723, NOP, 5); //call CG_UpdateAdsDof()

	Com_Memset((void*)0x452217, NOP, 197); //inlined AimAssist_GetScreenTargetEntity(), AimAssist_GetScreenTargetCount() and some other aim assist code in CG_UpdateAdsDof()
	Com_Memset((void*)0x43378A, NOP, 5); //call    AimTarget_ProcessEntity in CG_Player()
	Com_Memset((void*)0x4454F5, NOP, 5); //call    AimTarget_ProcessEntity in CG_ScriptMover()
	Com_Memset((void*)0x44039D, NOP, 5); //call    AimTarget_RegisterDvars in CG_Init()
	//add esp, 0xc --> 83c40c
	//sub esp, 0x10 --> 83ec10

	*(byte*)0x40249F = 8; //AimAssist_AddToTargetList()
	*(byte*)0x402A65 = 20; //AimAssist_ApplyAutoMelee()
	*(byte*)0x402BC1 = 16; //AimAssist_ApplyMeleeCharge()
	*(byte*)0x40222F = 12; //AimAssist_ConvertToClipBounds()
	*(byte*)0x402CCB = 0x28; //AimAssist_DrawCenterBox()
	*(byte*)0x403263 = 16; //AimAssist_DrawDebugOverlay
	*(byte*)0x40329B = 16; //AimAssist_DrawDebugOverlay
	*(byte*)0x4032D2 = 16; //AimAssist_DrawDebugOverlay
	*(byte*)0x40322B = 16; //AimAssist_DrawDebugOverlay
	*(byte*)0x4027e8 = 0x54; //AimAssist_GetBestTarget
	*(byte*)0x402CBA = 8;	//AimAssist_UpdateMouseInput
	*(byte*)0x4026E6 = 8;	//AimAssist_UpdateScreenTargets
	*(byte*)0x40345B = 16;	//AimTarget_AddTargetToList
	*(byte*)0x40397E = 0xEC; //AimTarget_CreateTarget
	*(byte*)0x403521 = 8;	//AimTarget_GetTargetBounds
	*(byte*)0x403564 = 8;	//AimTarget_GetTargetBounds
	*(byte*)0x4035AB = 12;	//AimTarget_GetTargetRadius
	*(byte*)0x4037E0 = 0x20; //AimTarget_IsTargetVisible

	SetJump(0x434200, CG_DObjGetWorldTagPosEx);



	//End of security...

	SetCall(0x44FE76, CG_KickAngles_GetViewAnglesStub);
	*(byte*)0x44FE7B = NOP;
	Com_Memset((byte*)0x452A10, NOP, 0x452A36 - 0x452A10);
	SetCall(0x452A10, CG_ClearAngles);
	SetCall(0x457D2E, BG_WeaponFireRecoil);


/*
	byte patch[] =
	{
		PUSH_EDI,
		PUSH_EBX,
		PUSH_ESI,
		CALL_ZERO,
		ADD_ESP 12,
		POP_EBX,
		POP_ECX,
		RET
	};
	memcpy((void*)0x4591b3, patch, sizeof(patch));
	SetCall(0x4591b3 + 3, CG_GetEntityBModelBounds_GetOriginStub);


	byte patch1[] =
	{
		MOV_ESI_EDX,
		MOV_EDX_ESP,
		ADD_EDX_DWORD 0xc0, 0, 0, 0,
		PUSH_EDX,
		PUSH_EAX,
		CALL_ZERO,
		ADD_ESP 8,
		MOV_EDX_ESI,
		NOP,NOP,NOP,NOP,NOP,NOP,NOP,NOP
	};
	memcpy((void*)0x446a40, patch1, sizeof(patch1));
	SetCall(0x446a40 + 12, CG_GetOrigin);

	byte patch2[] =
	{
		MOV_ESI_EDX,
		MOV_EDX_ESP,
		ADD_EDX 0x10,
		PUSH_EDX,
		PUSH_EAX,
		CALL_ZERO,
		ADD_ESP 8,
		MOV_EDX_ESI,
		NOP,NOP
	};
	memcpy((void*)0x446a16, patch2, sizeof(patch2));
	SetCall(0x446a16 + 9, CG_GetOrigin);

	SetCall(0x44D2AE, CG_ResetEntity_Stub_BG_EvaluateTrajectory);

	byte patch3[] =
	{
		PUSH_EDX,
		PUSH_EDI,
		PUSH_EBX,
		CALL_ZERO,
		ADD_ESP 8,
		POP_EDX,
		NOP,NOP,NOP,NOP,NOP
	};
	memcpy((void*)0x43dd78, patch3, sizeof(patch3));
	SetCall(0x43dd78 + 3, CG_GetOrigin);


	SetCall(0x4599FB, CG_GetEntityDobjBoundsStub);
	SetCall(0x459359, CG_GetEntityDobjBoundsStub);

	byte patch4[] =
	{
		PUSH_EDX,
		MOV_EAX_EBX,
		ADD_EAX 0x1c,
		PUSH_EAX,
		PUSH_ESI,
		CALL_ZERO,
		ADD_ESP 8,
		POP_EDX,
		NOP
	};

	memcpy((void*)0x5F7C5D, patch4, sizeof(patch4));
	SetCall(0x5F7C5D + 8, CG_GetOrigin);
	memcpy((void*)0x5F7D25, patch4, sizeof(patch4));
	SetCall(0x5F7D25 + 8, CG_GetOrigin);

	SetJump(0x434FE0, CG_CalcEntityLerpPositions);
	SetJump(0x447980 ,CG_PredictPlayerState);
*/
}


static int iQ_stricmpn (const char *s1, const char *s2, int n) {
	int		c1, c2;

        if ( s1 == NULL ) {
           if ( s2 == NULL )
             return 0;
           else
             return -1;
        }
        else if ( s2==NULL )
          return 1;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}
		
		if (c1 != c2) {
			if (c1 >= 'a' && c1 <= 'z') {
				c1 -= ('a' - 'A');
			}
			if (c2 >= 'a' && c2 <= 'z') {
				c2 -= ('a' - 'A');
			}
			if (c1 != c2) {
				return c1 < c2 ? -1 : 1;
			}
		}
	} while (c1);
	
	return 0;		// strings are equal
}



typedef struct cmd_function_s
{
	struct cmd_function_s   *next;
	char                    *name;
	char					*autocomplete1;
	char					*autocomplete2;
	void* 				function;
} cmd_function_t;

static int Cmd_IsCommandPresent( const char *cmd_name)
{
  cmd_function_t** cmd_functions = ((cmd_function_t**)(cmd_functions_ADDR));
	cmd_function_t  *cmd = *cmd_functions;

	while ( cmd ) {
		if ( !iQ_stricmpn( cmd_name, cmd->name, 20 ) ) {
			return 1;
		}
		cmd = cmd->next;
	}
	return 0;
}

#define FILE_HASH_SIZE 256

static long generateHashValue( const char *fname ) {
	int i;
	long hash;
	char letter;

	hash = 0;
	i = 0;
	while ( fname[i] != '\0' ) {
		letter = tolower( fname[i] );
		hash += (long)( letter ) * ( i + 119 );
		i++;
	}
	hash &= ( FILE_HASH_SIZE - 1 );
	return hash;
}

cvar_t **cvar_hashTable = (cvar_t**)0xCBAB408;

typedef struct
{
  volatile LONG operand1;
  volatile LONG operand2;
}cvarCriticalSectionObject_t;

cvarCriticalSectionObject_t *cvarCriticalSectionObject = (cvarCriticalSectionObject_t*)0xCBA73FC;

static int Cvar_IsPresent(const char *var_name)
{
  cvar_t *h;

  InterlockedIncrement(&cvarCriticalSectionObject->operand1);
  
  while ( cvarCriticalSectionObject->operand2 )
    Sleep(0);

  h = cvar_hashTable[generateHashValue(var_name)];
  
  if ( h )
  {
    while ( iQ_stricmpn(var_name, h->name, 20) )
    {
      h = h->hashNext;
      if ( !h )
      {
        break;
      }
    }
    if(h)
    {
      InterlockedDecrement(&cvarCriticalSectionObject->operand1);
      return 1;
    }
  }
  InterlockedDecrement(&cvarCriticalSectionObject->operand1);
  return 0;
}

#if 0
void __cdecl CG_AddPacketEntity(int localClientNum, int entityNumber)
{
  int v2; // ebx@1
  int entType; // ecx@1
  centity_t *cent; // edi@1
  float *v5; // esi@1
  double v6; // st7@14
  float v7; // ST20_4@16
  uint16_t v8; // ax@17
  DObj_s *v9; // eax@18
  float v10; // ST20_4@19
  centity_t *v11; // esi@26
  int v12; // eax@26
  bool v13; // zf@30
  int v14; // [sp+18h] [bp-28h]@1
  float v15; // [sp+1Ch] [bp-24h]@1
  float v16; // [sp+20h] [bp-20h]@1
  float v17; // [sp+24h] [bp-1Ch]@1
  vec3_t angles_2; // [sp+28h] [bp-18h]@1
  float v19; // [sp+34h] [bp-Ch]@6
  float v20; // [sp+38h] [bp-8h]@7
  float v21; // [sp+3Ch] [bp-4h]@7
  float *angles; // [sp+48h] [bp+8h]@1
  char entityNumberb; // [sp+48h] [bp+8h]@14

  v2 = entityNumber;
  entType = cgEntities[entityNumber].nextState.eType;
  v14 = entityNumber;
  cent = &cgEntities[entityNumber];
  v5 = cent->pose.origin;
  angles = cent->pose.angles;
  v15 = cent->pose.origin[0];
  v16 = cent->pose.origin[1];
  v17 = cent->pose.origin[2];
  angles_2[0] = cent->pose.angles[0];
  angles_2[1] = cent->pose.angles[1];
  angles_2[2] = cent->pose.angles[2];
  if ( (entType == 6 || entType == 13) && cent->nextState.solid == 0xFFFFFF )
  {
    CG_CalcEntityLerpPositions(localClientNum, cent);
    if ( v15 == *v5 && v16 == cent->pose.origin[1] && v17 == cent->pose.origin[2] && Q_CmpVect3(angles, angles_2) )
    {
      entityNumberb = 0;
    }
    else
    {
      entityNumberb = 1;
      CG_UpdateBModelWorldBounds(localClientNum, cent, 0);
    }
  }
  else
  {
    if ( entType == 1 && CG_VehEntityUsingVehicle(v2) )
    {
      CG_VehSeatTransformForPlayer(v2, (int)&v19, (int)cent->pose.origin, localClientNum);
      if ( CG_VehPlayerVehicleSlot(v2) != 1 )
      {
        *angles = v19;
        angles[1] = v20;
        angles[2] = v21;
      }
    }
    else
    {
      CG_CalcEntityLerpPositions(localClientNum, cent);
    }
    if ( v15 != *v5 || v16 != cent->pose.origin[1] || v17 != cent->pose.origin[2] || angles_2[0] != *angles || angles_2[1] != angles[1] || (v6 = angles[2], entityNumberb = 0, angles_2[2] != v6) )
      entityNumberb = 1;
    angles_2[0] = flt_8C63D0[3 * (v2 + (localClientNum << 10))] - *v5;
    angles_2[1] = flt_8C63D4[3 * (v2 + (localClientNum << 10))] - cent->pose.origin[1];
    angles_2[2] = flt_8C63D8[3 * (v2 + (localClientNum << 10))] - cent->pose.origin[2];
    v7 = angles_2[1] * angles_2[1] + angles_2[0] * angles_2[0] + angles_2[2] * angles_2[2];
    if ( v7 > 256.0 )
    {
      flt_8C63D0[3 * (v2 + (localClientNum << 10))] = *v5;
      v8 = clientObjMap[v2 + 1152 * localClientNum];
      flt_8C63D4[3 * (v2 + (localClientNum << 10))] = cent->pose.origin[1];
      flt_8C63D8[3 * (v2 + (localClientNum << 10))] = cent->pose.origin[2];
      if ( v8 )
      {
        v9 = &objBuf[(signed __int16)v8];
        if ( v9 )
        {
          v10 = v9->radius + 16.0;
          sub_6094F0(v2, v5, localClientNum, v10);
          sub_5FBE80(localClientNum, v2, (int)v5, v10);
        }
      }
    }
  }
  v11 = &cgEntities[v14];
  v12 = v2 + (localClientNum << 10);
  if ( cgEntities[v14].pose.physObjId != -1 )
  {
    if ( word_739960[10 * v12] )
    {
      v13 = entityNumberb == 0;
    }
    else
    {
      if ( cgEntities[v14].nextState.solid )
        goto LABEL_34;
      v13 = CG_EntityNeedsLinked(localClientNum, v2) == 0;
    }
    if ( v13 )
    {
LABEL_35:
      sub_433A20(localClientNum, v2, (int)v11);
      CG_ProcessEntity(localClientNum, v11);
      return;
    }
LABEL_34:
    CG_LinkEntity(localClientNum, v2);
    goto LABEL_35;
  }
  if ( word_739960[10 * v12] )
    CG_UnlinkEntity(localClientNum, v2);
}
#endif
