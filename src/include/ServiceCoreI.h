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

#ifndef DESURA_SERVICECOREI_H
#define DESURA_SERVICECOREI_H
#ifdef _WIN32
#pragma once
#endif

#define SERVICE_CORE "service_core_001"

typedef void (*CrashSettingFn)(const wchar_t*, bool);
typedef void (*DisconnectFn)();

class TracerI;

//! Service core handles all the tasks that need administration rights to perform
class ServiceCoreI
{
public:
	//! Set up da pipe to start communications from the client
	//!
	virtual void startPipe()=0;

	//! Stop da pipe
	//!
	virtual void stopPipe()=0;

	//! Stop windows service
	//!
	virtual bool stopService(const char* serviceName)=0;

	//! Destory this instance
	//!
	virtual void destroy()=0;

	//! Set the crash settings callback
	//!
	virtual void setCrashSettingCallback(CrashSettingFn crashSettingFn)=0;

	//! Set the disconnect callback
	//!
	virtual void setDisconnectCallback(DisconnectFn disconnectFn)=0;

	//! Sets the tracer
	//!
	virtual void setTracer(TracerI *pTracer) = 0;
};

#endif //DESURA_SERVICECOREI_H
