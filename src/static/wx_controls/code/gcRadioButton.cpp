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
#include "gcRadioButton.h"
#include "gcManagers.h"

gcRadioButton::gcRadioButton(wxWindow* parent, int id, wxString label, const wxPoint& pos, const wxSize size) : wxRadioButton(parent, id, label, pos, size)
{
	Managers::LoadTheme(this, "checkbox");
	SetBackgroundColour(GetParent()->GetBackgroundColour());
}
