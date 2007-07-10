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

// Doubly linked list.

/** Double list head type
 * \par Description:
 * This will result in a custom struct for the head of a doubly linked list.  Typedef it or use it
 * directly.
 ****************************************************************************/
#define LIST_HEAD(headtype, linktype)	\
	struct headtype { struct linktype *list_head; }

/** Double list entry type
 * \par Description:
 * This will result in a custom struct for a doubly linked list entry for a given type.
 * It should be typedef'd, and a field of that type included in the structure.
 ****************************************************************************/
#define LIST_ENTRY(linktype)			\
	struct					\
	{					\
		struct linktype *list_next;	\
		struct linktype **list_pprev;	\
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
#define LIST_INIT(head) ((head)->list_head = NULL)

/** Check to see if a list is empty
 * \par Description:
 * Check to see if the given list head has any entries on it
 * \par Note:
 * \par Warning:
 * \param head	List head to check
 * \return true if list is empty, false otherwise
 * \sa
 ****************************************************************************/
#define LIST_IS_EMPTY(head) ((head)->list_head == NULL)

/** Get the first entry in a doubly linked list
 * \par Description:
 * Get the first entry in the given doubly liked list.
 * \par Note:
 * \par Warning:
 * \param head		The head of the list
 * \return The first entry in the list
 * \sa
 ****************************************************************************/
#define LIST_FIRST(head) (head)->list_head

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
#define LIST_NEXT(entry, field) (entry)->field.list_next

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
#define LIST_APPEND(oldentry, newentry, field)						\
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
#define LIST_PREPEND(oldentry, newentry, field)					\
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
#define LIST_REMOVE(delentry, field)						\
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
#define LIST_ADDHEAD(head, entry, field)							\
	do											\
	{											\
		(entry)->field.list_next = (head)->list_head;					\
		if ((head)->list_head)								\
			(head)->list_head->field.list_pprev = &(entry)->field.list_next;	\
		(entry)->field.list_pprev = &(head)->list_head;					\
		(head)->list_head = (entry);							\
	} while (0)

