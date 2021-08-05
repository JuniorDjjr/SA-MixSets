#include "Common.h"
#include "MixSets.h"
#include "..\injector\assembly.hpp"
#include <game_sa\common.h>
#include "IMFX/Gunflashes.h"
#include "CGame.h"
#include "CGeneral.h"
#include <filesystem>
#include "CWorld.h"
#include "CMenuManager.h"
#include "CWeather.h"
#include "eAnimations.h"

using namespace plugin;
using namespace injector;
using namespace std;

void MixSets::ReadIni_BeforeFirstFrame()
{
	int i;
	float f;

	bVersionFailed = false;
	numOldCfgNotFound = 0;

	CIniReader ini("MixSets.ini");


	if (ini.data.size() <= 0) {
		lg << "\nERROR: MixSets.ini not found - MixSets.ini não encontrado \n";
		bIniFailed = true;
		return;
	} bIniFailed = false;


	CIniReader iniold("MixSets old.ini");
	if (iniold.data.size() <= 0)
	{
		bReadOldINI = false;
	}
	else {
		bReadOldINI = true;
		if (lang == languages::PT)
			lg << "\n'MixSets old.ini' encontrado. As configurações serão movidas para o 'MixSets.ini'.\n\n";
		else
			lg << "\n'MixSets old.ini' found. The settings will be moved to 'MixSets.ini'.\n\n";
	}


	// -- Mod
	if (ReadIniInt(ini, &lg, "Mod", "Language", &i)) {
		switch (i)
		{
		case 1:
			lang = languages::PT;
			break;
		case 2:
			lang = languages::EN;
			break;
		default:
			break;
		}
	}
	else {
		lang = languages::EN;
		lg << "Language not read. Set to english\n";
	}


	if (ReadIniBool(ini, &lg, "Mod", "Enabled")) {
		bEnabled = true;
	}
	else {
		bEnabled = false;
		if (lang == languages::PT)
			lg << "Desativado" << "\n\n";
		else
			lg << "Disabled" << "\n\n";
		lg.flush();
		return;
	}

	gameVersion = GetGameVersion();

	if (gameVersion != GAME_10US_COMPACT && gameVersion != GAME_10US_HOODLUM)
	{
		if (lang == languages::PT)
			lg << "\nERROR: O executável do seu jogo não é compatível. Use Crack 1.0 US Hoodlum ou Compact.\n";
		else
			lg << "\nERROR: Your game executable isn't compatible. Use Crack 1.0 US Hoodlum or Compact.\n";
		bVersionFailed = true;
		bEnabled = false;
		return;
	} bVersionFailed = false;


	if (ReadIniBool(ini, &lg, "Mod", "NoDensities")) {
		G_NoDensities = true;
	}
	else G_NoDensities = false;


	if (ReadIniBool(ini, &lg, "Mod", "SAMPdisadvantage")) {
		rpSAMP = true;
	}
	else rpSAMP = false;


	if (ReadIniBool(ini, &lg, "Mod", "LoadDistancesOnSAMP")) {
		dtSAMP = true;
	}
	else dtSAMP = false;


	G_ReloadCommand = ini.ReadString("Mod", "ReloadCommand", "");


	// -- System
	if (ReadIniInt(ini, &lg, "System", "ProcessPriority", &i)) {
		G_ProcessPriority = i;
	}
	else G_ProcessPriority = -1;

	if (ReadIniInt(ini, &lg, "System", "FPSlimit", &i)) {
		WriteMemory<uint8_t>(0xC1704C, i, false);
		G_FPSlimit = i;
	}


	// -- Gameplay
	if (!inSAMP && ReadIniBool(ini, &lg, "Gameplay", "NoReplay")) {
		WriteMemory<uint8_t>(0x460500, 0xC3, true);
	}

	if (ReadIniBool(ini, &lg, "Gameplay", "FixMouseStuck"))
	{
		injector::MakeInline<0x745423, 0x745423 + 8>([](injector::reg_pack& regs)
		{
			//mov     eax, [esp+8]
			//mov     ecx, [esp+4]
			regs.eax = *(DWORD*)(regs.esp + 0x8);
			regs.ecx = *(DWORD*)(regs.esp + 0x4);
			if (GetFocus() == *(HWND*)0xC97C1C)
			{
				*(uintptr_t*)(regs.esp - 0x4) = 0x74542B;
			}
			else
			{
				*(uintptr_t*)(regs.esp - 0x4) = 0x745433;
			}
		});
	}



	// -- Graphics
	if (ReadIniBool(ini, &lg, "Graphics", "DisplayDialogAnyAR")) {
		MakeNOP(0x7459E1, 2, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "ForceAnisotropic")) {
		WriteMemory<uint8_t>(0x730F9C, 0, true);
	}

	if (ReadIniInt(ini, &lg, "Graphics", "Anisotropic", &i)) {
		G_Anisotropic = i;
		MakeCALL(0x730F9F, CustomMaxAnisotropic, true);
	}

	if (!bReloading && ReadIniBool(ini, &lg, "Graphics", "ForceHighMirrorRes")) {
		ORIGINAL_MirrorsCreateBuffer = ReadMemory<uintptr_t>(0x72701D + 1, true);
		ORIGINAL_MirrorsCreateBuffer += (GetGlobalAddress(0x72701D) + 5);
		MakeCALL(0x72701D, ForceHighMirrorRes_MirrorsCreateBuffer, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "FxEmissionRateShare")) {
		G_FxEmissionRateShare = true;
	}
	else G_FxEmissionRateShare = false;

	if (ReadIniFloat(ini, &lg, "Graphics", "FxEmissionRateMult", &f)) {
		if (G_FxEmissionRateShare) {
			WriteMemory<float>(0x4A97B0 + 1, f, true);
			WriteMemory<float>(0x4A97C6 + 1, f, true);
			WriteMemory<float>(0x4A97DC + 1, f, true);
		}
		else {
			WriteMemory<float>(0x4A97B0 + 1, (0.5f * f), true);
			WriteMemory<float>(0x4A97C6 + 1, (0.75f * f), true);
			WriteMemory<float>(0x4A97DC + 1, (1.0f * f), true);
		}
	}
	else {
		if (G_FxEmissionRateShare) {
			WriteMemory<float>(0x4A97B0 + 1, 0.75f, true);
			WriteMemory<float>(0x4A97C6 + 1, 0.75f, true);
			WriteMemory<float>(0x4A97DC + 1, 0.75f, true);
		}
	}

	if (!bReloading && ReadIniFloat(ini, &lg, "Graphics", "FxDistanceMult", &f)) {
		if (ReadMemory<uintptr_t>(0x4AB032, true) == 0x00859AA0) {
			G_FxDistanceMult_A = 0.00390625f * f;
			WriteMemory<float*>(0x4AB032, &G_FxDistanceMult_A, true);
			G_FxDistanceMult_B = 0.015625f * f;
			WriteMemory<float*>(0x4A4247, &G_FxDistanceMult_B, true);
			WriteMemory<float*>(0x4A4255, &G_FxDistanceMult_B, true);
		}
		else {
			if (lang == languages::PT)
				lg << "\nAVISO: 'FxDistanceMult' não foi ativada pois outro mod alterou o valor.\n";
			else
				lg << "\nWARNING: 'FxDistanceMult' was not activated because another mod changed the value.\n";
		}
	}

	if (!inSAMP && ReadIniBool(ini, &lg, "Graphics", "NoCoronas")) {
		MakeNOP(0x53E18E, 5);
	}

	if (!bReloading) {
		if (ReadIniBool(ini, &lg, "Graphics", "NoStencilShadows")) {
			G_NoStencilShadows = true;
			WriteMemory<uint8_t>(0x7113C0, 0xEB, true);
			injector::MakeInline<0x706BB4, 0x706BB4 + 6>([](injector::reg_pack& regs)
			{
				//mov     eax, [esi+598h]
				regs.eax = *(uint32_t*)(regs.esi + 0x598);

				CPed* ped = (CPed*)regs.esi;
				if (ped->m_pIntelligence->m_TaskMgr.FindActiveTaskByType(TASK_COMPLEX_ENTER_CAR_AS_DRIVER) || (ped->m_nPedFlags.bInVehicle && ped->m_pVehicle))
				{
					if (ped->m_pShadowData)
					{
						ped->m_pShadowData->m_bCreated = false;
					}
					*(uintptr_t*)(regs.esp - 0x4) = 0x706BFD;
				}
			});
		}
		else G_NoStencilShadows = false;
	}

	if (ReadIniBool(ini, &lg, "Graphics", "StaticPedShadOnBike")) {
		G_StaticPedShadOnBike = true;
	}
	else G_StaticPedShadOnBike = false;

	if (G_NoStencilShadows || G_StaticPedShadOnBike)
	{
		injector::MakeInline<0x5E68B7, 0x5E68B7 + 19>([](injector::reg_pack& regs)
		{
			/*
			.text:005E68B7 080 8B 85 6C 04 00 00                       mov     eax, [ebp+46Ch]
			.text:005E68BD 080 F6 C4 01                                test    ah, 1
			.text:005E68C0 080 74 08                                   jz      short loc_5E68CA
			.text:005E68C2 080 8A 44 24 12                             mov     al, [esp+80h+bShadowNeeded]
			.text:005E68C6 080 84 C0                                   test    al, al
			.text:005E68C8 080 74 3E                                   jz      short loc_5E6908
			*/
			uint8_t bShadowNeeded = *(uint8_t*)(regs.esp + 0x80 - 0x6E);
			CPed* ped = (CPed*)regs.ebp;

			bool showShadow = bShadowNeeded || !ped->m_nPedFlags.bInVehicle;

			if (G_NoStencilShadows && ((g_fx.GetFxQuality() >= 2 && ped->IsPlayer()) || (g_fx.GetFxQuality() >= 3))) {
				showShadow = false;
			}

			if (ped->m_nPedFlags.bInVehicle && ped->m_pVehicle)
			{
				if (G_StaticPedShadOnBike)
				{
					if (ped->m_pVehicle->m_nVehicleSubClass == 9 || ped->m_pVehicle->m_nVehicleSubClass == 10)
					{
						showShadow = true;
					}
				}
				if (ped->m_pShadowData)
				{
					ped->m_pShadowData->m_bCreated = false;
				}
			}
			else {
				if (G_UseHighPedShadows == 0)
				{
					showShadow = true;
				}
			}
			if (showShadow)
			{
				*(uintptr_t*)(regs.esp - 0x4) = 0x5E68CA;
			}
			else
			{
				*(uintptr_t*)(regs.esp - 0x4) = 0x5E6908;
			}
		});
	}



	// -- Simple limit adjuster
	if (!bOLA && ReadIniInt(ini, &lg, "Simple Limit Adjuster", "VehicleStructs", &i) && i > 0) {
		IncreaseMemoryValueIfValid_Byte(0x5B8FE3 + 1, (50 * i), 0x6A, true);
	}
	if (ReadIniInt(ini, &lg, "Simple Limit Adjuster", "EnexEntries", &i) && i > 0) {
		if (gameVersion == GAME_10US_HOODLUM)
			IncreaseMemoryValueIfValid(0x156A798, (400 * i), 0x68, true);
		else
			IncreaseMemoryValueIfValid(0x43F928, (400 * i), 0x68, true);
	}
	if (ReadIniInt(ini, &lg, "Simple Limit Adjuster", "CarMatPipeDataPool", &i) && i > 0) {
		IncreaseMemoryValueIfValid(0x5DA08D + 1, (4096 * i), 0x68, true);
		IncreaseMemoryValueIfValid(0x5DA0C9 + 1, (1024 * i), 0x68, true);
		IncreaseMemoryValueIfValid(0x5DA105 + 1, (4096 * i), 0x68, true);
	}



	// -- Densities
	if (!G_NoDensities) {

		if (ReadIniInt(ini, &lg, "Densities", "MinDesiredLoadedVeh", &i)) {
			WriteMemory<uint8_t>(0x40B6AA + 2, i, true);
		}

		if (ReadIniInt(ini, &lg, "Densities", "DesiredLoadedVeh", &i)) {
			WriteMemory<uint32_t>(0x8A5A84, i, false);
			if (!bOLA) IncreaseMemoryValueIfValid_Byte(0x5B8FE3 + 1, (i * 5), 0x6A, true); // Note: if 12 (default), it will increase the VehicleStructs from 50 to 60 (kepping a margin)
		}

		if (ReadIniInt(ini, &lg, "Densities", "DelayLoadDesiredVeh", &i)) {
			WriteMemory<uint8_t>(0x40B9B6 + 6, i, true);
		}

		if (ReadIniInt(ini, &lg, "Densities", "MinLoadedGangVeh", &i)) {
			WriteMemory<uint8_t>(0x40ACA5 + 2, i, true);
		}

		if (!inSAMP && ReadIniFloat(ini, &lg, "Densities", "PedPopulationMult", &f)) {
			G_PedPopulationMult = f;
			injector::MakeInline<0x5BC1E9, 0x5BC1E9 + 7>([](injector::reg_pack& regs)
			{
				regs.eax = *(uint8_t*)(regs.esp + 0x0BC) * G_PedPopulationMult; //mov al, [esp+0BCh]
			});
		}
		else G_PedPopulationMult = 1.0f;

		if (!inSAMP && ReadIniFloat(ini, &lg, "Densities", "VehPopulationMult", &f)) {
			G_VehPopulationMult = f;
			injector::MakeInline<0x5BC1F0, 0x5BC1F0 + 7>([](injector::reg_pack& regs)
			{
				regs.ecx = *(uint8_t*)(regs.esp + 0x0F8) * G_VehPopulationMult; //mov cl, [esp+0F8h]
			});
		}
		else G_VehPopulationMult = 1.0f;
	}

	// -- Interface
	if (ReadIniBool(ini, &lg, "Interface", "WeaponIconScaleFix")) {
		WriteMemory<float*>(0x58D94D, &G_WeaponIconScaleFix, true); //fist
		WriteMemory<float*>(0x58D896, &G_WeaponIconScaleFix, true); //other
	}


	// -- World
	if (ReadIniInt(ini, &lg, "World", "HowManyMinsInDay", &i)) {
		i *= 41.666666667f;
		G_HowManyMinsInDay = i;
		//WriteMemory<uint32_t>(0xB7015C, i, false); // on MixSets.cpp
		WriteMemory<uint32_t>(0x5BA35F, i, true);
		WriteMemory<uint32_t>(0x53BDEC, i, true);
	}


	// -- Experimental
	if (ReadIniBool(ini, &lg, "Experimental", "NoTextures")) {
		WriteMemory<uint8_t>(0x884900, 0, true);
	}

	if (!inSAMP && ReadIniBool(ini, &lg, "Experimental", "ForceIPLcarSection")) {
		WriteMemory<uint8_t>(0x6F2EE3, 0x0C, true);
		WriteMemory<uint8_t>(0x6F2EFF, 0xCA, true);
	}

	if (ReadIniBool(ini, &lg, "Experimental", "NoSound")) {
		WriteMemory<uint8_t>(0x507750, 0xC3, true);
	}

	if (ReadIniBool(ini, &lg, "Experimental", "NoParticles")) {
		WriteMemory<uint32_t>(0x4AA440, 0x000020C2, true);
	}

	if (ReadIniBool(ini, &lg, "Experimental", "NoPostFx")) {
		WriteMemory<uint32_t>(0xC402CF, 1, true);
	}

	lg.flush();
}


///////////////////////////////////////////////////////////////////////////////////////////////////


void MixSets::ReadIni()
{
	if (!bEnabled) return;

	int i;
	float f;

	CIniReader ini("MixSets.ini");


	CIniReader iniold("MixSets old.ini");
	if (iniold.data.size() <= 0)
		bReadOldINI = false;
	else
		bReadOldINI = true;


	// -- System
	if (ReadIniInt(ini, &lg, "System", "StreamMemory", &i)) {
		if (i > 2047) {
			G_StreamMemory = 2147483647;
		}
		else {
			G_StreamMemory = (i * 1048576);
		}
		WriteMemory<uint32_t>(0x8A5A80, G_StreamMemory, true);
	}
	else G_StreamMemory = -1;

	if (ReadIniInt(ini, &lg, "System", "FPSlimit", &i)) {
		WriteMemory<uint8_t>(0xC1704C, i, false);
		G_FPSlimit = i;
	}
	else G_FPSlimit = -1;

	if (ReadIniBool(ini, &lg, "System", "MouseFix")) {
		Call<0x7469A0>();
	}

	if (ReadIniBool(ini, &lg, "System", "SkipShutdown")) {
		//MakeJMP(0x748E70, 0x748EE6, true); // jump all shutdown
		//MakeJMP(0x8246F3, 0x824701, true);
		//MakeJMP(0x824733, 0x824741, true);

		MakeNOP(0x748E6B, 5, true); // CGame::Shutdown
		MakeNOP(0x748E82, 5, true); // RsEventHandler rsRWTERMINATE
		MakeNOP(0x748E75, 5, true); // CAudioEngine::Shutdown

		injector::MakeInline<0x748EDF, 0x748EDF + 7>([](injector::reg_pack& regs)
		{
			SetErrorMode(0);
			_Exit(0); // exits faster https://www.geeksforgeeks.org/exit-vs-_exit-c-cpp/
		});

		//MakeNOP(0x748E9C, 5, true);
		//MakeNOP(0x748EA6, 5, true);
		//WriteMemory<uint8_t>(0x810C60, 0xC3, true);

		//WriteMemory<uint8_t>(0x801D50, 0xC3, true);

		//MakeJMP(0x53C902, 0x53CAF1, true);
	}


	// -- Graphics
	if (ReadIniInt(ini, &lg, "Graphics", "MotionBlurAlpha", &i)) {
		WriteMemory<uint8_t>(0x8D5104, i, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "NoMotionBlur")) {
		WriteMemory<uint8_t>(0x7030A0, 0xC3, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "NoVertigoEffect")) {
		WriteMemory<uint8_t>(0x524B3E, 0xEB, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "NoHeatEffect")) {
		WriteMemory<uint8_t>(0x72C1B7, 0xEB, true);
	}

	if (ReadIniInt(ini, &lg, "Graphics", "LicenseTextFilter", &i)) {
		WriteMemory<uint8_t>(0x884958, i, false);
	}

	if (ReadIniInt(ini, &lg, "Graphics", "TextureFilterMin", &i)) {
		WriteMemory<uint8_t>(0x88498C, i, false);
	}

	if (ReadIniInt(ini, &lg, "Graphics", "TextureFilterMax", &i)) {
		WriteMemory<uint8_t>(0x884988, i, false);
	}

	if (ReadIniInt(ini, &lg, "Graphics", "StencilShadowA", &i)) {
		WriteMemory<uint8_t>(0x71162C, i, true);
	}

	if (ReadIniInt(ini, &lg, "Graphics", "StencilShadowR", &i)) {
		WriteMemory<uint8_t>(0x711631 + 1, i, true);
	}
	if (ReadIniInt(ini, &lg, "Graphics", "StencilShadowG", &i)) {
		WriteMemory<uint8_t>(0x71162F + 1, i, true);
	}
	if (ReadIniInt(ini, &lg, "Graphics", "StencilShadowB", &i)) {
		WriteMemory<uint8_t>(0x71162D + 1, i, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "UnderwaterEffect")) {
		WriteMemory<uint8_t>(0xC402D3, 1, false);
	}

	if (!inSAMP && ReadIniBool(ini, &lg, "Graphics", "NoNitroBlur")) {
		MakeNOP(0x704E13, 17, true);
		WriteMemory<uint8_t>(0x704E24, 0xE9, true);
		WriteMemory<uint32_t>(0x704E25, 0xD4, true);
		WriteMemory<uint8_t>(0x704E29, 0x90, true);
	}

	if (ReadIniInt(ini, &lg, "Graphics", "UseHighPedShadows", &i)) {
		G_UseHighPedShadows = i;
	}
	else G_UseHighPedShadows = -1;

	if (ReadIniBool(ini, &lg, "Graphics", "NoLowPoleShadows")) {
		WriteMemory<uint8_t>(0x70C750, 0xC3, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "NoPlaneTrails")) {
		MakeNOP(0x7185B0, 5, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "No3DGunflash")) {
		MakeNOP(0x5E5F2A, 20, true);
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "GunflashEmissionMult", &f)) {
		G_GunflashEmissionMult = f;
	}
	else G_GunflashEmissionMult = -1.0f;

	if (!bReloading && ReadIniBool(ini, &lg, "Graphics", "Fix2DGunflash")) {
		if (bGunFuncs) {
			if (lang == languages::PT)
			{
				lg << "Fix2DGunflash desativado pois você já está usando a correção de efeito de tiro do GunFuncs." << "\n";
			}
			else {
				lg << "Fix2DGunflash disabled because you are already using gunflash GunFuncs fix." << "\n";
			}
			G_Fix2DGunflash = false;
		}
		else if (bIMFX && ReadMemory<uint8_t>(0x73306D, true) == 0x90) {
			if (lang == languages::PT)
			{
				lg << "Fix2DGunflash desativado pois você já está usando a correção de efeito de tiro do IMFX." << "\n";
			}
			else {
				lg << "Fix2DGunflash disabled because you are already using gunflash IMFX fix." << "\n";
			}
			G_Fix2DGunflash = false;
		}
		else {
			G_Fix2DGunflash = true;
			Gunflashes::Setup();
			Events::pedRenderEvent.before += Gunflashes::CreateGunflashEffectsForPed;
			/*injector::MakeInline<0x73F3A5, 0x73F3A5 + 6>([](injector::reg_pack& regs)
			{
				//mov     eax, [esi+460h]
				regs.eax = *(uint32_t*)(regs.esi + 0x460);

				// TESTS
				CVehicle *vehicle = (CVehicle *)regs.esi;
				CWeapon *weapon = *(CWeapon **)(regs.esp + 0x28);
				CVector *pointIn = (CVector *)(regs.esp + 0x2C);
				CVector *pointOut = (CVector *)(regs.esp + 0x44);
				CWeaponInfo *weaponInfo;

				CVector *gunshellPos;
				CVector gunshellDir;

				//showintlog(weapon->m_nType);
				//show3dlog(pointIn->x, 0.0, 0.0);

				float posOffset;
				float gunshellSize;

				switch (weapon->m_nType)
				{
				case WEAPON_PISTOL:
				case WEAPON_PISTOL_SILENCED:
				case WEAPON_DESERT_EAGLE:
				case WEAPON_SNIPERRIFLE:
					posOffset = 0.2;
					gunshellSize = 0x3E800000;
					goto LABEL_149;
				case WEAPON_SHOTGUN:
				case WEAPON_SAWNOFF:
				case WEAPON_SPAS12:
					posOffset = 0.30000001;
					gunshellSize = 0x3EE66666;
					goto LABEL_149;
				case WEAPON_MICRO_UZI:
				case WEAPON_MP5:
				case WEAPON_TEC9:
					posOffset = 0.2;
					gunshellSize = 0x3E99999A;
					goto LABEL_149;
				case WEAPON_AK47:
				case WEAPON_M4:
				case WEAPON_MINIGUN:
					weaponInfo = CWeaponInfo::GetWeaponInfo(weapon->m_nType, 1);
					if (((weaponInfo->m_fAnimLoopEnd - weaponInfo->m_fAnimLoopStart) * 900.0) >= 50 || (*(char*)0xC8A80C += 1, !(*(char*)0xC8A80C & 1)))
					{
						posOffset = 0.64999998;
						gunshellSize = 0x3E800000;
					LABEL_149:
						g_fx.TriggerGunshot(vehicle, *pointIn, *pointOut, true);
					}
					break;
				default:
					break;
				}
			});
			*/
		}
	}
	else {
		G_Fix2DGunflash = false;
	}


	if (ReadIniInt(ini, &lg, "Graphics", "PlaneTrailsSegments", &i)) {
		WriteMemory<uint32_t>(0x7172E6 + 2, i, true);
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "RainGroundSplashNum", &f)) {
		G_RainGroundSplashNum = f;
		WriteMemory<float*>(0x72AB16 + 2, &G_RainGroundSplashNum, true);
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "RainGroundSplashSize", &f)) {
		WriteMemory<float>(0x72AB87 + 1, f, true);
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "RainGroundSplashArea", &f)) {
		G_RainGroundSplashArea = f;
		WriteMemory<float*>(0x72ACEB + 2, &G_RainGroundSplashArea, true);
		WriteMemory<float*>(0x72AD12 + 2, &G_RainGroundSplashArea, true);
		G_RainGroundSplashArea_HALF = G_RainGroundSplashArea * 0.5f;
		WriteMemory<float*>(0x72AD1C + 2, &G_RainGroundSplashArea_HALF, true);
		WriteMemory<float*>(0x72ACF5 + 2, &G_RainGroundSplashArea_HALF, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "NoRainSteam")) {
		MakeNOP(0x72ADF0, 37, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "NoSandstormSteam")) {
		MakeNOP(0x72AAE0, 5, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "NoRainNoise")) {
		MakeNOP(0x705078, 5, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "NoPointLights")) {
		WriteMemory<uint8_t>(0x7000E0, 0xC3, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "NoVehSpecular")) {
		WriteMemory<uint8_t>(0x5D9ABE, 0, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "NoVehLighting")) {
		WriteMemory<uint8_t>(0x5D9A8F, 0, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "HeadlightSmoothMov")) {
		WriteMemory<uint32_t>(0x70C6A9, 0, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "VehShadowSmoothMov")) {
		WriteMemory<uint32_t>(0x70C2D6, 0, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "NoSkyStuff")) {
		MakeNOP(0x53DCA2, 5, true);
		MakeNOP(0x53DFA0, 5, true);
		MakeNOP(0x53E121, 5, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "NoSun")) {
		MakeNOP(0x53C136, 5, true);
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "SunSize", &f)) {
		G_SunSize = f;
		WriteMemory<float*>(0x6FC6EA, &G_SunSize, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "SunBlockedByVehicles")) {
		WriteMemory<bool>(0x6FAC5C, true, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "SunBlockedByPeds")) {
		WriteMemory<bool>(0x6FAC53, true, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "SunBlockedByObjects")) {
		WriteMemory<bool>(0x6FAC51, true, true);
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "VehSparkSpread", &f)) {
		WriteMemory<float>(0x5458E1, f, true);
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "BulletSparkForce", &f)) {
		WriteMemory<float>(0x49F47B, f, true);
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "BulletSparkSpread", &f)) {
		WriteMemory<float>(0x49F451, f, true);
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "BulletDustSize", &f)) {
		WriteMemory<float>(0x49F57E, f, true);
		WriteMemory<float>(0x49F4A5, f, true);
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "TireSmk_StartSize", &f)) {
		WriteMemory<float>(0x006DF1C8 + 1, f, true);
		f *= 0.7f;
		WriteMemory<float>(0x006DEF5A + 1, f, true);
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "TireSmk_DriftSize", &f)) {
		WriteMemory<float>(0x006DF20D, f, true);
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "TireSmk_BrakeSize", &f)) {
		f *= 0.7f;
		WriteMemory<float>(0x006DED24 + 1, f, true);
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "TireSmk_Life", &f)) {
		WriteMemory<float>(0x006DF1BE + 1, f, true);
		f *= 0.6f;
		WriteMemory<float>(0x006DED1A + 1, f, true);
		WriteMemory<float>(0x006DEF50 + 1, f, true);
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "TireSmk_UpForce", &f)) {
		G_TireSmk_UpForce = f;
		WriteMemory<float*>(0x006DF2B9 + 2, &G_TireSmk_UpForce, true);
	}


	if (ReadIniFloat(ini, &lg, "Graphics", "TireSmk_LumMin", &f)) {
		WriteMemory<float>(0x006DEE06, f, true);
		WriteMemory<float>(0x006DF046, f, true);
		WriteMemory<float>(0x006DF2A7, f, true);
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "TireSmk_LumMax", &f)) {
		WriteMemory<float>(0x006DEE01, f, true);
		WriteMemory<float>(0x006DF041, f, true);
		WriteMemory<float>(0x006DF2A2, f, true);
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "TireSmk_Alpha", &f)) {
		WriteMemory<float>(0x006DED29 + 1, f, true);
		WriteMemory<float>(0x006DEF5F + 1, f, true);
		WriteMemory<float>(0x006DF1CD + 1, f, true);
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "TireSmk_DriftAlpha", &f)) {
		WriteMemory<float>(0x006DF205, f, true);
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "TireEff_DustLife", &f)) {
		G_TireEff_DustLife = f;
		WriteMemory<float*>(0x004A079A + 2, &G_TireEff_DustLife, true);
		WriteMemory<float*>(0x004A0B4A + 2, &G_TireEff_DustLife, true);
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "TireEff_DustSize", &f)) {
		G_TireEff_DustSize = f;
		WriteMemory<float*>(0x004A07A4 + 2, &G_TireEff_DustSize, true);
		WriteMemory<float*>(0x004A0B5C + 2, &G_TireEff_DustSize, true);
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "TireEff_DustUpForce", &f)) {
		G_TireEff_DustUpForce = f;
		WriteMemory<float*>(0x004A0868 + 2, &G_TireEff_DustUpForce, true);
		WriteMemory<float*>(0x004A0C18 + 2, &G_TireEff_DustSize, true);
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "PlaneSmokeLife", &f)) {
		WriteMemory<float>(0x006CA9E5 + 1, f, true); //stuntplane
		WriteMemory<float>(0x006CA953 + 1, (f * 0.666666f), true); //cropduster
	}

	if (ReadIniBool(ini, &lg, "Graphics", "TaxiLights")) {
		G_TaxiLights = true;
	}
	else G_TaxiLights = false;

	if (!inSAMP && ReadIniFloat(ini, &lg, "Graphics", "BulletTraceThickness", &f)) {
		WriteMemory<float>(0x726CEA + 1, f, true);
		WriteMemory<float>(0x73AFB7 + 1, (f * 2), true);
	}

	if (!inSAMP && ReadIniInt(ini, &lg, "Graphics", "BulletTraceAlpha", &i)) {
		WriteMemory<uint8_t>(0x726CDD + 1, i, true);
		i = i * 2.142857f;
		if (i > 255) i = 255;
		WriteMemory<uint32_t>(0x73AFAD + 1, i, true);
	}

	if (!inSAMP && ReadIniInt(ini, &lg, "Graphics", "BulletTraceRGB", &i)) {
		WriteMemory<uint32_t>(0x723CBD + 1, i, true);
	}

	if (ReadIniInt(ini, &lg, "Graphics", "RopeAlpha", &i)) {
		WriteMemory<uint8_t>(0x5568A0 - 1, i, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "NoWaterFog")) {
		WriteMemory<uint8_t>(0x8D37D4, 0, false);
	}

	if (ReadIniInt(ini, &lg, "Graphics", "WaterFogDensity", &i)) {
		WriteMemory<uint32_t>(0x8D37E0, i, false);
	}


	if (ReadIniInt(ini, &lg, "Graphics", "CameraPhotoQuality", &i)) {
		G_CameraPhotoQuality = i;
		injector::MakeInline<0x5D04E1, 0x5D04E1 + 7>([](injector::reg_pack& regs)
		{
			*(uint32_t*)(regs.esp + 0x0E8) = regs.edi;
			((void(__cdecl*)(int, signed int, char))0x5C6FA0)(regs.ecx, G_CameraPhotoQuality, true);
		});
	}

	if (ReadIniBool(ini, &lg, "Graphics", "NoMirrors")) {
		MakeNOP(0x555854, 5, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "NoColorFilter")) {
		WriteMemory<uint8_t>(0x8D518C, 0, false);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "NoRainStreaks")) {
		MakeNOP(0x53E126, 5, true);
	}

	if (!inSAMP && ReadIniBool(ini, &lg, "Graphics", "NoCopHeliLight")) {
		WriteMemory<uint8_t>(0x006C712A, 0, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "NoGoggleNoise")) {
		MakeNOP(0x704EE8, 5, true);
		MakeNOP(0x704F59, 5, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "NoVolumetricClouds")) {
		MakeNOP(0x53E1B4, 5, true);
	}

	if (!inSAMP && ReadIniBool(ini, &lg, "Graphics", "NoMovingFog")) {
		MakeNOP(0x53E1AF, 5, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "NoBirds")) {
		MakeNOP(0x53E170, 5, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "NoLowClouds")) {
		MakeNOP(0x53E121, 5, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "NoGrass")) {
		WriteMemory<uint8_t>(0x5DBAE0, 0xC3, true);
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "MediumGrassDistMult", &f)) {
		G_MediumGrassDistMult = f;
		WriteMemory<float*>(0x5DAD88 + 2, &G_MediumGrassDistMult, true);
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "FireCoronaSize", &f)) {
		G_FireCoronaSize = f;
		WriteMemory<float*>(0x53B784 + 2, &G_FireCoronaSize, true);
	}

	if (ReadIniInt(ini, &lg, "Graphics", "FireLensflare", &i)) {
		WriteMemory<uint8_t>(0x53B759, i, true);
	}

	/*if (ReadIniBool(ini, &lg, "Graphics", "FireGroundLight")) {
		WriteMemory<uint8_t>(0x53B65A, 50, true); // the intensity value doesn't matter, idkw
		// already included on SilentPatch now
	}*/

	if (ReadIniBool(ini, &lg, "Graphics", "NoFireCoronas")) {
		MakeNOP(0x53B688, 10, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "NoDismemberment")) {
		WriteMemory<uint8_t>(0x4B3A3C, 0xEB, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "NoEnexCones")) {
		WriteMemory<uint8_t>(0x440D6D, 0xE9, true);
		WriteMemory<uint32_t>(0x440D6E, 0x200, true);
	}

	if (ReadIniBool(ini, &lg, "Graphics", "HideWeaponsOnVehicle")) {
		G_HideWeaponsOnVehicle = true;
	}
	else G_HideWeaponsOnVehicle = false;


	if (ReadIniFloat(ini, &lg, "Graphics", "ShadowsHeight", &f)) {
		G_ShadowsHeight = f;
		WriteMemory<float*>(0x709B2D + 2, &G_ShadowsHeight);
		WriteMemory<float*>(0x709B8C + 2, &G_ShadowsHeight);
		WriteMemory<float*>(0x709BC5 + 2, &G_ShadowsHeight);
		WriteMemory<float*>(0x709BF4 + 2, &G_ShadowsHeight);
		WriteMemory<float*>(0x709C91 + 2, &G_ShadowsHeight);

		WriteMemory<float*>(0x709E9C + 2, &G_ShadowsHeight);
		WriteMemory<float*>(0x709EBA + 2, &G_ShadowsHeight);
		WriteMemory<float*>(0x709ED5 + 2, &G_ShadowsHeight);

		WriteMemory<float*>(0x70B21F + 2, &G_ShadowsHeight);
		WriteMemory<float*>(0x70B371 + 2, &G_ShadowsHeight);
		WriteMemory<float*>(0x70B4CF + 2, &G_ShadowsHeight);
		WriteMemory<float*>(0x70B633 + 2, &G_ShadowsHeight);

		WriteMemory<float*>(0x7085A7 + 2, &G_ShadowsHeight);
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "ShadowsHeightHack", &f)) {
		WriteMemory<float>(0x8CD4F0, f, false);
	}

	if (ReadIniInt(ini, &lg, "Graphics", "WaterWavesRadius", &i)) {
		*(int*)(0x8D37D0) = i;
	}

	ReadIniFloat(ini, &lg, "Graphics", "WaveLightingCamHei", &G_WaveLightingCamHei); //0.1f
	ReadIniFloat(ini, &lg, "Graphics", "WaveLightingMult", &G_WaveLightingMult);

	if (G_WaveLightingCamHei != -1.0f || G_WaveLightingMult != -1.0f) {
		if (G_WaveLightingCamHei == -1.0f) G_WaveLightingCamHei = 0.0f;
		if (G_WaveLightingMult == -1.0f) G_WaveLightingMult = 1.0f;
		injector::MakeInline<0x6E7141, 0x6E7141 + 5>([](injector::reg_pack& regs) {
			reinterpret_cast<CVector*>(regs.ecx)->Normalise(); // original code
			if (CGame::currArea == 0)
			{
				float camHeight = TheCamera.GetPosition().z;
				if (camHeight < 0.0f) camHeight = 0.0f;
				*(float*)(regs.esi + 0x0) /= 1.0f + (camHeight * G_WaveLightingCamHei);
				*(float*)(regs.esi + 0x4) /= 1.0f + (camHeight * G_WaveLightingCamHei);
				*(float*)(regs.esi + 0x0) *= G_WaveLightingMult;
				*(float*)(regs.esi + 0x4) *= G_WaveLightingMult;
				if (*(float*)(regs.esi + 0x0) > 0.2f) *(float*)(regs.esi + 0x0) = 0.2f;
				if (*(float*)(regs.esi + 0x4) > 0.2f) *(float*)(regs.esi + 0x4) = 0.2f;
				if (*(float*)(regs.esi + 0x0) < -0.2f) *(float*)(regs.esi + 0x0) = -0.2f;
				if (*(float*)(regs.esi + 0x4) < -0.2f) *(float*)(regs.esi + 0x4) = -0.2f;
			}
		});
	}

	if (ReadIniFloat(ini, &lg, "Graphics", "BoatFoamLightingFix", &f)) {
		G_BoatFoamLightingFix = f;
	}
	else G_BoatFoamLightingFix = -1.0f;

	if (ReadIniFloat(ini, &lg, "Graphics", "NoWavesIfCamHeight", &f)) {
		G_NoWavesIfCamHeight = f;
	}
	else G_NoWavesIfCamHeight = -1.0f;

	if (ReadIniFloat(ini, &lg, "Graphics", "FOV", &f)) {
		WriteMemory<float>(0x6FF41B, f, true);
	}


	// -- Gameplay

	if ((!inSAMP || (inSAMP && rpSAMP)) && ReadIniBool(ini, &lg, "Gameplay", "ScrollReloadFix")) {
		MakeNOP(0x60B4FA, 6, true);
	}

	if ((!inSAMP || (inSAMP && rpSAMP)) && ReadIniBool(ini, &lg, "Gameplay", "VehBurnEngineBroke")) {
		injector::MakeInline<0x006A70ED, 0x006A70F3>([](injector::reg_pack& regs) {
			auto vehicle = (CVehicle*)regs.esi;
			vehicle->m_nVehicleFlags.bEngineOn = false;
			regs.eax = *(DWORD*)(regs.esi + 0x57C); //original code
		});

		injector::MakeInline<0x006A75F9, 0x006A75FF>([](injector::reg_pack& regs) {
			auto vehicle = (CVehicle*)regs.esi;
			if (vehicle->m_fHealth > 0.0f) vehicle->m_nVehicleFlags.bEngineOn = true;
			*(DWORD*)(regs.esi + 0x57C) = regs.edi; //original code
		});
	}

	G_ParaLandingFix = false;
	G_NoGarageRadioChange = false;
	G_NoEmergencyMisWanted = false;
	G_LureRancher = false;
	G_NoTutorials = false;
	G_SCMfixes = false;
	G_TuningChoose2colors = false;
	if (!inSAMP) {

		if (ReadIniBool(ini, &lg, "Gameplay", "ParaLandingFix")) {
			G_ParaLandingFix = true;
		}

		if (ReadIniBool(ini, &lg, "Gameplay", "NoGarageRadioChange")) {
			G_NoGarageRadioChange = true;
		}

		if (ReadIniBool(ini, &lg, "Gameplay", "NoEmergencyMisWanted")) {
			G_NoEmergencyMisWanted = true;
		}

		if (ReadIniBool(ini, &lg, "Gameplay", "SCMfixes")) {
			G_SCMfixes = true;
		}

		if (ReadIniBool(ini, &lg, "Gameplay", "TuningChoose2colors")) {
			G_TuningChoose2colors = true;
		}

		if (ReadIniBool(ini, &lg, "Gameplay", "UseLureRancher")) {
			G_LureRancher = true;
		}

		if (ReadIniBool(ini, &lg, "Gameplay", "NoFlyHeightLimit")) {
			WriteMemory<uint8_t>(0x6D261D, 0xEB, true);
		}

		if (ReadIniFloat(ini, &lg, "Gameplay", "JetpackHeightLimit", &f)) {
			WriteMemory<float>(0x8703D8, f, false);
		}

		if (ReadIniFloat(ini, &lg, "Gameplay", "BikePedImpact", &f)) {
			G_BikePedImpact = f;
			Default_BikePedImpact = ReadMemory<float>(0x8D22AC, false);
			injector::MakeInline<0x5F1200, 0x5F1200 + 6>([](injector::reg_pack& regs)
			{
				if (G_BikePedImpact != -1.0)
					asm_fmul(G_BikePedImpact);
				else
					asm_fmul(Default_BikePedImpact);
			});
		}

		if (ReadIniFloat(ini, &lg, "Gameplay", "CarPedImpact", &f)) {
			G_CarPedImpact = f;
			Default_CarPedImpact = ReadMemory<float>(0x8D22A8, false);
			injector::MakeInline<0x5F1208, 0x5F1208 + 6>([](injector::reg_pack& regs)
			{
				if (G_CarPedImpact != -1.0)
					asm_fmul(G_CarPedImpact);
				else
					asm_fmul(Default_CarPedImpact);
			});
		}

		if (ReadIniFloat(ini, &lg, "Gameplay", "VehPedImpactUpForce", &f)) {
			WriteMemory<float>(0x8D22A4, f, false);
		}

		ReadIniFloat(ini, &lg, "Gameplay", "VehExploDamage", &G_VehExploDamage);
		ReadIniFloat(ini, &lg, "Gameplay", "VehBulletDamage", &G_VehBulletDamage);

		if (G_VehBulletDamage != 1.0 || G_VehExploDamage != 1.0 || G_VehBulletDamage != -1.0 || G_VehExploDamage != -1.0) {
			injector::MakeInline<0x6D7FDA, 0x6D7FDA + 6>([](injector::reg_pack& regs)
			{
				regs.eax = *(uint32_t*)(regs.esi + 0x594); // mov     eax, [esi+594h]

				CVehicle* vehicle = (CVehicle*)regs.esi;
				if (!vehicle->m_nVehicleFlags.bIsRCVehicle && !(vehicle->m_pDriver && vehicle->m_pDriver->m_nCreatedBy == 2 && vehicle->m_nCreatedBy == eVehicleCreatedBy::MISSION_VEHICLE)) {
					if (regs.ebx == 51 || regs.ebx == 37) { // explosion or fire
						if (G_VehExploDamage != -1.0) { // check it because the var may be updated by ini reloading
							if (regs.ebx == 51) {
								*(float*)(regs.esp + 0x90) *= G_VehExploDamage;
							}
						}
					}
					else {
						if (G_VehBulletDamage != -1.0) {
							float thisDamage = *(float*)(regs.esp + 0x90);

							// For petrol cap (not 100% but this way we don't need to do another hook).
							if (thisDamage != vehicle->m_fHealth && thisDamage != 1000.0f) {
								*(float*)(regs.esp + 0x90) *= G_VehBulletDamage;
							}
						}
					}
				}
			});
		}

		if (ReadIniFloat(ini, &lg, "Gameplay", "VehFireDamage", &f)) {
			G_VehFireDamage = f;
			WriteMemory<float*>(0x53A6B7 + 2, &G_VehFireDamage, true);
		}

		if (ReadIniBool(ini, &lg, "Gameplay", "NoTutorials")) {
			WriteMemory<uint8_t>(0xC0BC15, 1, true);
			G_NoTutorials = true;
		}
		else G_NoTutorials = false;
	}

	if (ReadIniFloat(ini, &lg, "Gameplay", "BrakePower", &f)) {
		if (!inSAMP || f < 1.0f) {
			G_BrakePower = f;
			if (!inSAMP && ReadIniFloat(ini, &lg, "Gameplay", "BrakeMin", &f)) {
				G_BrakeMin = f / 100.0f;
			}
			else {
				G_BrakeMin = -1.0f;
			}
			injector::MakeInline<0x6B269F, 0x6B269F + 6>([](injector::reg_pack& regs)
			{
				//fmul[eax + tHandlingData.fBrakeDeceleration]
				float brakeDeceleration = *(float*)(regs.eax + 0x94);
				brakeDeceleration *= G_BrakePower;
				if (G_BrakeMin > 0.0 && brakeDeceleration < G_BrakeMin) brakeDeceleration = G_BrakeMin;
				G_f = brakeDeceleration;
				asm_fmul(brakeDeceleration);
			});
		}
	}

	if (ReadIniBool(ini, &lg, "Gameplay", "NoSteerSpeedLimit")) {
		WriteMemory<uint8_t>(0x6B2A15, 0xEB, true);
	}

	if (ReadIniBool(ini, &lg, "Gameplay", "NoFixHandsToBars")) {
		MakeNOP(0x601B90, 6, true);
	}

	if (ReadIniBool(ini, &lg, "Gameplay", "EnableCensorship")) {
		G_EnableCensorship = true;
	}
	else G_EnableCensorship = false;

	if (ReadIniBool(ini, &lg, "Gameplay", "BrakeReverseFix")) {
		injector::MakeCALL(0x006ADB80, BrakeReverseFix_ASM);
		injector::MakeNOP(0x006ADB85);
		injector::WriteMemory<uint32_t>(0x006ADB87, 0x8D, true);
	}

	if (!inSAMP && ReadIniBool(ini, &lg, "Gameplay", "NoCheats")) {
		MakeNOP(0x4384D0, 3, true);
		WriteMemory<uint8_t>(0x4384D3, 0xE9, true);
		WriteMemory<uint32_t>(0x4384D4, 0x000000CD, true);
	}

	if (ReadIniInt(ini, &lg, "Gameplay", "DeadPedFreezeDelay", &i)) {
		WriteMemory<uint32_t>(0x630D28, i, true);
		WriteMemory<uint32_t>(0x630D75, i, true);
	}

	if (ReadIniFloat(ini, &lg, "Gameplay", "VehElevatorSpeed", &f)) {
		if (inSAMP && f > 10.0f) f = 10.0f;
		WriteMemory<float>(0x871008, f, false);
	}

	if (ReadIniBool(ini, &lg, "Gameplay", "NoWheelTurnBack")) {
		MakeNOP(0x6B5579, 6, true);
		MakeNOP(0x6B568A, 6, true);
	}

	if (ReadIniFloat(ini, &lg, "Gameplay", "WheelTurnSpeed", &f)) {
		if (inSAMP && f > 0.2f) f = 0.2f;
		WriteMemory<float>(0x871058, f, false);
	}

	if (ReadIniFloat(ini, &lg, "Gameplay", "HeliSensibility", &f)) {
		if (inSAMP && f > 0.00392f) f = 0.00392f;
		G_HeliSensibility = f;
		WriteMemory<float*>(0x6C4867 + 2, &G_HeliSensibility, true);
	}

	if (!inSAMP && ReadIniBool(ini, &lg, "Gameplay", "VehFlipDontBurn")) {
		ReadIniFloat(ini, &lg, "Gameplay", "VehFlipDamage", &f);
		if (f > 0.0f) {
			G_VehFlipDamage = f / 8.0f;
			MakeCALL(0x006A776B, VehFlipDamage_ASM);
			MakeCALL(0x00570E7F, VehFlipDamage_Player_ASM);
			MakeNOP(0x006A7770);
			MakeNOP(0x00570E84);
		}
		else {
			// Patch ped vehicles damage when flipped
			WriteMemory<uint16_t>(0x6A776B, 0xD8DD, true); // fstp st0
			MakeNOP(0x6A776D, 4, true);

			// Patch player vehicle damage when flipped
			WriteMemory<uint16_t>(0x570E7F, 0xD8DD, true); // fstp st0
			MakeNOP(0x570E81, 4, true);
		}
	}

	if (ReadIniBool(ini, &lg, "Gameplay", "EnableTrainCams")) {
		WriteMemory<uint8_t>(0x52A52F, 0xAB, true);
	}

	if (ReadIniBool(ini, &lg, "Gameplay", "NoPauseWhenUnfocus")) {
		// from MTA
		// Disable MENU AFTER alt + tab
		//0053BC72   C605 7B67BA00 01 MOV BYTE PTR DS:[BA677B],1    
		injector::WriteMemory<uint8_t>(0x53BC78, 0x00, true);
		// ALLOW ALT+TABBING WITHOUT PAUSING
		injector::MakeNOP(0x748A8D, 6, true);
		injector::MakeJMP(0x6194A0, NoPauseWhenMinimize_AllowMouseMovement_ASM, true);
	}

	if (ReadIniBool(ini, &lg, "Gameplay", "NoPaintjobToWhite")) {
		MakeNOP(0x6D65C5, 11, true);
	}

	if (ReadIniBool(ini, &lg, "Gameplay", "NoIdleCam")) {
		WriteMemory<uint8_t>(0x522C80, 0xC3, true);
	}

	if (!inSAMP && ReadIniBool(ini, &lg, "Gameplay", "NoPedsTalkingToYou")) {
		WriteMemory<uint8_t>(0x43B0F0, 0xC3, true);
	}

	if (ReadIniBool(ini, &lg, "Gameplay", "NoInteriorMusic")) {
		MakeNOP(0x508450, 6, true);
		MakeNOP(0x508817, 6, true);
	}

	if (ReadIniBool(ini, &lg, "Gameplay", "KeepLightEngineOff")) {
		MakeNOP(0x6E1DBC, 8, true);
	}

	if (ReadIniBool(ini, &lg, "Gameplay", "NoWayForSiren")) {
		MakeNOP(0x6B2BED, 5, true);
	}

	if (!inSAMP && ReadIniBool(ini, &lg, "Gameplay", "NoUniqueJumps")) {
		MakeNOP(0x53C0C1, 5, true);
	}

	if (!inSAMP && ReadIniBool(ini, &lg, "Gameplay", "NoUniqueJumpsCam")) {
		MakeNOP(0x49C524, 5, true);
		MakeNOP(0x49C533, 5, true);
		MakeNOP(0x49C87E, 20, true);
		MakeNOP(0x49C89C, 46, true);
	}

	if (!inSAMP && ReadIniBool(ini, &lg, "Gameplay", "NoUniqueJumpsSlow")) {
		MakeNOP(0x49C892, 10, true);
		MakeNOP(0x49C529, 10, true);
	}

	if (!inSAMP && ReadIniBool(ini, &lg, "Gameplay", "NoTrainDerail")) {
		WriteMemory<uint32_t>(0x006F8C2A, 0x00441F0F, true); // nop dword ptr [eax+eax*1+00h]
		WriteMemory<uint8_t>(0x006F8C2E, 0x00, true);
		WriteMemory<uint16_t>(0x006F8C41, 0xE990, true); // jmp near
	}

	if (ReadIniBool(ini, &lg, "Gameplay", "FixMouseSensibility")) {
		if (ReadMemory<uint32_t>(0x005BC7B8, true) != 0x84) {
			WriteMemory<float*>(0x0050FB06, &_flt_2_4, true);
			WriteMemory<float*>(0x00510BBB, &_flt_1_8, true);
			WriteMemory<float*>(0x00511DE4, &_flt_2_4, true);
			WriteMemory<float*>(0x0052227F, &_flt_2_4, true);
			WriteMemory<float*>(0x0050F022, &_flt_2_4, true);
			if (ReadMemory<float*>(0x50F048, true) == &CCamera::m_fMouseAccelHorzntl)
			{
				WriteMemory<float*>(0x0050F048, &CCamera::m_fMouseAccelVertical, true);
				WriteMemory<float*>(0x0050FB28, &CCamera::m_fMouseAccelVertical, true);
				WriteMemory<float*>(0x00510C28, &CCamera::m_fMouseAccelVertical, true);
				WriteMemory<float*>(0x00511E0A, &CCamera::m_fMouseAccelVertical, true);
				WriteMemory<float*>(0x0052228E, &CCamera::m_fMouseAccelVertical, true);
			}

			WriteMemory<uint32_t>(0x005735E0, 0x00865450, true);

			WriteMemory<uint32_t>(0x005BC7B4, 0x1F0F6666, true);
			WriteMemory<uint32_t>(0x005BC7B8, 0x84, true);
			WriteMemory<uint16_t>(0x005BC7BC, 0x0, true);

			float hor = 0.0003125f + 0.0003125f / 2.0f;
			while (hor <= CCamera::m_fMouseAccelHorzntl)
			{
				hor += (0.005f / 16.0f);
			}
			hor -= 0.0003125f / 2.0f;
			if (hor != CCamera::m_fMouseAccelHorzntl)
			{
				CCamera::m_fMouseAccelHorzntl = hor;
				FrontEndMenuManager.SaveSettings();
			}
			hor *= (0.0015f / 0.0025f);
			CCamera::m_fMouseAccelVertical = hor;
		}
		else {
			lg << "Warning: FixMouseSensibility ignored. Biaxial Mouse Sensitivity Option mod is already installed.\n";
		}
	}

	if (!inSAMP && ReadIniBool(ini, &lg, "Gameplay", "OpenedHouses")) {
		G_OpenedHouses = true;
	}
	else G_OpenedHouses = false;

	if (ReadIniBool(ini, &lg, "Gameplay", "RandWheelDettach")) {

		injector::MakeInline<0x6B38C4, 0x6B38C4 + 17>([](injector::reg_pack& regs)
		{
			CAutomobile* automobile = (CAutomobile*)regs.esi;
			int nodeId = 5;
			switch (CGeneral::GetRandomNumberInRange(0, 5))
			{
			case 1:
				nodeId = 2;
				break;
			case 2:
				nodeId = 5;
				break;
			case 3:
				nodeId = 4;
				break;
			case 4:
				nodeId = 7;
				break;
			}
			RwFrame* frame = automobile->m_aCarNodes[nodeId];
			if (frame && frame->object.parent) {
				automobile->SpawnFlyingComponent(nodeId, 1);
				regs.eax = (uintptr_t)frame;
			}
			else {
				*(uintptr_t*)(regs.esp - 4) = 0x6B38F7;
			}
		});
	}

	if (!inSAMP && ReadIniBool(ini, &lg, "Gameplay", "HostileGangs")) {
		WriteMemory<uint16_t>(0x5FC88F, 0x9066, true);
		WriteMemory<uint8_t>(0x5FC8A8, 0xEB, true);
		WriteMemory<uint16_t>(0x5FC8C6, 0x9066, true);
		WriteMemory<uint16_t>(0x5FC8DD, 0xF66, true);
		WriteMemory<uint32_t>(0x5FC8DF, 0x441F, true);
		WriteMemory<uint32_t>(0x5FC992, 0x1F0F01B0, true);
		WriteMemory<uint8_t>(0x5FC996, 0, true);
	}
	 
	if (ReadIniBool(ini, &lg, "Gameplay", "FixBoatRadioAnim")) {
		injector::MakeInline<0x6DF4E7, 0x6DF4E7 + 5>([](injector::reg_pack& regs)
		{
			CPed* driver = (CPed*)regs.esi;
			if (RpAnimBlendClumpGetAssociation(driver->m_pRwClump, ANIM_DEFAULT_DRIVE_BOAT)) {
				regs.eax = 0;
			}
			else {
				regs.eax = (uint32_t)RpAnimBlendClumpGetAssociation(driver->m_pRwClump, ANIM_DEFAULT_CAR_TUNE_RADIO);
			}
			*(uintptr_t*)(regs.esp - 0x4) = 0x6DF4FC;
		});
	}

	if (!inSAMP && ReadIniBool(ini, &lg, "Gameplay", "NoStuntReward")) {
		G_NoStuntReward = true;
	}
	else G_NoStuntReward = false;

	if (ReadIniBool(ini, &lg, "Gameplay", "FixTwoPlayerVehSound")) {
		injector::MakeInline<0x4F570A, 0x4F570A + 6>([](injector::reg_pack& regs)
		{
			*(uint32_t*)(regs.esi + 0xA7) = 0; //mov     [esi+0A7h], bl
			CVehicle* veh = (CVehicle*)regs.eax;
			if (veh->m_pDriver == CWorld::Players[1].m_pPed) {
				secPlayerVehicle = veh;
			}
			else {
				secPlayerVehicle = nullptr;
			}
		});

		injector::MakeInline<0x5022A1, 0x5022A1 + 6>([](injector::reg_pack& regs)
		{
			CAEVehicleAudioEntity* audioEntity = (CAEVehicleAudioEntity*)regs.ecx;
			CVehicle* veh = (CVehicle*)regs.edi;

			if (CWorld::Players[1].m_pPed && veh->m_pDriver == CWorld::Players[1].m_pPed) {
				CAEVehicleAudioEntity::s_pPlayerDriver = CWorld::Players[1].m_pPed;
				audioEntity->m_bInhibitAccForLowSpeed = false;
			}
			else {
				if (CWorld::Players[0].m_pPed && veh->m_pDriver == CWorld::Players[0].m_pPed) {
					CAEVehicleAudioEntity::s_pPlayerDriver = CWorld::Players[0].m_pPed;
				}
			}

			regs.ecx = (uint32_t)CAEVehicleAudioEntity::s_pPlayerDriver;
		});

	}



	if (ReadIniBool(ini, &lg, "Gameplay", "NoDoorCamera")) {
		WriteMemory<uint16_t>(0x440390, 0x46EB, true);
	}

	if (!inSAMP && ReadIniInt(ini, &lg, "Gameplay", "MaxStarToCreateEmVeh", &i)) {
		WriteMemory<uint8_t>(0x42F9F8 + 3, i, true);
	}

	if (!inSAMP && ReadIniInt(ini, &lg, "Gameplay", "PassTimeWasted", &i)) {
		WriteMemory<uint32_t>(0x442FCA + 1, i, true);
	}

	if (!inSAMP && ReadIniInt(ini, &lg, "Gameplay", "PassTimeBusted", &i)) {
		WriteMemory<uint32_t>(0x443375 + 1, i, true);
	}

	if (!inSAMP && ReadIniInt(ini, &lg, "Gameplay", "PassTimeSaved", &i)) {
		WriteMemory<uint32_t>(0x618F9B + 1, i, true);
	}

	if (ReadIniBool(ini, &lg, "Gameplay", "NoCinematicCam")) {
		WriteMemory<uint8_t>(0x005281EF + 2, 4, true);
		WriteMemory<uint32_t>(0x00528210, 4, true);
		WriteMemory<uint8_t>(0x52A52F, 0xAB, true); //EnableTrainCams
	}

	if (!inSAMP && ReadIniFloat(ini, &lg, "Gameplay", "HeliRotorSpeed", &f)) {
		G_HeliRotorSpeed = f;
		WriteMemory<float*>(0x6C4EFE + 2, &G_HeliRotorSpeed, true);
	}

	if (!inSAMP && ReadIniInt(ini, &lg, "Gameplay", "SniperBulletLife", &i)) {
		WriteMemory<uint32_t>(0x7360A2, i, true); //1000
		WriteMemory<uint32_t>(0x73AFB3, (i * 0.75f), true); //750
		WriteMemory<uint32_t>(0x726CE2, (i * 0.3f), true); //300
	}

	if (!inSAMP && ReadIniInt(ini, &lg, "Gameplay", "RocketLife", &i)) {
		WriteMemory<uint32_t>(0x738091 + 2, i, true);
	}

	if (!inSAMP && ReadIniInt(ini, &lg, "Gameplay", "HSRocketLife", &i)) {
		WriteMemory<uint32_t>(0x7380AA + 1, i, true);
	}

	if (!inSAMP && ReadIniFloat(ini, &lg, "Gameplay", "HSRocketSpeed", &f)) {
		WriteMemory<float>(0x7380B3 + 4, f, true);
	}

	if (!inSAMP && ReadIniFloat(ini, &lg, "Gameplay", "RocketSpeed", &f)) {
		WriteMemory<float>(0x73809B + 4, f, true);
	}

	if (ReadIniBool(ini, &lg, "Gameplay", "NoPedVehDive")) {
		WriteMemory<uint32_t>(0x4C1197, 0xFFFF10E9, true);
		WriteMemory<uint8_t>(0x4C119B, 0xFF, true);

		WriteMemory<uint32_t>(0x4C1235, 0xFFFE72E9, true);
		WriteMemory<uint8_t>(0x4C1239, 0xFF, true);
	}

	if (ReadIniBool(ini, &lg, "Gameplay", "NoPedVehHandsUp")) {
		WriteMemory<uint32_t>(0x4C1175, 0, true);
	}

	if (ReadIniFloat(ini, &lg, "Gameplay", "VehCamHeightOffset", &f)) {
		G_VehCamHeightOffset = f;
		injector::MakeInline<0x524776, 0x524776 + 6>([](injector::reg_pack& regs)
		{
			regs.ecx = *(uint32_t*)(regs.edi + 0x4C8); //original code mov ecx, [edi+4C8h]
			*(float*)(regs.esp + 0x78 - 0x60) += G_VehCamHeightOffset;
		});
	}

	if (!inSAMP && ReadIniBool(ini, &lg, "Gameplay", "DuckAnyWeapon")) {
		injector::MakeInline<0x692653>([](injector::reg_pack& regs)
		{
			CWeaponInfo* weaponInfo = (CWeaponInfo*)regs.eax;
			if (weaponInfo->m_dwAnimGroup != 29 && weaponInfo->m_dwAnimGroup != 30) {
				*(uintptr_t*)(regs.esp - 0x4) = 0x69267B;
			}
			else {
				*(uintptr_t*)(regs.esp - 0x4) = 0x692677;
			}
		});
	}

	/*if (!inSAMP && ReadIniFloat(ini, &lg, "Gameplay", "WeaponRangeMult", &f)) {
		G_WeaponRangeMult = f;
		WriteMemory<float*>(0x73B421, &G_WeaponRangeMult, true);
		G_WeaponRangeMult2 = f * 2.0f;
		WriteMemory<float*>(0x73B40D, &G_WeaponRangeMult2, true);
		G_WeaponRangeMult3 = f * 3.0f;
		WriteMemory<float*>(0x73B41A, &G_WeaponRangeMult3, true);
	}*/

	if (!inSAMP && ReadIniBool(ini, &lg, "Gameplay", "GetOffJetpackOnAir")) {
		MakeNOP(0x67E821, 17, true);
	}

	if (!inSAMP && ReadIniBool(ini, &lg, "Gameplay", "NoSamSite")) {
		WriteMemory<uint8_t>(0x5A07D0, 0xC3, true);
		G_NoSamSite = true;
	}
	else G_NoSamSite = false;

	if (ReadIniBool(ini, &lg, "Gameplay", "SmoothAimIK")) {
		G_SmoothAimIK = true;
	}
	else G_SmoothAimIK = false;

	if (ReadIniBool(ini, &lg, "Gameplay", "FixDoubleSkidmark")) {
		patch::RedirectCall(0x6AB1FD, PreRender_AddSingleWheelParticles_FixDouble);
		patch::RedirectCall(0x6AB2B7, PreRender_AddSingleWheelParticles_FixDouble);
		patch::RedirectCall(0x6AB136, PreRender_AddSingleWheelParticles_FixDouble);
	}


	// -- Densities
	if (!G_NoDensities) {
		if (ReadIniFloat(ini, &lg, "Densities", "VehLodDist", &f)) {
			G_VehLodDist = f;
			WriteMemory<float*>(0x732924 + 2, &G_VehLodDist, true);
		}

		if ((!inSAMP || (inSAMP && dtSAMP)) && ReadIniFloat(ini, &lg, "Densities", "VehDrawDist", &f)) {
			G_VehDrawDist = f;
			WriteMemory<float*>(0x73293E + 2, &G_VehDrawDist, true);
		}

		if (ReadIniFloat(ini, &lg, "Densities", "CullDistNormalComps", &f)) {
			G_CullDistNormalComps = f;
			WriteMemory<float*>(0x7328CA + 2, &G_CullDistNormalComps, true);
		}

		if ((!inSAMP || (inSAMP && dtSAMP)) && ReadIniFloat(ini, &lg, "Densities", "VehOccupDrawDist", &f)) {
			G_VehOccupDrawDist = f;
			WriteMemory<float*>(0x5E77C6 + 2, &G_VehOccupDrawDist, true);
			G_VehOccupDrawDist_Boat = f * 1.6f;
			WriteMemory<float*>(0x5E77BE + 2, &G_VehOccupDrawDist_Boat, true);
		}

		if (ReadIniFloat(ini, &lg, "Densities", "CullDistBigComps", &f)) {
			G_CullDistBigComps = f;
			WriteMemory<float*>(0x7328F2 + 2, &G_CullDistBigComps, true);
		}

		if (ReadIniFloat(ini, &lg, "Densities", "VehMultiPassDist", &f)) {
			G_VehMultiPassDist = f;
			WriteMemory<float*>(0x73290A + 2, &G_VehMultiPassDist, true);
		}

		if ((!inSAMP || (inSAMP && dtSAMP)) && ReadIniFloat(ini, &lg, "Densities", "PedDrawDist", &f)) {
			G_PedDrawDist = f;
			WriteMemory<float*>(0x73295C + 2, &G_PedDrawDist, true);
		}

		if (!inSAMP) {

			if (ReadIniFloat(ini, &lg, "Densities", "RoadblockSpawnDist", &f)) {
				G_RoadblockSpawnDist = f;
				G_RoadblockSpawnDist_NEG = (f *= -1.0f);
				WriteMemory<float*>(0x462B1D + 2, &G_RoadblockSpawnDist_NEG, true);
				WriteMemory<float*>(0x462B76 + 2, &G_RoadblockSpawnDist_NEG, true);
				WriteMemory<float*>(0x462B32 + 2, &G_RoadblockSpawnDist, true);
				WriteMemory<float*>(0x462B8B + 2, &G_RoadblockSpawnDist, true);
				WriteMemory<float*>(0x462BB0 + 2, &G_RoadblockSpawnDist, true);
			}

			if (ReadIniFloat(ini, &lg, "Densities", "GangWaveMaxSpawnDist", &f)) {
				WriteMemory<float>(0x444AFA + 1, f, true);
				WriteMemory<float>(0x444AFF + 1, f, true);
			}

			if (ReadIniFloat(ini, &lg, "Densities", "GangWaveMinSpawnDist", &f)) {
				G_GangWaveMinSpawnDist = f;
				WriteMemory<float*>(0x444BB2 + 2, &G_GangWaveMinSpawnDist, true);
			}

			if (ReadIniFloat(ini, &lg, "Densities", "PedDensityExt", &f))
			{
				G_PedDensityExt = f;
			}
			else G_PedDensityExt = -1.0f;

			if (ReadIniFloat(ini, &lg, "Densities", "PedDensityInt", &f)) {
				if (f > 1.0f) f = 1.0f;
				G_PedDensityInt = f;
			}
			else G_PedDensityInt = -1.0f;

			if (G_PedDensityExt >= 0.0f || G_PedDensityInt >= 0.0f) {
				injector::MakeInline<0x614937, 0x614937 + 6>([](injector::reg_pack& regs)
				{
					if (CGame::currArea != 0) {
						float dens;
						if (G_PedDensityInt >= 0.0f)
						{
							dens = *(float*)0x008D2530 * G_PedDensityInt;
						}
						else {
							dens = *(float*)0x008D2530;
						}
						if (dens > 1.0) dens = 1.0;
						asm_fmul(dens);
					}
					else {
						if (G_PedDensityExt >= 0.0f)
						{
							asm_fmul(*(float*)0x008D2530 * G_PedDensityExt);
						}
						else asm_fmul(*(float*)0x008D2530);
					}
				});
			}


			if (ReadIniFloat(ini, &lg, "Densities", "VehDensity", &f))
			{
				G_VehDensity = f;
				injector::MakeInline<0x4300E9>([](injector::reg_pack& regs)
				{
					if (G_VehDensity != -1.0f)
					{
						*(float*)&regs.eax = *(float*)0x008A5B20 * G_VehDensity;
					}
					else *(float*)&regs.eax = *(float*)0x008A5B20;
				});
			}
			else G_VehDensity = -1.0f;

			if (ReadIniFloat(ini, &lg, "Densities", "VehDespawnOnScr", &f)) {
				G_VehDespawnOnScr = f;
				WriteMemory<float*>(0x4250F0, &G_VehDespawnOnScr, true);
			}

			if (ReadIniFloat(ini, &lg, "Densities", "VehDespawnOffScr", &f)) {
				WriteMemory<float>(0x42510F, f, true);
				f *= 0.6f;
				WriteMemory<float>(0x4250E2, f, true);
			}

			if (ReadIniFloat(ini, &lg, "Densities", "PedDespawnOnScr", &f)) {
				G_PedDespawnOnScr = f;
				WriteMemory<float*>(0x6120FF, &G_PedDespawnOnScr, true);
			}

			if (ReadIniFloat(ini, &lg, "Densities", "PedDespawnOffScr", &f)) {
				G_PedDespawnOffScr = f;
				WriteMemory<float*>(0x612128, &G_PedDespawnOffScr, true);
			}

			if (ReadIniFloat(ini, &lg, "Densities", "PedSpawnOnScr", &f)) {
				WriteMemory<float>(0x86D284, f, true);
			}

			if (ReadIniFloat(ini, &lg, "Densities", "PedSpawnOffScr", &f)) {
				WriteMemory<float>(0x86C850, f, true);
			}

			if (ReadIniFloat(ini, &lg, "Densities", "TowelSpawnOnScr", &f)) {
				WriteMemory<float>(0x86D288, f, true);
			}

			if (ReadIniFloat(ini, &lg, "Densities", "TowelSpawnOffScr", &f)) {
				G_TowelSpawnOffScr = f;
				WriteMemory<float*>(0x615E81 + 2, &G_TowelSpawnOffScr, true);
			}

			if (ReadIniFloat(ini, &lg, "Densities", "TrainSpawnDistance", &f)) {
				G_TrainSpawnDistance = f;
				WriteMemory<float*>(0x6F7AA8, &G_TrainSpawnDistance, true);
			}
		}
		else {
			G_VehDensity = -1.0f;
			G_PedDensityExt = -1.0f;
			G_PedDensityInt = -1.0f;
		}

		if ((!inSAMP || (inSAMP && dtSAMP)) && ReadIniFloat(ini, &lg, "Densities", "MinGrassDist", &f)) {
			WriteMemory<float>(0x5DDB42 + 1, f, true);
		}

		if ((!inSAMP || (inSAMP && dtSAMP)) && ReadIniFloat(ini, &lg, "Densities", "MaxGrassDist", &f)) {
			WriteMemory<float>(0x5DDB3D + 1, f, true);
		}

		if ((!inSAMP || (inSAMP && dtSAMP)) && ReadIniFloat(ini, &lg, "Densities", "PedWeaponDrawDist", &f)) {
			G_PedWeaponDrawDist = f;
			if (G_PedWeaponDrawDist != 1.0f) {
				MakeJMP(0x7336A8, PedWeaponDrawDist_ASM, true);
			}
		}

		if (!inSAMP && ReadIniInt(ini, &lg, "Densities", "DeadPedDeleteDelay", &i)) {
			WriteMemory<uint32_t>(0x612026, i, true);
		}

		if (ReadIniFloat(ini, &lg, "Densities", "ShadDist_Vehicles", &f)) {
			G_ShadDist_Vehicles = f;
			WriteMemory<float*>(0x70BEB6, &G_ShadDist_Vehicles, true);
			G_ShadDist_Vehicles_Sqr = G_ShadDist_Vehicles * G_ShadDist_Vehicles;
			WriteMemory<float*>(0x70BEA7, &G_ShadDist_Vehicles_Sqr, true);
		}

		if (ReadIniFloat(ini, &lg, "Densities", "ShadDist_SmallPlanes", &f)) {
			G_ShadDist_SmallPlanes = f;
			WriteMemory<float*>(0x70BE79, &G_ShadDist_SmallPlanes, true);
			G_ShadDist_SmallPlanes_Sqr = G_ShadDist_SmallPlanes * G_ShadDist_SmallPlanes;
			WriteMemory<float*>(0x70BE88, &G_ShadDist_SmallPlanes_Sqr, true);
		}

		if (ReadIniFloat(ini, &lg, "Densities", "ShadDist_BigPlanes", &f)) {
			G_ShadDist_BigPlanes = f;
			WriteMemory<float*>(0x0070BE9F, &G_ShadDist_BigPlanes, true);
			G_ShadDist_BigPlanes_Sqr = G_ShadDist_BigPlanes * G_ShadDist_BigPlanes;
			WriteMemory<float*>(0x0070BE90, &G_ShadDist_BigPlanes_Sqr, true);
		}

		if (ReadIniFloat(ini, &lg, "Densities", "ShadDist_Peds", &f)) {
			WriteMemory<float>(0x8D5240, f, true);
			WriteMemory<float>(0xC4B6B0, (f * f), true);
		}

		if (ReadIniFloat(ini, &lg, "Densities", "LighDist_VehLight", &f)) {
			G_ShadDist_CarLight = f;
			G_ShadDist_CarLight_Sqr = (G_ShadDist_CarLight * G_ShadDist_CarLight);
			WriteMemory<float*>(0x0070C582, &G_ShadDist_CarLight_Sqr, true);

			G_ShadDist_CarLight_Mid = (G_ShadDist_CarLight * 0.75f);
			WriteMemory<float*>(0x0070C5E4, &G_ShadDist_CarLight_Mid, true);

			G_ShadDist_CarLight -= G_ShadDist_CarLight_Mid;
			G_ShadDist_CarLight /= 3.0f;
			G_ShadDist_CarLight_Min = G_ShadDist_CarLight_Mid - G_ShadDist_CarLight;
			WriteMemory<float*>(0x0070C5FE, &G_ShadDist_CarLight_Min, true);

			G_ShadDist_CarLight_Scale = 1.0f / (G_ShadDist_CarLight * 4.0f);
			WriteMemory<float*>(0x0070C608, &G_ShadDist_CarLight_Scale, true);
		}

		if (ReadIniFloat(ini, &lg, "Densities", "ShadHeiLim_HeadLight", &f)) {
			WriteMemory<float>(0x0070C6B7, f, true);
			WriteMemory<float>(0x0070C719, f, true);
		}

		if (ReadIniFloat(ini, &lg, "Densities", "ShadHeiLim_Vehicles", &f)) {
			WriteMemory<float>(0x0070BDD6, f, true);
			WriteMemory<float>(0x0070C2F8, f, true);
		}

		if (ReadIniFloat(ini, &lg, "Densities", "ShadHeiLim_Aircraft", &f)) {
			WriteMemory<float>(0x0070C0ED, f, true);
			WriteMemory<float>(0x0070C113, f, true);
			WriteMemory<uint16_t>(0x0070C0D9, 0x0EEB, true);
			WriteMemory<uint16_t>(0x0070C0FF, 0x0EEB, true);
		}

		if (!inSAMP && ReadIniInt(ini, &lg, "Densities", "VehLimit", &i)) {
			WriteMemory<uint8_t>(0x434237, 0x73, true); // change condition to unsigned (0-255)
			WriteMemory<uint8_t>(0x434224, i, true);
			WriteMemory<uint8_t>(0x484D19, 0x83, true); // change condition to unsigned (0-255)
			WriteMemory<uint8_t>(0x484D17, i, true);
		}

		if (!inSAMP && ReadIniInt(ini, &lg, "Densities", "PedLimit", &i)) {
			WriteMemory<uint32_t>(0x8D2538, i, false);
		}

		if (ReadIniFloat(ini, &lg, "Densities", "FootstepsDist", &f)) {
			G_FootstepsDist = f;
			WriteMemory<float*>(0x5E5550, &G_FootstepsDist, true);
		}

		if (ReadIniInt(ini, &lg, "Densities", "FootstepsTime", &i)) {
			WriteMemory<uint16_t>(0x5E5597, i, true);
		}

		if (ReadIniInt(ini, &lg, "Densities", "FootstepsTimeBlood", &i)) {
			WriteMemory<uint32_t>(0x5E546C, i, true);
		}

		if (ReadIniInt(ini, &lg, "Densities", "TimeExplosionTex", &i)) {
			WriteMemory<int>(0x73743E, i, true);
		}

		if (ReadIniInt(ini, &lg, "Densities", "TimeBloodstainNPC", &i)) {
			WriteMemory<int>(0x49ED69, i, true);
		}

		if (ReadIniInt(ini, &lg, "Densities", "TimeBloodstainPlayer", &i)) {
			WriteMemory<int>(0x49EDC7, i, true);
		}

		if (ReadIniInt(ini, &lg, "Densities", "TimeBloodpoolTex", &i)) {
			WriteMemory<int>(0x630E79, i, true);
		}

		if (ReadIniInt(ini, &lg, "Densities", "TimeBleedingTex", &i)) {
			WriteMemory<int>(0x5E943C, i, true);
		}

		if ((!inSAMP || (inSAMP && dtSAMP)) && ReadIniFloat(ini, &lg, "Densities", "PickupsDrawDist", &f)) {
			G_PickupsDrawDist = f;
			WriteMemory<float*>(0x70C0ED, &G_PickupsDrawDist, true);
		}

		if (!inSAMP && ReadIniInt(ini, &lg, "Densities", "TimePickupShort", &i)) {
			WriteMemory<int>(0x457236, i, true);
		}

		if (!inSAMP && ReadIniInt(ini, &lg, "Densities", "TimePickupLong", &i)) {
			WriteMemory<int>(0x457243, i, true);
		}

		if (!inSAMP && ReadIniInt(ini, &lg, "Densities", "TimePickupMoney", &i)) {
			WriteMemory<int>(0x457250, i, true);
		}

		if (ReadIniBool(ini, &lg, "Densities", "NoLODduringFly")) {
			MakeNOP(0x5557CF, 7, true);
		}

		if (ReadIniFloat(ini, &lg, "Densities", "ShadDist_AllPerm", &f)) {
			WriteMemory<float>(0x70C995 + 1, f, true); //CShadows::UpdatePermanentShadows
			WriteMemory<float>(0x70C9F3 + 1, f, true); //CShadows::UpdatePermanentShadows
			WriteMemory<float>(0x630E26 + 1, f, true); //CTaskSimpleDead::ProcessPed
		}

		if (ReadIniFloat(ini, &lg, "Densities", "ShadDist_Poles", &f)) {
			WriteMemory<float>(0x70C88A + 1, f, true);
		}

		if (ReadIniFloat(ini, &lg, "Densities", "LighDist_TrafficL", &f)) {
			WriteMemory<float>(0x49DF79 + 1, f, true);
		}

		if (ReadIniFloat(ini, &lg, "Densities", "LighDist_Fire", &f)) {
			WriteMemory<float>(0x53B5E1 + 1, f, true);
		}

		if (ReadIniFloat(ini, &lg, "Densities", "LighDist_Entities", &f)) {
			WriteMemory<float>(0x6FD3A5 + 1, f, true);
			WriteMemory<float>(0x6FD44E + 1, f, true);
		}

		/*
		if (ReadIniFloat(ini, &lg, "Densities", "PropCollDist", &f)) {
			G_PropCollDist_NEG = f * -1.0f;
			G_PropCollDist_POS = f;

			WriteMemory<float*>(0x41047A + 2, &G_PropCollDist_NEG, true);
			WriteMemory<float*>(0x41048C + 2, &G_PropCollDist_POS, true);
			WriteMemory<float*>(0x41049E + 2, &G_PropCollDist_NEG, true);
			WriteMemory<float*>(0x4104B1 + 2, &G_PropCollDist_POS, true);
			WriteMemory<float*>(0x4084F5 + 2, &G_PropCollDist_POS, true);
			WriteMemory<float*>(0x4051BD + 2, &G_PropCollDist_POS, true);
			WriteMemory<float*>(0x4051EE + 2, &G_PropCollDist_POS, true);
			WriteMemory<float*>(0x5A2EE3 + 2, &G_PropCollDist_POS, true);
			WriteMemory<float*>(0x5A2EF0 + 4, &G_PropCollDist_POS, true);

			//WriteMemory<float>(0x858A14, G_PropCollDist_POS, true);
			//WriteMemory<float>(0x858BA0, G_PropCollDist_NEG, true);
		}

		if (ReadIniFloat(ini, &lg, "Densities", "PlantDist", &f)) {
			G_PlantDist = f;
			WriteMemory<float*>(0x5DC5F7 + 2, &G_PlantDist, true);
			WriteMemory<float*>(0x5DC64D + 2, &G_PlantDist, true);
			WriteMemory<float*>(0x5DC695 + 2, &G_PlantDist, true);
			WriteMemory<float*>(0x5DC6E6 + 2, &G_PlantDist, true);
		}
		WriteMemory<float>(0x86BF20, 1000000.0f, true);
		WriteMemory<float>(0x859AA4, 20000.0f, true);
		WriteMemory<float>(0x85F074, 500.0f, true);
		*/

	}


	// -- Skid marks
	if (ReadIniInt(ini, &lg, "Skid Marks", "SkidRate", &i)) {
		WriteMemory<uint8_t>(0x720B22, i, true);
	}

	if (ReadIniFloat(ini, &lg, "Skid Marks", "SkidHeight", &f)) {
		G_SkidHeight = f;
		WriteMemory<float*>(0x720819, &G_SkidHeight, true);
	}

	if (ReadIniInt(ini, &lg, "Skid Marks", "SkdVeryShort_FadeStr", &i)) {
		WriteMemory<uint16_t>(0x7205F6, i, true);
	}
	if (ReadIniInt(ini, &lg, "Skid Marks", "SkdVeryShort_FadeEnd", &i)) {
		WriteMemory<uint16_t>(0x7205FF, i, true);
	}
	if (ReadIniInt(ini, &lg, "Skid Marks", "SkdShort_FadeStr", &i)) {
		WriteMemory<uint16_t>(0x72060E, i, true);
	}
	if (ReadIniInt(ini, &lg, "Skid Marks", "SkdShort_FadeEnd", &i)) {
		WriteMemory<uint16_t>(0x720617, i, true);
	}
	if (ReadIniInt(ini, &lg, "Skid Marks", "SkdMedium_FadeStr", &i)) {
		WriteMemory<uint16_t>(0x72061F, i, true);
	}
	if (ReadIniInt(ini, &lg, "Skid Marks", "SkdMedium_FadeEnd", &i)) {
		WriteMemory<uint16_t>(0x720628, i, true);
	}
	if (ReadIniInt(ini, &lg, "Skid Marks", "SkdLong_FadeStr", &i)) {
		WriteMemory<uint16_t>(0x720CA6, i, true);
	}
	if (ReadIniInt(ini, &lg, "Skid Marks", "SkdLong_FadeEnd", &i)) {
		WriteMemory<uint16_t>(0x720CAB, i, true);
	}
	if (ReadIniInt(ini, &lg, "Skid Marks", "SkdThread_FadeStr", &i)) {
		WriteMemory<uint16_t>(0x720AF4, i, true);
	}
	if (ReadIniInt(ini, &lg, "Skid Marks", "SkdThread_FadeEnd", &i)) {
		WriteMemory<uint16_t>(0x720AF9, i, true);
	}



	// -- Interface
	if (!inSAMP && ReadIniBool(ini, &lg, "Interface", "NoStatsUpdateBox")) {
		WriteMemory<uint8_t>(0x55B980, 0xC3, true);
		WriteMemory<uint8_t>(0x559760, 0xC3, true);
	}
	if (ReadIniBool(ini, &lg, "Interface", "NoMoneyZeros")) {
		WriteMemory<char*>(0x58F4C8, G_NoMoneyZeros_Pos, true); //positive
		WriteMemory<char*>(0x58F50A, G_NoMoneyZeros_Neg, true); //negative
	}
	if ((!inSAMP || (inSAMP && rpSAMP)) && ReadIniBool(ini, &lg, "Interface", "NoTargetBlip")) {
		WriteMemory<uint8_t>(0x53E1EC, 0xEB, true);
	}
	if ((!inSAMP || (inSAMP && rpSAMP)) && ReadIniBool(ini, &lg, "Interface", "NoCrosshair")) {
		MakeNOP(0x58FBBF, 5);
	}
	if (ReadIniBool(ini, &lg, "Interface", "ColorableRadarDisc")) {
		WriteMemory<uint8_t>(0x58A9A2, 255, true);
		WriteMemory<uint8_t>(0x58A99A, 255, true);
		WriteMemory<uint8_t>(0x58A996, 255, true);
		WriteMemory<uint8_t>(0x58A8EE, 255, true);
		WriteMemory<uint8_t>(0x58A8E6, 255, true);
		WriteMemory<uint8_t>(0x58A8DE, 255, true);
		WriteMemory<uint8_t>(0x58A89A, 255, true);
		WriteMemory<uint8_t>(0x58A896, 255, true);
		WriteMemory<uint8_t>(0x58A894, 255, true);
		WriteMemory<uint8_t>(0x58A798, 255, true);
		WriteMemory<uint8_t>(0x58A790, 255, true);
		WriteMemory<uint8_t>(0x58A78E, 255, true);
	}


	// -- Audio
	if (ReadIniBool(ini, &lg, "Audio", "NoAmbientGuns")) {
		MakeNOP(0x507818, 11, true);
	}
	if (ReadIniBool(ini, &lg, "Audio", "NoHelpBoxSound")) {
		WriteMemory<uint32_t>(0x58B81F, 0x900CC483, true);
		MakeNOP(0x58B823, 1, true);
	}
	if (ReadIniBool(ini, &lg, "Audio", "SirenOnWithoutDriver")) {
		MakeJMP(0x004F9DA4, SirenOnWithoutDriver_ASM, true);
		MakeNOP(0x004F9DA9, 5, true);
	}



	if (!inSAMP) {

		// -- Wanted

		/*if (ReadIniInt(ini, &lg, "Wanted", "RoadBlockVeh_4", &i)) {
			WriteMemory<uint32_t>(0x461BE7, i, true);
		}
		if (ReadIniInt(ini, &lg, "Wanted", "RoadBlockVeh_5", &i)) {
			WriteMemory<uint32_t>(0x461BCC, i, true);
		}
		if (ReadIniInt(ini, &lg, "Wanted", "RoadBlockVeh_6", &i)) {
			WriteMemory<uint32_t>(0x461BB1, i, true);
		}*/
		if (ReadIniInt(ini, &lg, "Wanted", "MaxHydras", &i)) {
			WriteMemory<uint8_t>(0x6CD91C, i, true);
		}
		if (ReadIniInt(ini, &lg, "Wanted", "DelayHydras", &i)) {
			WriteMemory<uint32_t>(0x6CD8E0, i, true);
		}
		if (ReadIniInt(ini, &lg, "Wanted", "MilitaryZoneStar", &i)) {
			WriteMemory<uint8_t>(0x72DF2A, i, true);
		}
		if (ReadIniInt(ini, &lg, "Wanted", "BannedSFZoneLevel", &i)) {
			WriteMemory<uint8_t>(0x4418B0 + 1, i, true);
		}
		if (ReadIniInt(ini, &lg, "Wanted", "BannedLVZoneLevel", &i)) {
			WriteMemory<uint8_t>(0x44183C + 1, i, true);
		}
		if (ReadIniFloat(ini, &lg, "Wanted", "RandomHeliFireTime", &f)) {
			WriteMemory<float>(0x8D33A4, f, false);
		}
		if (ReadIniBool(ini, &lg, "Wanted", "NoCopHeliShots")) {
			MakeNOP(0x006C7773, 5, true);
			MakeNOP(0x006C777B, 18, true);
		}



		// -- Hydra
		if (ReadIniInt(ini, &lg, "Hydra", "HydraRocketDelay", &i)) {
			WriteMemory<uint32_t>(0x6D462E, i, true);
			WriteMemory<uint32_t>(0x6D4634, i, true);
		}
		if (ReadIniInt(ini, &lg, "Hydra", "HydraFlareDelay", &i)) {
			WriteMemory<uint32_t>(0x6E351B, i, true);
		}
		if (ReadIniInt(ini, &lg, "Hydra", "HydraLockDelay", &i)) {
			WriteMemory<uint32_t>(0x6E363A, i, true);
			WriteMemory<uint32_t>(0x6E36FB, i, true);
		}
		if (ReadIniBool(ini, &lg, "Hydra", "NoHydraSpeedLimit")) {
			WriteMemory<uint8_t>(0x6DADE8, 0xEB, true);
		}

		// -- Rhino
		if (ReadIniInt(ini, &lg, "Rhino", "RhinoFireDelay", &i)) {
			WriteMemory<uint32_t>(0x6AED10, i, true);
		}
		if (ReadIniFloat(ini, &lg, "Rhino", "RhinoFirePush", &f)) {
			f *= -1.0;
			WriteMemory<float>(0x871080, f, true);
		}
		if (ReadIniFloat(ini, &lg, "Rhino", "RhinoFireRange", &f)) {
			G_RhinoFireRange = f;
			WriteMemory<float*>(0x6AEF42, &G_RhinoFireRange, true);
			WriteMemory<float*>(0x6AEF56, &G_RhinoFireRange, true);
			WriteMemory<float*>(0x6AEF65, &G_RhinoFireRange, true);
		}
		if (ReadIniInt(ini, &lg, "Rhino", "RhinoFireType", &i)) {
			WriteMemory<uint8_t>(0x6AF0AC, i, true);
		}


		// -- World
		if (ReadIniBool(ini, &lg, "World", "LockHour")) {
			MakeNOP(0x53BFBD, 5, true);
		}

		if (ReadIniFloat(ini, &lg, "World", "Gravity", &f)) {
			WriteMemory<float>(0x863984, f, false);
		}

		if (ReadIniInt(ini, &lg, "World", "FreezeWeather", &i)) {
			G_FreezeWeather = i;
		}
		else {
			if (G_FreezeWeather >= 0) {
				CWeather::ReleaseWeather();
			}
			G_FreezeWeather = -1;
		}

		if (ReadIniBool(ini, &lg, "World", "NoWaterPhysics")) {
			WriteMemory<uint8_t>(0x6C2759, 1, true);
		}
	}
	else {
		G_FreezeWeather = -1;
	}

	// -- Post

	if (bReadOldINI) {

		if (numOldCfgNotFound > 0)
		{
			if (lang == languages::PT)
				lg << "\nAviso: " << numOldCfgNotFound << " configurações não foram encontradas no .ini antigo. Verifique acima.\n";
			else
				lg << "\nWarning: " << numOldCfgNotFound << " configurations has not found on old ini. Check it above.\n";
		}

		bErrorRename = false;

		try {
			filesystem::rename(PLUGIN_PATH("MixSets old.ini"), PLUGIN_PATH("MixSets backup.ini"));
		}
		catch (std::filesystem::filesystem_error& e) {
			if (lang == languages::PT)
			{
				lg << "\nERRO: Não foi possível renomear o arquivo 'MixSets old.ini'. Provavelmente você está com o jogo instalado na pasta Arquivos de Programas ou o arquivo está em uso.\n";
				lg << "Mova seu jogo para outra pasta para o melhor funcionamento deste e outros mods. Ou verifique o arquivo, tente de novo, renomei-o ou delete-o manualmente.\n";
			}
			else {
				lg << "\nERROR: Unable to rename 'MixSets old.ini' file. You probably have the game installed in the Program Files folder or the file is in use.\n";
				lg << "Move your game to another folder for the best working of this and other mods. Or check the file, try again, renamed it, or delete it manually.\n";
			}
			lg << "Error message: " << e.what() << "\n";
			bErrorRename = true;
		}


	}

	lg.flush();
}