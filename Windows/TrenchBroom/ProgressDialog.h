/*
 Copyright (C) 2010-2012 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "Controller/ProgressIndicator.h"
#include "Resource.h"

// ProgressDialog dialog


class ProgressDialog : public CDialogEx, public TrenchBroom::Controller::ProgressIndicator
{
	DECLARE_DYNAMIC(ProgressDialog)

public:
	CStatic m_label;
	CProgressCtrl m_progressbar;

	ProgressDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~ProgressDialog();

	virtual BOOL OnInitDialog();
	void setText(const string& text);

// Dialog Data
	enum { IDD = IDD_PROGRESSDIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void doReset();
	void doUpdate();
	DECLARE_MESSAGE_MAP()
};
