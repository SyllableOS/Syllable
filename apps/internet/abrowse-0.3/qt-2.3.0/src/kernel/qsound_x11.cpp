/****************************************************************************
** $Id$
**
** Implementation of QSound class and QAuServer internal class
**
** Created : 000117
**
** Copyright (C) 1999-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses for Unix/X11 may use this file in accordance with the Qt Commercial
** License Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#define QT_CLEAN_NAMESPACE

#include "qsound.h"

#ifndef QT_NO_SOUND

#include <qsocknot.h>
#include <qapplication.h>


#ifdef QT_NAS_SUPPORT

#include <audio/audiolib.h>
#include <audio/soundlib.h>

class QAuServerNAS : public QAuServer {
    Q_OBJECT

    AuServer *nas;
    QSocketNotifier* sn;

public:
    QAuServerNAS(QObject* parent);
    ~QAuServerNAS();

    void play(const QString& filename);
    void play(QAuBucket* id);
    QAuBucket* newBucket(const QString& filename);
    void deleteBucket(QAuBucket* id);
    bool okay();

public slots:
    void dataReceived();
};

class QAuBucket { }; // Just an ID - we cast to and from AuBucketID

QAuServerNAS::QAuServerNAS(QObject* parent) :
    QAuServer(parent,"Network Audio System")
{
    nas = AuOpenServer(NULL, 0, NULL, 0, NULL, NULL);
    if (nas) {
	// Ask Qt for async messages...
	sn=new QSocketNotifier(AuServerConnectionNumber(nas),
		QSocketNotifier::Read);
	QObject::connect(sn, SIGNAL(activated(int)),
		this, SLOT(dataReceived()));
    } else {
	sn = 0;
    }
}

QAuServerNAS::~QAuServerNAS()
{
    if ( nas )
	AuCloseServer( nas );
    delete sn;
}

void QAuServerNAS::play(const QString& filename)
{
    if (nas) {
	int iv=100;
	AuFixedPoint volume=AuFixedPointFromFraction(iv,100);
	AuSoundPlayFromFile(nas, filename, AuNone, volume, NULL, NULL, NULL, NULL, NULL, NULL);
	AuFlush(nas);
	dataReceived();
	AuFlush(nas);
	qApp->flushX();
    }
}

void QAuServerNAS::play(QAuBucket* id)
{
    if (nas) {
	int iv=100;
	AuFixedPoint volume=AuFixedPointFromFraction(iv,100);
	AuSoundPlayFromBucket(nas, (AuBucketID)id, AuNone, volume, NULL, NULL, 0, NULL, NULL, NULL, NULL);
	AuFlush(nas);
	dataReceived();
	AuFlush(nas);
	qApp->flushX();
    }
}

QAuBucket* QAuServerNAS::newBucket(const QString& filename)
{
    return nas ? (QAuBucket*)AuSoundCreateBucketFromFile(nas, filename,
		0 /*AuAccessAllMasks*/, NULL, NULL) : 0;
}

void QAuServerNAS::deleteBucket(QAuBucket* id)
{
    if ( nas )
	AuDestroyBucket( nas, (AuBucketID)id, NULL );
}

bool QAuServerNAS::okay()
{
    return !!nas;
}

void QAuServerNAS::dataReceived()
{
    AuHandleEvents(nas);
}

#include "qsound_x11.moc"

#endif


class QAuServerNull : public QAuServer {
public:
    QAuServerNull(QObject* parent);

    void play(const QString&) { }
    void play(QAuBucket*) { }
    QAuBucket* newBucket(const QString&) { return 0; }
    void deleteBucket(QAuBucket*) { }
    bool okay() { return FALSE; }
};

QAuServerNull::QAuServerNull(QObject* parent) :
    QAuServer(parent,"Null Audio Server")
{
}


QAuServer* qt_new_audio_server()
{
#ifdef QT_NAS_SUPPORT
    QAuServer* s=new QAuServerNAS(qApp);
    if (s->okay()) {
	return s;
    } else {
	delete s;
    }
#endif
    return new QAuServerNull(qApp);
}

#endif // QT_NO_SOUND
