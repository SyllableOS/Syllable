/*  This file is part of the KDE project
    Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "kapp.h"
#include "kconfig.h"
#ifndef __ATHEOS__
#include "dcopclient.h"
#include "kstddirs.h"
#endif

#include <unistd.h>
#include <qfile.h>

#ifdef __ATHEOS__
KApplication* qApp = NULL;
#endif

KApplication *KApplication::s_self = 0;

KApplication::KApplication( int argc, char **argv, const char *name )
#ifdef __ATHEOS__
    : KInstance( name )
#else
    : QApplication( argc, argv, name ), KInstance( name )
#endif
{
    s_self = this;

#ifndef __ATHEOS__
    m_dcopClient = new DCOPClient;
    KGlobal::dirs()->addResourceType("appdata", KStandardDirs::kde_default("data")
                                     + QString::fromLatin1(name) + '/');
#endif    
}

KApplication::~KApplication()
{
#ifndef __ATHEOS__
    delete m_dcopClient;
#endif
    s_self = 0;
}

#ifndef __ATHEOS__
bool KApplication::startServiceByDesktopName( const QString &, const QStringList &, QString * )
{
    return false;
}

bool KApplication::startServiceByDesktopPath( const QString & )
{
    return false;
}
#endif

bool checkAccess(const QString& pathname, int mode)
{
  int accessOK = access( QFile::encodeName(pathname), mode );
  if ( accessOK == 0 )
    return true;  // OK, I can really access the file

  // else
  // if we want to write the file would be created. Check, if the
  // user may write to the directory to create the file.
  if ( (mode & W_OK) == 0 )
    return false;   // Check for write access is not part of mode => bail out


  if (!access( QFile::encodeName(pathname), F_OK)) // if it already exists
      return false;

  //strip the filename (everything until '/' from the end
  QString dirName(pathname);
  int pos = dirName.findRev('/');
  if ( pos == -1 )
    return false;   // No path in argument. This is evil, we won't allow this

  dirName.truncate(pos); // strip everything starting from the last '/'

  accessOK = access( QFile::encodeName(dirName), W_OK );
  // -?- Can I write to the accessed diretory
  if ( accessOK == 0 )
    return true;  // Yes
  else
    return false; // No
}

#ifndef __ATHEOS__
#include "kapp.moc"
#endif
