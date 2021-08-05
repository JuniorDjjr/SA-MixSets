/*
	MixSets by Junior_Djjr and entire GTA 3D Era community
*/
#include "MixSets.h"
#include "ReadIni.h"
#include "Common.h"

// Game
#include "extensions/ScriptCommands.h"
#include "CPedIntelligence.h"
#include "CWeather.h"
#include "CTheScripts.h"
#include "CTimer.h"
#include "CTimeCycle.h"
#include "CEntryExitManager.h"
#include "CGeneral.h"
#include <game_sa\common.h>
#include "CMessages.h"
#include "CGame.h"
#include "IMFX/Gunflashes.h"

// Other
#include "..\injector\assembly.hpp"
#include "IniReader/IniReader.h"
#include "TestCheat.h"

using namespace plugin;
using namespace injector;
using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////////////

class MixSets mixSets;

MixSets::MixSets()
{
	lg.open("MixSets.log", fstream::out | fstream::trunc);
	lg << "v4.3.2" << "\n";
	lg.flush();
	 

	// -- Default values
	bEnabled = false;

	Read = false;

	G_VehFlipDamage = 0.1f;

	RETURN_FixMouseStuck = 0x74542B;
	altRETURN_FixMouseStuck = 0x745433;

	forceUpdateQualityFuncs = true;
	bProcessOnceOnScripts = false;
	bProcessOnceAfterIntro = false;
	bPlayerRenderWeaponInVehicleLastFrame = false;
	bPlayerTwoRenderWeaponInVehicleLastFrame = false;
	bNoCLEO = true;
	bOnEmergencyMissionLastFrame = false;
	curQuality = -1;
	lastQuality = -1;
	G_NoEmergencyMisWanted_MaxWantedLevel = -1;
	G_Backup_WavesRadius = 48;

	numOldCfgNotFound = 0;

	zero = 0.0;
	G_WeaponIconScaleFix = 47.0f;

	_flt_2_4 = 2.4f;
	_flt_1_8 = 1.8f;

	secPlayerVehicle = nullptr;

	strcpy_s(G_NoMoneyZeros_Pos, "$%d");
	strcpy_s(G_NoMoneyZeros_Neg, "-$%d");

	// -- End of Default values


	if (ReadMemory<uint8_t>(0x400088, true) != 0xCA)
	{
		lg << "ERROR: Game version not supported. Download GTA SA Crack 1.0 US." << "\n\n";
		lg.flush();
		MessageBoxA(0, "Game version not supported. Download GTA SA Crack 1.0 US.", "MixSets", 0);
		return;
	}

	Events::initRwEvent += []
	{
		if (!GetModuleHandleA("CLEO.asi")) {
			lg << "ERROR: CLEO isn't installed. It's required for some MixSets features." << "\n\n";
			bNoCLEO = true;
		}
		else bNoCLEO = false;

		if (GetModuleHandleA("SAMP.dll")) {
			lg << "SAMP = true" << "\n";
			inSAMP = true;
		}
		else inSAMP = false;

		if (GetModuleHandleA("IMFX.asi")) {
			lg << "IMFX = true" << "\n";
			bIMFX = true;
		}
		else bIMFX = false;

		if (GetModuleHandleA("GunFuncs.asi")) {
			lg << "GunFuncs = true" << "\n";
			bGunFuncs = true;
		}
		else bGunFuncs = false;

		if (GetModuleHandleA("III.VC.SA.LimitAdjuster.asi")) {
			lg << "Open Limit Adjuster = true" << "\n";
			bOLA = true;
		}
		else bOLA = false;

		hVehFuncs = GetModuleHandleA("VehFuncs.asi");
		if (hVehFuncs) {
			lg << "VehFuncs = true" << "\n";
			bVehFuncs = true;
			pVehFuncs_Ext_GetDoubleWheelOffset = (VehFuncs_Ext_GetDoubleWheelOffset)GetProcAddress(hVehFuncs, "Ext_GetDoubleWheelOffset");
		}
		else {
			bVehFuncs = false;
			pVehFuncs_Ext_GetDoubleWheelOffset = NULL;
		}
		lg << "\n";

		ReadIni_BeforeFirstFrame();

		if (bEnabled) {
			if (lang == languages::PT)
				lg << "\n" << "Terminado de ler o ini na inicialização do jogo" << "\n\n";
			else
				lg << "\n" << "Done read ini on game init" << "\n\n";
		}

		lg.flush();

		Read = true;
	};

	Events::initScriptsEvent += []
	{
		bProcessOnceOnScripts = true;
		bProcessOnceAfterIntro = true;
	};


	Events::processScriptsEvent.before += [] // Note: gameProcessEvent doesn't work on SAMP
	{
		if (bEnabled)
		{
			if (Read)
			{
				ReadIni();

				if (lang == languages::PT)
					lg << "\n" << "Terminado de ler o ini no primeiro frame." << "\n\n";
				else
					lg << "\n" << "Done read ini on first frame" << "\n\n";
				lg.flush();

				Read = false;
			}

			//////////////////////////////////////////////

			CPed* playerPed = FindPlayerPed(-1); // focused

			if (G_HideWeaponsOnVehicle)
			{
				int playerId = 0;
				while (playerId <= 1)
				{
					CPed* player = FindPlayerPed(playerId);

					if (player)
					{
						if (player->m_nPedFlags.bInVehicle)
						{
							bPlayerRenderWeaponInVehicleLastFrame = true;
							int camMode = TheCamera.m_aCams[0].m_nMode;
							CPad* pad = CPad::GetPad(playerId);
							if (pad->GetLookLeft() || pad->GetLookRight() ||
								(player->m_pVehicle->GetVehicleAppearance() == eVehicleApperance::VEHICLE_APPEARANCE_BIKE && pad->GetCarGunFired()) ||
								camMode == eCamMode::MODE_AIMWEAPON_FROMCAR || camMode == eCamMode::MODE_TWOPLAYER_IN_CAR_AND_SHOOTING)
							{
								player->m_pPlayerData->m_bRenderWeapon = true;
							}
							else
							{
								player->m_pPlayerData->m_bRenderWeapon = false;
							}
						}
						else
						{
							if (bPlayerRenderWeaponInVehicleLastFrame)
							{
								player->m_pPlayerData->m_bRenderWeapon = true;
								bPlayerRenderWeaponInVehicleLastFrame = false;
							}
						}
					}
					playerId++;
				}
			}

			if (G_SmoothAimIK)
			{
				// the movement is weird when starting to aim while running
				// not the best solution, but anyway
				if (playerPed->m_nMoveState <= 4)
				{
					injector::WriteMemory<float>(0x8D2E70, 0.17f, false);
				}
				else
				{
					injector::WriteMemory<float>(0x8D2E70, 0.3f, false);
				}
			}

			if (G_Fix2DGunflash) Gunflashes::ProcessPerFrame();

			if (G_FreezeWeather >= 0 && !inSAMP)
				CWeather::ForceWeatherNow(G_FreezeWeather);

			if (G_ProcessPriority > 0)
				ProcessPriority();

			curQuality = g_fx.GetFxQuality();

			if (curQuality != lastQuality || forceUpdateQualityFuncs)
			{
				forceUpdateQualityFuncs = false;

				if (G_NoStencilShadows)
					ProcessNoStencilShadows();

				if (G_UseHighPedShadows > -1)
					ProcessUseHighPedShadows();
			}

			lastQuality = curQuality;

			//////////////////////////////////////////////


			if (G_OpenedHouses && !inSAMP)
			{
				if (!CEntryExitManager::ms_bBurglaryHousesEnabled)
				{
					CEntryExitManager::EnableBurglaryHouses(true);
				}
			}

			if (G_StreamMemory > 0)
			{
				WriteMemory<uint32_t>(0x8A5A80, G_StreamMemory, false);
			}

			if (G_HowManyMinsInDay > 0 && !inSAMP)
			{
				WriteMemory<uint32_t>(0xB7015C, G_HowManyMinsInDay, false);
			}

			if (G_NoWavesIfCamHeight > 0)
			{
				if (CGame::currArea == 0 && G_NoWavesIfCamHeight >= 0.0f)
				{
					if (TheCamera.GetPosition().z > G_NoWavesIfCamHeight)
					{
						if (*(int*)(0x8D37D0) != 0)
						{
							G_Backup_WavesRadius = *(int*)(0x8D37D0);
							*(int*)(0x8D37D0) = 0;
						}
					}
					else
					{
						if (*(int*)(0x8D37D0) == 0)
						{
							*(int*)(0x8D37D0) = G_Backup_WavesRadius;
						}
					}
				}
			}

			if (G_BoatFoamLightingFix >= 0.0)
			{
				float balance = (CTimeCycle::m_CurrentColours.m_fWaterBlue + CTimeCycle::m_CurrentColours.m_fWaterGreen + CTimeCycle::m_CurrentColours.m_fWaterRed + CTimeCycle::m_CurrentColours.m_fWaterAlpha) / 255.0f / 4.0f;
				balance = powf(balance, 2) + 0.1f;
				balance *= G_BoatFoamLightingFix;
				if (balance > 1.0f) balance = 1.0f;

				// center
				*(float*)0x8D390C = 0.4f * balance;
				*(float*)0x8D391C = 0.4f * balance;

				// sides
				*(float*)0x8D3910 = 1.0f * balance;
				*(float*)0x8D3918 = 1.0f * balance;
			}

			if (!inSAMP && !bNoCLEO)
			{
				if (G_ParaLandingFix)
				{
					unsigned int script;
					Command<GET_SCRIPT_STRUCT_NAMED>("PLCHUTE", &script);
					if (script)
					{
						unsigned char* offset = reinterpret_cast<CRunningScript*>(script)->m_pBaseIP;
						offset += 4948;

						uint32_t test = ReadMemory<uint32_t>(offset + 6);
						if (test == 0x4C4C4146) // default value: not yet fixed; valid script
						{
							/*
								0812: AS_actor - 1 perform_animation "PARA_LAND" IFP "PARACHUTE" framedelta 10.0 loopA 0 lockX 1 lockY 1 lockF 0 time - 2
								12 08 04 FF 0E 09 50 41 52 41 5F 4C 41 4E 44 0E 09 50 41 52 41 43 48 55 54 45 06 00 00 20 41 04 00 04 01 04 01 04 00 04 FE
							*/
							const uint8_t playanim[] = { 0x12, 0x08, 0x04, 0xFF, 0x0E, 0x09, 0x50, 0x41, 0x52, 0x41, 0x5F, 0x4C, 0x41, 0x4E, 0x44, 0x0E, 0x09, 0x50, 0x41, 0x52, 0x41, 0x43, 0x48, 0x55, 0x54, 0x45, 0x06, 0x00, 0x00, 0x20, 0x41, 0x04, 0x00, 0x04, 0x01, 0x04, 0x01, 0x04, 0x00, 0x04, 0xFE };
							memcpy(offset, &playanim, sizeof(playanim));
							offset += sizeof(playanim);

							const uint8_t jump[] = { 0x02, 0x00, 0x01, 0x62, 0xEC, 0xFF, 0xFF }; // 0002: goto -5022
							memcpy(offset, &jump, sizeof(jump));
						}
					}
				}

				if (G_LureRancher)
				{
					unsigned int script;
					Command<GET_SCRIPT_STRUCT_NAMED>("DRIV6", &script);
					if (script)
					{
						unsigned char* offset = reinterpret_cast<CRunningScript*>(script)->m_pBaseIP;
						offset += 4445;

						uint32_t test = ReadMemory<uint16_t>(offset);
						uint32_t test2 = ReadMemory<uint16_t>(offset + 9);
						if (test == 489 && test2 == 489) // default value: not yet fixed; valid script
						{
							WriteMemory<uint16_t>(offset, 505, false);
							offset += 9;
							WriteMemory<uint16_t>(offset, 505, false);
							offset += 47;
							WriteMemory<uint16_t>(offset, 505, false);
							offset += 8385;
							WriteMemory<uint16_t>(offset, 505, false);
						}
					}
				}

				if (G_NoGarageRadioChange)
				{
					unsigned int script;
					Command<GET_SCRIPT_STRUCT_NAMED>("CARMOD", &script);
					if (script)
					{
						unsigned char* offset = reinterpret_cast<CRunningScript*>(script)->m_pBaseIP;
						offset += 1839;

						uint16_t test = ReadMemory<uint16_t>(offset);
						if (test == 0x0A26) // default value: not yet fixed; valid script
						{
							WriteMemory<uint16_t>(offset, 0x0000, false);
							offset += 905;
							WriteMemory<uint16_t>(offset, 0x0000, false);
							offset += 688;
							WriteMemory<uint16_t>(offset, 0x0000, false);
						}
					}
				}

				if (G_NoEmergencyMisWanted)
				{
					unsigned int script[3];
					Command<GET_SCRIPT_STRUCT_NAMED>("COPCAR", &script[0]);
					Command<GET_SCRIPT_STRUCT_NAMED>("AMBULAN", &script[1]);
					Command<GET_SCRIPT_STRUCT_NAMED>("FIRETRU", &script[2]);
					if (script[0] || script[1] || script[2])
					{
						if (FindPlayerPed()->GetWantedLevel() == 0)
						{
							if (G_NoEmergencyMisWanted_MaxWantedLevel == -1)
							{
								G_NoEmergencyMisWanted_MaxWantedLevel = CWanted::MaximumWantedLevel;
							}
							CWanted::SetMaximumWantedLevel(0);
						}
						else
						{
							if (bOnEmergencyMissionLastFrame) FindPlayerPed()->SetWantedLevel(0);
						}
						if (!bOnEmergencyMissionLastFrame)
						{
							WriteMemory<uint8_t>(0x5A07D0, 0xC3, true);
						}
						bOnEmergencyMissionLastFrame = true;
					}
					else
					{
						if (bOnEmergencyMissionLastFrame && !G_NoSamSite)
						{
							WriteMemory<uint8_t>(0x5A07D0, 0x83, true);
						}
						bOnEmergencyMissionLastFrame = false;
						if (G_NoEmergencyMisWanted_MaxWantedLevel != -1)
						{
							CWanted::SetMaximumWantedLevel(G_NoEmergencyMisWanted_MaxWantedLevel);
							G_NoEmergencyMisWanted_MaxWantedLevel = -1;
						}
					}
				}

				if (G_SCMfixes)
				{
					unsigned int script;
					Command<GET_SCRIPT_STRUCT_NAMED>("DRUGS4", &script);
					if (script)
					{
						unsigned char* offset = reinterpret_cast<CRunningScript*>(script)->m_pBaseIP;
						offset += 66597;

						// Reenable some subtitles
						uint32_t test = ReadMemory<uint32_t>(offset);
						if (test == 1174995925) // default value: not yet fixed; valid script
						{
							const uint8_t NOP5bytes[] = { 0x52, 0x07, 0x03, 0x00, 0x00 };
							memcpy(offset, &NOP5bytes, sizeof(NOP5bytes));
							offset += sizeof(NOP5bytes);

							memset(offset, 0, 72);
						}
					}
				}

				if (G_TuningChoose2colors)
				{
					unsigned int script;
					Command<GET_SCRIPT_STRUCT_NAMED>("CARMOD", &script);
					if (script)
					{
						*(CTheScripts::ScriptSpace + (10443 * 4)) = 2;
					}
				}
			}

			if (bProcessOnceOnScripts)
			{
				bProcessOnceOnScripts = false;
				ShowModMessages();
				if (G_FPSlimit > 0)
				{
					WriteMemory<uint8_t>(0xC1704C, G_FPSlimit, false);
				}
				if (G_EnableCensorship)
				{
					WriteMemory<uint8_t>(0xB9B7EC, false, true);
					WriteMemory<uint8_t>(0x56D188, false, true);
					WriteMemory<uint8_t>(0x56D231, false, true);
					WriteMemory<uint8_t>(0x56D271, false, true);
				}
			}
		}
		 
		if (bProcessOnceAfterIntro && *(CTheScripts::ScriptSpace + (24 * 4)) == 1 /* intro passed */)
		{
			bProcessOnceAfterIntro = false;
			ShowModMessages(); // second time, just to make sure
			if (bEnabled)
			{
				if (G_NoTutorials)
				{
					Command<Commands::TERMINATE_ALL_SCRIPTS_WITH_THIS_NAME>("HELP");
					Command<Commands::REMOVE_PICKUP>((CTheScripts::ScriptSpace + (669 * 4))); //Pickup_Info_Hospital
					Command<Commands::REMOVE_PICKUP>((CTheScripts::ScriptSpace + (670 * 4))); //Pickup_Info_Hospital_2
					Command<Commands::REMOVE_PICKUP>((CTheScripts::ScriptSpace + (671 * 4))); //Pickup_Info_Police
					*(int*)(CTheScripts::ScriptSpace + (119 * 4)) = true; //Help_Wasted_Shown
					*(int*)(CTheScripts::ScriptSpace + (126 * 4)) = true; //Help_Busted_Shown
				}
				if (G_NoStuntReward)
				{
					Command<Commands::TERMINATE_ALL_SCRIPTS_WITH_THIS_NAME>("HJ");
				}
			}
		}

		if (G_ReloadCommand.length() > 0)
		{
			if (TestCheat(&G_ReloadCommand[0]))
			{
				if (lang == languages::PT)
				{
					lg << "Recarregando..." << "\n\n";
					CMessages::AddMessageJumpQ("'MixSets.ini' recarregando.", 1000, false, false);
				}
				else
				{
					lg << "Reloading..." << "\n\n";
					CMessages::AddMessageJumpQ("'MixSets.ini' reloading.", 1000, false, false);
				}
				lg.flush();

				bReloading = true;

				MixSets::ReadIni_BeforeFirstFrame();
				MixSets::ReadIni();
				bProcessOnceAfterIntro = true;
				bProcessOnceOnScripts = true;

				bReloading = false;
			}
		}
	};

	Events::vehicleRenderEvent += [](CVehicle* vehicle) {

		if (bEnabled)
		{
			CAutomobile* automobile = (CAutomobile*)vehicle;

			if (G_TaxiLights)
			{
				if (vehicle->m_nModelIndex == MODEL_TAXI || vehicle->m_nModelIndex == MODEL_CABBIE)
				{
					if (vehicle->m_pDriver && vehicle->m_nNumPassengers == 0 && vehicle->m_nVehicleFlags.bEngineOn && vehicle->m_fHealth > 0.0f)
					{
						automobile->SetTaxiLight(true);
					}
					else
					{
						automobile->SetTaxiLight(false);
					}
				}
			}
		}

	};

	Events::onPauseAllSounds += []
	{
		lg.flush();
		if (G_ProcessPriority > 0)
			SetIdlePriority();
	};

	Events::onResumeAllSounds += []
	{
		if (G_ProcessPriority > 0)
			ProcessPriority();
	};

}


///////////////////////////////////////////////////////////////////////////////////////////////////


void MixSets::ShowModMessages()
{
	if (MixSets::bIniFailed)
	{
		CMessages::AddMessageJumpQ("~r~ERROR: MixSets.ini not found - MixSets.ini nao encontrado", 6000, false, false);
	}
	else
	{
		if (MixSets::bVersionFailed)
		{
			if (MixSets::lang == languages::PT)
				CMessages::AddMessageJumpQ("~r~MixSets: O executavel do seu jogo nao e compativel. Use Crack 1.0 US Hoodlum ou Compact.", 10000, false, false);
			else
				CMessages::AddMessageJumpQ("~r~MixSets: Your game executable isn't compatible. Use Crack 1.0 US Hoodlum or Compact.", 10000, false, false);
		}
		else
		{
			if (MixSets::bReadOldINI)
			{
				if (MixSets::bErrorRename)
				{
					if (MixSets::lang == languages::PT)
						CMessages::AddMessageJumpQ("~r~As configuracoes do 'MixSets old.ini' foram movidas, mas houve um erro ao renomea-lo. Leia o 'MixSets.log'.", 10000, false, false);
					else
						CMessages::AddMessageJumpQ("~r~The 'MixSets old.ini' settings was moved, but there is an error when renaming it. Read the 'MixSets.log'.", 10000, false, false);
				}
				else
				{
					if (MixSets::numOldCfgNotFound > 0)
					{
						if (MixSets::numOldCfgNotFound > 1)
						{
							if (MixSets::lang == languages::PT)
								CMessages::AddMessageJumpQWithNumber("~y~As configuracoes do 'MixSets old.ini' foram movidas. Ha ~1~ avisos, leia o 'MixSets.log'.", 8000, false, MixSets::numOldCfgNotFound, 0, 0, 0, 0, 0, false);
							else
								CMessages::AddMessageJumpQWithNumber("~y~The 'MixSets old.ini' settings was moved. There is ~1~ warnings, read the 'MixSets.log'.", 8000, false, MixSets::numOldCfgNotFound, 0, 0, 0, 0, 0, false);
						}
						else
						{
							if (MixSets::lang == languages::PT)
								CMessages::AddMessageJumpQWithNumber("~y~As configuracoes do 'MixSets old.ini' foram movidas. Ha ~1~ aviso, leia o 'MixSets.log'.", 8000, false, MixSets::numOldCfgNotFound, 0, 0, 0, 0, 0, false);
							else
								CMessages::AddMessageJumpQWithNumber("~y~The 'MixSets old.ini' settings was moved. There is ~1~ warning, read the 'MixSets.log'.", 8000, false, MixSets::numOldCfgNotFound, 0, 0, 0, 0, 0, false);
						}
					}
					else
					{
						if (MixSets::lang == languages::PT)
							CMessages::AddMessageJumpQ("~y~As configuracoes do 'MixSets old.ini' foram movidas com sucesso.", 6000, false, false);
						else
							CMessages::AddMessageJumpQ("~y~As configuracoes do 'MixSets old.ini' foram movidas com sucesso.", 6000, false, false);
					}
				}
			}
		}
	}
}

void MixSets::ProcessUseHighPedShadows()
{
	if (MixSets::G_UseHighPedShadows == 0)
	{
		WriteMemory<uint8_t>(0x5E6763 + 2, 99, false);
		WriteMemory<uint8_t>(0x706BC9 + 2, 99, false);
		WriteMemory<uint8_t>(0x5E6772 + 2, 99, false);
		if (MixSets::G_NoStencilShadows || MixSets::curQuality < 1)
		{
			WriteMemory<uint8_t>(0x5E682A + 1, 1, false);
			WriteMemory<uint8_t>(0x5E6852 + 1, 1, false);
		}
	}
	else
	{
		if (MixSets::G_UseHighPedShadows == 1) {
			WriteMemory<uint8_t>(0x5E6763 + 2, MixSets::curQuality, false);
			WriteMemory<uint8_t>(0x706BC9 + 2, MixSets::curQuality, false);
			WriteMemory<uint8_t>(0x5E6772 + 2, MixSets::curQuality, false);
			if (MixSets::G_NoStencilShadows || MixSets::curQuality < 1)
			{
				WriteMemory<uint8_t>(0x5E682A + 1, 0, false);
				WriteMemory<uint8_t>(0x5E6852 + 1, 0, false);
			}
		}
	}
}

void MixSets::ProcessNoStencilShadows()
{
	if (MixSets::curQuality < 1)
	{
		WriteMemory<uint8_t>(0x70BDAC, 0x85, false);
	}
	else
	{
		WriteMemory<uint8_t>(0x70BDAC, 0x84, false);
	}
}

void MixSets::ProcessPriority()
{
	HANDLE processHandle = GetCurrentProcess();
	int priority = 0;
	switch (MixSets::G_ProcessPriority)
	{
	case 1:
		priority = IDLE_PRIORITY_CLASS;
		break;
	case 2:
		priority = BELOW_NORMAL_PRIORITY_CLASS;
		break;
	case 3:
		priority = NORMAL_PRIORITY_CLASS;
		break;
	case 4:
		priority = ABOVE_NORMAL_PRIORITY_CLASS;
		break;
	case 5:
		priority = HIGH_PRIORITY_CLASS;
		break;
	case 6:
		priority = REALTIME_PRIORITY_CLASS;
		break;
	default:
		priority = NORMAL_PRIORITY_CLASS;
		break;
	}
	SetPriorityClass(processHandle, priority);
}

void MixSets::SetIdlePriority()
{
	HANDLE processHandle = GetCurrentProcess();
	SetPriorityClass(processHandle, IDLE_PRIORITY_CLASS);
}


///////////////////////////////////////////////////////////////////////////////////////////////////

void MixSets::ReadOldINI(CIniReader ini, fstream* lg, string section, string key)
{
	MixSets::bParsePreserveComments = true;

	CIniReader iniold("MixSets old.ini");
	string oldIniLine = iniold.ReadString(section, key, "");

	if (oldIniLine.length() <= 0)
	{
		if (MixSets::lang == languages::PT)
			*lg << "Aviso: " << section << ": " << key << " não encontrado no 'MixSets old.ini'. Se não é função nova, então o nome foi alterado na nova versão. Verifique.\n";
		else
			*lg << "Warning: " << section << ": " << key << " not found in 'MixSets old.ini'. If isn't new feature, then the name has changed in new version. Check it.\n";

		MixSets::bParsePreserveComments = false;

		MixSets::numOldCfgNotFound++;
		return;
	}

	CIniReader ininew("MixSets.ini");

	string newIniLine = ininew.ReadString(section, key, "");

	MixSets::bParsePreserveComments = false;

	int newIniLineLen = newIniLine.find_first_of(" #");
	int oldIniLineLen = oldIniLine.find_first_of(" #");

	newIniLine.replace(0, newIniLineLen, oldIniLine, 0, oldIniLineLen);

	int charsRemoved = (oldIniLineLen - newIniLineLen);

	if (charsRemoved < 0)
	{
		while (charsRemoved < 0)
		{
			newIniLine.insert(oldIniLineLen, " ");
			charsRemoved++;
		}
	}
	else
	{
		while (charsRemoved > 0)
		{
			if (&newIniLine[oldIniLineLen] == "#") break;
			newIniLine.replace(oldIniLineLen, 1, "");
			charsRemoved--;
		}
	}

	newIniLine.insert(0, " ");

	ininew.WriteString(section, key, newIniLine, false);
}

inline bool MixSets::FileExists(const std::string& name)
{
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool MixSets::ReadIniFloat(CIniReader ini, fstream* lg, string section, string key, float* f)
{
	if (MixSets::bReadOldINI) ReadOldINI(ini, lg, section, key);
	*f = ini.ReadFloat(section, key, -1);
	if (*f != -1)
	{
		*lg << section << ": " << key << " = " << fixed << *f << "\n";
		return true;
	}
	else return false;
}

bool MixSets::ReadIniInt(CIniReader ini, fstream* lg, string section, string key, int* i)
{
	if (MixSets::bReadOldINI) ReadOldINI(ini, lg, section, key);
	*i = ini.ReadInteger(section, key, -1);
	if (*i != -1)
	{
		*lg << section << ": " << key << " = " << *i << "\n";
		return true;
	}
	else return false;
}

bool MixSets::ReadIniBool(CIniReader ini, fstream* lg, string section, string key)
{
	if (MixSets::bReadOldINI) ReadOldINI(ini, lg, section, key);
	//bool b = ini.ReadBoolean(section, key, 0);
	bool b = ini.ReadInteger(section, key, 0) == 1;
	if (b == true)
	{
		*lg << section << ": " << key << " = true \n";
		return true;
	}
	else return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void MixSets::IncreaseMemoryValueIfValid(uintptr_t address, int32_t value, uint8_t validation, bool vp)
{
	if (ReadMemory<uint8_t>(address - 1, vp) != validation) return;
	if (ReadMemory<int32_t>(address, vp) < value) WriteMemory<int32_t>(address, value, vp);
}
void MixSets::IncreaseMemoryValueIfValid_Byte(uintptr_t address, int8_t value, uint8_t validation, bool vp)
{
	if (ReadMemory<uint8_t>(address - 1, vp) != validation) return;
	if (ReadMemory<int8_t>(address, vp) < value) WriteMemory<int8_t>(address, value, vp);
}

uint8_t MixSets::CustomMaxAnisotropic()
{
	return MixSets::G_Anisotropic;
}

void MixSets::ForceHighMirrorRes_MirrorsCreateBuffer()
{
	// To make it compatible with SilentPatch
	FxQuality_e qualityBackup = g_fx.GetFxQuality();
	g_fx.SetFxQuality(FXQUALITY_VERY_HIGH);
	((void(__cdecl*)())MixSets::ORIGINAL_MirrorsCreateBuffer)();
	g_fx.SetFxQuality(qualityBackup);
}

///////////////////////////////////////////////////////////////////////////////////////////////////


void __declspec(naked) PedWeaponDrawDist_ASM()
{
	MixSets::G_PedWeaponDrawDist_Final = MixSets::G_PedWeaponDrawDist * TheCamera.m_fLODDistMultiplier;
	__asm {
		fld     MixSets::G_PedWeaponDrawDist_Final
		push    7336AEh
		ret
	}
}

void MixSets::VehFlipDamage_Process_Damage(CPed* ped)
{
	if (ped->IsPlayer())
	{
		ped->m_fHealth -= (MixSets::G_VehFlipDamage * CTimer::ms_fTimeStep) * 50.0f;
	}
	else {
		ped->m_fHealth -= (MixSets::G_VehFlipDamage * CTimer::ms_fTimeStep);
	}
}


void MixSets::VehFlipDamage_Process(CVehicle* veh)
{
	if (veh)
	{
		if (veh->m_pDriver)
		{
			MixSets::VehFlipDamage_Process_Damage(veh->m_pDriver);
		}
		if (veh->m_nNumPassengers > 0)
		{
			for (int i = 0; i < 8; ++i)
			{
				if (veh->m_apPassengers[i] > 0) MixSets::VehFlipDamage_Process_Damage(veh->m_apPassengers[i]);
			}
		}
	}
}

void __fastcall PreRender_AddSingleWheelParticles_FixDouble(CVehicle* _this, int a, int wheelState, int a3, float a4, float a5, CColPoint* colPoint, CVector* from, int a8, signed int wheelId, int skidMarkType, bool *_bloodState, char flags)
{
	_this->AddSingleWheelParticles(wheelState, a3, a4, a5, colPoint, from, a8, wheelId, skidMarkType, _bloodState, flags);

	//1 = rear left
	//3 = rear right
	//5 = rear m left
	//6 = rear m right
	if (_this->m_pHandlingData->m_bDoubleRwheels && (wheelId == 1 || wheelId == 3 || wheelId == 5 || wheelId == 6))
	{
		CColPoint *colPoint2 = new CColPoint(*colPoint);
		float distance = 0.45f;

		bool left = (wheelId == 3 || wheelId == 6);

		if (MixSets::pVehFuncs_Ext_GetDoubleWheelOffset != NULL)
		{
			distance = (MixSets::pVehFuncs_Ext_GetDoubleWheelOffset)(_this, (int)left);
		}

		if (left)
		{
			distance *= -1.0f;
		}

		CVector pos1;
		pos1.x = from->x;
		pos1.y = from->y;
		pos1.z = from->z;

		CMatrixLink* matrix = _this->GetMatrix();

		pos1.x = matrix->right.x * distance;
		pos1.y = matrix->right.y * distance;
		pos1.z = matrix->right.z * distance;

		colPoint2->m_vecPoint.x = pos1.x;
		colPoint2->m_vecPoint.y = pos1.y;
		colPoint2->m_vecPoint.z = pos1.z;

		pos1.x += from->x;
		pos1.y += from->y;
		pos1.z += from->z;

		colPoint2->m_vecPoint.x += colPoint->m_vecPoint.x;
		colPoint2->m_vecPoint.y += colPoint->m_vecPoint.y;
		colPoint2->m_vecPoint.z += colPoint->m_vecPoint.z;

		_this->AddSingleWheelParticles(wheelState, a3, a4, a5, colPoint2, &pos1, a8, wheelId + 69696969, skidMarkType, _bloodState, flags);
	}
}

void __declspec(naked) VehFlipDamage_ASM()
{
	_asm
	{
		fstp st(0)

		push esi
		call MixSets::VehFlipDamage_Process
		add esp, 4 // params

		retn
	}
}

void __declspec(naked) VehFlipDamage_Player_ASM()
{
	_asm
	{
		fstp st(0)

		push ecx
		call MixSets::VehFlipDamage_Process
		add esp, 4 // params

		retn
	}
}

void __declspec(naked) BrakeReverseFix_ASM()
{
	__asm
	{
		push   eax
		fld    dword ptr[esi + 0x49C]
		mov    edx, 0x00858B50
		fcomp  dword ptr[edx]
		fnstsw ax
		test   ah, 0x44
		jp     do_stuff
		mov    edx, 0x00B734A4
		mov    eax, dword ptr[edx]
		test   ax, ax
		jne    dont_do_stuff

		do_stuff :
		mov    dword ptr[esi + 0x4A0], 0
			pop    eax
			mov    dword ptr[esi + 0x49C], eax
			ret

			dont_do_stuff :
		pop    eax
			ret
	}
}

void __declspec(naked) SirenOnWithoutDriver_ASM()
{
	__asm
	{
		cmp[esi + 0x328], 0
		jz back

		mov eax, 0x004EF850
		call eax

		mov[esi + 0x17C], ebx

		back :
		mov eax, 0x004F9DAF
			jmp eax
	}
}

void __declspec(naked) NoPauseWhenMinimize_AllowMouseMovement_ASM()
{
	__asm
	{
		mov MixSets::_EAX, eax
		mov eax, dword ptr ds : [0x8D621C]
		cmp eax, 0
		jne label1
		mov eax, MixSets::_EAX
		ret

		label1 :
		mov eax, MixSets::_EAX
			mov MixSets::_EAX, 0x7453F0
			jmp MixSets::_EAX
	}
}

void MixSets::asm_fmul(float f)
{
	__asm {fmul f}
}
