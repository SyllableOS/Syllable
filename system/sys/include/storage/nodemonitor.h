/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2004 Syllable Team
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */

#ifndef __F_STORAGE_NODEMONITOR_H__
#define __F_STORAGE_NODEMONITOR_H__

#include <atheos/filesystem.h>
#include <util/string.h>

namespace os {
#if 0
} // Fool Emacs autoindent
#endif

class Looper;
class Handler;
class Messenger;
class FSNode;
class Directory;
class FileReference;


/** Filesystem node monitor
 * \ingroup storage
 * \par Description:
 *	The node monitor allow you to monitor changes done in the
 *	filesystem. This is very useful for the desktop manager, file
 *	requesters and other things that display the content of a
 *	directory to the user since it makes it possible to keep
 *	the directory list in sync with the actual directory without
 *	any need for polling. It might also be useful for other
 *	purposes like a daemon who want to monitor it's config file
 *	and reload it whenever it has changed.
 *
 *	You can monitor file creations and deletions as well as file
 *	renaming inside a directory. You can also monitor a file or
 *	directory for changes to the attributes or stat info (size,
 *	time-stamps, etc etc).
 *
 *	A node monitor will produce asyncronous events that will be
 *	sendt to a os::Looper whenever a relevant change is made to
 *	the node it is monitoring. When creating a NodeMonitor you
 *	must provide a bit-mask telling what event's you are
 *	interrested in and what os::Looper/os::Handler should receive
 *	the event message.
 *
 *	Here is a list of the flags controlling what to monitor:
 *
 *	- NWATCH_NAME
 *		Notify if the name of the watched node is changed.
 *	- NWATCH_STAT
 *		Notify if the information returned by the stat()
 *		function on the watched node changes.
 *	- NWATCH_ATTR
 *		Notify if a attribute on the watched node is created,
 *		deleted, or modified.
 *	- NWATCH_DIR
 *		Notify if the content of a directory changes. This include
 *		creation and deletion of files as well as renaming files
 *		within the directory or moving files to or from the
 *		directory. This flag will have no effect if applied to
 *		a file.
 *	- NWATCH_ALL
 *		A convinient combination of all the above.
 *	- NWATCH_FULL_DST_PATH
 *		This can be used in conjunction with the NWATCH_NAME
 *		and NWATCH_DIR flag to include the full path of one
 *		of the directories involved in the rename operation.
 *		Which directory depends on wether a node watched
 *		with NWATCH_NAME was renamed or wether the file
 *		was moved to or from a directory watched with
 *		NWATCH_DIR. Don't set this flag unless you really
 *		needs it. Retrieving the path of a directory can
 *		be a very expensive operation.
 *	- NWATCH_MOUNT
 *		Notify when filesystems are mounted and unmounted.
 *	- NWATCH_WORLD
 *		Whatch for all file creations, deletions and renamings
 *		on all filesystem mounted (NOT IMPLEMENTED YET).
 *
 *	When a relevant change happens to the filesystem one of the
 *	following messages are sendt to the target of the monitor:
 *
 *	All messages contain at least 3 elements:
 *
 *	<TABLE BORDER>
 *	<TR>
 *	<TD> \b Field								</TD>
 *	<TD> \b Type								</TD>
 *	<TD> \b Description							</TD>
 *	<TR>
 *	<TD>"event"								</TD>
 *	<TD>	\b T_INT32							</TD>
 *	<TD>	One of the <B>NWEVENT_*</B> event codes				</TD>
 *	<TR>
 *	<TD>"device"								</TD>
 *	<TD>	\b T_INT32							</TD>
 *	<TD>	The <B>dev_t</B> device number of the FS containing the
 *		affected node(s)						</TD>
 *	<TR>
 *	<TD>"node"								</TD>
 *	<TD>	\b T_INT64							</TD>
 *	<TD>	The \b ino_t i-node number of the affected node			</TD>
 *	</TABLE>
 *
 *	<B>NWEVENT_CREATED</B><P>
 *		Sendt when a file is created in a directory watched with
 *		NWATCH_DIR.
 *
 *	<TABLE BORDER>
 *	<TR>
 *	<TD>\b Field								</TD>
 *	<TD>\b Type								</TD>
 *	<TD>\b Description							</TD>
 *	<TR>
 *	<TD>"event"								</TD>
 *	<TD>	\b T_INT32							</TD>
 *	<TD>	\b NWEVENT_CREATED						</TD>
 *	<TR>
 *	<TD>"device"								</TD>
 *	<TD>	\b T_INT32							</TD>
 *	<TD>	The <B>dev_t</B> device number of the FS where the file
 *		was created.							</TD>
 *	<TR>
 *	<TD>"node"</TD>
 *	<TD>	\b T_INT64							</TD>
 *	<TD>	The \b ino_t i-node number of the new node			</TD>
 *	<TR>
 *	<TD>"dir_node"								</TD>
 *	<TD>	\b T_INT64							</TD>
 *	<TD>	The \b ino_t of the directory where the node was created.	</TD>
 *	<TR>
 *	<TD>"name"								</TD>
 *	<TD>	\b T_STRING							</TD>
 *	<TD>	The name of the new node					</TD>
 *	</TABLE>
 *
 *	<B>NWEVENT_DELETED</B><P>
 *		Sendt when a file watched with NWATCH_NAME or that lives
 *		in a directory watched with NWATCH_DIR is deleted.
 *
 *	<TABLE BORDER>
 *	<TR>
 *	<TD>\b Field								</TD>
 *	<TD>\b Type								</TD>
 *	<TD>\b Description							</TD>
 *	<TR>
 *	<TD>"event"								</TD>
 *	<TD>	\b T_INT32							</TD>
 *	<TD>	\b NWEVENT_DELETED						</TD>
 *	<TR>
 *	<TD>"device"								</TD>
 *	<TD>	\b T_INT32							</TD>
 *	<TD>	The <B>dev_t</B> device number of the FS where the file
 *		was deleted.							</TD>
 *	<TR>
 *	<TD>"node"</TD>
 *	<TD>	\b T_INT64							</TD>
 *	<TD>	The \b ino_t i-node number of the node that was deleted.	</TD>
 *	<TR>
 *	<TD>"dir_node"								</TD>
 *	<TD>	\b T_INT64							</TD>
 *	<TD>	The \b ino_t of the directory from where the node was deleted.	</TD>
 *	<TR>
 *	<TD>"name"								</TD>
 *	<TD>	\b T_STRING							</TD>
 *	<TD>	The name of the deleted node					</TD>
 *	</TABLE>
 *
 *	<B>NWEVENT_MOVED</B><P>
 *		Sendt when a file watched with NWATCH_NAME or that lives
 *		in a directory watched with NWATCH_DIR is deleted.
 *	<TABLE BORDER>
 *	<TR>
 *	<TD>\b Field								</TD>
 *	<TD>\b Type								</TD>
 *	<TD>\b Description							</TD>
 *	<TR>
 *	<TD>"event"								</TD>
 *	<TD>	\b T_INT32							</TD>
 *	<TD>	\b NWEVENT_MOVED						</TD>
 *	<TR>
 *	<TD>"device"								</TD>
 *	<TD>	\b T_INT32							</TD>
 *	<TD>	The <B>dev_t</B> device number of the FS containing the
 *		moved/renamed file.						</TD>
 *	<TR>
 *	<TD>"node"</TD>
 *	<TD>	\b T_INT64									</TD>
 *	<TD>	The \b ino_t i-node number of the node that was moved/renamed.			</TD>
 *	<TR>
 *	<TD>"old_dir"										</TD>
 *	<TD>	\b T_INT64									</TD>
 *	<TD>	The \b ino_t i-node number of the directory the file was moved from.		</TD>
 *	<TR>
 *	<TD>"new_dir"										</TD>
 *	<TD>	\b T_INT64									</TD>
 *	<TD>	The \b ino_t i-node number of the directory the file was moved to.
 *		If this is the same as "old_dir" the file was only renamed and not moved.	</TD>
 *	<TR>
 *	<TD>"name"										</TD>
 *	<TD>	\b T_STRING									</TD>
 *	<TD>	The new file name.								</TD>
 *	<TR>
 *	<TD>"old_path"										</TD>
 *	<TD>	\b T_STRING									</TD>
 *	<TD>	Only present if the \b NWATCH_FULL_DST_PATH was set and the file was moved
 *		into a directory watched with \b NWATCH_DIR. The string contain the full path
 *		of the directory the file was moved from.					</TD>
 *	<TR>
 *	<TD>"new_path"										</TD>
 *	<TD>	\b T_STRING									</TD>
 *	<TD>	Only present if the \b NWATCH_FULL_DST_PATH was set and the file was moved
 *		out of a directory watched with \b NWATCH_DIR or the moved/renamed file
 *		itself was watched with \b NWATCH_NAME (and \b NWATCH_FULL_DST_PATH)		</TD>
 *	</TABLE>
 *
 *	- NWEVENT_STAT_CHANGED
 *	- NWEVENT_ATTR_WRITTEN
 *	- NWEVENT_ATTR_DELETED
 *	- NWEVENT_FS_MOUNTED
 *	- NWEVENT_FS_UNMOUNTED
 *
 * \sa os::Looper, os::Handler, os::FSNode
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class NodeMonitor
{
public:
    NodeMonitor();
    NodeMonitor( const String& cPath, uint32 nFlags, const Handler* pcHandler, const Looper* pcLooper = NULL );
    NodeMonitor( const String& cPath, uint32 nFlags, const Messenger& cTarget );
    NodeMonitor( const Directory& cDir, const String& cPath, uint32 nFlags, const Handler* pcHandler, const Looper* pcLooper = NULL );
    NodeMonitor( const Directory& cDir, const String& cPath, uint32 nFlags, const Messenger& cTarget );
    NodeMonitor( const FileReference& cRef, uint32 nFlags, const Handler* pcHandler, const Looper* pcLooper = NULL );
    NodeMonitor( const FileReference& cRef, uint32 nFlags, const Messenger& cTarget );
    NodeMonitor( const FSNode* pcNode, uint32 nFlags, const Handler* pcHandler, const Looper* pcLooper = NULL );
    NodeMonitor( const FSNode* pcNode, uint32 nFlags, const Messenger& cTarget );
    ~NodeMonitor();

    bool IsValid() const;

    status_t Unset();
    status_t SetTo( const String& cPath, uint32 nFlags, const Handler* pcHandler, const Looper* pcLooper = NULL );
    status_t SetTo( const String& cPath, uint32 nFlags, const Messenger& cTarget );
    status_t SetTo( const Directory& cDir, const String& cPath, uint32 nFlags, const Handler* pcHandler, const Looper* pcLooper = NULL );
    status_t SetTo( const Directory& cDir, const String& cPath, uint32 nFlags, const Messenger& cTarget );
    status_t SetTo( const FileReference& cRef, uint32 nFlags, const Handler* pcHandler, const Looper* pcLooper = NULL );
    status_t SetTo( const FileReference& cRef, uint32 nFlags, const Messenger& cTarget );
    status_t SetTo( const FSNode* pcNode, uint32 nFlags, const Handler* pcHandler, const Looper* pcLooper = NULL );
    status_t SetTo( const FSNode* pcNode, uint32 nFlags, const Messenger& cTarget );


    int	GetMonitor() const;
    
private:
    int _CreateMonitor( const String& cPath, uint32 nFlags, const Messenger& cTarget );
    int _CreateMonitor( const Directory& cDir, const String& cPath, uint32 nFlags, const Messenger& cTarget );
    int _CreateMonitor( const FileReference& cRef, uint32 nFlags, const Messenger& cTarget );
    int _CreateMonitor( const FSNode* pcNode, uint32 nFlags, const Messenger& cTarget );

	class Private;
	Private *m;
};

} // end of namespace

#endif // __F_STORAGE_NODEMONITOR_H__
