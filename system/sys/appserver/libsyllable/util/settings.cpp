#include <util/settings.h>
#include <util/application.h>
#include <util/variant.h>
#include <storage/file.h>
#include <sys/stat.h>

using namespace os;

/** \internal */
class Settings::Private
{
      public:
	Private()
	{
		m_pcIO = NULL;
		m_bUserStream = false;
	}

	~Private()
	{
		if( m_pcIO )
			delete m_pcIO;
	}

	String GetAppName()
	{
		Application *pApp = Application::GetInstance();

		if( pApp )
		{
			const char *pzAppSig = pApp->GetName().c_str(  );
 			const char *pzAppName = strrchr( pzAppSig, '-' );
 
 			// First check for a '-'
 			if( pzAppName != NULL )
 				pzAppName++;
 
 			// If not found, check for '/'
 			else if( strrchr( pzAppSig, '/' ) != NULL )
 			{
 				pzAppName = strrchr( pzAppSig, '/' ) + 1;
 			}
 
 			// Simply use the name directly
 			else
 				pzAppName = pzAppSig;

			if( pzAppName && strlen( pzAppName ) > 1 )
			{
				return String( pzAppName );
			}
		}

		throw "Error! :(";
	}

	void CreatePath( const String & cPath )
	{
		String cDirPath;
		const char *pzPath = cPath.c_str();
		char *pzBfr = new char[strlen( pzPath ) + 1];
		const char *pzPart;

		strcpy( pzBfr, pzPath );

		if( pzBfr[0] == '/' )
			cDirPath = "/";

		pzPart = strtok( pzBfr, "/" );
		while( pzPart )
		{
			struct stat psStat;

			cDirPath += pzPart;
			if( stat( cDirPath.c_str(), &psStat ) < 0 )
			{
				//cout << "mkdir: " << cDirPath << endl;
				mkdir( cDirPath.c_str(), 0700 );
			}
			//cout << "\"" << pzPart << "\"" << endl;
			pzPart = strtok( NULL, "/" );
			cDirPath += "/";
		}
	}

	void SetPath( const String & cFile = "", const String & cPath = "" )
	{
		if( cFile == "" )
		{
			m_cFile = "Settings";
		}
		else
		{
			m_cFile = cFile;
		}
		if( cPath == "" )
		{
			m_cPath = getenv( "HOME" );
			m_cPath += String("/Settings/") + GetAppName() + String("/");
		}
		else
		{
			m_cPath = cPath;
		}
	}

	void SetIO( SeekableIO * pcIO )
	{
		if( m_pcIO )
			delete m_pcIO;

		m_pcIO = pcIO;
		m_bUserStream = m_pcIO != NULL;
	}

	status_t GetStream( uint32 mode = O_RDONLY )
	{
		if( m_bUserStream )
		{
			if( m_pcIO )
			{
				m_pcIO->Seek( 0, SEEK_SET );
				return 0;
			}
			else
			{
				return -1;
			}
		}
		else
		{
			try
			{
				m_pcIO = new File( m_cPath + m_cFile, mode );
				if( m_pcIO )
					return 0;
			}
			catch( ... )
			{
			}
			if( mode != O_RDONLY )
			{
				CreatePath( m_cPath );
				try
				{
					m_pcIO = new File( m_cPath + m_cFile, mode );
					if( m_pcIO )
						return 0;
				}
				catch( ... )
				{
				}
			}
		}
		return -1;
	}

	void FreeStream()
	{
		if( !m_bUserStream )
		{
			if( m_pcIO )
				delete m_pcIO;

			m_pcIO = NULL;
		}
	}

	SeekableIO *m_pcIO;
	bool m_bUserStream;
	String m_cFile;
	String m_cPath;
};

/** Default constructor
 * \par		Description:
 *		Creates a Settings object for the current application and the
 *		current user. The default path will be
 *		"~/Settings/[Application name]/Settings".
 * \note	This method relies on the Application object, and the app
 *		signature passed to it. If you haven't created an Application
 *		object, you will have to specify the path manually.
 * \sa Load(), Save(), os::Application
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
Settings::Settings()
{
	m = new Private;
	m->SetPath();
}

/** Constructor
 * \par		Description:
 *		Creates a Settings object bound to a specific stream object.
 * \param	pcFile SeekableIO object, e.g. os::File.
 * \sa Load(), Save(), os::File, os::SeekableIO
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
Settings::Settings( SeekableIO * pcFile )
{
	m = new Private;
	m->SetIO( pcFile );
}

Settings::~Settings()
{
	delete m;
}

/** Load settings
 * \par		Description:
 *		Loads settings from a settings file.
 * \sa Save()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t Settings::Load( void )
{
	status_t ret = -1;

	if( m->GetStream() == 0 )
	{
		FileHeader fh;

		if( m->m_pcIO->Read( &fh, sizeof( fh ) ) == sizeof( fh ) )
		{
			if( fh.nMagic == SETTINGS_MAGIC && fh.nVersion <= SETTINGS_VERSION )
			{
				m->m_pcIO->Seek( fh.nHeaderSize, SEEK_SET );

				if( fh.nSize > 0 && fh.nSize < 0x800000 )
				{
					uint8 *bfr = new uint8[fh.nSize];

					if( ( uint32 )m->m_pcIO->Read( bfr, fh.nSize ) == fh.nSize )
					{
						Unflatten( bfr );
						ret = 0;
					}
					delete bfr;
				}
			}
		}
		m->FreeStream();
	}
	return ret;
}

/** Save settings
 * \par		Description:
 *		Writes settings to a settings file.
 * \sa Load()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t Settings::Save( void ) const
{
	if( m->GetStream( O_CREAT | O_TRUNC ) == 0 )
	{
		FileHeader fh;

		fh.nMagic = SETTINGS_MAGIC;
		fh.nVersion = SETTINGS_VERSION;
		fh.nHeaderSize = sizeof( fh );
		fh.nSize = GetFlattenedSize();
		m->m_pcIO->Write( &fh, sizeof( fh ) );
		uint8 *bfr = new uint8[fh.nSize];

		Flatten( bfr, fh.nSize );
		m->m_pcIO->Write( bfr, fh.nSize );
		delete bfr;

		m->FreeStream();
		return 0;
	}
	return -1;
}

/** Get directory path
 * \par		Description:
 *		Returns the directory path to the settings file, excluding the
 *		file itself.
 * \sa GetFile(), SetFile(), SetPath()
 * \author Kristian Van Der Vliet (vanders@liqwyd.com)
 *****************************************************************************/
Path Settings::GetPath() const
{
	Path cPath( m->m_cPath );
	return cPath;
}

/** Get filename
 * \par		Description:
 *		Returns the name of the settings file, excluding path.
 * \sa GetPath()
 * \author Kristian Van Der Vliet (vanders@liqwyd.com)
 * \sa SetFile(), GetPath(), SetPath()
 *****************************************************************************/
File Settings::GetFile() const
{
	File cFile( m->m_cFile );
	return cFile;
}

/** Set directory path.
 * \par		Description:
 *		Set directory path.
 * \param pcPath The new path.
 * \author Kristian Van Der Vliet (vanders@liqwyd.com)
 * \sa GetFile(), SetFile(), GetPath()
 *****************************************************************************/
void Settings::SetPath( Path* pcPath )
{
	if( pcPath )
		m->SetPath( m->m_cFile, pcPath->GetPath() + String( "/" ) );
}

/** Set file.
 * \par		Description:
 *		Change the file which should be used to save and load Settings.
 * \param pcFile The new file.
 * \author Kristian Van Der Vliet (vanders@liqwyd.com)
 * \sa GetFile(), GetPath(), SetPath()
 *****************************************************************************/
void Settings::SetFile( SeekableIO* pcFile )
{
	if( pcFile )
		m->SetIO( pcFile );
}

/** Set filename.
 * \par		Description:
 *		Change the filename which should be used to save and load Settings.
 * \param cFilename The new filename.  The filename will be appended to the current path.
 * \author Kristian Van Der Vliet (vanders@liqwyd.com)
 * \sa GetFile(), GetPath(), SetPath()
 *****************************************************************************/
void Settings::SetFile( String cFilename )
{
	m->SetPath( cFilename, m->m_cPath );
}

/** Copy a Settings object.
 * \par		Description:
 *		Copies both Message data and Path/File/Stream information.
 * \param cSource Object to copy from.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
Settings & Settings::operator=( const Settings & cSource )
{
	Message::operator=( cSource );
	if( cSource.m->m_bUserStream )
	{
		m->m_bUserStream = cSource.m->m_bUserStream;
		m->m_pcIO = cSource.m->m_pcIO;
	}
	else
	{
		m->m_cPath = cSource.m->m_cPath;
		m->m_cFile = cSource.m->m_cFile;
	}
	return *this;
}

/** Set Message data.
 * \par		Description:
 *		Copies only Message data from cSource to this object.
 * \param cSource Object to copy from.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
Settings & Settings::operator=( const Message & cSource )
{
	Message::operator=( cSource );
	return *this;
}

/** Get a string
 * \par		Description:
 *		Retrieve a string value from a Settings object.
 * \param pzName Name of the item to get.
 * \param pzDefault Default value to return, if the item is not present in the
 *	  Settings object.
 * \param nIndex If several items with the same name exists you can use this
 *        parameter to specify which one you want.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
String Settings::GetString( const char *pzName, const char *pzDefault, int nIndex ) const
{
	String v;

	if( FindString( pzName, &v, nIndex ) < 0 )
	{
		v = pzDefault;
	}

	return v;
}

/** Get an integer
 * \par		Description:
 *		Retrieve an 8 bit integer value from a Settings object.
 * \param pzName Name of the item to get.
 * \param pzDefault Default value to return, if the item is not present in the
 *	  Settings object.
 * \param nIndex If several items with the same name exists you can use this
 *        parameter to specify which one you want.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
int8 Settings::GetInt8( const char *pzName, int8 nDefault, int nIndex ) const
{
	int8 v;

	if( FindInt8( pzName, &v, nIndex ) < 0 )
	{
		v = nDefault;
	}

	return v;
}

/** Get an integer
 * \par		Description:
 *		Retrieve a 16 bit integer value from a Settings object.
 * \param pzName Name of the item to get.
 * \param pzDefault Default value to return, if the item is not present in the
 *	  Settings object.
 * \param nIndex If several items with the same name exists you can use this
 *        parameter to specify which one you want.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
int16 Settings::GetInt16( const char *pzName, int16 nDefault, int nIndex ) const
{
	int16 v;

	if( FindInt16( pzName, &v, nIndex ) < 0 )
	{
		v = nDefault;
	}

	return v;
}

/** Get an integer
 * \par		Description:
 *		Retrieve a 32 bit integer value from a Settings object.
 * \param pzName Name of the item to get.
 * \param pzDefault Default value to return, if the item is not present in the
 *	  Settings object.
 * \param nIndex If several items with the same name exists you can use this
 *        parameter to specify which one you want.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
int32 Settings::GetInt32( const char *pzName, int32 nDefault, int nIndex ) const
{
	int32 v;

	if( FindInt32( pzName, &v, nIndex ) < 0 )
	{
		v = nDefault;
	}

	return v;
}

/** Get an integer
 * \par		Description:
 *		Retrieve a 64 bit integer value from a Settings object.
 * \param pzName Name of the item to get.
 * \param pzDefault Default value to return, if the item is not present in the
 *	  Settings object.
 * \param nIndex If several items with the same name exists you can use this
 *        parameter to specify which one you want.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
int64 Settings::GetInt64( const char *pzName, int64 nDefault, int nIndex ) const
{
	int64 v;

	if( FindInt64( pzName, &v, nIndex ) < 0 )
	{
		v = nDefault;
	}

	return v;
}

/** Get a boolean
 * \par		Description:
 *		Retrieve a boolean value from a Settings object.
 * \param pzName Name of the item to get.
 * \param pzDefault Default value to return, if the item is not present in the
 *	  Settings object.
 * \param nIndex If several items with the same name exists you can use this
 *        parameter to specify which one you want.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
bool Settings::GetBool( const char *pzName, bool bDefault, int nIndex ) const
{
	bool v;

	if( FindBool( pzName, &v, nIndex ) < 0 )
	{
		v = bDefault;
	}

	return v;
}

/** Get a float
 * \par		Description:
 *		Retrieve a floating point value from a Settings object.
 * \param pzName Name of the item to get.
 * \param pzDefault Default value to return, if the item is not present in the
 *	  Settings object.
 * \param nIndex If several items with the same name exists you can use this
 *        parameter to specify which one you want.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
float Settings::GetFloat( const char *pzName, float vDefault, int nIndex ) const
{
	float v;

	if( FindFloat( pzName, &v, nIndex ) < 0 )
	{
		v = vDefault;
	}

	return v;
}

/** Get a double
 * \par		Description:
 *		Retrieve a double precision floating point value from a Settings object.
 * \param pzName Name of the item to get.
 * \param pzDefault Default value to return, if the item is not present in the
 *	  Settings object.
 * \param nIndex If several items with the same name exists you can use this
 *        parameter to specify which one you want.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
double Settings::GetDouble( const char *pzName, double vDefault, int nIndex ) const
{
	double v;

	if( FindDouble( pzName, &v, nIndex ) < 0 )
	{
		v = vDefault;
	}

	return v;
}

/** Get a Rect
 * \par		Description:
 *		Retrieve a Rect value from a Settings object.
 * \param pzName Name of the item to get.
 * \param pzDefault Default value to return, if the item is not present in the
 *	  Settings object.
 * \param nIndex If several items with the same name exists you can use this
 *        parameter to specify which one you want.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
Rect Settings::GetRect( const char *pzName, const Rect & cDefault, int nIndex ) const
{
	Rect v;

	if( FindRect( pzName, &v, nIndex ) < 0 )
	{
		v = cDefault;
	}

	return v;
}

/** Get an IRect
 * \par		Description:
 *		Retrieve an IRect value from a Settings object.
 * \param pzName Name of the item to get.
 * \param pzDefault Default value to return, if the item is not present in the
 *	  Settings object.
 * \param nIndex If several items with the same name exists you can use this
 *        parameter to specify which one you want.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
IRect Settings::GetIRect( const char *pzName, const IRect & cDefault, int nIndex ) const
{
	IRect v;

	if( FindIRect( pzName, &v, nIndex ) < 0 )
	{
		v = cDefault;
	}

	return v;
}

/** Get a Point
 * \par		Description:
 *		Retrieve a Point value from a Settings object.
 * \param pzName Name of the item to get.
 * \param pzDefault Default value to return, if the item is not present in the
 *	  Settings object.
 * \param nIndex If several items with the same name exists you can use this
 *        parameter to specify which one you want.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
Point Settings::GetPoint( const char *pzName, const Point & cDefault, int nIndex ) const
{
	Point v;

	if( FindPoint( pzName, &v, nIndex ) < 0 )
	{
		v = cDefault;
	}

	return v;
}

/** Get an IPoint
 * \par		Description:
 *		Retrieve an IPoint value from a Settings object.
 * \param pzName Name of the item to get.
 * \param pzDefault Default value to return, if the item is not present in the
 *	  Settings object.
 * \param nIndex If several items with the same name exists you can use this
 *        parameter to specify which one you want.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
IPoint Settings::GetIPoint( const char *pzName, const IPoint & cDefault, int nIndex ) const
{
	IPoint v;

	if( FindIPoint( pzName, &v, nIndex ) < 0 )
	{
		v = cDefault;
	}

	return v;
}

/** Get a Color32_s
 * \par		Description:
 *		Retrieve a 32 bit colour (Color32_s) value from a Settings object.
 * \param pzName Name of the item to get.
 * \param pzDefault Default value to return, if the item is not present in the
 *	  Settings object.
 * \param nIndex If several items with the same name exists you can use this
 *        parameter to specify which one you want.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
Color32_s Settings::GetColor32( const char *pzName, const Color32_s & cDefault, int nIndex ) const
{
	Color32_s v;

	if( FindColor32( pzName, &v, nIndex ) < 0 )
	{
		v = cDefault;
	}

	return v;
}

/** Get a Variant
 * \par		Description:
 *		Retrieve a Variant value from a Settings object.
 * \param pzName Name of the item to get.
 * \param pzDefault Default value to return, if the item is not present in the
 *	  Settings object.
 * \param nIndex If several items with the same name exists you can use this
 *        parameter to specify which one you want.
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
Variant Settings::GetVariant( const char *pzName, const Variant & cDefault, int nIndex ) const
{
	Variant v;

	if( FindVariant( pzName, &v, nIndex ) < 0 )
	{
		v = cDefault;
	}

	return v;
}

/** Add or change a Variant member.
 * \par Description:
 *	Add or change a Variant object member.
 * \par
 *	See SetData() and os::Message::AddData() for more details on how to
 *	add and change member values.
 * \param pzName
 *	A name uniquely identifying the object within the settings object.
 *	It is possible to add multiple objects of the same type under
 *	the same name but avoid adding different types of object under the
 *	same name. If you really need to do that, use Variants.
 * \param cValue
 *	The data to add.
 * \param nIndex
 *	Index to add the data to, if several objects have been added under
 *	the same name. An index outside the range of the array will add the
 *	object to the end. Leave this at 0 if you don't add multiple objects
 *	with the same name.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory.
 *	</dl>
 * \sa GetVariant(), SetData(), os::Message::FindVariant(),
 *	os::Message::AddVariant(), os::Message::AddData(),
 *	os::Message::FindData()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t Settings::SetVariant( const char *pzName, const Variant & cValue, int nIndex )
{
	DataArray_s *psArray = _FindArray( pzName, T_VARIANT );
	Chunk_s *psChunk;
	size_t nSize = cValue.GetFlattenedSize();

	if( nIndex < 0 )
		nIndex = 0x7FFFFFFF;

	if( psArray == NULL )
	{
		return AddVariant( pzName, cValue );
	}
	else
	{
		psChunk = _GetChunkAddr( psArray, nIndex );
		if( psChunk == NULL )
		{
			return AddVariant( pzName, cValue );
		}
		else
		{
			psChunk = _ResizeChunk( psArray, psChunk, nSize );
			if( psChunk )
			{
				cValue.Flatten( ( char * )psChunk + 4, nSize );
				return 0;
			}
			errno = ENOMEM;
			return -1;
		}
	}
}

/** Add or change a string member.
 * \par Description:
 *	Add or change a string object member.
 * \par
 *	See SetData() and os::Message::AddData() for more details on how to
 *	add and change member values.
 * \param pzName
 *	A name uniquely identifying the object within the settings object.
 *	It is possible to add multiple objects of the same type under
 *	the same name but avoid adding different types of object under the
 *	same name. If you really need to do that, use Variants.
 * \param cValue
 *	The data to add.
 * \param nIndex
 *	Index to add the data to, if several objects have been added under
 *	the same name. An index outside the range of the array will add the
 *	object to the end. Leave this at 0 if you don't add multiple objects
 *	with the same name.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory.
 *	</dl>
 * \sa GetString(), SetData(), os::Message::FindString(),
 *	os::Message::AddString(), os::Message::AddData(),
 *	os::Message::FindData()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t Settings::SetString( const char *pzName, const String & cValue, int nIndex )
{
	return SetData( pzName, T_STRING, cValue.c_str(), cValue.size(  ) + 1, nIndex, false );
}

/** Add or change an int8 member.
 * \par Description:
 *	Add or change a 8 bit integer value.
 * \par
 *	See SetData() and os::Message::AddData() for more details on how to
 *	add and change member values.
 * \param pzName
 *	A name uniquely identifying the object within the settings object.
 *	It is possible to add multiple objects of the same type under
 *	the same name but avoid adding different types of object under the
 *	same name. If you really need to do that, use Variants.
 * \param nValue
 *	The data to add.
 * \param nIndex
 *	Index to add the data to, if several objects have been added under
 *	the same name. An index outside the range of the array will add the
 *	object to the end. Leave this at 0 if you don't add multiple objects
 *	with the same name.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory.
 *	</dl>
 * \sa GetInt8(), SetData(), os::Message::FindInt8(),
 *	os::Message::AddInt8(), os::Message::AddData(),
 *	os::Message::FindData()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t Settings::SetInt8( const char *pzName, int8 nValue, int nIndex )
{
	return SetData( pzName, T_INT8, &nValue, sizeof( nValue ), nIndex, true );
}

/** Add or change an int16 member.
 * \par Description:
 *	Add or change a 16 bit integer value.
 * \par
 *	See SetData() and os::Message::AddData() for more details on how to
 *	add and change member values.
 * \param pzName
 *	A name uniquely identifying the object within the settings object.
 *	It is possible to add multiple objects of the same type under
 *	the same name but avoid adding different types of object under the
 *	same name. If you really need to do that, use Variants.
 * \param nValue
 *	The data to add.
 * \param nIndex
 *	Index to add the data to, if several objects have been added under
 *	the same name. An index outside the range of the array will add the
 *	object to the end. Leave this at 0 if you don't add multiple objects
 *	with the same name.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory.
 *	</dl>
 * \sa GetInt16(), SetData(), os::Message::FindInt16(),
 *	os::Message::AddInt16(), os::Message::AddData(),
 *	os::Message::FindData()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t Settings::SetInt16( const char *pzName, int16 nValue, int nIndex )
{
	return SetData( pzName, T_INT16, &nValue, sizeof( nValue ), nIndex, true );
}

/** Add or change an int32 member.
 * \par Description:
 *	Add or change a 32 bit integer value.
 * \par
 *	See SetData() and os::Message::AddData() for more details on how to
 *	add and change member values.
 * \param pzName
 *	A name uniquely identifying the object within the settings object.
 *	It is possible to add multiple objects of the same type under
 *	the same name but avoid adding different types of object under the
 *	same name. If you really need to do that, use Variants.
 * \param nValue
 *	The data to add.
 * \param nIndex
 *	Index to add the data to, if several objects have been added under
 *	the same name. An index outside the range of the array will add the
 *	object to the end. Leave this at 0 if you don't add multiple objects
 *	with the same name.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory.
 *	</dl>
 * \sa GetInt32(), SetData(), os::Message::FindInt32(),
 *	os::Message::AddInt32(), os::Message::AddData(),
 *	os::Message::FindData()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t Settings::SetInt32( const char *pzName, int32 nValue, int nIndex )
{
	return SetData( pzName, T_INT32, &nValue, sizeof( nValue ), nIndex, true );
}

/** Add or change an int64 member.
 * \par Description:
 *	Add or change a 64 bit integer value.
 * \par
 *	See SetData() and os::Message::AddData() for more details on how to
 *	add and change member values.
 * \param pzName
 *	A name uniquely identifying the object within the settings object.
 *	It is possible to add multiple objects of the same type under
 *	the same name but avoid adding different types of object under the
 *	same name. If you really need to do that, use Variants.
 * \param nValue
 *	The data to add.
 * \param nIndex
 *	Index to add the data to, if several objects have been added under
 *	the same name. An index outside the range of the array will add the
 *	object to the end. Leave this at 0 if you don't add multiple objects
 *	with the same name.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory.
 *	</dl>
 * \sa GetInt64(), SetData(), os::Message::FindInt64(),
 *	os::Message::AddInt64(), os::Message::AddData(),
 *	os::Message::FindData()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t Settings::SetInt64( const char *pzName, int64 nValue, int nIndex )
{
	return SetData( pzName, T_INT64, &nValue, sizeof( nValue ), nIndex, true );
}

/** Add or change a bool member.
 * \par Description:
 *	Add or change a boolean value member.
 * \par
 *	See SetData() and os::Message::AddData() for more details on how to
 *	add and change member values.
 * \param pzName
 *	A name uniquely identifying the object within the settings object.
 *	It is possible to add multiple objects of the same type under
 *	the same name but avoid adding different types of object under the
 *	same name. If you really need to do that, use Variants.
 * \param bValue
 *	The data to add.
 * \param nIndex
 *	Index to add the data to, if several objects have been added under
 *	the same name. An index outside the range of the array will add the
 *	object to the end. Leave this at 0 if you don't add multiple objects
 *	with the same name.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory.
 *	</dl>
 * \sa GetBool(), SetData(), os::Message::FindBool(),
 *	os::Message::AddBool(), os::Message::AddData(),
 *	os::Message::FindData()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t Settings::SetBool( const char *pzName, bool bValue, int nIndex )
{
	return SetData( pzName, T_BOOL, &bValue, sizeof( bValue ), nIndex, true );
}

/** Add or change a float member.
 * \par Description:
 *	Add or change a float point value member.
 * \par
 *	See SetData() and os::Message::AddData() for more details on how to
 *	add and change member values.
 * \param pzName
 *	A name uniquely identifying the object within the settings object.
 *	It is possible to add multiple objects of the same type under
 *	the same name but avoid adding different types of object under the
 *	same name. If you really need to do that, use Variants.
 * \param vValue
 *	The data to add.
 * \param nIndex
 *	Index to add the data to, if several objects have been added under
 *	the same name. An index outside the range of the array will add the
 *	object to the end. Leave this at 0 if you don't add multiple objects
 *	with the same name.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory.
 *	</dl>
 * \sa GetFloat(), SetData(), os::Message::FindFloat(),
 *	os::Message::AddFloat(), os::Message::AddData(),
 *	os::Message::FindData()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t Settings::SetFloat( const char *pzName, float vValue, int nIndex )
{
	return SetData( pzName, T_FLOAT, &vValue, sizeof( vValue ), nIndex, true );
}

/** Add or change a double member.
 * \par Description:
 *	Add or change a double precision floating point object member.
 * \par
 *	See SetData() and os::Message::AddData() for more details on how to
 *	add and change member values.
 * \param pzName
 *	A name uniquely identifying the object within the settings object.
 *	It is possible to add multiple objects of the same type under
 *	the same name but avoid adding different types of object under the
 *	same name. If you really need to do that, use Variants.
 * \param vValue
 *	The data to add.
 * \param nIndex
 *	Index to add the data to, if several objects have been added under
 *	the same name. An index outside the range of the array will add the
 *	object to the end. Leave this at 0 if you don't add multiple objects
 *	with the same name.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory.
 *	</dl>
 * \sa GetFloat(), SetData(), os::Message::FindFloat(),
 *	os::Message::AddFloat(), os::Message::AddData(),
 *	os::Message::FindData()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t Settings::SetDouble( const char *pzName, double vValue, int nIndex )
{
	return SetData( pzName, T_DOUBLE, &vValue, sizeof( vValue ), nIndex, true );
}

/** Add or change an Rect member.
 * \par Description:
 *	Add or change an Rect object member.
 * \par
 *	See SetData() and os::Message::AddData() for more details on how to
 *	add and change member values.
 * \param pzName
 *	A name uniquely identifying the object within the settings object.
 *	It is possible to add multiple objects of the same type under
 *	the same name but avoid adding different types of object under the
 *	same name. If you really need to do that, use Variants.
 * \param cValue
 *	The data to add.
 * \param nIndex
 *	Index to add the data to, if several objects have been added under
 *	the same name. An index outside the range of the array will add the
 *	object to the end. Leave this at 0 if you don't add multiple objects
 *	with the same name.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory.
 *	</dl>
 * \sa GetRect(), SetData(), os::Message::FindRect(),
 *	os::Message::AddRect(), os::Message::AddData(),
 *	os::Message::FindData()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t Settings::SetRect( const char *pzName, const Rect & cValue, int nIndex )
{
	return SetData( pzName, T_RECT, &cValue, sizeof( cValue ), nIndex, true );
}

/** Add or change an IRect object member.
 * \par Description:
 *	Add or change an IRect object member.
 * \par
 *	See SetData() and os::Message::AddData() for more details on how to
 *	add and change member values.
 * \param pzName
 *	A name uniquely identifying the object within the settings object.
 *	It is possible to add multiple objects of the same type under
 *	the same name but avoid adding different types of object under the
 *	same name. If you really need to do that, use Variants.
 * \param cValue
 *	The data to add.
 * \param nIndex
 *	Index to add the data to, if several objects have been added under
 *	the same name. An index outside the range of the array will add the
 *	object to the end. Leave this at 0 if you don't add multiple objects
 *	with the same name.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory.
 *	</dl>
 * \sa GetIRect(), SetData(), os::Message::FindIRect(),
 *	os::Message::AddIRect(), os::Message::AddData(),
 *	os::Message::FindData()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t Settings::SetIRect( const char *pzName, const IRect & cValue, int nIndex )
{
	return SetData( pzName, T_IRECT, &cValue, sizeof( cValue ), nIndex, true );
}

/** Add or change a Point object member.
 * \par Description:
 *	Add or change a Point object member.
 * \par
 *	See SetData() and os::Message::AddData() for more details on how to
 *	add and change member values.
 * \param pzName
 *	A name uniquely identifying the object within the settings object.
 *	It is possible to add multiple objects of the same type under
 *	the same name but avoid adding different types of object under the
 *	same name. If you really need to do that, use Variants.
 * \param cValue
 *	The data to add.
 * \param nIndex
 *	Index to add the data to, if several objects have been added under
 *	the same name. An index outside the range of the array will add the
 *	object to the end. Leave this at 0 if you don't add multiple objects
 *	with the same name.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory.
 *	</dl>
 * \sa GetPoint(), SetData(), os::Message::FindPoint(),
 *	os::Message::AddPoint(), os::Message::AddData(),
 *	os::Message::FindData()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t Settings::SetPoint( const char *pzName, const Point & cValue, int nIndex )
{
	return SetData( pzName, T_POINT, &cValue, sizeof( cValue ), nIndex, true );
}

/** Add or change an IPoint object member.
 * \par Description:
 *	Add or change a IPoint object member.
 * \par
 *	See SetData() and os::Message::AddData() for more details on how to
 *	add and change member values.
 * \param pzName
 *	A name uniquely identifying the object within the settings object.
 *	It is possible to add multiple objects of the same type under
 *	the same name but avoid adding different types of object under the
 *	same name. If you really need to do that, use Variants.
 * \param cValue
 *	The data to add.
 * \param nIndex
 *	Index to add the data to, if several objects have been added under
 *	the same name. An index outside the range of the array will add the
 *	object to the end. Leave this at 0 if you don't add multiple objects
 *	with the same name.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory.
 *	</dl>
 * \sa GetIPoint(), SetData(), os::Message::FindIPoint(),
 *	os::Message::AddIPoint(), os::Message::AddData(),
 *	os::Message::FindData()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t Settings::SetIPoint( const char *pzName, const IPoint & cValue, int nIndex )
{
	return SetData( pzName, T_IPOINT, &cValue, sizeof( cValue ), nIndex, true );
}

/** Add or change a Color32_s object member.
 * \par Description:
 *	Add or change a Color32_s object member.
 * \par
 *	See SetData() and os::Message::AddData() for more details on how to
 *	add and change member values.
 * \param pzName
 *	A name uniquely identifying the object within the settings object.
 *	It is possible to add multiple objects of the same type under
 *	the same name but avoid adding different types of object under the
 *	same name. If you really need to do that, use Variants.
 * \param cValue
 *	The data to add.
 * \param nIndex
 *	Index to add the data to, if several objects have been added under
 *	the same name. An index outside the range of the array will add the
 *	object to the end. Leave this at 0 if you don't add multiple objects
 *	with the same name.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory.
 *	</dl>
 * \sa GetColor32(), SetData(), os::Message::FindColor32(),
 *	os::Message::AddColor32(), os::Message::AddData(),
 *	os::Message::FindData()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t Settings::SetColor32( const char *pzName, const Color32_s & cValue, int nIndex )
{
	return SetData( pzName, T_COLOR32, &cValue, sizeof( cValue ), nIndex, true );
}

/** Add or change a message object member.
 * \par Description:
 *	Add or change a message object member.
 * \par
 *	See SetData() and os::Message::AddData() for more details on how to
 *	add and change member values.
 * \param pzName
 *	A name uniquely identifying the object within the settings object.
 *	It is possible to add multiple objects of the same type under
 *	the same name but avoid adding different types of object under the
 *	same name. If you really need to do that, use Variants.
 * \param cValue
 *	The data to add.
 * \param nIndex
 *	Index to add the data to, if several objects have been added under
 *	the same name. An index outside the range of the array will add the
 *	object to the end. Leave this at 0 if you don't add multiple objects
 *	with the same name.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory to expand the message.
 *	</dl>
 * \sa GetMessage(), SetData(), os::Message::FindMessage(),
 *	os::Message::AddMessage(), os::Message::AddData(),
 *	os::Message::FindData()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t Settings::SetMessage( const char *pzName, const Message & cValue, int nIndex )
{
	DataArray_s *psArray = _FindArray( pzName, T_MESSAGE );
	Chunk_s *psChunk;
	size_t nSize = cValue.GetFlattenedSize();

	if( nIndex < 0 )
		nIndex = 0x7FFFFFFF;

	if( psArray == NULL )
	{
		return AddMessage( pzName, &cValue );
	}
	else
	{
		psChunk = _GetChunkAddr( psArray, nIndex );
		if( psChunk == NULL )
		{
			return AddMessage( pzName, &cValue );
		}
		else
		{
			psChunk = _ResizeChunk( psArray, psChunk, nSize );
			if( psChunk )
			{
				cValue.Flatten( ( uint8 * )psChunk + 4, ( uint32 )nSize );
				return 0;
			}
			errno = ENOMEM;
			return ( -1 );
		}
	}
}

/** Change a data member.
 * \par Description:
 *	Add or change data in the os::Message data container, upon which the
 *	settings class is based. See os::Message::AddData() for more
 *	information on the internal storage mechanism.
 * \par
 *	SetData() is a "catch all" function that can be used to add or change
 *	an untyped blob of data to the message. Normally you will use one
 *	of the other Set*() members that change a specificly typed data member
 *	but I will describe the general principle of changing members here.
 * \par
 *	When you add a member to a message (or settings object) you must give
 *	it a unique name that will later be used to lookup that member. It is
 *	possible to add multiple elements with the same name but they must
 *	then be	of the same type and subsequent elements will be appended to
 *	an array associated with this name. Individual elements from the
 *	array can later be destined with the name/index pair passed to
 *	the various Get*() members.
 * \note
 *	The data buffer is copied into the message and you retain ownership
 *	over it.
 * \param pzName
 *	The name used to identify the member. If there already exist a member
 *	with this name in the message the new member will be appended to
 *	an array of elements under this name. It is an error to append
 *	objects with different types under the same name.
 * \param nType
 *	Data type. This should be one of the predefined
 *	\ref os_util_typecodes "type codes".
 *	Normally you should only use this member to add T_RAW type data.
 *	All the more specific data-types should be added with one of
 *	the specialized Add*() members.
 * \param pData
 *	Pointer to the data to be add. The data will be copied into the message.
 * \param nSize
 *	Size of the \p pData buffer.
 * \param nIndex
 *	Array index. Always use 0 unless you've added multiple objects under the
 *	same name. A value outside of the range of the array will cause the object
 *	to be appended to the end of the array.
 * \param bFixedSize
 *	If you plan to make an array of members under the same name you must
 *	let the message know if each element can have a different size. If
 *	the message know that all the elements have the same size it can
 *	optimize the data a bit by only storing the size once. It will also
 *	greatly speed up array-element lookups if each element have the
 *	same size. If all elements in an array will have the same size or
 *	if you plan to add only one member under this name \p bFixedSize
 *	should be \b true. Otherwhice it should be \b false.
 * \param nMaxCountHint
 *	An estimate of how many members are going to be added to this array.
 *	If you plan to add many elements under the same name and you know
 *	up-front how many you are going to add it is a good idea to let
 *	the message know when adding the first element. The \p nMaxCountHint
 *	will be used to decide how much memory to be allocated for the array
 *	and if the estimate is correct it will avoid any expensive reallocations
 *	during element insertions.
 * \return On success 0 is returned. On error -1 is returned and an error code
 *	is written to the global variable \b errno.
 * \sa os::Message::FindData(), os::Message::SetData()
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
status_t Settings::SetData( const char *pzName, int nType, const void *pData, uint32 nSize, int nIndex, bool bFixedSize, int nMaxCountHint )
{
	DataArray_s *psArray = _FindArray( pzName, nType );

	if( nIndex < 0 )
		nIndex = 0x7FFFFFFF;

	if( psArray == NULL || nIndex >= psArray->nCount )
	{
		return AddData( pzName, nType, pData, nSize, bFixedSize, nMaxCountHint );
	}
	else
	{
		if( bFixedSize )
		{
			memcpy( ( uint8 * )psArray + sizeof( DataArray_s ) + psArray->nNameLength + nIndex * psArray->nChunkSize, pData, nSize );
			return 0;
		}
		else
		{
			Chunk_s *psChunk;

			psChunk = _GetChunkAddr( psArray, nIndex );
			if( psChunk == NULL )
			{
				return AddData( pzName, nType, pData, nSize, bFixedSize, nMaxCountHint );
			}
			else
			{
				psChunk = _ResizeChunk( psArray, psChunk, nSize );
				if( psChunk )
				{
					memcpy( ( char * )psChunk + 4, pData, nSize );
					return 0;
				}
				errno = ENOMEM;
				return ( -1 );
			}
		}
	}
}

/** \internal
 * Expand or shrink a chunk (a part of a variable-size element data array).
 */
Message::Chunk_s * Settings::_ResizeChunk( Message::DataArray_s * psArray, Message::Chunk_s * psChunk, uint32 nSize )
{
	if( nSize != ( uint32 )psChunk->nDataSize )
	{
		int nNewSize = psArray->nCurSize + nSize - psChunk->nDataSize;
		DataArray_s *psNewArray = ( DataArray_s * ) malloc( nNewSize );

		if( psNewArray != NULL )
		{
			_RemoveArray( psArray );
			uint32 nChunkStart = ( ( uint32 )psChunk - ( uint32 )psArray );
			uint32 nCurChunkEnd = nChunkStart + psChunk->nDataSize + 4;
			uint32 nNewChunkEnd = nCurChunkEnd + nSize - psChunk->nDataSize;
			int32 nSizeAfterChunk = ( nNewSize - nNewChunkEnd );

			// Copy all the stuff before the chunk we want to resize
			memcpy( psNewArray, psArray, nChunkStart );

			// Copy all the stuff (if any) after the chunk we want to resize
			if( nSizeAfterChunk > 0 )
			{
				memcpy( ( ( uint8 * )psNewArray + nNewChunkEnd ), ( ( uint8 * )psChunk + psChunk->nDataSize + 4 ), nSizeAfterChunk );
			}

			// Set the size field of the resized chunk
			( ( Chunk_s * ) ( ( uint8 * )psNewArray + nChunkStart ) )->nDataSize = nSize;

			// Clear the contents
			memset( ( ( uint8 * )psNewArray + nChunkStart + 4 ), '\0', nSize );

			// Calculate total size
			m_nFlattenedSize = m_nFlattenedSize - psArray->nCurSize + nNewSize;

			// Free the old array
			free( psArray );
			psArray = psNewArray;
			psArray->psNext = NULL;
			psArray->nMaxSize = nNewSize;
			psArray->nCurSize = nNewSize;

			Chunk_s *psNewChunk = ( Chunk_s * ) ( ( uint8 * )psArray + nChunkStart );

			_AddArray( psArray );

			return psNewChunk;
		}
	}
	else
	{
		// If the size hasn't changed
		return psChunk;
	}

	return ( NULL );
}
