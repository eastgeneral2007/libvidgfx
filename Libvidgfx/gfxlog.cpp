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

#include "gfxlog.h"
#include <QtCore/QRect>
#include <QtCore/QRectF>
#include <QtCore/QSize>
#include <QtCore/QStringList>

//=============================================================================
// GfxLogData class

class GfxLogData
{
public: // Members ------------------------------------------------------------
	int					ref;
	QString				cat;
	GfxLog::LogLevel	lvl;
	QString				msg;

public: // Constructor/destructor ---------------------------------------------
	GfxLogData()
		: ref(0)
		, cat()
		, lvl(GfxLog::Notice)
		, msg()
	{
	}
};

//=============================================================================
// GfxLog class

static void defaultLog(
	const QString &cat, const QString &msg, GfxLogLevel lvl)
{
	// No-op
}
VidgfxLogCallback *GfxLog::s_callbackFunc = &defaultLog;

GfxLog::GfxLog()
	: d(new GfxLogData())
{
	d->ref++;
}

GfxLog::GfxLog(const GfxLog &log)
	: d(log.d)
{
	d->ref++;
}

GfxLog::~GfxLog()
{
	d->ref--;
	if(d->ref)
		return; // Temporary object

	// Forward to the callback
	if(s_callbackFunc != NULL)
		s_callbackFunc(d->cat, d->msg, (GfxLogLevel)(d->lvl));

	delete d;
}

GfxLog operator<<(GfxLog log, const QString &msg)
{
	log.d->msg.append(msg);
	return log;
}

GfxLog operator<<(GfxLog log, const QByteArray &msg)
{
	return log << QString::fromUtf8(msg);
}

GfxLog operator<<(GfxLog log, const char *msg)
{
	return log << QString(msg);
}

GfxLog operator<<(GfxLog log, int msg)
{
	return log << QString::number(msg);
}

GfxLog operator<<(GfxLog log, unsigned int msg)
{
	return log << QString::number(msg);
}

GfxLog operator<<(GfxLog log, qint64 msg)
{
	return log << QString::number(msg);
}

GfxLog operator<<(GfxLog log, quint64 msg)
{
	return log << QString::number(msg);
}

GfxLog operator<<(GfxLog log, qreal msg)
{
	return log << QString::number(msg);
}

GfxLog operator<<(GfxLog log, float msg)
{
	return log << QString::number(msg);
}

GfxLog operator<<(GfxLog log, bool msg)
{
	if(msg)
		return log << QStringLiteral("true");
	return log << QStringLiteral("false");
}

GfxLog operator<<(GfxLog log, const QPoint &msg)
{
	return log << QStringLiteral("Point(%1, %2)")
		.arg(msg.x())
		.arg(msg.y());
}

GfxLog operator<<(GfxLog log, const QPointF &msg)
{
	return log << QStringLiteral("Point(%1, %2)")
		.arg(msg.x())
		.arg(msg.y());
}

GfxLog operator<<(GfxLog log, const QRect &msg)
{
	return log << QStringLiteral("Rect(%1, %2, %3, %4)")
		.arg(msg.left())
		.arg(msg.top())
		.arg(msg.width())
		.arg(msg.height());
}

GfxLog operator<<(GfxLog log, const QRectF &msg)
{
	return log << QStringLiteral("Rect(%1, %2, %3, %4)")
		.arg(msg.left())
		.arg(msg.top())
		.arg(msg.width())
		.arg(msg.height());
}

GfxLog operator<<(GfxLog log, const QSize &msg)
{
	return log << QStringLiteral("Size(%1, %2)")
		.arg(msg.width())
		.arg(msg.height());
}

GfxLog operator<<(GfxLog log, const QSizeF &msg)
{
	return log << QStringLiteral("Size(%1, %2)")
		.arg(msg.width())
		.arg(msg.height());
}

GfxLog gfxLog(const QString &category, GfxLog::LogLevel lvl)
{
	GfxLog log;
	log.d->cat = category;
	log.d->lvl = lvl;
	return log;
}

GfxLog gfxLog(GfxLog::LogLevel lvl)
{
	return gfxLog("", lvl);
}
