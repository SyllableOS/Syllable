/* 
   This file is part of the KDE libraries
   Copyright (c) 1999 Waldo Bastian <bastian@kde.org>
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef _KTEMPFILE_H_
#define _KTEMPFILE_H_

#include <qfile.h>
#include <qstring.h>
#include <stdio.h>
#include <errno.h>

class QTextStream;
class QDataStream;
class KSaveFile;
class KTempFilePrivate;

/**
 * The KTempFile class creates and opens a unique file for temporary use.
 *
 * @author Waldo Bastian <bastian@kde.org>
 */
class KTempFile
{
   friend class KSaveFile;
public:
   /**
    * Create a temporary file with the name:
    *  <filePrefix><some number><fileExtension>
    *
    * The default filePrefix is "/tmp/appname"
    * The default fileExtension is ".tmp"
    **/
   KTempFile(QString filePrefix=QString::null, 
             QString fileExtension=QString::null, 
             int mode = 0600 );


   /**
    * The destructor closes the file.
    * If autoDelete is enabled the file gets unlinked as well.
    **/
   ~KTempFile();

   /**
    * Turn automatic deletion on or off.
    * Automatic deletion is off by default.
    **/
   void setAutoDelete(bool autoDelete) { bAutoDelete = autoDelete; }

   /**
    * Returns the status of the file based on errno. (see errno.h) 
    * 0 means OK.
    *
    * You should check the status after object creation to check 
    * whether a file could be created in the first place.
    *
    * You may check the status after closing the file to verify that
    * the file has indeed been written correctly.
    **/
   int status();

   /**
    * The name of the file.
    **/
   QString name();
   
   /**
    * An integer file descriptor open for writing to the file 
    **/
   int handle();
   
   /**
    * @return FILE* stream open for writing to the file
    **/
   FILE *fstream();

   /**
    * @return QTextStream open for writing to the file
    **/
   QTextStream *textStream();

   /**
    * @return QDataStream open for writing to the file
    **/
   QDataStream *dataStream();

   /**
    * A QFile open for writing to the file
    **/
   QFile *file();

   /**
    * Unlinks the file from the directory. The file is
    * deleted once the last reader/writer closes it.
    **/
   void unlink();   

   /**
    * Closes the file.
    * Returns 'true' is successfull, or 'false' if an error has occured.
    * See status() for details about errors.
    **/
   bool close();

protected:
   /**
    * Constructor used by KSaveFile
    **/
   KTempFile(bool);

   /**
    * @internal
    * Create function used internally by KTempFile and KSaveFile
    **/
   bool create(const QString &filePrefix, 
               const QString &fileExtension, int mode);

   void setError(int error) { mError = error; }
private:
   int mError;
   QString mTmpName;
   int mFd;
   FILE *mStream;
   QFile *mFile;
   QTextStream *mTextStream;
   QDataStream *mDataStream;
   bool bOpen;
   bool bAutoDelete;

   KTempFilePrivate *d;
};

#endif
