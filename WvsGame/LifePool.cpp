#include "LifePool.h"
#include "..\Common\Net\OutPacket.h"
#include "User.h"
#include "MobTemplate.h"
#include "Field.h"
#include "Net\InPacket.h"
#include "Controller.h"

#include <cmath>

LifePool::LifePool()
	: m_pCtrlNull(new Controller(nullptr))
{
}


LifePool::~LifePool()
{
	for (auto& p : m_aMobGen)
		delete p.second;
	for (auto& p : m_aNpcGen)
		delete p.second;
	for (auto& p : m_hCtrl)
		delete p.second;
	delete m_pCtrlNull;
}

void LifePool::Init(Field* pField, int nFieldID)
{
	m_pField = pField;

	int sizeX = max(MAX_WINDOW_VIEW_X, m_pField->GetMapSizeX());
	int sizeY = max(max(m_pField->GetMapSizeY() - 450, MAX_WINDOW_VIEW_Y), m_pField->GetMapSizeY()); //I dont know
	int genSize = (int)(((double)sizeX * sizeY) * 0.0000078125f);
	if (genSize < 1)
		genSize = 1;
	else if (genSize >= MAX_MOB_GEN)
		genSize = MAX_MOB_GEN;
	m_nMobCapacityMin = genSize;
	m_nMobCapacityMax = genSize * 2;

	auto& mapWz = stWzResMan->GetWz(Wz::Map)["Map"]["Map" + std::to_string(nFieldID / 100000000)][std::to_string(nFieldID)];
	auto& lifeData = mapWz["life"];
	for (auto& node : lifeData)
	{
		const auto &typeFlag = (std::string)node["type"];
		if (typeFlag == "n")
			LoadNpcData(node);
		else if (typeFlag == "m")
			LoadMobData(node);
	}	

	//強制生成所有NPC
	for (auto& npc : m_lNpc)
		CreateNpc(npc);

	TryCreateMob(false);
	//mapWz.ReleaseData();
}

void LifePool::SetFieldObjAttribute(FieldObj* pFieldObj, WZ::Node& dataNode)
{
	try {
		pFieldObj->SetPosX(dataNode["x"]);
		pFieldObj->SetPosY(dataNode["y"]);
		pFieldObj->SetF(dataNode["f"]);
		pFieldObj->SetHide(dataNode["hide"]);
		pFieldObj->SetFh(dataNode["fh"]);
		pFieldObj->SetCy(dataNode["cy"]);
		pFieldObj->SetRx0(dataNode["rx0"]);
		pFieldObj->SetRx1(dataNode["rx1"]);
		pFieldObj->SetTemplateID(atoi(((std::string)dataNode["id"]).c_str()));
	}
	catch (std::exception& e) {
		printf("讀取地圖物件發生錯誤，訊息:%s\n", e.what());
	}
}

void LifePool::LoadNpcData(WZ::Node& dataNode)
{
	Npc npc;
	SetFieldObjAttribute(&npc, dataNode);
	m_lNpc.push_back(npc);
}

void LifePool::LoadMobData(WZ::Node& dataNode)
{
	Mob mob;
	SetFieldObjAttribute(&mob, dataNode);
	mob.SetMobTemplate(MobTemplate::GetMobTemplate(mob.GetTemplateID()));
	//MobTemplate::GetMobTemplate(mob.GetTemplateID());
	m_lMob.push_back(mob);
}

void LifePool::CreateNpc(const Npc& npc)
{
	std::lock_guard<std::mutex> lock(m_lifePoolMutex);
	Npc* newNpc = new Npc();
	*newNpc = npc; //Should notice pointer data assignment
	newNpc->SetFieldObjectID(atomicObjectCounter++);
	m_aNpcGen.insert({ newNpc->GetFieldObjectID(), newNpc });
}

void LifePool::TryCreateMob(bool reset)
{
	/*
	if reset, kill all monsters and respawns
	*/
	if(m_lMob.size() > 0)
	for (int i = 0; i < m_nMobCapacityMax - (m_aMobGen.size()); ++i) {
		auto& mob = m_lMob[rand() % m_lMob.size()];
		CreateMob(mob, mob.GetPosX(), mob.GetPosY(), mob.GetFh(), 0, -2, 0, 0, 0, nullptr);
	}
}

void LifePool::CreateMob(const Mob& mob, int x, int y, int fh, int bNoDropPriority, int nType, unsigned int dwOption, int bLeft, int nMobType, Controller* pOwner)
{
	std::lock_guard<std::mutex> lock(m_lifePoolMutex);
	Controller* pController = pOwner;
	if (m_hCtrl.size() > 0)
		pController = m_hCtrl.begin()->second;

	if (pController != nullptr 
		&& (pController->GetMobCtrlCount() + pController->GetNpcCtrlCount() - (pController->GetMobCtrlCount() != 0) >= 50)
		&& (nType != -3 || nMobType != 2 || !GiveUpMobController(pController)))
		pController = nullptr;

	if (pController) 
	{
		Mob* newMob = new Mob;
		*newMob = mob;
		newMob->SetFieldObjectID(atomicObjectCounter++);

		int moveAbility = newMob->GetMobTemplate()->m_nMoveAbility;
		newMob->SetMovePosition(x, y, bLeft & 1 | 2 * (moveAbility == 3 ? 6 : (moveAbility == 0 ? 1 : 0) + 1), fh);
		newMob->SetMoveAction(5); //怪物 = 5 ?

		OutPacket createMobPacket;
		newMob->MakeEnterFieldPacket(&createMobPacket);
		m_pField->BroadcastPacket(&createMobPacket);

		newMob->SetController(pController);
		newMob->SendChangeControllerPacket(pController->GetUser(), 1);
		pController->AddCtrlMob(newMob);

		m_aMobGen.insert({ newMob->GetFieldObjectID(), newMob });
	}
}

void LifePool::OnEnter(User *pUser)
{
	std::lock_guard<std::mutex> lock(m_lifePoolMutex);
	InsertController(pUser);

	for (auto& npc : m_aNpcGen)
	{
		OutPacket oPacket;
		npc.second->MakeEnterFieldPacket(&oPacket);
		pUser->SendPacket(&oPacket);
	}
	for (auto& mob : m_aMobGen)
	{
		OutPacket oPacket;
		mob.second->MakeEnterFieldPacket(&oPacket);
		pUser->SendPacket(&oPacket);
	}
}

void LifePool::InsertController(User* pUser)
{
	Controller* controller = new Controller(pUser);
	auto& iter = m_hCtrl.insert({ 0, controller });
	m_mController.insert({ pUser, iter });
	RedistributeLife();
}

void LifePool::RemoveController(User* pUser)
{
	std::lock_guard<std::mutex> lock(m_lifePoolMutex);

	//找到pUser對應的iterator
	auto& iter = m_mController.find(pUser);

	//根據iterator找到controller指標
	auto pController = iter->second->second;

	//從hCtrl中移除此controller
	m_hCtrl.erase(iter->second);

	//從pUser中移除iter
	m_mController.erase(iter);

	auto& controlled = pController->GetMobCtrlList();
	for (auto pMob : controlled)
	{
		Controller* pCtrlNew = m_pCtrlNull;
		if (m_hCtrl.size() > 0)
			pCtrlNew = m_hCtrl.begin()->second;
		pMob->SendChangeControllerPacket(pUser, 0);
		pMob->SetController(pCtrlNew);
		pCtrlNew->AddCtrlMob(pMob);
		if (pCtrlNew != m_pCtrlNull)
		{
			pMob->SendChangeControllerPacket(pCtrlNew->GetUser(), 1);
			UpdateCtrlHeap(pCtrlNew);
		}
	}

	//銷毀
	delete pController;
}

void LifePool::UpdateCtrlHeap(Controller * pController)
{
	//根據controller找到對應的pUser
	auto pUser = pController->GetUser();

	//找到pUser對應的iterator
	auto& iter = m_mController.find(pUser);


	//從hCtrl中移除此controller，並重新插入 [新的數量為key]
	m_hCtrl.erase(iter->second);
	m_mController[pUser] = m_hCtrl.insert({ pController->GetTotalControlledCount(), pController });
}

bool LifePool::GiveUpMobController(Controller * pController)
{
	return false;
}

void LifePool::RedistributeLife()
{
	Controller* pCtrl = nullptr;
	int nCtrlCount = (int)m_hCtrl.size();
	if (nCtrlCount > 0)
	{
		auto& nonControlled = m_pCtrlNull->GetMobCtrlList();
		for (auto pMob : nonControlled)
		{
			pCtrl = m_hCtrl.begin()->second;

			//控制NPC與怪物數量總和超過50，重新配置
			if (pCtrl->GetTotalControlledCount() >= 50)
				break;
			pCtrl->AddCtrlMob(pMob);

			pMob->SetController(pCtrl);
			pMob->SendChangeControllerPacket(pCtrl->GetUser(), 1);
			UpdateCtrlHeap(pCtrl);
		}
		//NPC

		Controller* minCtrl, *maxCtrl;
		int nMaxNpcCtrl, nMaxMobCtrl, nMinNpcCtrl, nMinMobCtrl;
		//重新調配每個人的怪物控制權
		if (nCtrlCount >= 2) //至少一個minCtrl與maxCtrl
		{
			while (1) 
			{
				minCtrl = m_hCtrl.begin()->second;
				maxCtrl = m_hCtrl.rbegin()->second;
				nMaxNpcCtrl = maxCtrl->GetNpcCtrlCount();
				nMaxMobCtrl = maxCtrl->GetMobCtrlCount();
				nMinNpcCtrl = minCtrl->GetNpcCtrlCount();
				nMinMobCtrl = minCtrl->GetMobCtrlCount();
				//已經足夠平衡不需要再重新配給
				if ((nMaxNpcCtrl + nMaxMobCtrl - (nMaxMobCtrl != 0) <= (nMinNpcCtrl - (nMinMobCtrl != 0) + nMinMobCtrl + 1))
					|| ((nMaxNpcCtrl + nMaxMobCtrl - (nMaxMobCtrl != 0)) <= 20))
					break;
				Mob* pMob = maxCtrl->GetMobCtrlList().back();
				maxCtrl->GetMobCtrlList().pop_back();
				pMob->SendChangeControllerPacket(maxCtrl->GetUser(), 0);

				minCtrl->AddCtrlMob(pMob);
				pMob->SetController(minCtrl);
				pMob->SendChangeControllerPacket(minCtrl->GetUser(), 1);
				UpdateCtrlHeap(minCtrl);
				UpdateCtrlHeap(maxCtrl);
			}
		}
	}
}

void LifePool::Update()
{
	TryCreateMob(false);
}

void LifePool::OnPacket(User * pUser, int nType, InPacket * iPacket)
{
	if (nType >= 0x369 && nType <= 0x38F)
	{
		OnMobPacket(pUser, nType, iPacket);
	}
}

void LifePool::OnMobPacket(User * pUser, int nType, InPacket * iPacket)
{
	int dwMobID = iPacket->Decode4();
	std::lock_guard<std::mutex> lock(m_lifePoolMutex);

	auto mobIter = m_aMobGen.find(dwMobID);
	if (mobIter != m_aMobGen.end()) {
		switch (nType)
		{
		case 0x369:
			m_pField->OnMobMove(pUser, mobIter->second, iPacket);
			break;
		}
	}
	else {
		//Release Controller
	}
}
