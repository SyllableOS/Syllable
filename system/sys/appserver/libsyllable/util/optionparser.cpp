#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>

#include <util/optionparser.h>
#include <util/string.h>


using namespace os;

struct OptionDesc
{
	OptionDesc( char nOption, const String & cLongName, const String & cDesc ):m_nOption( nOption ), m_cLongName( cLongName ), m_cDescription( cDesc )
	{
	}
	char m_nOption;

	String m_cLongName;
	String m_cDescription;
};

class OptionParser::Private
{
      public:
	int m_nCurrentOption;
	std::vector < OptionDesc > m_cOptionMap;
	std::vector < String > m_cArgs;
	std::vector < String > m_cFileArgs;
	std::vector < option > m_cOptionList;
};

OptionParser::OptionParser( int argc, const char *const *argv )
{
	m = new Private;
	for( int i = 0; i < argc; ++i )
	{
		m->m_cArgs.push_back( argv[i] );
	}
	m->m_nCurrentOption = 0;
}

OptionParser::~OptionParser()
{
	delete m;
}

void OptionParser::AddArgMap( const String & cLongArg, char nShortArg, const String & cDescription )
{
	m->m_cOptionMap.push_back( OptionDesc( nShortArg, cLongArg, cDescription ) );
}

void OptionParser::AddArgMap( const argmap * pasMap )
{
	for( int i = 0; pasMap[i].long_name != NULL; ++i )
	{
		m->m_cOptionMap.push_back( OptionDesc( pasMap[i].opt, pasMap[i].long_name, pasMap[i].desc ) );
	}
}


void OptionParser::ParseOptions( const char *pzOptions )
{
	m->m_cFileArgs.clear();
	m->m_cOptionList.clear();

	bool bDoParse = true;

	for( uint i = 1; i < m->m_cArgs.size(); ++i )
	{
		const String & cArg = m->m_cArgs[i];

		if( bDoParse == false )
		{
			m->m_cFileArgs.push_back( cArg );
			continue;
		}
		if( bDoParse && cArg == "--" )
		{
			bDoParse = false;
			continue;
		}
		if( cArg.size() < 2 || cArg[0] != '-' )
		{
			m->m_cFileArgs.push_back( cArg );
			continue;
		}
		option sOption;

		if( cArg.size() == 2 )
		{
			const char *pzOptChr = strchr( pzOptions, cArg[1] );

			if( pzOptChr == NULL )
			{
				m->m_cFileArgs.push_back( cArg );
			}
			else
			{
				if( pzOptChr[1] != ':' || i == m->m_cArgs.size() - 1 || m->m_cArgs[i + 1][0] == '-' )
				{
					sOption.opt = cArg[1];
					sOption.raw_opt = cArg;
					sOption.has_value = false;
				}
				else
				{
					sOption.opt = cArg[1];
					sOption.raw_opt = cArg;
					sOption.value = m->m_cArgs[i + 1];
					sOption.has_value = true;
					i++;
				}
				m->m_cOptionList.push_back( sOption );
			}
		}
		else if( cArg[1] == '-' )
		{
			int nOffset = cArg.const_str().find( '=' );

			if( nOffset == String::npos )
			{
				sOption.long_name = cArg.c_str() + 2;
				sOption.opt = _MapLongOpt( sOption.long_name );
				sOption.raw_opt = cArg;
				sOption.has_value = false;
			}
			else
			{
				sOption.long_name = String( cArg.begin() + 2, cArg.begin(  ) + nOffset );
				sOption.opt = _MapLongOpt( sOption.long_name );
				sOption.value = String( cArg.begin() + nOffset + 1, cArg.end(  ) );
				sOption.raw_opt = cArg;
				sOption.has_value = true;
			}
			m->m_cOptionList.push_back( sOption );
		}
		else
		{
			m->m_cFileArgs.push_back( cArg );
		}
	}
}

int OptionParser::GetOptionCount() const
{
	return ( m->m_cOptionList.size() );
}

int OptionParser::GetFileCount() const
{
	return ( m->m_cFileArgs.size() );
}

const OptionParser::option * OptionParser::FindOption( char nOpt ) const
{
	for( uint i = 0; i < m->m_cOptionList.size(); ++i )
	{
		if( m->m_cOptionList[i].opt == nOpt )
		{
			return ( &m->m_cOptionList[i] );
		}
	}
	return ( NULL );
}

const OptionParser::option * OptionParser::FindOption( const String & cLongName ) const
{
	for( uint i = 0; i < m->m_cOptionList.size(); ++i )
	{
		if( m->m_cOptionList[i].long_name == cLongName )
		{
			return ( &m->m_cOptionList[i] );
		}
	}
	return ( NULL );
}

const OptionParser::option * OptionParser::GetNextOption()
{
	if( m->m_nCurrentOption >= int ( m->m_cOptionList.size() ) )
	{
		return ( NULL );
	}
	else
	{
		return ( &m->m_cOptionList[m->m_nCurrentOption++] );
	}
}

void OptionParser::RewindOptions()
{
	m->m_nCurrentOption = 0;
}

const OptionParser::option * OptionParser::GetOption( uint nIndex ) const
{
	if( nIndex < m->m_cOptionList.size() )
	{
		return ( &m->m_cOptionList[nIndex] );
	}
	else
	{
		return ( NULL );
	}
}

String OptionParser::operator[]( int nIndex )
    const
    {
	    return ( m->m_cFileArgs[nIndex] );
    }

    const std::vector < String > &OptionParser::GetArgs() const
{
	return ( m->m_cArgs );
}

const std::vector < String > &OptionParser::GetFileArgs() const
{
	return ( m->m_cFileArgs );
}

String OptionParser::GetHelpText( int nWidth ) const
{
	if( nWidth == 0 )
	{
		struct winsize sWinSize;

		if( ioctl( 1, TIOCGWINSZ, &sWinSize ) == 0 )
		{
			nWidth = ( sWinSize.ws_col > 16 ) ? sWinSize.ws_col : 16;
		}
		else
		{
			nWidth = 80;
		}
	}
	uint nMaxArgLen = 0;

	for( uint i = 0; i < m->m_cOptionMap.size(); ++i )
	{
		if( m->m_cOptionMap[i].m_cLongName.size() > nMaxArgLen )
		{
			nMaxArgLen = m->m_cOptionMap[i].m_cLongName.size();
		}
	}
	int nTotArgLen = nMaxArgLen + 10;

	bool bToSmall = false;

	if( nTotArgLen + 8 >= nWidth )
	{
		bToSmall = true;
	}
	String cBuffer;

	for( uint i = 0; i < m->m_cOptionMap.size(); ++i )
	{
		String cLine = "  ";
		if( m->m_cOptionMap[i].m_nOption != 0 )
		{
			cLine += "-";
			cLine += m->m_cOptionMap[i].m_nOption;
			if( m->m_cOptionMap[i].m_cLongName.empty() )
			{
				cLine += "  ";
			}
			else
			{
				cLine += ", ";
			}
		}
		else
		{
			cLine += "    ";
		}
		if( m->m_cOptionMap[i].m_cLongName.size() > 0 )
		{
			cLine += "--";
			cLine += m->m_cOptionMap[i].m_cLongName;
			if( bToSmall == false )
			{
				cLine.resize( cLine.size() + nMaxArgLen + 2 - m->m_cOptionMap[i].m_cLongName.size(  ), ' ' );
			}
			else
			{
				cLine += "\n";
			}
		}
		else
		{
			if( bToSmall == false )
			{
				cLine.resize( cLine.size() + nMaxArgLen + 4, ' ' );
			}
			else
			{
				cLine += "\n";
			}
		}
		String cDesc( m->m_cOptionMap[i].m_cDescription );
		uint nSegLen = nWidth - nTotArgLen;

		if( bToSmall )
		{
			nSegLen = nWidth;
		}

		bool bFirst = true;

		while( cDesc.size() > 0 )
		{
			uint nLen = 0;
			uint nEraseLen = 0;

			if( bToSmall == false && bFirst == false )
			{
				cLine.resize( cLine.size() + nTotArgLen, ' ' );
			}
			bFirst = false;
			if( cDesc.size() <= nSegLen )
			{
				cLine += cDesc;
				break;
			}
			else
			{
				if( bToSmall == false )
				{
					for( ;; )
					{
						int nWordLen = 0;

						for( uint i = nLen; i <= cDesc.size(); ++i )
						{
							if( i == cDesc.size() || isspace( cDesc[i] ) )
							{
								nWordLen = i - nLen;
								break;
							}
						}
						int nNumSpaces = 0;

						while( nLen + nWordLen + nNumSpaces < cDesc.size() && isspace( cDesc[nLen + nWordLen + nNumSpaces] ) )
						{
							nNumSpaces++;
						}
						if( nLen + nWordLen > nSegLen )
						{
							if( nLen == 0 )
							{
								nLen = nSegLen;
								nEraseLen = nSegLen;
							}
							else
							{
								nEraseLen = nLen;
								while( nEraseLen < cDesc.size() && isspace( cDesc[nEraseLen] ) )
								{
									nEraseLen++;
								}
							}
							break;
						}
						else
						{
							nLen += nWordLen;
							if( nLen + nNumSpaces > nSegLen )
							{
								nEraseLen = nLen + nNumSpaces;
								break;
							}
							else
							{
								nLen += nNumSpaces;
							}
						}
					}
				}
				else
				{
					nLen = nWidth;
					nEraseLen = nWidth;
				}
				cLine.str().insert( cLine.end(), cDesc.begin(  ), cDesc.begin(  ) + nLen );
				cDesc.str().erase( cDesc.begin(), cDesc.begin(  ) + nEraseLen );
				cLine += "\n";
			}
		}
		cLine += "\n";
		cBuffer += cLine;
	}
	return ( cBuffer );
}

void OptionParser::PrintHelpText( int nWidth ) const
{
	printf( "%s", GetHelpText( nWidth ).c_str() );
}

void OptionParser::PrintHelpText( StreamableIO* pcStream, int nWidth ) const
{
	if( pcStream )
	{
		String cHelpText = GetHelpText( nWidth );
		pcStream->Write( (void*)cHelpText.c_str(), cHelpText.CountChars() );
	}
}

char OptionParser::_MapLongOpt( const String & cOpt ) const
{
	for( uint i = 0; i < m->m_cOptionMap.size(); ++i )
	{
		if( m->m_cOptionMap[i].m_cLongName == cOpt )
		{
			return ( m->m_cOptionMap[i].m_nOption );
		}
	}
	return ( 0 );
}

