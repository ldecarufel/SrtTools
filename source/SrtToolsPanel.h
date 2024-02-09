// SRT Tools Notepad++ Plugin
// Copyright (C)2024 Louis de Carufel (https://github.com/ldecarufel)

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef GOTILINE_DLG_H
#define GOTILINE_DLG_H

#include "DockingFeature\DockingDlgInterface.h"
#include "resource.h"
#include <string>

class SrtToolPanel : public DockingDlgInterface
{
public :
	SrtToolPanel() : DockingDlgInterface(IDD_SRTTOOLS_PANEL){};

    virtual void display(bool toShow = true) const
	{
        DockingDlgInterface::display(toShow);
        if (toShow)
            ::SetFocus(::GetDlgItem(_hSelf, ID_OFFSET_EDIT));
    };

	void setParent(HWND parent2set)
	{
		_hParent = parent2set;
	};

	void updateDialogState();
	void setCleanOutput(bool clean);

protected :
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void applyOperations();

private :
	HFONT m_boldFont = {};
};

LRESULT CALLBACK offsetEditSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
LRESULT CALLBACK indexEditSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

HWND getCurrentScintillaHandle();


#endif //GOTILINE_DLG_H
