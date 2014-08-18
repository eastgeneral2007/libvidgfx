//*****************************************************************************
// Libvidgfx: A graphics library for video compositing
//
// Copyright (C) 2014 Lucas Murray <lucas@polyflare.com>
// All rights reserved.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//*****************************************************************************

#ifndef PCIIDPARSER_H
#define PCIIDPARSER_H

#include <QtCore/QString>
#include <QtCore/QByteArray>

#define COMPRESS_PCI_IDS 0

//=============================================================================
/// <summary>
/// A simple compressed "pci.ids" file parser allowing for PCI ID lookups. The
/// base file can be found at http://pciids.sourceforge.net/ and can be
/// compressed by setting the `COMPRESS_PCI_IDS` definition to `1`.
/// </summary>
class PCIIDParser
{
protected: // Members ---------------------------------------------------------
	QByteArray	m_data;

public: // Constructor/destructor ---------------------------------------------
	PCIIDParser(const QString &filename);
	~PCIIDParser();

public: // Methods ------------------------------------------------------------
	bool	lookup(
		uint vendorId, uint deviceId, uint subSysId,
		QString &vendorStrOut, QString &deviceStrOut, QString &subSysStrOut);
};
//=============================================================================

#endif // PCIIDPARSER_H
