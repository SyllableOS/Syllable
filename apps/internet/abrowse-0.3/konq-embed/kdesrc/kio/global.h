/* This file is part of the KDE libraries
   Copyright (C) 2000 David Faure <faure@kde.org>

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
#ifndef __global_h__
#define __global_h__ "$Id$"

#include <qstring.h>
#include <qvaluelist.h>
#include <qlist.h>
#include <qdatastream.h>
#include <qdatetime.h>
#include <qmap.h>

/**
 * @short A namespace for KIO globals
 *
 */
namespace KIO
{

  /**
   * Convert size from bytes to the string representation.
   *
   * @param  size  size in bytes
   * @return converted size as a string - e.g. 123.4 kB , 12.0 MB
   */
  QString convertSize( unsigned long size );

  /**
   * Calculate remaining time from total size, processed size and speed.
   *
   * @param  totalSize      total size in bytes
   * @param  processedSize  processed size in bytes
   * @param  speed          speed in bytes pre second
   * @return calculated remaining time
   */
  QTime calculateRemaining( unsigned long totalSize, unsigned long processedSize, unsigned long speed );

  /**
   * Helper for showing information about a set of files and directories
   * @p items = @p files + @p dirs + number of symlinks :)
   * @param size the sum of the size of the _files_
   * @param showSize whether to show the size in the result
   */
  QString itemsSummaryString(uint items, uint files, uint dirs, unsigned long size, bool showSize);

  /**
   * Encode (from the text displayed to the real filename)
   * This translates % into %% and / into %2f
   * Used by KIO::link, for instance.
   */
  QString encodeFileName( const QString & str );
  /**
   * Decode (from the filename to the text displayed)
   * This translates %2[fF] into / and %% into %
   */
  QString decodeFileName( const QString & str );

  /**
   * Error codes that can be emitted by KIO.
   */
  enum Error {
    ERR_CANNOT_OPEN_FOR_READING = 1,
    ERR_CANNOT_OPEN_FOR_WRITING = 2,
    ERR_CANNOT_LAUNCH_PROCESS = 3,
    ERR_INTERNAL = 4,
    ERR_MALFORMED_URL = 5,
    ERR_UNSUPPORTED_PROTOCOL = 6,
    ERR_NO_SOURCE_PROTOCOL = 7,
    ERR_UNSUPPORTED_ACTION = 8,
    ERR_IS_DIRECTORY = 9, // ... where a file was expected
    // If one tries to get a listing of a file. This only works for directories :-)
    ERR_IS_FILE = 10,
    ERR_DOES_NOT_EXIST = 11,
    ERR_FILE_ALREADY_EXIST = 12,
    ERR_DIR_ALREADY_EXIST = 13,
    ERR_UNKNOWN_HOST = 14,
    ERR_ACCESS_DENIED = 15,
    ERR_WRITE_ACCESS_DENIED = 16,
    ERR_CANNOT_ENTER_DIRECTORY = 17,
    ERR_PROTOCOL_IS_NOT_A_FILESYSTEM = 18,
    ERR_CYCLIC_LINK = 19,
    ERR_USER_CANCELED = 20,
    ERR_CYCLIC_COPY = 21,
    ERR_COULD_NOT_CREATE_SOCKET = 22,
    ERR_COULD_NOT_CONNECT = 23,
    ERR_CONNECTION_BROKEN = 24,
    ERR_NOT_FILTER_PROTOCOL = 25,
    ERR_COULD_NOT_MOUNT = 26,
    ERR_COULD_NOT_UNMOUNT = 27,
    ERR_COULD_NOT_READ = 28,
    ERR_COULD_NOT_WRITE = 29,
    ERR_COULD_NOT_BIND = 30,
    ERR_COULD_NOT_LISTEN = 31,
    ERR_COULD_NOT_ACCEPT = 32,
    ERR_COULD_NOT_LOGIN = 33,
    ERR_COULD_NOT_STAT = 34,
    ERR_COULD_NOT_CLOSEDIR = 35,
    ERR_COULD_NOT_MKDIR = 37,
    ERR_COULD_NOT_RMDIR = 38,
    ERR_CANNOT_RESUME = 39,
    ERR_CANNOT_RENAME = 40,
    ERR_CANNOT_CHMOD = 41,
    ERR_CANNOT_DELETE = 42,
    // The text argument is the protocol that the dead slave supported.
    // This means for example: file, ftp, http, ...
    ERR_SLAVE_DIED = 43,
    ERR_OUT_OF_MEMORY = 44,
    ERR_UNKNOWN_PROXY_HOST = 45,
    ERR_COULD_NOT_AUTHENTICATE = 46,
    ERR_ABORTED = 47, // Action got aborted from application side
    // ERR_IS_REALLY_A_WARNING = 47, // HUH???
    // ERR_WARNING = 47, // Errors terminate the job now. Use warning() instead.
    ERR_INTERNAL_SERVER = 48,
    ERR_SERVER_TIMEOUT = 49,
    ERR_SERVICE_NOT_AVAILABLE = 50,
    ERR_UNKNOWN = 51,
    // (was a warning) ERR_CHECKSUM_MISMATCH = 52,
    ERR_UNKNOWN_INTERRUPT = 53,
    ERR_CANNOT_DELETE_ORIGINAL = 54,
    ERR_CANNOT_DELETE_PARTIAL = 55,
    ERR_CANNOT_RENAME_ORIGINAL = 56,
    ERR_CANNOT_RENAME_PARTIAL = 57,
    ERR_NEED_PASSWD = 58,
    ERR_CANNOT_SYMLINK = 59
  };

  /**
   * Constants used to specify the type of a KUDSAtom.
   */
  enum UDSAtomTypes {
    // First let's define the item types
    UDS_STRING = 1,
    UDS_LONG = 2,
    UDS_TIME = 4 | UDS_LONG,

    // Size of the file
    UDS_SIZE = 8 | UDS_LONG,
    // User ID of the file owner
    UDS_USER = 16 | UDS_STRING,
    // Group ID of the file owner
    UDS_GROUP =	32 | UDS_STRING,
    // Filename
    UDS_NAME = 64 | UDS_STRING,
    // Access permissions (part of the mode returned by stat)
    UDS_ACCESS = 128 | UDS_LONG,
    // The last time the file was modified
    UDS_MODIFICATION_TIME = 256 | UDS_TIME,
    // The last time the file was opened
    UDS_ACCESS_TIME = 512 | UDS_TIME,
    // The time the file was created
    UDS_CREATION_TIME = 1024 | UDS_TIME,
    // File type, part of the mode returned by stat
    // (for a link, this returns the file type of the pointed item)
    // check UDS_LINK_DEST to know if this is a link
    UDS_FILE_TYPE = 2048 | UDS_LONG,
    // Name of the file where the link points to
    // Allows to check for a symlink (don't use S_ISLNK !)
    UDS_LINK_DEST = 4096 | UDS_STRING,
    // An alternative URL (If different from the caption)
    UDS_URL = 8192 | UDS_STRING,
    // A mime type; prevents guessing
    UDS_MIME_TYPE = 16384 | UDS_STRING,
    // A mime type to be used for displaying only.
    // But when 'running' the file, the mimetype is re-determined
    UDS_GUESSED_MIME_TYPE = 16392 | UDS_STRING
  };

  QString findDeviceMountPoint( const QString& device );

    /**
    * checks if the path belongs to a file system that is probably
    * slow. It checks for NFS or for paths belonging to automounted
    * paths not yet mounted
    */
    bool probably_slow_mounted(const QString& filename);

/************
 *
 * Universal Directory Service
 *
 * Any file or URL can be represented by the UDSEntry type below
 * A UDSEntry is a list of atoms
 * Each atom contains a specific bit of information for the file
 *
 * The following UDS constants represent the different possible values
 * for m_uds in the UDS atom structure below
 *
 * Each atom contains a specific bit of information for the file
 */
class UDSAtom
{
public:
  /**
   * Whether 'm_str' or 'm_long' is used depends on the value of 'm_uds'.
   */
  QString m_str;
  long m_long;

  /**
   * Holds one of the UDS_XXX constants
   */
  unsigned int m_uds;
};

/**
 * An entry is the list of atoms containing all the informations for a file or URL
 */
typedef QValueList<UDSAtom> UDSEntry;
typedef QValueList<UDSEntry> UDSEntryList;
typedef QValueListIterator<UDSEntry> UDSEntryListIterator;
typedef QValueListConstIterator<UDSEntry> UDSEntryListConstIterator;

class MetaData : public QMap<QString, QString>
{
public:
   MetaData() : QMap<QString, QString>() { };
};
}

#endif
