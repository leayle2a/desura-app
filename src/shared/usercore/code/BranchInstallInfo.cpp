/*
Copyright (C) 2011 Mark Chandler (Desura Net Pty Ltd)
Copyright (C) 2014 Bad Juju Games, Inc.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA.

Contact us at legal@badjuju.com.

*/


#include "Common.h"
#include "BranchInstallInfo.h"
#include "sqlite3x.hpp"
#include "sql/ItemInfoSql.h"

#include "ItemInfo.h"

#include "managers/WildcardManager.h"
#include "XMLMacros.h"

namespace UM = UserCore::Item;

class InsCheck
{
public:
	InsCheck(const char *c, const char *p)
	{
		check = gcString(c);
		path = gcString(p);
	}

	gcString check;
	gcString path;
};

using namespace UserCore::Item;


BranchInstallInfo::BranchInstallInfo(uint32 biId, gcRefPtr<BranchItemInfoI> itemInfo, UTIL::FS::UtilFSI* pFileSystem)
	: m_BiId(biId)
	, m_pItem(itemInfo)
	, m_ItemId(itemInfo->getId())
	, m_uiInstallSize(0)
	, m_uiDownloadSize(0)
	, m_pFileSystem(pFileSystem)
{
}

BranchInstallInfo::~BranchInstallInfo()
{
}

void BranchInstallInfo::deleteFromDb(sqlite3x::sqlite3_connection* db)
{
	try
	{
		{
			sqlite3x::sqlite3_command cmd(*db, "DELETE FROM installinfo WHERE itemid=? AND biid=?;");
			cmd.bind(1, (long long int)m_ItemId.toInt64());
			cmd.bind(2, (int)m_BiId);
			cmd.executenonquery();
		}

		{
			sqlite3x::sqlite3_command cmd(*db, "DELETE FROM installinfoex WHERE itemid=? AND biid=?;");
			cmd.bind(1, (long long int)m_ItemId.toInt64());
			cmd.bind(2, (int)m_BiId);
			cmd.executenonquery();
		}

		{
			sqlite3x::sqlite3_command cmd(*db, "DELETE FROM exe WHERE itemid=? AND biid=?;");
			cmd.bind(1, (long long int)m_ItemId.toInt64());
			cmd.bind(2, (int)m_BiId);
			cmd.executenonquery();
		}
	}
	catch (...)
	{
	}
}

void BranchInstallInfo::saveDb(sqlite3x::sqlite3_connection* db)
{
	sqlite3x::sqlite3_command cmd(*db, "REPLACE INTO installinfo VALUES (?,?,?,?, ?,?,?,?);");

	cmd.bind(1, (long long int)m_ItemId.toInt64());
	cmd.bind(2, (int)m_BiId);
	cmd.bind(3, UTIL::OS::getRelativePath(m_szPath));
	cmd.bind(4, UTIL::OS::getRelativePath(m_szInsCheck));
	cmd.bind(5, UTIL::OS::getRelativePath(m_szInsPrim));
	cmd.bind(6, (long long int)m_iInstalledMod.toInt64());
	cmd.bind(7, (int)m_INBuild);
	cmd.bind(8, (int)m_LastBuild);

	cmd.executenonquery();

	for (auto ei : m_vExeList)
	{
		sqlite3x::sqlite3_command cmd(*db, "REPLACE INTO exe VALUES (?,?,?,?,?,?,?);");

		cmd.bind(1, (long long int)m_ItemId.toInt64());
		cmd.bind(2, (int)m_BiId);
		cmd.bind(3, ei->m_szName);
		cmd.bind(4, UTIL::OS::getRelativePath(ei->m_szExe));
		cmd.bind(5, ei->m_szExeArgs);
		cmd.bind(6, ei->m_szUserArgs);
		cmd.bind(7, (int)ei->m_uiRank);

		cmd.executenonquery();
	}

	if (isInstalled())
	{
		for (gcString &check : m_vInstallChecks)
		{
			sqlite3x::sqlite3_command cmd(*db, "REPLACE INTO installinfoex VALUES (?,?,?);");

			cmd.bind(1, (long long int)m_ItemId.toInt64());
			cmd.bind(2, (int)m_BiId);
			cmd.bind(3, check);

			cmd.executenonquery();
		}
	}
	else
	{
		sqlite3x::sqlite3_command cmd(*db, "DELETE FROM installinfoex WHERE itemid=? AND biid=?;");
		cmd.bind(1, (long long int)m_ItemId.toInt64());
		cmd.bind(2, (int)m_BiId);
		cmd.executenonquery();
	}
}

void BranchInstallInfo::loadDb(sqlite3x::sqlite3_connection* db)
{
	if (!db)
		return;

	{
		sqlite3x::sqlite3_command cmd(*db, "SELECT * FROM installinfo WHERE itemid=? AND biid=?;");
		cmd.bind(1, (long long int)m_ItemId.toInt64());
		cmd.bind(2, (int)m_BiId);

		sqlite3x::sqlite3_reader reader = cmd.executereader();
		reader.read();

		m_szPath		= UTIL::OS::getAbsPath(reader.getstring(2)); //install path
		m_szInsCheck	= UTIL::OS::getAbsPath(reader.getstring(3)); //install check
		m_szInsPrim		= UTIL::OS::getAbsPath(reader.getstring(4)); //install primary

		m_iInstalledMod = DesuraId(reader.getint64(5));
		m_INBuild		= MCFBuild::BuildFromInt(reader.getint(6));
		m_LastBuild		= MCFBuild::BuildFromInt(reader.getint(7));
	}

	if (isInstalled())
	{
		sqlite3x::sqlite3_command cmd(*db, "SELECT installcheck FROM installinfoex WHERE itemid=? AND biid=?;");
		cmd.bind(1, (long long int)m_ItemId.toInt64());
		cmd.bind(2, (int)m_BiId);

		sqlite3x::sqlite3_reader reader = cmd.executereader();

		while (reader.read())
		{
			m_vInstallChecks.push_back(reader.getstring(0));
		}

		if (!isValidFile(m_szInsCheck))
		{
			for (auto file : m_vInstallChecks)
			{
				if (isValidFile(file))
				{
					m_szInsCheck = file;
					break;
				}
			}
		}
	}

	{
		sqlite3x::sqlite3_command cmd(*db, "SELECT * FROM exe WHERE itemid=? AND biid=?;");
		cmd.bind(1, (long long int)m_ItemId.toInt64());
		cmd.bind(2, (int)m_BiId);

		sqlite3x::sqlite3_reader reader = cmd.executereader();

		while (reader.read())
		{
			gcString name = reader.getstring(2);
			gcString exe = UTIL::OS::getAbsPath(reader.getstring(3));
			gcString exeargs = reader.getstring(4);
			gcString userargs = reader.getstring(5);
			uint32 rank = reader.getint(6);

			gcRefPtr<ExeInfo> ei;

			for (auto exe : m_vExeList)
			{
				if (exe->m_szName == name)
				{
					ei = exe;
					break;
				}
			}

			if (!ei)
			{
				ei = gcRefPtr<ExeInfo>::create(name.c_str());
				m_vExeList.push_back(ei);
			}

			ei->m_szExe = exe;
			ei->m_szExeArgs = exeargs;
			ei->m_szUserArgs = userargs;
			ei->m_uiRank = rank;
		}
	}
}


void BranchInstallInfo::extractInstallChecks(const XML::gcXMLElement &icsNode, gcRefPtr<WildcardManager> &pWildCard, std::vector<InsCheck> &vInsChecks)
{
	icsNode.for_each_child("installlocation", [&vInsChecks, pWildCard, this](const XML::gcXMLElement &icNode)
	{
		const gcString iCheck = icNode.GetChild("check");
		const gcString iPath = icNode.GetChild("path");

		if (iCheck.empty() || iPath.empty())
			return;

		try
		{
			gcString strCheckRes = pWildCard->constructPath(iCheck.c_str());

			if (isInstalled())
			{
				if (!updateInstallCheck(strCheckRes, iPath))
					return;

				vInsChecks.push_back(InsCheck(strCheckRes.c_str(), m_szPath.c_str()));
			}
			else
			{
				vInsChecks.push_back(InsCheck(strCheckRes.c_str(), iPath.c_str()));
			}
		}
		catch (...)
		{
		}
	});
}

bool BranchInstallInfo::updateInstallCheck(gcString &strCheckRes, const gcString &strPath)
{
	VERIFY_OR_RETURN(isInstalled(), true);

	//if we are installed we might be updating our install check path. So if the install
	// check and install path start with the same folder names strip them and replace with
	// current install path

	UTIL::FS::Path insCheckPath(strCheckRes, "", true);
	UTIL::FS::Path insPath(strPath, "", false);

	size_t x=0;
	while (insCheckPath.getFolderCount() > x && insPath.getFolderCount() > x && insCheckPath.getFolder(x) == insPath.getFolder(x))
	{
		x++;
	}

	if (x == 0 || x != insPath.getFolderCount())
		return false;

	strCheckRes = m_szPath;

	for (size_t y = x; y < insCheckPath.getFolderCount(); ++y)
	{
		strCheckRes += UTIL::FS::Path::GetDirSeperator() + insCheckPath.getFolder(y);
	}

	strCheckRes += UTIL::FS::Path::GetDirSeperator() + insCheckPath.getFile().getFile();
	return true;
}

void BranchInstallInfo::UpdateInstallCheckList(const std::vector<InsCheck> &vInsChecks, gcRefPtr<WildcardManager> &pWildCard)
{
	VERIFY_OR_RETURN(!!pWildCard, );

	for (const InsCheck &check : vInsChecks)
	{
		gcString strPath;

		try
		{
			strPath = pWildCard->constructPath(check.path.c_str());
		}
		catch (...)
		{
			continue;
		}

		if (strPath.find(m_szPath) != 0)
			continue;

		if (std::find(begin(m_vInstallChecks), end(m_vInstallChecks), check.check) != m_vInstallChecks.end())
			continue;

		m_vInstallChecks.push_back(check.check);
	}
}

ProcessResult BranchInstallInfo::processSettings(const XML::gcXMLElement &setNode, gcRefPtr<WildcardManager> &pWildCard, bool reset, bool hasBroughtItem, const char* cipPath)
{
	ProcessResult pr;
	pr.found = false;
	pr.useCip = false;
	pr.notFirst = false;

	auto icsNode = setNode.FirstChildElement("installlocations");

	if (!isInstalled())
		m_vInstallChecks.clear();

	if (icsNode.IsValid())
	{
		std::vector<InsCheck> vInsChecks;

		extractInstallChecks(icsNode, pWildCard, vInsChecks);
		UpdateInstallCheckList(vInsChecks, pWildCard);

		size_t size = vInsChecks.size();

		//only care about the first item for bought items when we are not already installed
		if ((reset || (hasBroughtItem && !isInstalled())) && size)
			size = 1;

		for (size_t x=0; x<size; x++)
		{
			if (isValidFile(vInsChecks[x].check))
			{
				setInsCheck(vInsChecks[x].check.c_str());

				if (isInstalled())
					pWildCard->updateInstallWildcard("INSTALL_PATH", m_szPath.c_str());
				else
					pWildCard->updateInstallWildcard("INSTALL_PATH", vInsChecks[x].path.c_str());

				pr.found = true;
				pr.notFirst = (x != 0);
				break;
			}
		}

		if (!pr.found && vInsChecks.size()>0)
		{
			if (isInstalled())
			{
				pWildCard->updateInstallWildcard("INSTALL_PATH", m_szPath.c_str());
			}
			else if (cipPath)
			{
				pWildCard->updateInstallWildcard("INSTALL_PATH", cipPath);
				pr.useCip = true;
			}
			else
			{
				char* CheckRes = nullptr;
				try
				{
					pWildCard->constructPath(vInsChecks[0].check.c_str(), &CheckRes);

					setInsCheck(CheckRes);
					pWildCard->updateInstallWildcard("INSTALL_PATH", vInsChecks[0].path.c_str());
				}
				catch (gcException)
				{
				}

				safe_delete(CheckRes);
			}
		}

		char *iPathRes = nullptr;
		char* insPrim = nullptr;
		char* insPrimRes = nullptr;

		setNode.GetChild("installprimary", insPrim);

		try
		{
			pWildCard->constructPath("%INSTALL_PATH%", &iPathRes);
			if (iPathRes)
				setPath(iPathRes);
			else
				Warning("ItemInfo: Install path for {0} is nullptr.\n");
		}
		catch (gcException &)
		{
		}

		try
		{
			pWildCard->constructPath(insPrim, &insPrimRes);
			if (insPrimRes)
				setInsPrimary(insPrimRes);
			else
				setInsPrimary("");
		}
		catch (gcException &)
		{
			setInsPrimary("");
		}

		safe_delete(iPathRes);
		safe_delete(insPrim);
		safe_delete(insPrimRes);

		processExes(setNode, pWildCard, pr.useCip);
		launchExeHack();
	}

	if (m_vInstallChecks.size() == 0)
		m_vInstallChecks.push_back(m_szInsCheck);

	pr.insCheck = m_szInsCheck;
	return pr;
}


gcRefPtr<UserCore::Item::Misc::ExeInfoI> BranchInstallInfo::getActiveExe()
{
	gcRefPtr<UserCore::Item::Misc::ExeInfoI> ei;

	for (auto exe : m_vExeList)
	{
		if (m_szActiveExe == exe->getName())
		{
			ei = exe;
			break;
		}
	}

	if (!ei)
	{
		uint32 rank = 0;

		for (auto exe : m_vExeList)
		{
			if (!ei || exe->m_uiRank > rank)
			{
				ei = exe;
				rank = exe->m_uiRank;
			}
		}
	}

	return ei;
}

void BranchInstallInfo::setActiveExe(const char* name)
{
	gcString n(name);

	for (size_t x=0; x<m_vExeList.size(); x++)
	{
		if (n == m_vExeList[x]->getName())
		{
			m_szActiveExe = name;
			return;
		}
	}

	Warning("Failed to set active exe to [{0}]\n", name);
}

void BranchInstallInfo::getExeList(std::vector<gcRefPtr<UserCore::Item::Misc::ExeInfoI>> &list)
{
	for (auto exe : m_vExeList)
	{
		if (isValidFile(exe->getExe()))
			list.push_back(exe.get());
	}

	if (list.size() == 0 && m_vExeList.size() > 0)
		list.push_back(m_vExeList[0].get());

	std::sort(list.begin(), list.end(), [](const gcRefPtr<UserCore::Item::Misc::ExeInfoI> &a, const gcRefPtr<UserCore::Item::Misc::ExeInfoI> &b) -> bool
	{
		auto ae = gcRefPtr<ExeInfo>::dyn_cast(a);
		auto be = gcRefPtr<ExeInfo>::dyn_cast(b);

		return ae->m_uiRank < be->m_uiRank;
	});
}

uint32 BranchInstallInfo::getExeCount(bool setActive)
{
	uint32 count = 0;
	gcRefPtr<ExeInfo> ei;

	for (auto exe : m_vExeList)
	{
		if (isValidFile(exe->getExe()))
		{
			count++;

			if (!ei)
				ei = exe;
		}
	}

	if (setActive && count == 1 && ei)
		setActiveExe(ei->getName());

	//should always have 1 exe
	if (count == 0)
		count = 1;

	return count;
}

bool BranchInstallInfo::isInstalled()
{
	return (m_pItem->getStatus() & UM::ItemInfoI::STATUS_INSTALLED) == UM::ItemInfoI::STATUS_INSTALLED;
}

void BranchInstallInfo::setPath(const char *path)
{
	if (m_szPath == path)
		return;

	//should never set path when installed
	VERIFY_OR_RETURN(!isInstalled(), );

	if (!path)
		m_szPath = gcString();
	else
		m_szPath = UTIL::FS::PathWithFile(path).getFullPath();
}

void BranchInstallInfo::setInsPrimary(const char* path)
{
	if (m_szInsPrim == path)
		return;

	//should never set path when installed
	VERIFY_OR_RETURN(!isInstalled(), );

	if (!path)
		m_szInsPrim = gcString();
	else
		m_szInsPrim = UTIL::FS::PathWithFile(path).getFullPath();
}

//only can change this if it is not installed
void BranchInstallInfo::setInsCheck(const char* path)
{
	if (m_szInsCheck == path)
		return;

	bool bCheckFailed = false;

	if (isInstalled())
	{
		UTIL::FS::Path p1(m_szInsCheck, "", false);
		UTIL::FS::Path p2(path, "", false);

		//should not change dir when updating install check when installed
		bCheckFailed = p1.getFolderPath() == p2.getFolderPath();
	}

	//should never set path when installed
	VERIFY_OR_RETURN(!bCheckFailed, );

	if (!path)
		m_szInsCheck = gcString();
	else
		m_szInsCheck = UTIL::FS::PathWithFile(path).getFullPath();
}

void BranchInstallInfo::launchExeHack()
{
#ifdef WIN32
	//deus ex
	if (m_ItemId.getType() == DesuraId::TYPE_GAME && m_ItemId.getItem() == 14)
	{
		std::string steam = UTIL::OS::getConfigValue("HKEY_CURRENT_USER\\SOFTWARE\\Valve\\Steam\\SteamPath");
		gcString steamPath = UTIL::FS::Path(steam, "", false).getFullPath();

		if (steamPath.size() > 0)
		{
			for (auto ei : m_vExeList)
			{
				if (ei->m_szExe.find(steamPath) != std::string::npos)
				{
					ei->m_szExe = gcString("{0}\\steam.exe", steamPath);

					if (ei->m_szName == "Play")
						ei->m_szExeArgs = gcString("-applaunch 6910");
					else
						ei->m_szExeArgs += gcString("-applaunch 6910");
				}
			}
		}
	}
#endif
}

void BranchInstallInfo::processExes(const XML::gcXMLElement &setNode, gcRefPtr<WildcardManager> &pWildCard, bool useCip)
{
	uint32 rank = 0;

	setNode.FirstChildElement("executes").for_each_child("execute", [&](const XML::gcXMLElement &xmlChild)
	{
		gcString ePath;
		gcString args;
		gcString name;

		xmlChild.GetChild("name", name);
		xmlChild.GetChild("exe", ePath);
		xmlChild.GetChild("args", args);

		if (ePath.size() == 0)
			return;

		if (name.size() == 0)
			name = "Play";

		char* ePathRes = nullptr;
		char* argsRes = nullptr;

		gcRefPtr<ExeInfo> ei;

		for (auto exe : m_vExeList)
		{
			if (exe->m_szName == name)
			{
				ei = exe;
				break;
			}
		}

		if (!ei)
		{
			ei = gcRefPtr<ExeInfo>::create(name.c_str());
			m_vExeList.push_back(ei);
		}

		ei->m_uiRank = rank;

		try
		{
			pWildCard->constructPath(ePath.c_str(), &ePathRes);
			if (ePathRes)
			{
				ei->setExe(ePathRes);

				//cip needs an install check
				if (useCip)
					setInsCheck(ePathRes);
			}
			else
			{
				ei->setExe(ePath.c_str());
			}
		}
		catch (gcException &)
		{
			ei->setExe(ePath.c_str());
		}

		try
		{
			pWildCard->constructPath(args.c_str(), &argsRes, false);
			ei->m_szExeArgs = argsRes ? argsRes : args;
		}
		catch (gcException &)
		{
			ei->m_szExeArgs = args;
		}

		safe_delete(ePathRes);
		safe_delete(argsRes);
		rank++;
	});
}


void BranchInstallInfo::updated()
{
	if (m_NextBuild > m_INBuild)
	{
		m_LastBuild = m_INBuild;
		m_INBuild = m_NextBuild;
	}
}

bool BranchInstallInfo::setInstalledMcf(MCFBuild build)
{
	m_LastBuild = m_INBuild;
	m_INBuild = build;

	if (m_NextBuild == m_INBuild)
	{
		m_NextBuild = MCFBuild();
		return true;
	}

	return false;
}

void BranchInstallInfo::overideInstalledBuild(MCFBuild build)
{
	m_LastBuild = m_INBuild;
	m_INBuild = build;
}

void BranchInstallInfo::resetInstalledMcf()
{
	m_LastBuild = MCFBuild();
	m_INBuild = MCFBuild();
}

bool BranchInstallInfo::processUpdateXml(const XML::gcXMLElement &branch)
{
	auto mcfEl = branch.FirstChildElement("mcf");
	if (mcfEl.IsValid())
	{
		const std::string id = mcfEl.GetAtt("id");

		if (!id.empty())
		{
			uint32 build = -1;
			mcfEl.GetChild("build", build);

			if (m_INBuild == 0 || build > m_INBuild)
				m_NextBuild = MCFBuild::BuildFromInt(build);
			else
				m_NextBuild = m_INBuild;
		}
	}

	return (m_NextBuild > m_INBuild);
}

void BranchInstallInfo::setLinkInfo(const char* exe, const char* args)
{
	if (m_vExeList.size() == 0)
		m_vExeList.push_back(gcRefPtr<ExeInfo>::create("Link"));

	gcRefPtr<ExeInfo>& info = m_vExeList[0];

	info->setExe(exe);
	info->setUserArgs(args);
}

bool BranchInstallInfo::isValidFile(const gcString &strFile)
{
	return m_pFileSystem->isValidFile(UTIL::FS::PathWithFile(strFile));
}

void BranchInstallInfo::setLinkInfo(const char* szPath, const char* szExe, const char* szArgs)
{
	if ((m_pItem->getStatus() & ItemInfo::STATUS_LINK) != ItemInfo::STATUS_LINK)
	{
		gcAssert(false);
		return;
	}

	setPath(szPath);
	setInsCheck(szExe);
	setLinkInfo(szExe, szArgs);
}


#ifdef WITH_GTEST

namespace UnitTest
{
	using namespace UserCore;
	using namespace UserCore::Item;

	class StubBranchItemInfo : public BranchItemInfoI
	{
	public:
		DesuraId getId() override
		{
			return DesuraId("1", "games");
		}

		uint32 getStatus() override
		{
			return m_nStatus;
		}

		uint32 m_nStatus = 0;

		gc_IMPLEMENT_REFCOUNTING(StubBranchItemInfo);
	};

	class TestBranchInstallInfo : public BranchInstallInfo
	{
	public:
		TestBranchInstallInfo(uint32 biId, gcRefPtr<BranchItemInfoI> itemInfo)
			: BranchInstallInfo(biId, itemInfo, UTIL::FS::g_pDefaultUTILFS)
		{
		}

		bool isValidFile(const gcString &strFile) override
		{
			UTIL::FS::Path a(UTIL::FS::PathWithFile(strFile));

			for (auto &strFileIt : m_vValidFiles)
			{
				UTIL::FS::Path b(UTIL::FS::PathWithFile(strFileIt));

				if (a == b)
					return true;
			}

			return false;
		}

		std::vector<gcString> m_vValidFiles;
	};



	class BranchInstallInfoFixture : public ::testing::Test
	{
	public:
		BranchInstallInfoFixture()
			: m_BranchItemInfo(gcRefPtr<StubBranchItemInfo>::create())
			, m_BranchInstallInfo(gcRefPtr<TestBranchInstallInfo>::create(1, m_BranchItemInfo))
		{
		}

		void setBuild(MCFBuild nBuild)
		{
			m_BranchInstallInfo->m_INBuild = nBuild;
		}

		MCFBuild getNextBuild()
		{
			return m_BranchInstallInfo->m_NextBuild;
		}

		bool processUpdateXml(const XML::gcXMLElement& branch)
		{
			return m_BranchInstallInfo->processUpdateXml(branch);
		}

		ProcessResult processSettings(const XML::gcXMLElement& setNode, gcRefPtr<WildcardManager> &pWildCard, bool reset, bool hasBroughtItem, const char* cipPath)
		{
			return m_BranchInstallInfo->processSettings(setNode, pWildCard, reset, hasBroughtItem, cipPath);
		}

		void setInstallInfo(const gcString &strPath, const gcString &strCheck, const gcString &strPrim)
		{
			auto strNormCheck = UTIL::FS::PathWithFile(strCheck).getFullPath();

			m_BranchInstallInfo->m_szPath = UTIL::FS::Path(strPath, "", false).getFullPath();
			m_BranchInstallInfo->m_szInsCheck = strNormCheck;
			m_BranchInstallInfo->m_szInsPrim = UTIL::FS::Path(strPrim, "", false).getFullPath();

			m_BranchInstallInfo->m_vInstallChecks.push_back(strNormCheck);
		}

		void checkInstallInfo(const gcString &strPath, const gcString &strCheck, const gcString &strPrim, int nValidFileCount = -1)
		{
			ASSERT_PATHEQ(strPath, m_BranchInstallInfo->m_szPath);
			ASSERT_FILEEQ(strCheck, m_BranchInstallInfo->m_szInsCheck);
			ASSERT_PATHEQ(strPrim, m_BranchInstallInfo->m_szInsPrim);

			if (m_BranchInstallInfo->isInstalled())
			{
				if (nValidFileCount == -1)
					ASSERT_NE(0, m_BranchInstallInfo->m_vInstallChecks.size());
				else
					ASSERT_EQ(nValidFileCount, m_BranchInstallInfo->m_vInstallChecks.size());
			}

		}

		bool updateInstallCheck(gcString &strCheckRes, const gcString &strPath)
		{
			return m_BranchInstallInfo->updateInstallCheck(strCheckRes, strPath);
		}

		void extractInstallChecks(const XML::gcXMLElement &icsNode, gcRefPtr<WildcardManager> &pWildCard, std::vector<InsCheck> &vInsChecks)
		{
			m_BranchInstallInfo->extractInstallChecks(icsNode, pWildCard, vInsChecks);
		}

		gcRefPtr<StubBranchItemInfo> m_BranchItemInfo;
		gcRefPtr<TestBranchInstallInfo> m_BranchInstallInfo;
	};


	TEST_F(BranchInstallInfoFixture, processUpdateXml_NewBuild)
	{
		setBuild(MCFBuild::BuildFromInt(2));

		tinyxml2::XMLDocument doc;
		doc.Parse("<branch><mcf id=\"123\"><build>3</build></mcf></branch>");

		ASSERT_TRUE(processUpdateXml(doc.RootElement())); //processUpdateXml should of returned true indicating new build
		ASSERT_EQ(MCFBuild::BuildFromInt(3), getNextBuild());
	}

	TEST_F(BranchInstallInfoFixture, processUpdateXml_SameBuild)
	{
		setBuild(MCFBuild::BuildFromInt(2));

		tinyxml2::XMLDocument doc;
		doc.Parse("<branch><mcf id=\"123\"><build>2</build></mcf></branch>");

		ASSERT_FALSE(processUpdateXml(doc.RootElement())); //processUpdateXml should of returned false indicating no new build");
		ASSERT_EQ(MCFBuild::BuildFromInt(2), getNextBuild());
	}

	TEST_F(BranchInstallInfoFixture, processUpdateXml_OldBuild)
	{
		setBuild(MCFBuild::BuildFromInt(2));

		tinyxml2::XMLDocument doc;
		doc.Parse("<branch><mcf id=\"123\"><build>1</build></mcf></branch>");

		ASSERT_FALSE(processUpdateXml(doc.RootElement())); //processUpdateXml should of returned false indicating no new build");
		ASSERT_EQ(MCFBuild::BuildFromInt(2), getNextBuild());
	}


	static const char* gs_szSettingsXml =
			"<branch>"
				"<installprimary>insprim</installprimary>"
				"<installlocations>"
					"<installlocation><check>path_a\\check_a.txt</check><path>path_a</path></installlocation>"
					"<installlocation><check>path_b\\check_b.txt</check><path>path_b</path></installlocation>"
					"<installlocation><check>path_c\\check_c.txt</check><path>path_c</path></installlocation>"
				"</installlocations>"
				"<executes>"
					"<execute><name>Test</name><exe>test.exe</exe><args></args></execute>"
				"</executes>"
			"</branch>";

	static const char* gs_szInstallLocationsXml =
				"<installlocations>"
					"<installlocation><check>%WILDCARD_A%\\%WILDCARD_B%\\a.txt</check><path>%WILDCARD_A%\\%WILDCARD_B%</path></installlocation>"
					"<installlocation><check>relativepath\\b.txt</check><path>relativepath</path></installlocation>"
				"</installlocations>";

	static const char* gs_szWildcardXml =
		"<wcards>"
			"<wcard name=\"WILDCARD_A\" type=\"path\">alpha</wcard>"
			"<wcard name=\"WILDCARD_B\" type=\"path\">bravo</wcard>"
		"</wcards>";


	TEST_F(BranchInstallInfoFixture, processSettings_InitEmpty)
	{
		tinyxml2::XMLDocument doc;
		doc.Parse(gs_szSettingsXml);

		m_BranchInstallInfo->m_vValidFiles.push_back("path_a\\check_a.txt");

		auto wc = gcRefPtr<WildcardManager>::create();
		auto res = processSettings(doc.RootElement(), wc, false, false, nullptr);

		ASSERT_TRUE(res.found);
		ASSERT_FILEEQ("path_a\\check_a.txt", res.insCheck.c_str());

		checkInstallInfo("path_a", "path_a\\check_a.txt", "insprim");
	}

	TEST_F(BranchInstallInfoFixture, processSettings_ExistingSame)
	{
		tinyxml2::XMLDocument doc;
		doc.Parse(gs_szSettingsXml);

		m_BranchInstallInfo->m_vValidFiles.push_back("path_a\\check_a.txt");

		setInstallInfo("path_a", "path_a\\check_a.txt", "insprim");

		auto wc = gcRefPtr<WildcardManager>::create();
		auto res = processSettings(doc.RootElement(), wc, false, false, nullptr);

		ASSERT_TRUE(res.found);
		ASSERT_FILEEQ("path_a\\check_a.txt", res.insCheck.c_str());

		checkInstallInfo("path_a", "path_a\\check_a.txt", "insprim");
	}

	TEST_F(BranchInstallInfoFixture, processSettings_ExistingDifferent_NotInstalled)
	{
		tinyxml2::XMLDocument doc;
		doc.Parse(gs_szSettingsXml);

		m_BranchInstallInfo->m_vValidFiles.push_back("path_b\\check_b.txt");

		setInstallInfo("path_a", "path_a\\check_a.txt", "insprim");

		auto wc = gcRefPtr<WildcardManager>::create();
		auto res = processSettings(doc.RootElement(), wc, false, false, nullptr);

		ASSERT_TRUE(res.found);
		ASSERT_FILEEQ("path_b\\check_b.txt", res.insCheck.c_str());

		//as we are not installed can freely change the path here
		checkInstallInfo("path_b", "path_b\\check_b.txt", "insprim");
	}

	TEST_F(BranchInstallInfoFixture, processSettings_ExistingDifferent_Installed)
	{
		m_BranchItemInfo->m_nStatus = ItemInfo::STATUS_INSTALLED;


		tinyxml2::XMLDocument doc;
		doc.Parse(gs_szSettingsXml);

		m_BranchInstallInfo->m_vValidFiles.push_back("path_a\\check_b.txt");

		setInstallInfo("path_a", "path_a\\check_a.txt", "insprim");

		auto wc = gcRefPtr<WildcardManager>::create();
		auto res = processSettings(doc.RootElement(), wc, false, false, nullptr);

		ASSERT_TRUE(res.found);
		ASSERT_FILEEQ("path_a\\check_b.txt", res.insCheck.c_str());

		checkInstallInfo("path_a", "path_a\\check_b.txt", "insprim", 3);
	}


	TEST_F(BranchInstallInfoFixture, updateInstallCheck_RelatedPaths)
	{
		m_BranchItemInfo->m_nStatus = ItemInfo::STATUS_INSTALLED;
		setInstallInfo("D:\\test", "D:\\test\\oldcheck.txt", "insprim");

		gcString strPath("C:\\abc\\def\\check.txt");

		ASSERT_TRUE(updateInstallCheck(strPath, "C:\\abc"));
		ASSERT_FILEEQ("D:\\test\\def\\check.txt", strPath.c_str());
	}

	TEST_F(BranchInstallInfoFixture, updateInstallCheck_DiffPaths_SameDrive)
	{
		m_BranchItemInfo->m_nStatus = ItemInfo::STATUS_INSTALLED;
		setInstallInfo("D:\\test", "D:\\test\\oldcheck.txt", "insprim");

		gcString strPath("C:\\abc\\def\\check.txt");

		ASSERT_FALSE(updateInstallCheck(strPath, "C:\\123"));
		ASSERT_FILEEQ("C:\\abc\\def\\check.txt", strPath.c_str());
	}

	TEST_F(BranchInstallInfoFixture, updateInstallCheck_SamePaths_DiffDrive)
	{
		m_BranchItemInfo->m_nStatus = ItemInfo::STATUS_INSTALLED;
		setInstallInfo("D:\\test", "D:\\test\\oldcheck.txt", "insprim");

		gcString strPath("C:\\abc\\def\\check.txt");

		ASSERT_FALSE(updateInstallCheck(strPath, "D:\\abc"));
		ASSERT_FILEEQ("C:\\abc\\def\\check.txt", strPath.c_str());
	}


	TEST_F(BranchInstallInfoFixture, extractInstallChecks_expandPath)
	{
		auto wildCards = gcRefPtr<WildcardManager>::create();

		{
			tinyxml2::XMLDocument doc;
			doc.Parse(gs_szWildcardXml);
			wildCards->parseXML(doc.RootElement());
		}

		std::vector<InsCheck> vInsChecks;

		tinyxml2::XMLDocument doc;
		doc.Parse(gs_szInstallLocationsXml);

		extractInstallChecks(doc.RootElement(), wildCards, vInsChecks);

		ASSERT_EQ(2, vInsChecks.size());

		ASSERT_FILEEQ("alpha\\bravo\\a.txt", vInsChecks[0].check.c_str());
		ASSERT_PATHEQ("%WILDCARD_A%\\%WILDCARD_B%", vInsChecks[0].path.c_str());

		ASSERT_FILEEQ("relativepath\\b.txt", vInsChecks[1].check.c_str());
		ASSERT_PATHEQ("relativepath", vInsChecks[1].path.c_str());
	}
}

#endif
