#include <storage/memfile.h>
#include <stdlib.h>
#include <iostream>

#define MIN_BLOCK_SIZE		1024
//#define MIN_BLOCK_SIZE                2

//#include <iostream.h>

using namespace os;

/** \internal */
class MemFile::Private
{
      public:

	/** \internal */
	struct Block
	{
		Block *m_psPrev;
		Block *m_psNext;
		uint32 m_nSize;
		uint8 *m_pData;
		bool m_bReadOnly;

		Block( uint32 size, Block * prev )
		{
			if( size )
				m_pData = new uint8[size];
			m_nSize = size;
			m_psPrev = prev;
			m_psNext = NULL;
			m_bReadOnly = false;
		}

		Block( uint32 size, const uint8 *buffer, Block * prev )
		{
			m_pData = ( uint8 * )buffer;
			m_nSize = size;
			m_psPrev = prev;
			m_psNext = NULL;
			m_bReadOnly = true;
		}

		void MakeWritable()
		{
			if( m_bReadOnly )
			{
				//cout << "Making const node writable\n";
				uint8 *p = new uint8[m_nSize];
				memcpy( p, m_pData, m_nSize );
				m_pData = p;

				m_bReadOnly = false;
			}
		}
	};

	Private()
	{
		m_nSize = 0;
		m_nPos = 0;
		m_nEOF = 0;
		m_psFirst = NULL;
		m_psLast = NULL;
	}

	~Private()
	{
	}

	void SetBfrSize( uint32 nSize )
	{
		if( m_nSize < nSize )
		{
			nSize -= m_nSize;
			uint32 v = ( nSize < MIN_BLOCK_SIZE ) ? MIN_BLOCK_SIZE : nSize;
			Block *nb = new Block( v, m_psLast );

			if( m_psLast )
			{
				m_psLast->m_psNext = nb;
			}
			else
			{
				m_psFirst = nb;
			}
			m_psLast = nb;
			m_nSize += v;
		}
		else if( m_nSize > nSize )
		{
			std::cout << "Free" << std::endl;

			uint32 v = nSize - m_nSize;

			while( v > m_psLast->m_nSize )
			{
				v -= m_psLast->m_nSize;
				m_nSize -= m_psLast->m_nSize;
				Block *p = m_psLast->m_psPrev;
				delete m_psLast;
				m_psLast = p;

				if( m_psLast )
				{
					m_psLast->m_psNext = NULL;
				}
			}
		}
	}

/*	void PrintAll( void )
	{
		Block*	p = m_psFirst;
		uint32	pos = 0;
		if( p ) do {
			char bfr[256];
			memcpy(bfr, p->m_pData, p->m_nSize);
			bfr[ p->m_nSize ] = 0;
			cout << pos << ": \"" << bfr << "\"" << endl;
			pos += p->m_nSize;
			p = p->m_psNext;
		} while( p );
	}	*/

	uint32 Write( uint32 nPos, const uint8 *pData, uint32 nSize )
	{
		uint32 nEnd = nSize + nPos;

		if( nEnd > m_nEOF )
		{
			m_nEOF = nEnd;
		}
		if( nEnd > m_nSize )
		{
			SetBfrSize( nEnd );
		}
		Block *p = m_psFirst;
		uint32 pos = 0;
		uint32 bfrpos = 0;

		if( p )
			do
			{
				if( pos + p->m_nSize > nPos )
				{
					uint32 blockpos = pos < nPos ? nPos - pos : 0;
					uint32 size = nSize > ( p->m_nSize - blockpos )? p->m_nSize - blockpos : nSize;

					if( size )
					{
						nSize -= size;
						p->MakeWritable();
						memcpy( &p->m_pData[blockpos], &pData[bfrpos], size );

						bfrpos += size;
					}
					else
					{
						return bfrpos;
					}
				}
				pos += p->m_nSize;
				p = p->m_psNext;
			}
			while( p );
		return bfrpos;
	}

	uint32 Read( uint32 nPos, uint8 *pData, uint32 nSize )
	{
		Block *p = m_psFirst;
		uint32 pos = 0;
		uint32 bfrpos = 0;

		if( p )
			do
			{
				if( pos + p->m_nSize > nPos )
				{
					uint32 blockpos = pos < nPos ? nPos - pos : 0;
					uint32 size = nSize > ( p->m_nSize - blockpos )? p->m_nSize - blockpos : nSize;

					if( size )
					{
						if( size + blockpos + pos > m_nEOF )
						{
							size = m_nEOF - pos - blockpos;
						}
						nSize -= size;
						memcpy( &pData[bfrpos], &p->m_pData[blockpos], size );

						bfrpos += size;
					}
					else
					{
						return bfrpos;
					}
				}
				pos += p->m_nSize;
				p = p->m_psNext;
			}
			while( p );
		return bfrpos;
	}

	uint32 m_nSize;
	uint32 m_nPos;
	uint32 m_nEOF;
	Block *m_psFirst;
	Block *m_psLast;
};

/** Default constructor
 * \par Description:
 *	Initializes a MemFile object to default values (ie. an empty MemFile).
 */

MemFile::MemFile()
{
	m = new Private;
}

/** Constructor
 * \par		Description:
 *		Creates a MemFile object with the data you specify. The data
 *		is not copied, unless the object is written to. This reduces
 *		overhead, since the data will often be static.<BR>
 *		If the object is written to, memory is allocated and the
 *		data is copied to the newly allocated memory. Your original
 *		data will not be affected in any way.
 *
 * \param	pData Pointer to the data you want to access as a stream.
 * \param	nLen Length of the data.
 *
 * \par		Example:
 * \code
 * const char bitmap[] = {
 *	// Bitmap data
 * };
 *
 * MemFile cBitmapStream( bitmap, sizeof( bitmap ) );
 *
 * // Now cBitmapStream may be used like an ordinary stream object:
 *
 * Image *cImg = new BitmapImage( &cBitmapStream );
 * \endcode
 * \note	The object pointed to by pData must live as long as the MemFile
 *		object lives. (That is, unless you force MemFile to move it
 *		into it's own memory, with a dummy write).
 *
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
MemFile::MemFile( const void *pData, uint32 nLen )
{
	m = new Private;
	m->m_psFirst = m->m_psLast = new Private::Block( nLen, ( const uint8 * )pData, NULL );
	m->m_nEOF = m->m_nSize = nLen;
}

MemFile::~MemFile()
{
	/* Delete all blocks */
	Private::Block* pcBlock = m->m_psFirst;
	while( pcBlock )
	{
		Private::Block* pcCurrent = pcBlock;
		pcBlock = pcCurrent->m_psNext;
		delete( pcCurrent );
	}
	
	delete m;
}

ssize_t MemFile::Read( void *pBuffer, ssize_t nSize )
{
	ssize_t size;

	size = m->Read( m->m_nPos, ( uint8 * )pBuffer, nSize );
	m->m_nPos += size;
	return size;
}

ssize_t MemFile::Write( const void *pBuffer, ssize_t nSize )
{
	ssize_t size;

	size = m->Write( m->m_nPos, ( const uint8 * )pBuffer, nSize );
	m->m_nPos += size;
	return size;
}

ssize_t MemFile::ReadPos( off_t nPos, void *pBuffer, ssize_t nSize )
{
	return m->Read( nPos, ( uint8 * )pBuffer, nSize );
}

ssize_t MemFile::WritePos( off_t nPos, const void *pBuffer, ssize_t nSize )
{
	return m->Write( nPos, ( const uint8 * )pBuffer, nSize );
}

off_t MemFile::Seek( off_t nPos, int nMode )
{
	off_t pos = m->m_nPos;

	switch ( nMode )
	{
	case SEEK_SET:
		m->m_nPos = nPos;
		break;
	case SEEK_END:
		m->m_nPos = m->m_nEOF - nPos;
		break;
	case SEEK_CUR:
		m->m_nPos += nPos;
		break;
	}
	return pos;
}

/** Set the size of the internal buffer
 * \par		Description:
 *		Set the size of the internal buffer. Any data past the end of
 *		new buffer will be discarded. Setting the buffer size to 0
 *		will cause the MemFile object to free all allocated memory.<BR>
 *		You may want to set a default buffer size, to optimize
 *		performance and reduce overhead. Normally, the buffer will be
 *		allocated incrementally, as needed, with at least MIN_BLOCKSIZE
 *		(currently 1 kbyte, but this may change in future versions).<BR>
 *		Example:<BR>
 *		If you know you will be writing at most 500 bytes, it makes
 *		sense to set the buffer to 500 bytes.<BR>
 *		If you know you will be writing at least 200 k, it makes sense
 *		to set the buffer to 200 k. Even if you write more, you will
 *		still have reduced the number of blocks required.
 *
 * \param	nSize The new buffer size.
 *
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
void MemFile::SetSize( uint32 nSize )
{
	m->SetBfrSize( nSize );
}
