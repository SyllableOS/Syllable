/*
 * Copyright 2003 by Daniel Gryniewicz
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __F_SYLLABLE_DLIST_H__
#define __F_SYLLABLE_DLIST_H__

#ifdef __cplusplus
extern "C" {
#if 0
} /*make emacs indention work */
#endif
#endif

// Typesafe doubly linked list.

/** Double list head type
 * \par Description:
 * This will result in a custom struct for the head of a doubly linked list.  Typedef it or use it
 * directly.
 ****************************************************************************/
#define DLIST_HEAD(headtype, linktype)	\
	struct headtype						\
	{									\
		struct linktype *list_head;		\
		struct linktype **list_ptail;	\
	}

/** Initialize a list head
 * \par Description:
 * Initialize the given list head to be empty
 * \par Note:
 * \par Warning:
 * \param head	List head to initialize
 * \return none
 * \sa
 ****************************************************************************/
#define DLIST_HEAD_INIT(head)								\
	do												\
	{												\
		(head)->list_head = NULL;					\
		(head)->list_ptail = &(head)->list_head;	\
	} while (0)

/** Double list entry type
 * \par Description:
 * This will result in a custom struct for a doubly linked list entry for a given type.
 * It should be typedef'd, and a field of that type included in the structure.
 ****************************************************************************/
#define DLIST_ENTRY(linktype)			\
	struct					\
	{					\
		struct linktype *list_next;	\
		struct linktype **list_pprev;	\
	}

/** Initialize a list entry
 * \par Description:
 * Initialize the given list entry to be empty
 * \par Note:
 * \par Warning:
 * \param entry	List entry to initialize
 * \return none
 * \sa
 ****************************************************************************/
#define DLIST_ENTRY_INIT(entry, field) do							\
	{											\
		(entry)->field.list_next = NULL;						\
		(entry)->field.list_pprev = NULL;						\
	} while (0)

/** Check to see if a list is empty
 * \par Description:
 * Check to see if the given list head has any entries on it
 * \par Note:
 * \par Warning:
 * \param head	List head to check
 * \return true if list is empty, false otherwise
 * \sa
 ****************************************************************************/
#define DLIST_IS_EMPTY(head) ((head)->list_head == NULL)

/** Get the first entry in a doubly linked list
 * \par Description:
 * Get the first entry in the given doubly liked list.
 * \par Note:
 * \par Warning:
 * \param head		The head of the list
 * \return The first entry in the list
 * \sa
 ****************************************************************************/
#define DLIST_FIRST(head) (head)->list_head

/** Get the last entry in a doubly linked list
 * \par Description:
 * Get the last entry in the given doubly liked list.
 * \par Note:
 * \par Warning:
 * \param head		The head of the list
 * \param head		The head of the list
 * \return The first entry in the list
 * \sa
 ****************************************************************************/
#define DLIST_LAST(head) (*(((typeof((head)))((head)->list_ptail))->list_ptail))

/** Get the next entry in a doubly linked list
 * \par Description:
 * Get the next entry after the given one in a doubly liked list.
 * \par Note:
 * \par Warning:
 * \param entry		The previous entry in the list
 * \param field		The name of the entry field in the struct
 * \return The entry after the given on, or NULL if it was last
 * \sa
 ****************************************************************************/
#define DLIST_NEXT(entry, field) (entry)->field.list_next

/** Check to see if an entry is on a list
 * \par Description:
 * Check to see if the given entry is currently on a list.
 * \par Note:
 * \par Locks Required:
 * \par Locks Taken:
 * \par Warning:
 * \param entry		Entry to check
 * \param field		The name of the entry field in the struct
 * \return TRUE if entry is on a list, FALSE otherwise
 * \sa
 ****************************************************************************/
#define DLIST_ON_LIST(entry, field) ((entry)->field.list_next != NULL)

/** Append an entry after another one on a doubly linked list
 * \par Description:
 * Add the given new entry after the given old entry in a doubly liked list.
 * \par Note:
 * \par Warning:
 * \param oldentry	The entry currently in the list to append to
 * \param newentry	The entry to append
 * \param field		The name of the entry field in the struct
 * \return none
 * \sa
 ****************************************************************************/
#define DLIST_APPEND(oldentry, newentry, field)						\
	do										\
	{										\
		if ((oldentry)->field.list_next)					\
		{									\
			(newentry)->field.list_next = (oldentry)->field.list_next;	\
			(oldentry)->field.list_next->field.list_pprev =			\
				&(newentry)->field.list_next;				\
		}									\
		(newentry)->field.list_pprev = &(oldentry)->field.list_next;		\
		(oldentry)->field.list_next = (newentry);				\
	} while (0)

/** Prepend an entry before another one on a doubly linked list
 * \par Description:
 * Add the given new entry before the given old entry in a doubly liked list.
 * \par Note:
 * \par Warning:
 * \param oldentry	The entry currently in the list to prepend to
 * \param newentry	The entry to prepend
 * \param field		The name of the entry field in the struct
 * \return none
 * \sa
 ****************************************************************************/
#define DLIST_PREPEND(oldentry, newentry, field)					\
	do									\
	{									\
		(newentry)->field.list_next = (oldentry);			\
		(newentry)->field.list_pprev = (oldentry)->field.list_pprev;	\
		(oldentry)->field.list_pprev = &(newentry)->field.list_next;	\
		*(newentry)->field.list_pprev = (newentry);			\
	} while (0)

/** Remove an entry from a doubly linked list
 * \par Description:
 * Remove the given entry from the list, using the given previous entry.
 * \par Note:
 * \par Warning:
 * \param delentry		The entry entry to be deleted
 * \param field		The name of the entry field in the struct
 * \return none
 * \sa
 ****************************************************************************/
#define DLIST_REMOVE(delentry, field)						\
	do									\
	{									\
		if ((delentry)->field.list_next)				\
			(delentry)->field.list_next->field.list_pprev =		\
				(delentry)->field.list_pprev;			\
		*(delentry)->field.list_pprev = (delentry)->field.list_next;	\
		(delentry)->field.list_next = NULL;				\
	} while (0)

/** Add an entry to the front of a doubly linked list
 * \par Description:
 * Add the given entry to the head of the given list
 * \par Note:
 * \par Warning:
 * \param head	Head pointer
 * \param entry	Entry to add
 * \param field	Tne name of the entry field in the struct
 * \return none
 * \sa
 ****************************************************************************/
#define DLIST_ADDHEAD(head, entry, field)							\
	do											\
	{											\
		(entry)->field.list_next = (head)->list_head;					\
		if ((head)->list_head)								\
			(head)->list_head->field.list_pprev = &(entry)->field.list_next;	\
		(entry)->field.list_pprev = &(head)->list_head;					\
		(head)->list_head = (entry);							\
	} while (0)

/** Add an entry to the tail of a doubly linked list
 * \par Description:
 * Add the given entry to the tail of the given list
 * \par Note:
 * \par Warning:
 * \param head	Head pointer
 * \param entry	Entry to add
 * \param field	Tne name of the entry field in the struct
 * \return none
 * \sa
 ****************************************************************************/
#define DLIST_ADDTAIL(head, entry, field)				\
	do													\
	{													\
		(entry)->field.list_next = NULL;				\
		(entry)->field.list_pprev = (head)->list_ptail;	\
		*(head)->list_ptail = (entry);					\
		(head)->list_ptail = &(entry)->field.list_next;	\
	} while (0)

/** Iterate over a list.  Not deletion safe
 * \par Description:
 * Iterate over a list.  This is not deletion safe
 * \par Note:
 * \par Locks Required:
 * \par Locks Taken:
 * \par Warning:
 * \param head	Head pointer
 * \param entry	Entry pointer to use for iterating
 * \param field	Tne name of the entry field in the struct
 * \return none
 * \sa DLIST_FOR_EACH_SAFE
 ****************************************************************************/
#define DLIST_FOR_EACH(head, entry, field)							\
	for ( (entry) = (head)->list_head; (entry); (entry) = (entry)->field.list_next )

/** Iterate over a list.  Deletion safe
 * \par Description:
 * Iterate over a list.  This is deletion safe, but has local storage overhead
 * \par Note:
 * \par Locks Required:
 * \par Locks Taken:
 * \par Warning:
 * \param head	Head pointer
 * \param entry	Entry pointer to use for iterating
 * \param tmp	Entry pointer for temporary internal use
 * \param field	Tne name of the entry field in the struct
 * \return none
 * \sa DLIST_FOR_EACH
 ****************************************************************************/
#define DLIST_FOR_EACH_SAFE(head, entry, tmp, field)						\
	for ( (entry) = (head)->list_head, (tmp) = ((entry) ? (entry)->field.list_next : NULL); (entry); (entry) = (tmp), (tmp) = ((tmp) ? (tmp)->field.list_next : NULL))

#ifdef __cplusplus
}
#endif

#endif /* __F_SYLLABLE_DLIST_H__ */

