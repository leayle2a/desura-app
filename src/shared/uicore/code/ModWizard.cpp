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
#include "ModWizard.h"
#include "ModWizardFinPage.h"
#include "MainApp.h"


ModWizardForm::ModWizardForm(wxWindow* parent)
	: gcFrame(parent, wxID_ANY, wxT("#MW_TITLE"), wxDefaultPosition, wxSize( 370,160 ), wxCAPTION|wxCLOSE_BOX|wxFRAME_FLOAT_ON_PARENT|wxSYSTEM_MENU|wxTAB_TRAVERSAL)
{
	m_pPage = nullptr;

	SetTitle(Managers::GetString(L"#MW_TITLE"));

	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	m_bsSizer = new wxBoxSizer( wxVERTICAL );

	m_pPage = new MWInfoPage( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_bsSizer->Add( m_pPage, 1, wxEXPAND, 0 );

	this->SetSizer( m_bsSizer );
	this->Layout();

	centerOnParent();

	Bind(wxEVT_CLOSE_WINDOW, &ModWizardForm::onFormClose, this);
}

ModWizardForm::~ModWizardForm()
{
}

void ModWizardForm::onFormClose( wxCloseEvent& event )
{
	if (!m_bDisabled)
		g_pMainApp->closeForm(this->GetId());
	else
		event.Skip();
}

void ModWizardForm::finish()
{
	cleanUpPages();

	m_pPage =  new MWFinishPage( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_bsSizer->Add( m_pPage, 1, wxEXPAND, 0 );
	Layout();
	Refresh();

	if (!m_bDisabled)
		m_pPage->run();
}

void ModWizardForm::run(bool addToAccount)
{
	cleanUpPages();

	MWProgressPage * page = new MWProgressPage( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	page->setAddToAccount(addToAccount);

	m_pPage = page;
	m_bsSizer->Add( m_pPage, 1, wxEXPAND, 0 );
	Layout();
	Refresh();

	if (!m_bDisabled)
		m_pPage->run();
}


void ModWizardForm::cleanUpPages()
{
	m_bsSizer->Clear(false);

	if (m_pPage)
	{
		m_pPage->Show(false);
		m_pPage->Close();
		m_pPage->dispose();
		m_pPage->Destroy();
		m_pPage = nullptr;
	}
}


