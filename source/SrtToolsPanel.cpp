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


#include "SrtToolsPanel.h"
#include "PluginDefinition.h"
#include <stdio.h>
#include <sstream>
#include "Commctrl.h"
#include "richedit.h"

#undef max
#undef min
#include "SrtFile.h"

INT_PTR CALLBACK SrtToolPanel::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			// Set the edit boxes properties
			WCHAR offsetBanner[] = L"Offset in ms";
			::SendDlgItemMessage(_hSelf, ID_OFFSET_EDIT, EM_LIMITTEXT, 10, 0);
			::SendDlgItemMessage(_hSelf, ID_OFFSET_EDIT, EM_SETCUEBANNER, 0, (LPARAM)offsetBanner);

			WCHAR indexBanner[] = L"Start index";
			::SendDlgItemMessageW(_hSelf, ID_INDEX_EDIT, EM_LIMITTEXT, 10, 0);
			::SendDlgItemMessageW(_hSelf, ID_INDEX_EDIT, EM_SETCUEBANNER, 0, (LPARAM)indexBanner);

			// Subclass the edit controls
			SetWindowSubclass(GetDlgItem(_hSelf, ID_OFFSET_EDIT), &offsetEditSubclassProc, 0, 0);
			SetWindowSubclass(GetDlgItem(_hSelf, ID_INDEX_EDIT), &indexEditSubclassProc, 0, 0);

			// Set bold font for titles
			m_boldFont = CreateFont(0, 0, 0, 0, FW_BOLD, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, NULL);
			::SendDlgItemMessageW(_hSelf, ID_OFFSET_TITLE, WM_SETFONT, 0, (LPARAM)m_boldFont);
			::SendDlgItemMessageW(_hSelf, ID_INDEX_TITLE, WM_SETFONT, 0, (LPARAM)m_boldFont);
			::SendDlgItemMessageW(_hSelf, ID_CLEANUP_TITLE, WM_SETFONT, 0, (LPARAM)m_boldFont);

			updateDialogState();

			return TRUE;
		}
		break;

		case WM_DESTROY :
		{
			DeleteObject(m_boldFont);
			return TRUE;
		}

		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case ID_OFFSET_CHECK:
				{
					updateDialogState();
					return TRUE;
				}

				case ID_INDEX_CHECK:
				{
					updateDialogState();
					return TRUE;
				}

				case ID_APPLY_BUTTON:
				{
					applyOperations();
					return TRUE;
				}

			}
			return FALSE;
		}

		default :
			return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
	}
}

void SrtToolPanel::updateDialogState()
{
	bool doOffsetTime = ::SendDlgItemMessage(_hSelf, ID_OFFSET_CHECK, BM_GETCHECK, 0, 0) == BST_CHECKED;
	bool doRenumber = ::SendDlgItemMessage(_hSelf, ID_INDEX_CHECK, BM_GETCHECK, 0, 0) == BST_CHECKED;

	::EnableWindow(::GetDlgItem(_hSelf, ID_OFFSET_EDIT), doOffsetTime);
	::EnableWindow(::GetDlgItem(_hSelf, ID_OFFSET_UNITS), doOffsetTime);
	::EnableWindow(::GetDlgItem(_hSelf, ID_INDEX_EDIT), doRenumber);
	::EnableWindow(::GetDlgItem(_hSelf, ID_APPLY_BUTTON), doOffsetTime || doRenumber);
}

void SrtToolPanel::setCleanOutput(bool clean)
{
	::SendDlgItemMessage(_hSelf, ID_CLEANUP_CHECK, BM_SETCHECK, clean ? BST_CHECKED : BST_UNCHECKED, 0);
}

void SrtToolPanel::applyOperations()
{
	// Get parameter values
	char controlText[256];
	controlText[255] = 0;
	::SendDlgItemMessageA(_hSelf, ID_OFFSET_EDIT, WM_GETTEXT, 255, (LPARAM)controlText);
	long timeOffset = atol(controlText);

	::SendDlgItemMessageA(_hSelf, ID_INDEX_EDIT, WM_GETTEXT, 255, (LPARAM)controlText);
	long startIndex = atol(controlText);

	bool doOffsetTime = ::SendDlgItemMessage(_hSelf, ID_OFFSET_CHECK, BM_GETCHECK, 0, 0) == BST_CHECKED;
	bool doRenumber = ::SendDlgItemMessage(_hSelf, ID_INDEX_CHECK, BM_GETCHECK, 0, 0) == BST_CHECKED;
	bool doCleanupText = ::SendDlgItemMessage(_hSelf, ID_CLEANUP_CHECK, BM_GETCHECK, 0, 0) == BST_CHECKED;

	// Retrieve input text from Notepad++
	HWND hCurrScintilla = getCurrentScintillaHandle();

	const char* inputText = nullptr;
	long inputTextLength = 0;

	size_t start = ::SendMessage(hCurrScintilla, SCI_GETSELECTIONSTART, 0, 0);
	size_t end = ::SendMessage(hCurrScintilla, SCI_GETSELECTIONEND, 0, 0);
	if (start == end)
	{
		// No selection, get whole document
		inputTextLength = (long)::SendMessage(hCurrScintilla, SCI_GETTEXTLENGTH, 0, 0);
		inputText = new char[inputTextLength+1];
		::SendMessage(hCurrScintilla, SCI_GETTEXT, inputTextLength, (LPARAM)inputText);
	}
	else
	{
		// Get selected text
		inputTextLength = (long)::SendMessage(hCurrScintilla, SCI_GETSELTEXT, 0, 0);
		inputText = new char[inputTextLength+1];
		::SendMessage(hCurrScintilla, SCI_GETSELTEXT, 0, (LPARAM)inputText);
	}

	// Parse text and apply operations to subtitles
	std::stringstream inputStream(inputText, std::ios_base::in);
	SrtFile srtFile(inputStream);
	if (srtFile.IsValid())
	{
		if (doOffsetTime && timeOffset != 0)
			srtFile.OffsetInMilliseconds(timeOffset);
		if (doRenumber && startIndex > 0)
			srtFile.Renumber(startIndex);
	}

	delete[] inputText;

	// Generate output text
	std::stringstream outputStream;
	srtFile.WriteToFile(outputStream, doCleanupText);

	// Send output text to Notepad++
	if (start == end)
	{
		::SendMessage(hCurrScintilla, SCI_SETTEXT, 0, (LPARAM)outputStream.str().c_str());
	}
	else
	{
		size_t length = outputStream.str().length();
		::SendMessage(hCurrScintilla, SCI_REPLACESEL, 0, (LPARAM)outputStream.str().c_str());
		::SendMessage(hCurrScintilla, SCI_SETSEL, (WPARAM)start, (LPARAM)(start + length));
	}
}

LRESULT CALLBACK offsetEditSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
{
	if (message == WM_CHAR && (wParam < '0' || wParam > '9') && wParam != '-' && wParam != 8)
		return 0; // Reject the character
	return ::DefSubclassProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK indexEditSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
{
	if (message == WM_CHAR && (wParam < '0' || wParam > '9') && wParam != 8)
		return 0; // Reject the character
	return ::DefSubclassProc(hWnd, message, wParam, lParam);
}
