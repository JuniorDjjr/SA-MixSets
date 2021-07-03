/*
Credits for DK22Pac - IMFX
If you consider fixing something here, you should also consider fixing there: https://github.com/DK22Pac/imfx
*/
#include "Gunflashes.h"
#include "imfx.h"
#include "plugin.h"
#include "game_sa\FxManager_c.h"
#include "game_sa\common.h"
#include "game_sa\CWeaponInfo.h"
#include "game_sa\CGeneral.h"
#include "game_sa\CCamera.h"
#include "game_sa\CTimer.h"
#include <fstream>
#include <string>

#include "..\MixSets.h"

using namespace plugin;

//RwMatrix Gunflashes::matrixAry[20];
unsigned int Gunflashes::matrixCounter = 0;
PedExtendedData<Gunflashes::PedExtension> Gunflashes::pedExt;
//std::vector<GunflashInfo> Gunflashes::gunflashInfos;
bool Gunflashes::bLeftHand = false;
bool Gunflashes::bVehicleGunflash = false;

Gunflashes::PedExtension::PedExtension(CPed *) {
	bLeftHandGunflashThisFrame = false;
	bRightHandGunflashThisFrame = false;
	bInVehicle = false;
	pMats[0] = nullptr;
	pMats[1] = nullptr;
    Reset();
}

void Gunflashes::PedExtension::Reset() {
    bLeftHandGunflashThisFrame = false;
    bRightHandGunflashThisFrame = false;
    bInVehicle = false;
}

Gunflashes::PedExtension::~PedExtension() {
	if (pMats[0] != nullptr) delete pMats[0];
	if (pMats[1] != nullptr) delete pMats[1];
}

void Gunflashes::Setup() {
    patch::Nop(0x73306D, 9); // Remove default gunflashes
    patch::Nop(0x7330FF, 9); // Remove default gunflashes
    patch::SetUShort(0x5DF425, 0xE990); // Remove default gunflashes
    patch::SetUChar(0x741353, 0); // Add gunflash for cuntgun
    patch::RedirectCall(0x742299, DoDriveByGunflash);
    patch::RedirectJump(0x4A0DE0, MyTriggerGunflash);
    patch::SetPointer(0x86D744, MyProcessUseGunTask);
    //ReadSettings();
}

/*void Gunflashes::ReadSettings() {
    std::ifstream settingsFile(PLUGIN_PATH("imfx\\gunflash.dat"));
    if (settingsFile.is_open()) {
        for (std::string line; getline(settingsFile, line); ) {
            if (line[0] != ';' && line[0] != '#') {
                GunflashInfo info;
                unsigned int rotation; unsigned int smoke;
                if (sscanf(line.c_str(), "%d %s %d %d", &info.weapId, info.fxName, &rotation, &smoke) == 4) {
                    info.rotate = rotation ? true : false;
                    info.smoke = smoke ? true : false;
                    gunflashInfos.push_back(info);
                }
            }
        }
    }
}*/

void Gunflashes::ProcessPerFrame() {
	Gunflashes::matrixCounter = 0;
    for (int i = 0; i < CPools::ms_pPedPool->m_nSize; i++) {
        CPed *ped = CPools::ms_pPedPool->GetAt(i);
        if (ped)
            pedExt.Get(ped).Reset();
    }
}

bool __fastcall Gunflashes::MyProcessUseGunTask(CTaskSimpleUseGun *task, int, CPed *ped) {
    if (task->m_pWeaponInfo == CWeaponInfo::GetWeaponInfo(ped->m_aWeapons[ped->m_nActiveWeaponSlot].m_nType, ped->GetWeaponSkill())) {
        if (task->bRightHand) {
            bLeftHand = false;
            CallMethod<0x61EB10>(task, ped, false);
        }
        if (task->bLefttHand) {
            bLeftHand = true;
            CallMethod<0x61EB10>(task, ped, true);
            bLeftHand = false;
        }
        //*reinterpret_cast<unsigned char *>(&task->m_nFlags) = 0;
		task->bRightHand = false;
		task->bLefttHand = false;
    }
    return 0;
}

void __fastcall Gunflashes::DoDriveByGunflash(CPed *driver, int, int, bool leftHand) {
    bLeftHand = leftHand;
    bVehicleGunflash = true;
    MyTriggerGunflash(&g_fx, 0, driver, CVector(0.0f, 0.0f, 0.0f), CVector(0.0f, 0.0f, 0.0f), true);
}

void __fastcall Gunflashes::MyTriggerGunflash(Fx_c *fx, int, CEntity *entity, CVector &origin, CVector &target, bool doGunflash) {
    if (entity && entity->m_nType == ENTITY_TYPE_PED) {
        CPed *owner = reinterpret_cast<CPed *>(entity);
        pedExt.Get(owner).bLeftHandGunflashThisFrame = bLeftHand;
        pedExt.Get(owner).bRightHandGunflashThisFrame = !bLeftHand;
        pedExt.Get(owner).bInVehicle = bVehicleGunflash;
    }
    else {
        if (DistanceBetweenPoints(target, origin) > 0.0f) {
            RwMatrix fxMat;
            fx->CreateMatFromVec(&fxMat, &origin, &target);
            RwV3d offset = { 0.0f, 0.0f, 0.0f };
            FxSystem_c *gunflashFx = g_fxMan.CreateFxSystem("gunflash", &offset, &fxMat, false);
			if (MixSets::G_GunflashEmissionMult > -1.0f) gunflashFx->SetRateMult(MixSets::G_GunflashEmissionMult);
            if (gunflashFx) {
                gunflashFx->CopyParentMatrix();
                gunflashFx->PlayAndKill();
            }
            FxSystem_c *smokeFx = g_fxMan.CreateFxSystem("gunsmoke", &offset, &fxMat, false);
            if (smokeFx) {
                smokeFx->CopyParentMatrix();
                smokeFx->PlayAndKill();
            }
        }
    }
    bLeftHand = false;
    bVehicleGunflash = false;
}

void Gunflashes::CreateGunflashEffectsForPed(CPed *ped) {
    bool ary[2];
    ary[0] = pedExt.Get(ped).bLeftHandGunflashThisFrame;
    ary[1] = pedExt.Get(ped).bRightHandGunflashThisFrame;
    bool inVehicle = pedExt.Get(ped).bInVehicle;
    for (int i = 0; i < 2; i++) {
        if (ary[i]) {

			if (pedExt.Get(ped).pMats[i] == nullptr) pedExt.Get(ped).pMats[i] = new RwMatrix();
            RwMatrix *mat = pedExt.Get(ped).pMats[i];
			if (!mat) break;

            bool leftHand = i == 0;
            if (ped->m_pRwObject && ped->m_pRwObject->type == rpCLUMP) {
                bool rotate = true;
                bool smoke = true;
                char *fxName = "gunflash";
                /*for (GunflashInfo &info : gunflashInfos) {
                    if (info.weapId == ped->m_aWeapons[ped->m_nActiveWeaponSlot].m_nType) {
                        rotate = info.rotate;
                        smoke = info.smoke;
                        fxName = info.fxName;
                        break;
                    }
                }*/
                char weapSkill = ped->GetWeaponSkill(ped->m_aWeapons[ped->m_nActiveWeaponSlot].m_nType);
                CWeaponInfo *weapInfo = CWeaponInfo::GetWeaponInfo(ped->m_aWeapons[ped->m_nActiveWeaponSlot].m_nType, weapSkill);
                RwV3d offset = weapInfo->m_vecFireOffset.ToRwV3d();
                if (leftHand)
                    offset.z *= -1.0f;
                static RwV3d axis_y = { 0.0f, 1.0f, 0.0f };
                static RwV3d axis_z = { 0.0f, 0.0f, 1.0f };
                RpHAnimHierarchy *hierarchy = GetAnimHierarchyFromSkinClump(ped->m_pRwClump);
                RwMatrix *boneMat = &RpHAnimHierarchyGetMatrixArray(hierarchy)[RpHAnimIDGetIndex(hierarchy, 24 + (10 * leftHand))];
                memcpy(mat, boneMat, sizeof(RwMatrix));
				RwMatrixUpdate(mat);
				FxSystem_c *gunflashFx = g_fxMan.CreateFxSystem(fxName, &offset, mat, true);
				//if (MixSets::G_GunflashEmissionMult > -1.0f) gunflashFx->SetRateMult(MixSets::G_GunflashEmissionMult);
                if (gunflashFx) {
					if (ped->m_nPedFlags.bInVehicle) gunflashFx->m_pParentMatrix = boneMat;
                    RwMatrixRotate(&gunflashFx->m_localMatrix, &axis_z, -90.0f, rwCOMBINEPRECONCAT);
                    if (rotate) {
                        RwMatrixRotate(&gunflashFx->m_localMatrix, &axis_y, CGeneral::GetRandomNumberInRange(0.0f, 360.0f), rwCOMBINEPRECONCAT);
                    }
                    gunflashFx->PlayAndKill();
                }
                if (smoke) {
					if (!ped->m_pVehicle || ped->m_pVehicle->m_vecMoveSpeed.Magnitude() < 0.15f) {
						FxSystem_c *smokeFx = g_fxMan.CreateFxSystem("gunsmoke", &offset, mat, true);
						if (smokeFx) {
							RwMatrixRotate(&smokeFx->m_localMatrix, &axis_z, -90.0f, rwCOMBINEPRECONCAT);
							smokeFx->PlayAndKill();
						}
					}
                }
            }
        }
    }
    pedExt.Get(ped).Reset();
}