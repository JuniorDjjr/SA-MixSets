/*
	MixSets by Junior_Djjr and entire GTA 3D Era community
*/
#include "ReadIni.h"
#include "Common.h"

// Game
#include "extensions/ScriptCommands.h"
#include "CPedIntelligence.h"
#include "CWeather.h"
#include "CTheScripts.h"
#include "Fx_c.h"
#include "CTimer.h"
#include "CTimeCycle.h"
#include "CEntryExitManager.h"
#include "CGeneral.h"
#include "CWorld.h"
#include <game_sa\common.h>
#include "CMessages.h"
#include "CCamera.h"
#include "IMFX/Gunflashes.h"

// Other
#include "..\injector\assembly.hpp"
#include "IniReader/IniReader.h"
#include "TestCheat.h"


using namespace plugin;
using namespace injector;
using namespace std;

const unsigned int GET_SCRIPT_STRUCT_NAMED = 0x10AAA;

// Global vars
bool Read = false, forceUpdateQualityFuncs = true, bProcessOnceOnScripts = false, bProcessOnceAfterIntro = false,
bPlayerRenderWeaponInVehicleLastFrame = false, bPlayerTwoRenderWeaponInVehicleLastFrame = false, bNoCLEO = true,
bOnEmergencyMissionLastFrame = false;
int curQuality = -1, lastQuality = -1, G_NoEmergencyMisWanted_MaxWantedLevel = -1;
fstream lg;
languages lang;


// External vars from ReadIni
extern bool bEnabled, bReadOldINI, bErrorRename, inSAMP, rpSAMP, dtSAMP, bIniFailed, bIMFX, bGunFuncs, G_NoStencilShadows,
G_OpenedHouses, G_RandWheelDettach, G_TaxiLights, G_ParaLandingFix, G_NoGarageRadioChange, G_NoEmergencyMisWanted,
G_NoStuntReward, G_NoTutorials, G_EnableCensorship, G_HideWeaponsOnVehicle, bReloading, G_Fix2DGunflash;
extern int G_i, numOldCfgNotFound, G_ProcessPriority, G_FreezeWeather, G_FPSlimit, G_UseHighPedShadows, G_StreamMemory,
G_Anisotropic, G_HowManyMinsInDay;
extern string G_ReloadCommand;
extern float G_f;


///////////////////////////////////////////////////////////////////////////////////////////////////


class MixSets {
public:
	MixSets() {

		lg.open("MixSets.log", fstream::out | fstream::trunc);
		lg << "v4 final 3" << "\n";
		lg.flush();

		bEnabled = false;

		if (!ReadMemory<uint8_t>(0x400088, true) == 0xCA) {
			lg << "ERROR: Game version not supported. Download GTA SA Crack 1.0 US (Hoodlum preferable)." << "\n\n";
			lg.flush();
			MessageBoxA(0, "Game version not supported. Download GTA SA Crack 1.0 US (Hoodlum preferable).", "MixSets", 0);
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
				lg << "SAMP = true" << "\n\n";
				inSAMP = true;
			}
			else inSAMP = false;

			if (GetModuleHandleA("IMFX.asi")) {
				lg << "IMFX = true" << "\n\n";
				bIMFX = true;
			}
			else bIMFX = false;

			if (GetModuleHandleA("GunFuncs.asi")) {
				lg << "GunFuncs = true" << "\n\n";
				bGunFuncs = true;
			}
			else bGunFuncs = false;

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


		Events::processScriptsEvent.after += [] { // Note: gameProcessEvent doesn't work on SAMP

			if (bEnabled) {

				if (Read) {
					ReadIni();

					if (lang == languages::PT)
						lg << "\n" << "Terminado de ler o ini no primeiro frame." << "\n\n";
					else
						lg << "\n" << "Done read ini on first frame" << "\n\n";
					lg.flush();

					Read = false;
				}
				
				//////////////////////////////////////////////

				CPed *playerPed = FindPlayerPed(-1); // focused

				if (G_HideWeaponsOnVehicle)
				{
					CPed *playerOne = FindPlayerPed(0);
					CPed *playerTwo = FindPlayerPed(1);

					if (playerOne) {
						if (playerOne->m_nPedFlags.bInVehicle) {
							bPlayerRenderWeaponInVehicleLastFrame = true;
							int camMode = TheCamera.m_aCams[0].m_nMode;
							CPad *pad = CPad::GetPad(0);
							if (pad->GetLookLeft() || pad->GetLookRight() || pad->GetCarGunFired() ||
								camMode == eCamMode::MODE_AIMWEAPON_FROMCAR || camMode == eCamMode::MODE_TWOPLAYER_IN_CAR_AND_SHOOTING) {
								playerOne->m_pPlayerData->m_bRenderWeapon = true;
								if (playerTwo) playerTwo->m_pPlayerData->m_bRenderWeapon = true;
							}
							else {
								playerOne->m_pPlayerData->m_bRenderWeapon = false;
								if (playerTwo) playerTwo->m_pPlayerData->m_bRenderWeapon = false;
							}
						}
						else {
							if (bPlayerRenderWeaponInVehicleLastFrame) {
								playerOne->m_pPlayerData->m_bRenderWeapon = true;
								if (playerTwo) playerTwo->m_pPlayerData->m_bRenderWeapon = true;
								bPlayerRenderWeaponInVehicleLastFrame = false;
							}
						}
					}

					if (playerTwo) {
						if (playerTwo->m_nPedFlags.bInVehicle && playerTwo->m_pPlayerData) {
							bPlayerTwoRenderWeaponInVehicleLastFrame = true;
							int camMode = TheCamera.m_aCams[0].m_nMode;
							CPad *pad = CPad::GetPad(0);
							if (pad->GetLookLeft() || pad->GetLookRight() || pad->GetCarGunFired() ||
								camMode == eCamMode::MODE_AIMWEAPON_FROMCAR || camMode == eCamMode::MODE_TWOPLAYER_IN_CAR_AND_SHOOTING) {
								playerTwo->m_pPlayerData->m_bRenderWeapon = true;
								if (playerTwo) playerTwo->m_pPlayerData->m_bRenderWeapon = true;
							}
							else {
								playerTwo->m_pPlayerData->m_bRenderWeapon = false;
								if (playerTwo) playerTwo->m_pPlayerData->m_bRenderWeapon = false;
							}
						}
						else {
							if (bPlayerTwoRenderWeaponInVehicleLastFrame) {
								playerTwo->m_pPlayerData->m_bRenderWeapon = true;
								if (playerTwo) playerTwo->m_pPlayerData->m_bRenderWeapon = true;
								bPlayerTwoRenderWeaponInVehicleLastFrame = false;
							}
						}
					}
				}

				if (G_Fix2DGunflash) Gunflashes::ProcessPerFrame();

				if (G_FreezeWeather >= 0 && !inSAMP)
					CWeather::ForceWeatherNow(G_FreezeWeather);

				if (G_ProcessPriority > 0)
					ProcessPriority();

				curQuality = g_fx.GetFxQuality();

				if (curQuality != lastQuality || forceUpdateQualityFuncs) {
					forceUpdateQualityFuncs = false;

					if (G_NoStencilShadows)
						ProcessNoStencilShadows();

					if (G_UseHighPedShadows > -1)
						ProcessUseHighPedShadows();
				}

				lastQuality = curQuality;

				//////////////////////////////////////////////


				if (G_OpenedHouses && !inSAMP) {
					if (!CEntryExitManager::ms_bBurglaryHousesEnabled) {
						CEntryExitManager::EnableBurglaryHouses(true);
					}
				}

				if (G_StreamMemory > 0) {
					WriteMemory<uint32_t>(0x8A5A80, G_StreamMemory, false);
				}

				if (G_HowManyMinsInDay > 0 && !inSAMP) {
					WriteMemory<uint32_t>(0xB7015C, G_HowManyMinsInDay, false);
				}

				if (G_RandWheelDettach) {
					int wheel = 2;
					switch (CGeneral::GetRandomNumberInRange(1, 5))
					{
					case 1:
						wheel = 2;
						break;
					case 2:
						wheel = 5;
						break;
					case 3:
						wheel = 4;
						break;
					case 4:
						wheel = 7;
						break;
					}
					WriteMemory<uint8_t>(0x6B38BC, wheel, false);
					WriteMemory<uint8_t>(0x6B38C7, wheel, false);
					int wheelOffset = (wheel * 4) + 0x648;
					WriteMemory<uint8_t>(0x6B38D1, wheelOffset, false);
				}

				if (G_ParaLandingFix && !inSAMP && !bNoCLEO) {
					unsigned int script;
					Command<GET_SCRIPT_STRUCT_NAMED>("PLCHUTE", &script);
					if (script) {
						unsigned char *offset = reinterpret_cast<CRunningScript *>(script)->m_pBaseIP;
						offset += 4948;

						uint32_t test = ReadMemory<uint32_t>(offset + 6);
						if (test == 0x4C4C4146) { // default value: not yet fixed; valid script
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

				if (G_NoGarageRadioChange && !inSAMP && !bNoCLEO) {
					unsigned int script;
					Command<GET_SCRIPT_STRUCT_NAMED>("CARMOD", &script);
					if (script) {
						unsigned char *offset = reinterpret_cast<CRunningScript *>(script)->m_pBaseIP;
						offset += 1839;

						uint16_t test = ReadMemory<uint16_t>(offset);
						if (test == 0x0A26) { // default value: not yet fixed; valid script
							WriteMemory<uint16_t>(offset, 0x0000, false);
							offset += 905;
							WriteMemory<uint16_t>(offset, 0x0000, false);
							offset += 688;
							WriteMemory<uint16_t>(offset, 0x0000, false);
						}
					}
				}

				if (G_NoEmergencyMisWanted && !inSAMP && !bNoCLEO) {
					unsigned int script[3];
					Command<GET_SCRIPT_STRUCT_NAMED>("COPCAR", &script[0]);
					Command<GET_SCRIPT_STRUCT_NAMED>("AMBULAN", &script[1]);
					Command<GET_SCRIPT_STRUCT_NAMED>("FIRETRU", &script[2]);
					if (script[0] || script[1] || script[2]) {
						if (FindPlayerPed()->GetWantedLevel() == 0) {
							if (G_NoEmergencyMisWanted_MaxWantedLevel == -1) {
								G_NoEmergencyMisWanted_MaxWantedLevel = CWanted::MaximumWantedLevel;
							}
							CWanted::SetMaximumWantedLevel(0);
						}
						else {
							if (bOnEmergencyMissionLastFrame) FindPlayerPed()->SetWantedLevel(0);
						}
						bOnEmergencyMissionLastFrame = true;
					}
					else {
						bOnEmergencyMissionLastFrame = false;
						if (G_NoEmergencyMisWanted_MaxWantedLevel != -1) {
							CWanted::SetMaximumWantedLevel(G_NoEmergencyMisWanted_MaxWantedLevel);
							G_NoEmergencyMisWanted_MaxWantedLevel = -1;
						}
					}
				}

				if (bProcessOnceOnScripts) {
					bProcessOnceOnScripts = false;
					ShowModMessages();
					if (G_FPSlimit > 0) {
						WriteMemory<uint8_t>(0xC1704C, G_FPSlimit, false);
					}
					if (G_EnableCensorship) {
						WriteMemory<uint8_t>(0xB9B7EC, false, true);
						WriteMemory<uint8_t>(0x56D188, false, true);
						WriteMemory<uint8_t>(0x56D231, false, true);
						WriteMemory<uint8_t>(0x56D271, false, true);
					}
				}
			}

			if (bProcessOnceAfterIntro && *(CTheScripts::ScriptSpace + (24 * 4)) == 1 /* intro passed */) {
				bProcessOnceAfterIntro = false;
				ShowModMessages(); // second time, just to make sure
				if (bEnabled) {
					if (G_NoTutorials) {
						Command<Commands::TERMINATE_ALL_SCRIPTS_WITH_THIS_NAME>("HELP");
						Command<Commands::REMOVE_PICKUP>((CTheScripts::ScriptSpace + (669 * 4))); //Pickup_Info_Hospital
						Command<Commands::REMOVE_PICKUP>((CTheScripts::ScriptSpace + (670 * 4))); //Pickup_Info_Hospital_2
						Command<Commands::REMOVE_PICKUP>((CTheScripts::ScriptSpace + (671 * 4))); //Pickup_Info_Police
						*(int*)(CTheScripts::ScriptSpace + (119 * 4)) = true; //Help_Wasted_Shown
						*(int*)(CTheScripts::ScriptSpace + (126 * 4)) = true; //Help_Busted_Shown
					}
					if (G_NoStuntReward) {
						Command<Commands::TERMINATE_ALL_SCRIPTS_WITH_THIS_NAME>("HJ");
					}
				}
			}

			if (G_ReloadCommand.length() > 0) {
				if (TestCheat(&G_ReloadCommand[0])) {

					if (lang == languages::PT)
					{
						lg << "Recarregando..." << "\n\n";
						CMessages::AddMessageJumpQ("'MixSets.ini' recarregando.", 1000, false, false);
					}
					else {
						lg << "Reloading..." << "\n\n";
						CMessages::AddMessageJumpQ("'MixSets.ini' reloading.", 1000, false, false);
					}
					lg.flush();

					bReloading = true;

					ReadIni_BeforeFirstFrame();
					ReadIni();
					bProcessOnceOnScripts = true;
					bProcessOnceAfterIntro = true;

					bReloading = false;
				}
			}
		};


		Events::vehicleRenderEvent += [](CVehicle *vehicle) {
			CAutomobile *automobile = (CAutomobile *)vehicle;

			if (bEnabled) {

				if (G_TaxiLights) {
					if (vehicle->m_nModelIndex == MODEL_TAXI || vehicle->m_nModelIndex == MODEL_CABBIE) {
						if (vehicle->m_nVehicleFlags.bLightsOn && !vehicle->ms_forceVehicleLightsOff) {
							automobile->SetTaxiLight(true);
						}
						else {
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

	/////////////////////////////////////////////

	static void ShowModMessages() {
		if (bIniFailed) {
			CMessages::AddMessageJumpQ("~r~ERROR: MixSets.ini not found - MixSets.ini nao encontrado", 6000, false, false);
		}
		else {
			if (bReadOldINI) {
				if (bErrorRename) {
					if (lang == languages::PT)
						CMessages::AddMessageJumpQ("~r~As configuracoes do 'MixSets old.ini' foram movidas, mas houve um erro ao renomea-lo. Leia o 'MixSets.log'.", 10000, false, false);
					else
						CMessages::AddMessageJumpQ("~r~The 'MixSets old.ini' settings was moved, but there is an error when renaming it. Read the 'MixSets.log'.", 10000, false, false);
				}
				else {
					if (numOldCfgNotFound > 0) {
						if (numOldCfgNotFound > 1)
						{
							if (lang == languages::PT)
								CMessages::AddMessageJumpQWithNumber("~y~As configuracoes do 'MixSets old.ini' foram movidas. Ha ~1~ avisos, leia o 'MixSets.log'.", 8000, false, numOldCfgNotFound, 0, 0, 0, 0, 0, false);
							else
								CMessages::AddMessageJumpQWithNumber("~y~The 'MixSets old.ini' settings was moved. There is ~1~ warnings, read the 'MixSets.log'.", 8000, false, numOldCfgNotFound, 0, 0, 0, 0, 0, false);
						}
						else {
							if (lang == languages::PT)
								CMessages::AddMessageJumpQWithNumber("~y~As configuracoes do 'MixSets old.ini' foram movidas. Ha ~1~ aviso, leia o 'MixSets.log'.", 8000, false, numOldCfgNotFound, 0, 0, 0, 0, 0, false);
							else
								CMessages::AddMessageJumpQWithNumber("~y~The 'MixSets old.ini' settings was moved. There is ~1~ warning, read the 'MixSets.log'.", 8000, false, numOldCfgNotFound, 0, 0, 0, 0, 0, false);
						}
					}
					else {
						if (lang == languages::PT)
							CMessages::AddMessageJumpQ("~y~As configuracoes do 'MixSets old.ini' foram movidas com sucesso.", 6000, false, false);
						else
							CMessages::AddMessageJumpQ("~y~As configuracoes do 'MixSets old.ini' foram movidas com sucesso.", 6000, false, false);
					}
				}
			}
		}
	}

	static void ProcessUseHighPedShadows() {
		if (G_UseHighPedShadows == 0) {
			WriteMemory<uint8_t>(0x5E6763 + 2, 99, false);
			WriteMemory<uint8_t>(0x706BC9 + 2, 99, false);
			WriteMemory<uint8_t>(0x5E6772 + 2, 99, false);
			if (G_NoStencilShadows || curQuality < 1) {
				WriteMemory<uint8_t>(0x5E682A + 1, 1, false);
				WriteMemory<uint8_t>(0x5E6852 + 1, 1, false);
			}
		}
		else {
			if (G_UseHighPedShadows == 1) {
				WriteMemory<uint8_t>(0x5E6763 + 2, curQuality, false);
				WriteMemory<uint8_t>(0x706BC9 + 2, curQuality, false);
				WriteMemory<uint8_t>(0x5E6772 + 2, curQuality, false);
				if (G_NoStencilShadows || curQuality < 1) {
					WriteMemory<uint8_t>(0x5E682A + 1, 0, false);
					WriteMemory<uint8_t>(0x5E6852 + 1, 0, false);
				}
			}
		}
	}

	static void ProcessNoStencilShadows() {
		if (curQuality < 1) {
			WriteMemory<uint8_t>(0x70BDAC, 0x85, false);
		}
		else {
			WriteMemory<uint8_t>(0x70BDAC, 0x84, false);
		}
	}

	static void ProcessPriority() {
		HANDLE processHandle = GetCurrentProcess();
		int priority = 0;
		switch (G_ProcessPriority) {
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

	static void SetIdlePriority() {
		HANDLE processHandle = GetCurrentProcess();
		SetPriorityClass(processHandle, IDLE_PRIORITY_CLASS);
	}

} mixSets;

///////////////////////////////////////////////////////////////////////////////////////////////////


