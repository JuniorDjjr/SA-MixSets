#pragma once

#ifndef MIXSETS_H
#define MIXSETS_H

#include "Common.h"
#include "IniReader/IniReader.h"

using namespace std;

class MixSets
{
public:

	static inline bool Read;
	static inline std::fstream lg;

	static inline float G_VehFlipDamage;

	static inline DWORD RETURN_FixMouseStuck;
	static inline DWORD altRETURN_FixMouseStuck;


	static inline bool forceUpdateQualityFuncs, bProcessOnceOnScripts, bProcessOnceAfterIntro,
		bPlayerRenderWeaponInVehicleLastFrame, bPlayerTwoRenderWeaponInVehicleLastFrame, bNoCLEO,
		bOnEmergencyMissionLastFrame;
	static inline int curQuality, lastQuality, G_NoEmergencyMisWanted_MaxWantedLevel, G_Backup_WavesRadius;
	static inline languages lang;


	static inline int gameVersion;
	static inline bool bEnabled, bReadOldINI, bParsePreserveComments, bErrorRename, inSAMP, rpSAMP, dtSAMP, bIMFX, bIMFXgunflash, bGunFuncs, bOLA, bVehFuncs, bIniFailed, bVersionFailed,
		G_NoDensities, G_FixBicycleImpact, G_NoStencilShadows, G_OpenedHouses, G_TaxiLights, G_ParaLandingFix, G_NoEmergencyMisWanted, G_SCMfixes,
		G_NoGarageRadioChange, G_NoStuntReward, G_NoTutorials, G_EnableCensorship, G_HideWeaponsOnVehicle, bReloading, G_Fix2DGunflash, G_NoSamSite, G_LureRancher,
		G_SmoothAimIK, G_StaticPedShadOnBike, G_TuningChoose2colors;

	static inline int G_i, G_FPSlimit, G_ProcessPriority, G_FreezeWeather, G_CameraPhotoQuality, G_UseHighPedShadows, G_StreamMemory, G_Anisotropic, G_HowManyMinsInDay;

	static inline float G_f, G_CullDistNormalComps, G_CullDistBigComps, G_VehLodDist, G_VehDrawDist, G_PedDrawDist, G_VehMultiPassDist, G_GangWaveMinSpawnDist,
		G_VehBulletDamage, G_VehFireDamage, G_VehExploDamage, G_HeliRotorSpeed, G_PedDensityExt, G_PedDensityInt, G_VehDensity,
		G_VehDespawnOnScr, G_PedDespawnOnScr, G_PedDespawnOffScr, G_TowelSpawnOffScr, G_TrainSpawnDistance, G_ShadDist_Vehicles,
		G_ShadDist_Vehicles_Sqr, G_ShadDist_SmallPlanes, G_ShadDist_SmallPlanes_Sqr, G_ShadDist_BigPlanes, G_ShadDist_BigPlanes_Sqr;
	static inline float G_ShadDist_CarLight, G_ShadDist_CarLight_Sqr, G_ShadDist_CarLight_Mid, G_ShadDist_CarLight_Min, G_ShadDist_CarLight_Scale,
		G_FootstepsDist, G_PickupsDrawDist, G_CarPedImpact, G_BikePedImpact, Default_BikePedImpact, Default_CarPedImpact,
		G_HeliSensibility, G_PlaneTrailSegments, G_SkidHeight, G_SunSize, G_RhinoFireRange, G_VehOccupDrawDist,
		G_VehOccupDrawDist_Boat, G_BrakePower, G_BrakeMin, G_TireEff_DustLife, G_TireEff_DustFreq, G_TireEff_DustSize;
	static inline float G_TireEff_DustUpForce, G_TireSmk_UpForce, G_PedWeaponDrawDist, G_PedWeaponDrawDist_Final, G_PropCollDist_NEG, G_PropCollDist_POS,
		G_MediumGrassDistMult, G_FireCoronaSize, G_DistBloodpoolTex, G_RainGroundSplashNum, G_RainGroundSplashArea, G_RainGroundSplashArea_HALF, G_RoadblockSpawnDist,
		G_RoadblockSpawnDist_NEG, G_PedPopulationMult, G_VehPopulationMult, G_FxEmissionRateShare, G_GunflashEmissionMult, G_VehCamHeightOffset,
		G_ShadowsHeight, G_FxDistanceMult_A, G_FxDistanceMult_B, G_WaveLightingCamHei, G_WaveLightingMult, G_BoatFoamLightingFix, G_NoWavesIfCamHeight;
	static inline float zero;
	static inline float G_WeaponIconScaleFix;

	static inline HMODULE hVehFuncs;
	typedef float(__cdecl* VehFuncs_Ext_GetDoubleWheelOffset)(CVehicle*, int);
	static inline VehFuncs_Ext_GetDoubleWheelOffset pVehFuncs_Ext_GetDoubleWheelOffset;

	static inline DWORD _EAX;

	static inline uintptr_t ORIGINAL_MirrorsCreateBuffer;

	static inline int numOldCfgNotFound;

	static inline std::string G_ReloadCommand;
	static inline float _flt_2_4;
	static inline float _flt_1_8;
	
	static inline char G_NoMoneyZeros_Pos[4];
	static inline char G_NoMoneyZeros_Neg[5];

	static inline CVehicle* secPlayerVehicle;

	////////////////////////////////////////////////

	MixSets();

	static void ReadIni();
	static void ReadIni_BeforeFirstFrame();

	static void ShowModMessages();
	static void ProcessNoStencilShadows();
	static void ProcessPriority();
	static void SetIdlePriority();
	static void ProcessUseHighPedShadows();
	static inline bool FileExists(const string& name);
	static void ReadOldINI(CIniReader ini, fstream* lg, string section, string key);
	static bool ReadIniFloat(CIniReader ini, fstream* lg, string section, string key, float* f);
	static bool ReadIniInt(CIniReader ini, fstream* lg, string section, string key, int* i);
	static bool ReadIniBool(CIniReader ini, fstream* lg, string section, string key);

	static void IncreaseMemoryValueIfValid(uintptr_t address, int32_t value, uint8_t validation, bool vp);
	static void IncreaseMemoryValueIfValid_Byte(uintptr_t address, int8_t value, uint8_t validation, bool vp);
	static uint8_t CustomMaxAnisotropic();
	static void ForceHighMirrorRes_MirrorsCreateBuffer();

	static void asm_fmul(float f);

	static void VehFlipDamage_Process_Damage(CPed* ped);

	static void VehFlipDamage_Process(CVehicle* veh);
};

void __fastcall PreRender_AddSingleWheelParticles_FixDouble(CVehicle* _this, int a, int wheelState, int a3, float a4, float a5, CColPoint* colPoint, CVector* from, int id, signed int wheelId, int skidMarkType, bool *_bloodState, char a12);
void __declspec() PedWeaponDrawDist_ASM();
void __declspec() PedWeaponDrawDist_ASM();
void __declspec() VehFlipDamage_ASM();
void __declspec() VehFlipDamage_Player_ASM();
void __declspec() BrakeReverseFix_ASM();
void __declspec() SirenOnWithoutDriver_ASM();
void __declspec() NoPauseWhenMinimize_AllowMouseMovement_ASM();

const unsigned int GET_SCRIPT_STRUCT_NAMED = 0x10AAA;

#endif