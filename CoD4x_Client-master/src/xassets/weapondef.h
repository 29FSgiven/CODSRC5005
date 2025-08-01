#ifndef __WEAPONDEF_H__
#define __WEAPONDEF_H__

#include "../q_shared.h"
#include "xmodel.h"
#include "sounds.h"
#include "material.h"
#include "../dobj.h"

typedef void FxEffectDef;

/* 743 */
typedef enum 
{
  MISSILE_GUIDANCE_NONE = 0x0,
  MISSILE_GUIDANCE_SIDEWINDER = 0x1,
  MISSILE_GUIDANCE_HELLFIRE = 0x2,
  MISSILE_GUIDANCE_JAVELIN = 0x3,
  MISSILE_GUIDANCE_COUNT = 0x4,
}guidedMissileType_t;


/* 758 */
typedef enum 
{
  WEAPSTICKINESS_NONE = 0x0,
  WEAPSTICKINESS_ALL = 0x1,
  WEAPSTICKINESS_GROUND = 0x2,
  WEAPSTICKINESS_GROUND_WITH_YAW = 0x3,
  WEAPSTICKINESS_COUNT = 0x4,
}WeapStickinessType;

/* 748 */
typedef enum 
{
  AMMO_COUNTER_CLIP_NONE = 0x0,
  AMMO_COUNTER_CLIP_MAGAZINE = 0x1,
  AMMO_COUNTER_CLIP_SHORTMAGAZINE = 0x2,
  AMMO_COUNTER_CLIP_SHOTGUN = 0x3,
  AMMO_COUNTER_CLIP_ROCKET = 0x4,
  AMMO_COUNTER_CLIP_BELTFED = 0x5,
  AMMO_COUNTER_CLIP_ALTWEAPON = 0x6,
  AMMO_COUNTER_CLIP_COUNT = 0x7,
}ammoCounterClipType_t;

/* 745 */
typedef enum 
{
  WEAPPROJEXP_GRENADE = 0x0,
  WEAPPROJEXP_ROCKET = 0x1,
  WEAPPROJEXP_FLASHBANG = 0x2,
  WEAPPROJEXP_NONE = 0x3,
  WEAPPROJEXP_DUD = 0x4,
  WEAPPROJEXP_SMOKE = 0x5,
  WEAPPROJEXP_HEAVY = 0x6,
  WEAPPROJEXP_NUM = 0x7,
}weapProjExposion_t;

/* 746 */
typedef enum 
{
  VEH_ACTIVE_RETICLE_NONE = 0x0,
  VEH_ACTIVE_RETICLE_PIP_ON_A_STICK = 0x1,
  VEH_ACTIVE_RETICLE_BOUNCING_DIAMOND = 0x2,
  VEH_ACTIVE_RETICLE_COUNT = 0x3,
}activeReticleType_t;

/* 761 */
typedef enum 
{
  WEAPOVERLAYRETICLE_NONE = 0x0,
  WEAPOVERLAYRETICLE_CROSSHAIR = 0x1,
  WEAPOVERLAYRETICLE_NUM = 0x2,
}weapOverlayReticle_t;

/* 762 */
typedef enum 
{
  WEAPSTANCE_STAND = 0x0,
  WEAPSTANCE_DUCK = 0x1,
  WEAPSTANCE_PRONE = 0x2,
  WEAPSTANCE_NUM = 0x3,
}weapStance_t;

/* 763 */
typedef enum 
{
  WEAPON_ICON_RATIO_1TO1 = 0x0,
  WEAPON_ICON_RATIO_2TO1 = 0x1,
  WEAPON_ICON_RATIO_4TO1 = 0x2,
  WEAPON_ICON_RATIO_COUNT = 0x3,
}weaponIconRatioType_t;

/* 744 */
typedef enum 
{
  OFFHAND_CLASS_NONE = 0x0,
  OFFHAND_CLASS_FRAG_GRENADE = 0x1,
  OFFHAND_CLASS_SMOKE_GRENADE = 0x2,
  OFFHAND_CLASS_FLASH_GRENADE = 0x3,
  OFFHAND_CLASS_COUNT = 0x4,
}OffhandClass;

/* 749 */
typedef enum 
{
  PENETRATE_TYPE_NONE = 0x0,
  PENETRATE_TYPE_SMALL = 0x1,
  PENETRATE_TYPE_MEDIUM = 0x2,
  PENETRATE_TYPE_LARGE = 0x3,
  PENETRATE_TYPE_COUNT = 0x4,
}PenetrateType;

/* 750 */
typedef enum 
{
  IMPACT_TYPE_NONE = 0x0,
  IMPACT_TYPE_BULLET_SMALL = 0x1,
  IMPACT_TYPE_BULLET_LARGE = 0x2,
  IMPACT_TYPE_BULLET_AP = 0x3,
  IMPACT_TYPE_SHOTGUN = 0x4,
  IMPACT_TYPE_GRENADE_BOUNCE = 0x5,
  IMPACT_TYPE_GRENADE_EXPLODE = 0x6,
  IMPACT_TYPE_ROCKET_EXPLODE = 0x7,
  IMPACT_TYPE_PROJECTILE_DUD = 0x8,
  IMPACT_TYPE_COUNT = 0x9,
}ImpactType;

/* 759 */
typedef enum 
{
  WEAPCLASS_RIFLE = 0x0,
  WEAPCLASS_MG = 0x1,
  WEAPCLASS_SMG = 0x2,
  WEAPCLASS_SPREAD = 0x3,
  WEAPCLASS_PISTOL = 0x4,
  WEAPCLASS_GRENADE = 0x5,
  WEAPCLASS_ROCKETLAUNCHER = 0x6,
  WEAPCLASS_TURRET = 0x7,
  WEAPCLASS_NON_PLAYER = 0x8,
  WEAPCLASS_ITEM = 0x9,
  WEAPCLASS_NUM = 0xA,
}weapClass_t;

/* 753 */
typedef enum 
{
  WEAPINVENTORY_PRIMARY = 0x0,
  WEAPINVENTORY_OFFHAND = 0x1,
  WEAPINVENTORY_ITEM = 0x2,
  WEAPINVENTORY_ALTMODE = 0x3,
  WEAPINVENTORYCOUNT = 0x4,
}weapInventoryType_t;

/* 754 */
typedef enum 
{
  WEAPTYPE_BULLET = 0x0,
  WEAPTYPE_GRENADE = 0x1,
  WEAPTYPE_PROJECTILE = 0x2,
  WEAPTYPE_BINOCULARS = 0x3,
  WEAPTYPE_NUM = 0x4,
}weapType_t;

/* 755 */
typedef enum 
{
  WEAPOVERLAYINTERFACE_NONE = 0x0,
  WEAPOVERLAYINTERFACE_JAVELIN = 0x1,
  WEAPOVERLAYINTERFACE_TURRETSCOPE = 0x2,
  WEAPOVERLAYINTERFACECOUNT = 0x3,
}WeapOverlayInteface_t;

/* 756 */
typedef enum 
{
  WEAPON_FIRETYPE_FULLAUTO = 0x0,
  WEAPON_FIRETYPE_SINGLESHOT = 0x1,
  WEAPON_FIRETYPE_BURSTFIRE2 = 0x2,
  WEAPON_FIRETYPE_BURSTFIRE3 = 0x3,
  WEAPON_FIRETYPE_BURSTFIRE4 = 0x4,
  WEAPON_FIRETYPECOUNT = 0x5,
}weapFireType_t;


/* 778 */
typedef enum 
{
  IT_BAD = 0x0,
  IT_WEAPON = 0x1,
}itemType_t;

/* 7141 */
typedef struct 
{
  itemType_t giType;
}gitem_s;


/* 7062 */
typedef struct 
{
  const char *szInternalName;
  const char *szDisplayName;
  const char *szOverlayName;
  XModel *gunXModel[16];
  XModel *handXModel;
  const char *szXAnims[33];
  const char *szModeName;
  unsigned __int16 hideTags[8];
  unsigned __int16 notetrackSoundMapKeys[16];
  unsigned __int16 notetrackSoundMapValues[16];
  int playerAnimType;
  weapType_t weapType;
  weapClass_t weapClass; //Okay
  PenetrateType penetrateType;
  ImpactType impactType;
  weapInventoryType_t inventoryType; //Okay
  weapFireType_t fireType; //okay
  OffhandClass offhandClass;
  weapStance_t stance; //Okay
  FxEffectDef *viewFlashEffect;
  FxEffectDef *worldFlashEffect;
  snd_alias_list_t *pickupSound;
  snd_alias_list_t *pickupSoundPlayer;
  snd_alias_list_t *ammoPickupSound;
  snd_alias_list_t *ammoPickupSoundPlayer;
  snd_alias_list_t *projectileSound;
  snd_alias_list_t *pullbackSound;
  snd_alias_list_t *pullbackSoundPlayer;
  snd_alias_list_t *fireSound;
  snd_alias_list_t *fireSoundPlayer;
  snd_alias_list_t *fireLoopSound;
  snd_alias_list_t *fireLoopSoundPlayer;
  snd_alias_list_t *fireStopSound;
  snd_alias_list_t *fireStopSoundPlayer;
  snd_alias_list_t *fireLastSound;
  snd_alias_list_t *fireLastSoundPlayer;
  snd_alias_list_t *emptyFireSound; 
  snd_alias_list_t *emptyFireSoundPlayer;
  snd_alias_list_t *meleeSwipeSound;
  snd_alias_list_t *meleeSwipeSoundPlayer;
  snd_alias_list_t *meleeHitSound;
  snd_alias_list_t *meleeMissSound;
  snd_alias_list_t *rechamberSound;
  snd_alias_list_t *rechamberSoundPlayer;
  snd_alias_list_t *reloadSound;
  snd_alias_list_t *reloadSoundPlayer;
  snd_alias_list_t *reloadEmptySound;
  snd_alias_list_t *reloadEmptySoundPlayer;
  snd_alias_list_t *reloadStartSound;
  snd_alias_list_t *reloadStartSoundPlayer;
  snd_alias_list_t *reloadEndSound;
  snd_alias_list_t *reloadEndSoundPlayer;
  snd_alias_list_t *detonateSound;
  snd_alias_list_t *detonateSoundPlayer;
  snd_alias_list_t *nightVisionWearSound;
  snd_alias_list_t *nightVisionWearSoundPlayer;
  snd_alias_list_t *nightVisionRemoveSound;
  snd_alias_list_t *nightVisionRemoveSoundPlayer;
  snd_alias_list_t *altSwitchSound;
  snd_alias_list_t *altSwitchSoundPlayer;
  snd_alias_list_t *raiseSound;
  snd_alias_list_t *raiseSoundPlayer;
  snd_alias_list_t *firstRaiseSound;
  snd_alias_list_t *firstRaiseSoundPlayer;
  snd_alias_list_t *putawaySound;
  snd_alias_list_t *putawaySoundPlayer;
  snd_alias_list_t **bounceSound;
  FxEffectDef *viewShellEjectEffect;
  FxEffectDef *worldShellEjectEffect;
  FxEffectDef *viewLastShotEjectEffect;
  FxEffectDef *worldLastShotEjectEffect;
  Material *reticleCenter;
  Material *reticleSide;
  int iReticleCenterSize;
  int iReticleSideSize;
  int iReticleMinOfs;
  activeReticleType_t activeReticleType; //Okay
  float vStandMove[3];
  float vStandRot[3];
  float vDuckedOfs[3];
  float vDuckedMove[3];
  float vDuckedRot[3];
  float vProneOfs[3];
  float vProneMove[3];
  float vProneRot[3];
  float fPosMoveRate;
  float fPosProneMoveRate;
  float fStandMoveMinSpeed;
  float fDuckedMoveMinSpeed;
  float fProneMoveMinSpeed;
  float fPosRotRate;
  float fPosProneRotRate;
  float fStandRotMinSpeed;
  float fDuckedRotMinSpeed;
  float fProneRotMinSpeed;
  XModel *worldModel[16];
  XModel *worldClipModel;
  XModel *rocketModel;
  XModel *knifeModel;
  XModel *worldKnifeModel;
  Material *hudIcon;
  weaponIconRatioType_t hudIconRatio;
  Material *ammoCounterIcon;
  weaponIconRatioType_t ammoCounterIconRatio;
  ammoCounterClipType_t ammoCounterClip; //Okay
  int iStartAmmo;
  const char *szAmmoName;
  int iAmmoIndex;
  const char *szClipName; //Okay 0x32c
  int iClipIndex;
  int iMaxAmmo;
  int iClipSize;
  int shotCount;
  const char *szSharedAmmoCapName;
  int iSharedAmmoCapIndex;
  int iSharedAmmoCap;
  int damage;
  int playerDamage;
  int iMeleeDamage;
  int iDamageType;
  int iFireDelay;
  int iMeleeDelay;
  int meleeChargeDelay;
  int iDetonateDelay;
  int iFireTime;
  int iRechamberTime;
  int iRechamberBoltTime;
  int iHoldFireTime;
  int iDetonateTime;
  int iMeleeTime;
  int meleeChargeTime;
  int iReloadTime;
  int reloadShowRocketTime;
  int iReloadEmptyTime;
  int iReloadAddTime;
  int iReloadStartTime;
  int iReloadStartAddTime;
  int iReloadEndTime;
  int iDropTime;
  int iRaiseTime;
  int iAltDropTime;
  int iAltRaiseTime;
  int quickDropTime;
  int quickRaiseTime;
  int iFirstRaiseTime;
  int iEmptyRaiseTime;
  int iEmptyDropTime;
  int sprintInTime;
  int sprintLoopTime;
  int sprintOutTime;
  int nightVisionWearTime;
  int nightVisionWearTimeFadeOutEnd;
  int nightVisionWearTimePowerUp;
  int nightVisionRemoveTime;
  int nightVisionRemoveTimePowerDown;
  int nightVisionRemoveTimeFadeInStart;
  int fuseTime;
  int aiFuseTime;
  int requireLockonToFire;
  int noAdsWhenMagEmpty;
  int avoidDropCleanup;
  float autoAimRange;
  float aimAssistRange;
  float aimAssistRangeAds;
  float aimPadding;
  float enemyCrosshairRange;
  int crosshairColorChange;
  float moveSpeedScale;
  float adsMoveSpeedScale;
  float sprintDurationScale;
  float fAdsZoomFov;
  float fAdsZoomInFrac;
  float fAdsZoomOutFrac;
  Material *overlayMaterial;
  Material *overlayMaterialLowRes;
  weapOverlayReticle_t overlayReticle;
  WeapOverlayInteface_t overlayInterface; //Offset error +1 int needed 0x438
  float overlayWidth;
  float overlayHeight;
  float fAdsBobFactor;
  float fAdsViewBobMult;
  float fHipSpreadStandMin;
  float fHipSpreadDuckedMin;
  float fHipSpreadProneMin;
  float hipSpreadStandMax;
  float hipSpreadDuckedMax;
  float hipSpreadProneMax;
  float fHipSpreadDecayRate;
  float fHipSpreadFireAdd;
  float fHipSpreadTurnAdd;
  float fHipSpreadMoveAdd;
  float fHipSpreadDuckedDecay;
  float fHipSpreadProneDecay;
  float fHipReticleSidePos;
  int iAdsTransInTime;
  int iAdsTransOutTime;
  float fAdsIdleAmount;
  float fHipIdleAmount;
  float adsIdleSpeed;
  float hipIdleSpeed;
  float fIdleCrouchFactor;
  float fIdleProneFactor;
  float fGunMaxPitch;
  float fGunMaxYaw;
  float swayMaxAngle;
  float swayLerpSpeed;
  float swayPitchScale;
  float swayYawScale;
  float swayHorizScale;
  float swayVertScale;
  float swayShellShockScale;
  float adsSwayMaxAngle;
  float adsSwayLerpSpeed;
  float adsSwayPitchScale;
  float adsSwayYawScale;
  float adsSwayHorizScale;
  float adsSwayVertScale;
  int bRifleBullet;
  int armorPiercing;
  int bBoltAction;
  int aimDownSight;
  int bRechamberWhileAds;
  float adsViewErrorMin;
  float adsViewErrorMax;
  int bCookOffHold;
  int bClipOnly;
  int adsFireOnly;
  int cancelAutoHolsterWhenEmpty;
  int suppressAmmoReserveDisplay;
  int enhanced;
  int laserSightDuringNightvision;
  Material *killIcon;
  weaponIconRatioType_t killIconRatio;
  int flipKillIcon;
  Material *dpadIcon;
  weaponIconRatioType_t dpadIconRatio;
  int bNoPartialReload;
  int bSegmentedReload;
  int iReloadAmmoAdd;
  int iReloadStartAdd;
  const char *szAltWeaponName;
  unsigned int altWeaponIndex;
  int iDropAmmoMin;
  int iDropAmmoMax;
  int blocksProne;
  int silenced;
  int iExplosionRadius;
  int iExplosionRadiusMin;
  int iExplosionInnerDamage;
  int iExplosionOuterDamage;
  float damageConeAngle;
  int iProjectileSpeed;
  int iProjectileSpeedUp;
  int iProjectileSpeedForward;
  int iProjectileActivateDist;
  float projLifetime;
  float timeToAccelerate;
  float projectileCurvature;
  XModel *projectileModel;
  weapProjExposion_t projExplosion;
  FxEffectDef *projExplosionEffect;
  int projExplosionEffectForceNormalUp;
  FxEffectDef *projDudEffect;
  snd_alias_list_t *projExplosionSound;
  snd_alias_list_t *projDudSound;
  int bProjImpactExplode;
  WeapStickinessType stickiness;
  int hasDetonator;
  int timedDetonation;
  int rotate;
  int holdButtonToThrow;
  int freezeMovementWhenFiring;
  int lowAmmoWarningThreshold;
  float parallelBounce[29];
  float perpendicularBounce[29];
  FxEffectDef *projTrailEffect;
  float vProjectileColor[3];
  guidedMissileType_t guidedMissileType;
  float maxSteeringAccel;
  int projIgnitionDelay;
  FxEffectDef *projIgnitionEffect;
  snd_alias_list_t *projIgnitionSound;
  float fAdsAimPitch;
  float fAdsCrosshairInFrac;
  float fAdsCrosshairOutFrac;
  int adsGunKickReducedKickBullets;
  float adsGunKickReducedKickPercent;
  float fAdsGunKickPitchMin;
  float fAdsGunKickPitchMax;
  float fAdsGunKickYawMin;
  float fAdsGunKickYawMax;
  float fAdsGunKickAccel;
  float fAdsGunKickSpeedMax;
  float fAdsGunKickSpeedDecay;
  float fAdsGunKickStaticDecay;
  float fAdsViewKickPitchMin;
  float fAdsViewKickPitchMax;
  float fAdsViewKickYawMin;
  float fAdsViewKickYawMax;
  float fAdsViewKickCenterSpeed;
  float fAdsViewScatterMin;
  float fAdsViewScatterMax;
  float fAdsSpread;
  int hipGunKickReducedKickBullets;
  float hipGunKickReducedKickPercent;
  float fHipGunKickPitchMin;
  float fHipGunKickPitchMax;
  float fHipGunKickYawMin;
  float fHipGunKickYawMax;
  float fHipGunKickAccel;
  float fHipGunKickSpeedMax;
  float fHipGunKickSpeedDecay;
  float fHipGunKickStaticDecay;
  float fHipViewKickPitchMin;
  float fHipViewKickPitchMax;
  float fHipViewKickYawMin;
  float fHipViewKickYawMax;
  float fHipViewKickCenterSpeed;
  float fHipViewScatterMin;
  float fHipViewScatterMax;
  float fightDist;
  float maxDist;
  const char *accuracyGraphName[2];
  float (*accuracyGraphKnots[2])[2];
  float (*originalAccuracyGraphKnots[2])[2];
  int accuracyGraphKnotCount[2];
  int originalAccuracyGraphKnotCount[2];
  int iPositionReloadTransTime;
  float leftArc;
  float rightArc;
  float topArc;
  float bottomArc;
  float accuracy;
  float aiSpread;
  float playerSpread;
  float minTurnSpeed[2];
  float maxTurnSpeed[2];
  float pitchConvergenceTime;
  float yawConvergenceTime;
  float suppressTime;
  float maxRange;
  float fAnimHorRotateInc;
  float fPlayerPositionDist;
  const char *szUseHintString;
  const char *dropHintString;
  int iUseHintStringIndex;
  int dropHintStringIndex;
  float horizViewJitter;
  float vertViewJitter;
  const char *szScript;
  float fOOPosAnimLength[2];
  int minDamage;
  int minPlayerDamage;
  float fMaxDamageRange;
  float fMinDamageRange;
  float destabilizationRateTime;
  float destabilizationCurvatureMax;
  int destabilizeDistance;
  float locationDamageMultipliers[19];
  const char *fireRumble;
  const char *meleeImpactRumble;
  float adsDofStart;
  float adsDofEnd;
}WeaponDef;


/* 1918 */
#pragma pack(push, 4)
typedef struct weaponInfo_s
{
  DObj_t *viewModelDObj;
  XModel *handModel;
  XModel *gogglesModel;
  XModel *rocketModel;
  XModel *knifeModel;
  char weapModelIdx;
  char pad[3];
  int partBits[4];
  int iPrevAnim;
  XAnimTree_s *tree;
  int registered;
  gitem_s *item;
  const char *translatedDisplayName;
  const char *translatedModename;
  const char *translatedAIOverlayDescription;
}weaponInfo_t;
#pragma pack(pop)



extern WeaponDef *bg_weaponDefs[];
extern weaponInfo_t cg_weaponsArray[];
extern Material* weaponHudIcons[];
WeaponDef* BG_GetWeaponDef(int index);
int BG_PlayerWeaponCountPrimaryTypes(playerState_t *ps);
int BG_GetNumWeapons();
void ChangeViewmodelDObj(int localClientNum, int weaponNum, char weaponModel, XModel *newHands, XModel *newGoggles, XModel *newRocket, XModel *newKnife, byte updateClientInfo);

#endif