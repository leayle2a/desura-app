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

#ifndef DESURA_DOWNLOADTHREAD_H
#define DESURA_DOWNLOADTHREAD_H
#ifdef _WIN32
#pragma once
#endif

#include "util_thread/BaseThread.h"

#include "BaseItemTask.h"
#include "usercore/ItemHelpersI.h"
#include "mcfcore/MCFI.h"
#include "usercore/ToolManagerI.h"

#include "BDManager.h"

namespace UserCore
{
	namespace Task
	{
		class DownloadBannerTask;
	}
	namespace ItemTask
	{
		class DownloadTask : public UserCore::ItemTask::BaseItemTask, public UserCore::Misc::BannerNotifierI
		{
		public:
			DownloadTask(gcRefPtr<UserCore::Item::ItemHandleI> handle, const char* mcfPath);
			virtual ~DownloadTask();

		protected:
			void doRun();

			virtual void onPause();
			virtual void onUnpause();
			virtual void onStop();
			virtual void cancel();

			void clearEvents();

			void onProgress(MCFCore::Misc::ProgressInfo& p);

			void onBannerError(gcException &e);
			void onBannerComplete(MCFCore::Misc::DownloadProvider &info);

			void onNewProvider(MCFCore::Misc::DP_s& dp);

			void onError(gcException &e);
			void sortLocalMcfs(MCFBranch branch, MCFBuild build);

			void onComplete(gcString &savePath);
			void onToolComplete();

			void startToolDownload();

			gc_IMPLEMENT_REFCOUNTING(DownloadTask);

		private:
			MCFBuild m_uiOldBuild;
			MCFBranch m_uiOldBranch;

			ToolTransactionId m_ToolTTID = -1;

			bool m_bInError = false;
			bool m_bToolDownloadComplete = false;
			bool m_bInitFinished = false;

			gcString m_szMcfPath;
		};
	}
}

#endif //DESURA_DOWNLOADTHREAD_H
