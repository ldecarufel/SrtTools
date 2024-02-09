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


#ifndef RESOURCE_H
#define RESOURCE_H

#define VERSION_VALUE "1.0\0"
#define VERSION_DIGITALVALUE 1, 0, 0, 0

#ifndef IDC_STATIC
#define IDC_STATIC	-1
#endif

#define	IDD_SRTTOOLS_PANEL		2600
#define	ID_OFFSET_TITLE	(IDD_SRTTOOLS_PANEL + 1)
#define	ID_OFFSET_DESC  (IDD_SRTTOOLS_PANEL + 2)
#define ID_OFFSET_CHECK (IDD_SRTTOOLS_PANEL + 3)
#define	ID_OFFSET_EDIT  (IDD_SRTTOOLS_PANEL + 4)
#define	ID_OFFSET_UNITS  (IDD_SRTTOOLS_PANEL + 5)

#define	ID_INDEX_TITLE	(IDD_SRTTOOLS_PANEL + 6)
#define	ID_INDEX_DESC  (IDD_SRTTOOLS_PANEL + 7)
#define	ID_INDEX_CHECK  (IDD_SRTTOOLS_PANEL + 8)
#define	ID_INDEX_EDIT  (IDD_SRTTOOLS_PANEL + 9)

#define	ID_CLEANUP_TITLE	(IDD_SRTTOOLS_PANEL + 10)
#define	ID_CLEANUP_DESC  (IDD_SRTTOOLS_PANEL + 11)
#define	ID_CLEANUP_CHECK  (IDD_SRTTOOLS_PANEL + 12)

#define	ID_APPLY_BUTTON	(IDD_SRTTOOLS_PANEL + 13)

#endif // RESOURCE_H

