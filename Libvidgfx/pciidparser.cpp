//*****************************************************************************
// Libvidgfx: A graphics library for video compositing
//
// Copyright (C) 2014 Lucas Murray <lmurray@undefinedfire.com>
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

#include "pciidparser.h"
#include <QtCore/QBuffer>
#include <QtCore/QFile>

PCIIDParser::PCIIDParser(const QString &filename)
	: m_data()
{
	QFile file(filename);
	if(!file.open(QIODevice::ReadOnly))
		return;
	m_data = file.readAll();
	file.close();

#if COMPRESS_PCI_IDS
	// If `COMPRESS_PCI_IDS` is set then the specified file is uncompressed and
	// we want to compress it. Creates a new file in the Mishira data directory
	// that is compressed.
#ifndef QT_DEBUG
#error The "pci.ids" file in the resource directory should be already compressed.
#endif
	QString newFilename = App->getDataDirectory().filePath("pci.ids");
	QFile newFile(newFilename);
	if(!newFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return;
	newFile.write(qCompress(m_data));
	newFile.close();
#else
	// Decompress file
	m_data = qUncompress(m_data);
#endif // COMPRESS_PCI_IDS
}

PCIIDParser::~PCIIDParser()
{
}

/// <summary>
/// Looks up the vendor and device strings for the specified IDs
/// </summary>
/// <returns>True if at least a matching vendor was found</returns>
bool PCIIDParser::lookup(
	uint vendorId, uint deviceId, uint subSysId,
	QString &vendorStrOut, QString &deviceStrOut, QString &subSysStrOut)
{
	vendorStrOut = QString();
	deviceStrOut = QString();
	subSysStrOut = QString();
	if(!m_data.size())
		return false;

	// Create IO buffer for our data
	QBuffer buf(&m_data);
	if(!buf.open(QIODevice::ReadOnly))
		return false;

	// Read each line of the file
	bool foundVendor = false;
	bool foundDevice = false;
	while(!buf.atEnd()) {
		QByteArray line = buf.readLine();

		// Skip comments and empty lines
		if(line.at(0) == '#' || line.size() == 0)
			continue;

		// What sort of line is this?
		if(line.size() >= 2 && line.at(0) == '\t' && line.at(1) == '\t') {
			// It is a subsystem line
			if(!foundDevice)
				continue; // Still searching for device
			if(line.size() < 14)
				continue; // Line is corrupted, ignore it

			// Parse subvendor and subsystem ID
			uint subven = line.mid(2, 4).toUInt(NULL, 16);
			uint subsys = line.mid(7, 4).toUInt(NULL, 16);
			if(((subsys << 16) | subven) == subSysId) {
				foundDevice = true;
				subSysStrOut = QString::fromUtf8(line.mid(13));
			}
		} else if(line.size() >= 1 && line.at(0) == '\t') {
			// It is a device line
			if(foundDevice)
				break; // No more lines to parse
			if(!foundVendor)
				continue; // Still searching for vendor
			if(line.size() < 8)
				continue; // Line is corrupted, ignore it

			// Parse device ID
			uint dev = line.mid(1, 4).toUInt(NULL, 16);
			if(dev == deviceId) {
				foundDevice = true;
				deviceStrOut = QString::fromUtf8(line.mid(7));
			}
		} else {
			// It is a vendor line
			if(foundVendor)
				break; // No more lines to parse
			if(line.size() < 7)
				continue; // Line is corrupted, ignore it

			// Parse vendor ID
			uint ven = line.mid(0, 4).toUInt(NULL, 16);
			if(ven == vendorId) {
				foundVendor = true;
				vendorStrOut = QString::fromUtf8(line.mid(6));
			}
		}
	}

	return foundVendor;
}
