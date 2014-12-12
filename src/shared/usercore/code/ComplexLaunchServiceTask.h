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

#ifndef DESURA_COMPLEXLAUNCHSERVICE_H
#define DESURA_COMPLEXLAUNCHSERVICE_H
#ifdef _WIN32
#pragma once
#endif


#include "BaseItemServiceTask.h"
#include "mcfcore/ProgressInfo.h"

class IPCComplexLaunch;

namespace UserCore
{

	namespace Item
	{
		namespace Helper
		{
			class InstallerHandleHelperI;
		}
	}

	namespace ItemTask
	{
		enum class RemoveResult
		{
			Failed,
			Started,
			Ignored
		};

		class ComplexLaunchServiceTask : public BaseItemServiceTask
		{
		public:
			ComplexLaunchServiceTask(gcRefPtr<UserCore::Item::ItemHandleI> handle, bool clean, MCFBranch branch, MCFBuild build, gcRefPtr<UserCore::Item::Helper::InstallerHandleHelperI> &ihh);
			~ComplexLaunchServiceTask();

			void completeStage()
			{
				m_bLaunch = false;
				m_bCompleteStage = true;
			}

			void launch()
			{
				m_bLaunch = true;
				m_bCompleteStage = false;
			}

		protected:
			bool initService();
			void onComplete();

			void onMcfError(gcException &e);
			void onProgress(MCFCore::Misc::ProgressInfo& prog);
			void onError(gcException &e);

			bool backUp();
			bool install();
			RemoveResult remove();
			bool removeAndInstall();

			void completeRemove();
			void completeInstall();

			gcString getFullMcf();
			void onFinish();

			void onTrueComplete();
			bool isFilesToRestore();

		private:
			bool m_bClean;
			bool m_bHashMissMatch = false;
			bool m_bCompleteStage = false;
			bool m_bLaunch = false;

			uint8 m_iTier = 0;
			uint8 m_iMode = 0;

			DesuraId m_iRemoveId;

			std::shared_ptr<IPCComplexLaunch> m_pIPCIM;
			std::shared_ptr<gcException> m_pException;

			gcRefPtr<UserCore::Item::Helper::InstallerHandleHelperI> m_pIHH;
		};
	}
}

#endif //DESURA_COMPLEXLAUNCHSERVICE_H
