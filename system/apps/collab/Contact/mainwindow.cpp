#include "mainwindow.h"
#include "messages.h"
#include <unistd.h>
#include "resources/Contact.h"


MainWindow::MainWindow( const os::String& cArgv ) : os::Window( os::Rect( 0, 0, 700, 500 ), "main_wnd", MSG_MAINWND_TITLE )
{
	/* Set Icon */
	os::Resources cCol( get_image_id() );
	os::ResStream *pcStream = cCol.GetResourceStream( "icon24x24.png" );
	os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
	SetIcon( pcIcon->LockBitmap() );
	delete( pcIcon );

	/* Set an icon on the Contacts-folder */
	os::String zPath = getenv( "HOME" );
	zPath += os::String("/Contacts");
	mkdir( zPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO );
	int nFile = open( zPath.c_str(), O_RDWR | O_NOTRAVERSE );
	write_attr( nFile, "os::Icon", O_TRUNC, ATTR_TYPE_STRING, "/system/icons/filetypes/application_x_contact.png", 0, strlen( "/system/icons/filetypes/application_x_contact.png" ) );
  	close( nFile );

	/* Stuff */
	os::LayoutView* pcView = new os::LayoutView( GetBounds(), "layout_view" );
	#include "mainwindowLayout.cpp"
	pcView->SetRoot( m_pcRoot );
	AddChild( pcView );

	/* Set NodeMonitor */
	os::String cMonitorPath = getenv( "HOME" );
	cMonitorPath += os::String("/Contacts");
	m_pcMonitor = new NodeMonitor( cMonitorPath,NWATCH_ALL,this );

	/* Load the Categories and Contacts */
	LoadCategoryList();
	LoadSettings();
	LoadContactList();
	bContactChanged = false;

	/* We might want to load a Contact at startup... */
	if( cArgv != "empty" )
	{
		cContactPath = cArgv;

		bLoadContact = false;
		bLoadSearch = false;
		bLoadStartup = true;

		ClearContact();
		LoadContact();
	}
}


void MainWindow::HandleMessage( os::Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case M_NODE_MONITOR:
		{
			LoadCategoryList();
			LoadContactList();

			break;
		}


		case M_MENU_HELP:
		case M_MENU_WEB:
		{
			if( fork() == 0 )
			{
				set_thread_priority( -1, 0 );
				execlp( "/Applications/Webster/Webster","/Applications/Webster/Webster","http://www.syllable.org",NULL );
				exit(0);
			}

			break;
		}


		case M_MENU_ABOUT:
		{
			os::String cText = MSG_ABOUTALERT_VERSION + os::String(" 0.4.1\n\n") + MSG_ABOUTALERT_AUTHOR + os::String(" Flemming H. Sørensen (BurningShadow)\n\n") + MSG_ABOUTALERT_DESC + os::String(" ") + MSG_ABOUTALERT_DESCTEXT + os::String("\n");
			os::Alert* pcAlert = new Alert( MSG_ABOUTALERT_TITLE, cText, Alert::ALERT_INFO,0, MSG_ABOUTALERT_OK.c_str(),NULL);
			pcAlert->Go( new os::Invoker( 0 ) );
			pcAlert->MakeFocus();

			break;
		}


		case M_MENU_DELETE:
		case M_TOOLBAR_DELETE:
		{
			int nSelected = -1;
			nSelected = m_pcContactList->GetLastSelected();
			if( nSelected != -1 )
			{
				ListViewStringRow* pcRow = static_cast<os::ListViewStringRow*>( m_pcContactList->GetRow( nSelected ) );
				os::String cTemp = MSG_DELCONTACTALERT_TEXT + os::String( "\n" ) + pcRow->GetString(0);
				os::Alert* pcAlert = new Alert( MSG_DELCONTACTALERT_TITLE, cTemp, Alert::ALERT_WARNING,0, MSG_DELCONTACTALERT_YES.c_str(),MSG_DELCONTACTALERT_NO.c_str(),NULL);
				os::Invoker* pcInvoker = new os::Invoker( new os::Message( M_CONTACT_DELETE_ALERT ), this );
				pcAlert->Go( pcInvoker );
				pcAlert->MakeFocus();
			}

			break;
		}


		case M_CONTACT_DELETE_ALERT:
		{
			/* Delete (Yes) or Cancel (No) */
			int32 nCode;
			if( pcMessage->FindInt32( "which", &nCode ) == 0 )
			{
				switch( nCode )
				{
					case 0: /* Delete */
					{
						int nSelected = -1;
						nSelected = m_pcContactList->GetLastSelected();
						if( m_pcContactList->GetRowCount() > 0 && nSelected != -1 )
						{
							ListViewStringRow* pcRow = static_cast<os::ListViewStringRow*>( m_pcContactList->GetRow( nSelected ) );
							os::String zExec = os::String( "rm -f \"" ) + getenv( "HOME" ) + os::String( "/Contacts/" ) + pcRow->GetString(0) + os::String( "\"" );
							system( zExec.c_str() );
						}
					}
					case 1: /* Cancel */
						break;
				}
			}

			break;
		}


		case M_MENU_NEW:
		case M_TOOLBAR_NEW:
		{
			cChangesChecked = os::String( "new" );
			CheckChanges();
			if( bContactChanged == false )
			{
				cChangesChecked = os::String( "" );
				ClearContact();
			}

			break;
		}


		case M_MENU_SAVE:
		case M_TOOLBAR_SAVE:
		{
			if( m_pcPriAddressZCSDropdown->GetSelection() != 0 )
				SaveContact();
			else
			{
				os::Alert* pcAlert = new os::Alert( MSG_SAVECONTACTALERT_TITLE, MSG_SAVECONTACTALERT_TEXT, Alert::ALERT_WARNING,0, MSG_SAVECONTACTALERT_OK.c_str(),NULL);
				pcAlert->Go( new os::Invoker( 0 ) );
				pcAlert->MakeFocus();
			}

			break;
		}


		case M_CONTACT_CATEGORY:
		{
			LoadContactList();

			break;
		}


		case M_CONTACT_SHOW:
		{
			cChangesChecked = os::String( "contact" );
			CheckChanges();
			if( m_pcContactList->GetLastSelected() >= 0 && bContactChanged == false )
			{
				cChangesChecked = os::String( "" );
				bLoadContact = true;
				bLoadSearch = false;
				bLoadStartup = false;

				ClearContact();
				LoadContact();
			}

			break;
		}


		case M_SEARCH_NOW:
		{
			/* Clear listview */
			m_pcContactList->Clear();

			/* Get Search Type */
			int nSearchType = m_pcSearchDropdown->GetSelection();
			os::String cSearchType = os::String( m_pcSearchDropdown->GetItem( nSearchType ) );

			/* Get Search String */
			os::String cSearchString = m_pcSearchData->GetValue();

			/* Get Category */
			int nContactCategory = m_pcContactCategory->GetSelection();
			os::String cContactCategory = os::String( m_pcContactCategory->GetItem( nContactCategory ) );

			/* Open up contacts directory and check it actually contains anything */
			os::String zSearchPath = getenv( "HOME" );
			zSearchPath += os::String("/Contacts");
			pDir = opendir( zSearchPath.c_str() );
			if (pDir == NULL)
			{
				return;
			}

			/* Loop through directory and check each contact */
			int i = 0;
			while ( (psEntry = readdir( pDir )) != NULL)
			{
				/* If special directory (i.e. dot(.) and dotdot(..) then ignore */
				if (strcmp( psEntry->d_name, ".") == 0 || strcmp( psEntry->d_name, ".." ) == 0)
				{
					continue;
				}
				else
				{
					bool bContactCategory = false;
					os::String cTempSearchPath = zSearchPath + os::String( "/" ) + psEntry->d_name;

					/* Chect the Categories */
					char delims[] = ";;";
					char *result = NULL;
					os::FSNode cFileNode;
					cFileNode.SetTo( cTempSearchPath );
					char zBuffer[PATH_MAX];
					memset( zBuffer, 0, PATH_MAX );
					cFileNode.ReadAttr( "Contact::Category", ATTR_TYPE_STRING, zBuffer, 0, PATH_MAX );
					result = strtok( zBuffer, delims );
					while( result != NULL )
					{
						os::String cResult = result;
						if( cResult == "Family" )
							cResult = MSG_MAINWND_CATEGORY_FAMILY;
						else if( cResult == "Friends" )
							cResult = MSG_MAINWND_CATEGORY_FRIENDS;
						else if( cResult == "Colleagues" )
							cResult = MSG_MAINWND_CATEGORY_COLLEAGUES;
						else if( cResult == "Work" )
							cResult = MSG_MAINWND_CATEGORY_WORK;
						else if( cResult == "Mailinglists" )
							cResult = MSG_MAINWND_CATEGORY_MAILINGLISTS;

						if( cContactCategory == cResult || nContactCategory == 0 )
						{
							bContactCategory = true;
						}

						result = strtok( NULL, delims );
					}

					/* If it's the right category, we will see if we got a search-match */
					if( bContactCategory == true )
					{
						try
						{
							os::Settings* pcSettings = new os::Settings(new File( cTempSearchPath ));
							pcSettings->Load();

							if( pcSettings->GetInt8("Version",0) <= 2 )
							{
								bool bMatchFound = false;

								/* Search Contact-names */
								if( cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_FIRSTNAME || cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_MIDDLENAME || cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_LASTNAME || cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_NICKNAME || cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_COMPANY || nSearchType == 0 && bMatchFound == false )
								{
									if( cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_FIRSTNAME || nSearchType == 0 && bMatchFound == false )
									{
										if( strstr( pcSettings->GetString( "FirstName","" ).c_str(), cSearchString.c_str() ) != NULL )
											bMatchFound = true;
									}
									if( cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_MIDDLENAME || nSearchType == 0 && bMatchFound == false )
									{
										if( strstr( pcSettings->GetString( "MiddleName","" ).c_str(), cSearchString.c_str() ) != NULL )
											bMatchFound = true;
									}
									if( cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_LASTNAME || nSearchType == 0 && bMatchFound == false )
									{
										if( strstr( pcSettings->GetString( "LastName","" ).c_str(), cSearchString.c_str() ) != NULL )
											bMatchFound = true;
									}
									if( cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_NICKNAME || nSearchType == 0 && bMatchFound == false )
									{
										if( strstr( pcSettings->GetString( "NickName","" ).c_str(), cSearchString.c_str() ) != NULL )
											bMatchFound = true;
									}
									if( cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_COMPANY || nSearchType == 0 && bMatchFound == false )
									{
										if( strstr( pcSettings->GetString( "Company","" ).c_str(), cSearchString.c_str() ) != NULL )
											bMatchFound = true;
									}
								}

								/* Search Contact-address */
								if( cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_ADDRESS || cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_ZIP || cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_CITY || cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_STATE || cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_COUNTRY || nSearchType == 0 && bMatchFound == false )
								{
									if( cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_ADDRESS || nSearchType == 0 && bMatchFound == false )
									{
										/* V1 data - start */
										if( strstr( pcSettings->GetString( "PriAddressStreet","" ).c_str(), cSearchString.c_str() ) != NULL )
											bMatchFound = true;
										if( strstr( pcSettings->GetString( "SecAddressStreet","" ).c_str(), cSearchString.c_str() ) != NULL )
											bMatchFound = true;
										/* V1 data - end */
										if( strstr( pcSettings->GetString( "PriAddressAddress1","" ).c_str(), cSearchString.c_str() ) != NULL )
											bMatchFound = true;
										if( strstr( pcSettings->GetString( "PriAddressAddress2","" ).c_str(), cSearchString.c_str() ) != NULL )
											bMatchFound = true;
										if( strstr( pcSettings->GetString( "PriAddressAddress3","" ).c_str(), cSearchString.c_str() ) != NULL )
											bMatchFound = true;
										if( strstr( pcSettings->GetString( "SecAddressAddress1","" ).c_str(), cSearchString.c_str() ) != NULL )
											bMatchFound = true;
										if( strstr( pcSettings->GetString( "SecAddressAddress2","" ).c_str(), cSearchString.c_str() ) != NULL )
											bMatchFound = true;
										if( strstr( pcSettings->GetString( "SecAddressAddress3","" ).c_str(), cSearchString.c_str() ) != NULL )
											bMatchFound = true;
									}
									if( cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_ZIP || nSearchType == 0 && bMatchFound == false )
									{
										if( strstr( pcSettings->GetString( "PriAddressZip","" ).c_str(), cSearchString.c_str() ) != NULL )
											bMatchFound = true;
										if( strstr( pcSettings->GetString( "SecAddressZip","" ).c_str(), cSearchString.c_str() ) != NULL )
											bMatchFound = true;
									}
									if( cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_CITY || nSearchType == 0 && bMatchFound == false )
									{
										if( strstr( pcSettings->GetString( "PriAddressCity","" ).c_str(), cSearchString.c_str() ) != NULL )
											bMatchFound = true;
										if( strstr( pcSettings->GetString( "SecAddressCity","" ).c_str(), cSearchString.c_str() ) != NULL )
											bMatchFound = true;
									}
									if( cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_STATE || nSearchType == 0 && bMatchFound == false )
									{
										if( strstr( pcSettings->GetString( "PriAddressState","" ).c_str(), cSearchString.c_str() ) != NULL )
											bMatchFound = true;
										if( strstr( pcSettings->GetString( "SecAddressState","" ).c_str(), cSearchString.c_str() ) != NULL )
											bMatchFound = true;
									}
									if( cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_COUNTRY || nSearchType == 0 && bMatchFound == false )
									{
										if( strstr( pcSettings->GetString( "PriAddressCountry","" ).c_str(), cSearchString.c_str() ) != NULL )
											bMatchFound = true;
										if( strstr( pcSettings->GetString( "SecAddressCountry","" ).c_str(), cSearchString.c_str() ) != NULL )
											bMatchFound = true;
									}
								}

								/* Search Other Contact Info */
								if( cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_PHONE || cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_CELLPHONE || cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_FAX || cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_PAGER || cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_EMAIL || cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_ICQ || cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_MSN || 
									cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_JABBER || cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_YAHOO || cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_AOL || cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_SKYPE || cSearchType == MSG_MAINWND_CONTACT_SEARCHTYPES_WEBSITE || nSearchType == 0 && bMatchFound == false )
								{
									int counter;
									for( counter = 0; counter <= 999 ; counter = counter + 1)
									{
										char cEntry[3];
										int iEntry = counter;
										sprintf( cEntry, "%i", iEntry );

										if( cSearchType == pcSettings->GetString(cEntry,"",0) || nSearchType == 0 && strstr( pcSettings->GetString(cEntry,"",2).c_str(), cSearchString.c_str() ) != NULL )
										{
											bMatchFound = true;
											counter = 1000 ;
										}
									}
								}

								/* If we found a match, we add the Contact to the list */
								if( bMatchFound == true )
								{
									ListViewStringRow *row = new ListViewStringRow();
									row->AppendString( psEntry->d_name );
									m_pcContactList->InsertRow( row );
								}
							}
							delete( pcSettings );
						}
						catch(...)
						{
						}
					}
				}
				++i;
			}
			closedir( pDir );

			break;
		}


		case M_INFO_BUTTON_CONTACT:
		{
			os::String cTempType;
			os::String cTempData;
			int nSelected = m_pcInfoList->GetLastSelected();
			if( nSelected >= 0 )
			{
				ListViewStringRow* pcRow = static_cast<os::ListViewStringRow*>( m_pcInfoList->GetRow( nSelected ) );
				cTempType = pcRow->GetString(0);
				cTempData = pcRow->GetString(2);
			}

			if( cTempType == "Website" )
			{
				if( fork() == 0 )
				{
					set_thread_priority( -1, 0 );
					execlp( "/Applications/Webster/Webster","/Applications/Webster/Webster",cTempData.c_str(),NULL );
					exit(0);
				}
			}
			else if( cTempType != "" )
			{
				/* Copy data to clipboard */
				os::Clipboard cClipboard;
				cClipboard.Lock();
				cClipboard.Clear();
				os::Message* pcMsg = cClipboard.GetData();
				pcMsg->AddString("text/plain",cTempData);
				cClipboard.Commit();
				cClipboard.Unlock();

				/* ...and let the user know */
				char zClipboardAlert[128];
				sprintf( zClipboardAlert, MSG_CLIPBOARDALERT_TEXT.c_str(), cTempData.c_str() );
				os::Alert* pcAlert = new os::Alert( MSG_CLIPBOARDALERT_TITLE, zClipboardAlert, Alert::ALERT_INFO,0, MSG_CLIPBOARDALERT_OK.c_str(),NULL);
				pcAlert->Go( new os::Invoker( 0 ) );
				pcAlert->MakeFocus();
			}

			break;
		}


		case M_INFO_BUTTON_MOVEUP:
		{
			int nSelected = m_pcInfoList->GetLastSelected();
			if( nSelected > 0 )
			{
				/* Get the Line... */
				ListViewStringRow* pcRow = static_cast<os::ListViewStringRow*>( m_pcInfoList->GetRow( nSelected ) );
				os::String cTempType = pcRow->GetString(0);
				os::String cTempHWO = pcRow->GetString(1);
				os::String cTempData = pcRow->GetString(2);
				m_pcInfoList->RemoveRow( nSelected );

				/* ...and put it somewhere else */
				ListViewStringRow *row = new ListViewStringRow();
				row->AppendString( cTempType );
				row->AppendString( cTempHWO );
				row->AppendString( cTempData );
				m_pcInfoList->InsertRow( nSelected-1, row );
			}

			break;
		}


		case M_INFO_BUTTON_MOVEDOWN:
		{
			uint16 nSelected = m_pcInfoList->GetLastSelected();
			if( nSelected < m_pcInfoList->GetRowCount()-1 && m_pcInfoList->GetRowCount() > 1 )
			{
				/* Get the Line... */
				ListViewStringRow* pcRow = static_cast<os::ListViewStringRow*>( m_pcInfoList->GetRow( nSelected ) );
				os::String cTempType = pcRow->GetString(0);
				os::String cTempHWO = pcRow->GetString(1);
				os::String cTempData = pcRow->GetString(2);
				m_pcInfoList->RemoveRow( nSelected );

				/* ...and put it somewhere else */
				ListViewStringRow *row = new ListViewStringRow();
				row->AppendString( cTempType );
				row->AppendString( cTempHWO );
				row->AppendString( cTempData );
				m_pcInfoList->InsertRow( nSelected+1, row );
			}

			break;
		}


		case M_INFO_BUTTON_REMOVE:
		{
			uint16 nSelected = m_pcInfoList->GetLastSelected();
			if( nSelected < m_pcInfoList->GetRowCount() && m_pcInfoList->GetRowCount() > 0 )
			{
				m_pcInfoList->RemoveRow( nSelected );
			}

			break;
		}


		case M_INFO_BUTTON_ADD:
		{
			/* Get the contact-type */
			int nType = m_pcInfoContactTypeDropdown->GetSelection();
			os::String cTempType = os::String( m_pcInfoContactTypeDropdown->GetItem(nType) );
			if( cTempType == MSG_MAINWND_INFO_TYPE_NEWTYPE )
			{
				cTempType = m_pcInfoContactTypeText->GetValue();
			}
			if( cTempType == "" || cTempType == MSG_MAINWND_INFO_TYPE_CHOOSETYPE )
			{
				os::Alert* pcAlert = new os::Alert( MSG_ADDINFOALERT_TITLE, MSG_ADDINFOALERT_SELECTTYPE, Alert::ALERT_WARNING,0, MSG_ADDINFOALERT_OK.c_str(),NULL);
				pcAlert->Go( new os::Invoker( 0 ) );
				pcAlert->MakeFocus();
			}

			/* Get the contact data */
			os::String cTempData = m_pcInfoContactDataText->GetValue();
			if( cTempData == "" )
			{
				os::Alert* pcAlert = new os::Alert( MSG_ADDINFOALERT_TITLE, MSG_ADDINFOALERT_WRITEDATA, Alert::ALERT_WARNING,0, MSG_ADDINFOALERT_OK.c_str(),NULL);
				pcAlert->Go( new os::Invoker( 0 ) );
				pcAlert->MakeFocus();
			}

			/* Find out where */
			os::String cTempHWO;
			if( m_pcInfoContactHome->GetValue() == "true" )
			{
				cTempHWO = MSG_MAINWND_INFO_WHERE_HOME;
			}
			else if( m_pcInfoContactWork->GetValue() == "true" )
			{
				cTempHWO = MSG_MAINWND_INFO_WHERE_WORK;
			}
			else
			{
				cTempHWO = m_pcInfoContactOtherText->GetValue();
			}

			if( cTempHWO == "" )
			{
				os::Alert* pcAlert = new os::Alert( MSG_ADDINFOALERT_TITLE, MSG_ADDINFOALERT_WHERE, Alert::ALERT_WARNING,0, MSG_ADDINFOALERT_OK.c_str(),NULL);
				pcAlert->Go( new os::Invoker( 0 ) );
				pcAlert->MakeFocus();
			}

			/* Add to the list */
			if(  cTempData != "" && cTempHWO != "" && cTempType != "" && cTempType != MSG_MAINWND_INFO_TYPE_CHOOSETYPE )
			{
				ListViewStringRow *row = new ListViewStringRow();
				row->AppendString( cTempType );
				row->AppendString( cTempHWO );
				row->AppendString( cTempData );
				m_pcInfoList->InsertRow( row );
				m_pcInfoContactDataText->Clear();
				m_pcInfoContactTypeText->Set( MSG_MAINWND_INFO_TYPE_NEWTYPE.c_str() );
			}

			break;
		}


		case M_NAME_CATEGORY_ADD:
		{
			/* Get the contact-category */
			bool bDuplicate = false;
			int nTempCat = m_pcContactCategoryDropdown->GetSelection();
			os::String cTempCat = os::String( m_pcContactCategoryDropdown->GetItem( nTempCat ) );
			if( cTempCat == MSG_MAINWND_CATEGORY_NEWCATEGORY )
			{
				cTempCat = m_pcContactCategoryText->GetValue();
			}

			/* Make cure it's not already there */
			int nCount = m_pcCategoryList->GetRowCount();
			if( nCount > 0 )
			{
				int counter;
				for( counter = 0; counter < nCount ; counter = counter + 1)
				{
					ListViewStringRow* pcRow = static_cast<os::ListViewStringRow*>( m_pcCategoryList->GetRow( counter ) );
					if( cTempCat == pcRow->GetString(0) )
					{
						bDuplicate = true;
					}
				}
			}

			/* Add to the list */
			if( bDuplicate == false && cTempCat != "" && cTempCat != MSG_MAINWND_CATEGORY_CHOOSECATEGORY && cTempCat != MSG_MAINWND_CATEGORY_NEWCATEGORY )
			{
				ListViewStringRow *row = new ListViewStringRow();
				row->AppendString( cTempCat );
				m_pcCategoryList->InsertRow( row );
				m_pcContactCategoryDropdown->SetSelection( (int) 0 );
				m_pcContactCategoryText->Set( MSG_MAINWND_CATEGORY_NEWCATEGORY.c_str() );
			}
			else if( bDuplicate == false )
			{
				os::Alert* pcAlert = new os::Alert( MSG_CATEGORYALERT_TITLE, MSG_CATEGORYALERT_TEXT, Alert::ALERT_WARNING,0, MSG_CATEGORYALERT_OK.c_str(),NULL);
				pcAlert->Go( new os::Invoker( 0 ) );
				pcAlert->MakeFocus();
			}

			break;
		}


		case M_NAME_CATEGORY_REMOVE:
		{
			uint16 nSelected = m_pcCategoryList->GetLastSelected();
			if( nSelected < m_pcCategoryList->GetRowCount() && m_pcCategoryList->GetRowCount() > 0 )
			{
				m_pcCategoryList->RemoveRow( nSelected );
			}

			break;
		}


		case M_CONTACT_CHANGED_ALERT:
		{
			/* Save or Continue or Cancel */
			int32 nCode;
			if( pcMessage->FindInt32( "which", &nCode ) == 0 )
			{
				switch( nCode )
				{
					case 0: /* Save */
						if( m_pcPriAddressZCSDropdown->GetSelection() != 0 )
							SaveContact();
						else
						{
							os::Alert* pcAlert = new os::Alert( MSG_SAVECONTACTALERT_TITLE, MSG_SAVECONTACTALERT_TEXT, Alert::ALERT_WARNING,0, MSG_SAVECONTACTALERT_OK.c_str(),NULL);
							pcAlert->Go( new os::Invoker( 0 ) );
							pcAlert->MakeFocus();
							break;
						}
					case 1: /* Continue */
						bContactChanged = false;
						if( cChangesChecked == "new" )
						{
							ClearContact();
						}
						else if( cChangesChecked == "contact" )
						{
							if( m_pcContactList->GetLastSelected() >= 0 && bContactChanged == false )
							{
								bLoadContact = true;
								bLoadSearch = false;
								ClearContact();
								LoadContact();
							}
						}
						else if( cChangesChecked == "search" )
						{
							if( m_pcContactList->GetLastSelected() >= 0 && bContactChanged == false )
							{
								bLoadContact = false;
								bLoadSearch = true;
								ClearContact();
								LoadContact();
							}
						}
					case 2: /* Cancel */
						break;
				}
			}

			break;
		}


		case M_MENU_QUIT:
		case M_APP_QUIT:
		{
			OkToQuit();
		}
		break;
	}
}


void MainWindow::ClearContact()
{
	m_pcCategoryList->Clear();
	m_pcContactCategoryDropdown->SetSelection( (int) 0 );
	m_pcContactCategoryText->Set( MSG_MAINWND_CATEGORY_NEWCATEGORY.c_str() );
	m_pcGenderDropdown->SetSelection( (int) 0 );
	m_pcNamePreFirstnameData->Clear();
	m_pcNameFirstnameData->Clear();
	m_pcNamePreMiddlenameData->Clear();
	m_pcNameMiddlenameData->Clear();
	m_pcNamePreLastnameData->Clear();
	m_pcNameLastnameData->Clear();
	m_pcNameNicknameData->Clear();
	m_pcNameCompanyData->Clear();
	m_pcPriAddressAddress1Data->Clear();
	m_pcPriAddressAddress2Data->Clear();
	m_pcPriAddressAddress3Data->Clear();
	m_pcPriAddressZCS1Data->Clear();
	m_pcPriAddressZCS2Data->Clear();
	m_pcPriAddressZCS3Data->Clear();
	m_pcPriAddressCountryData->Clear();
	m_pcSecAddressAddress1Data->Clear();
	m_pcSecAddressAddress2Data->Clear();
	m_pcSecAddressAddress3Data->Clear();
	m_pcSecAddressZCS1Data->Clear();
	m_pcSecAddressZCS2Data->Clear();
	m_pcSecAddressZCS3Data->Clear();
	m_pcSecAddressCountryData->Clear();
	m_pcInfoList->Clear();
	m_pcInfoContactTypeDropdown->SetSelection( (int) 0 );
	m_pcInfoContactTypeText->Set( MSG_MAINWND_INFO_TYPE_NEWTYPE.c_str() );
	m_pcInfoContactDataText->Clear();
	m_pcInfoContactOtherText->Set( MSG_MAINWND_INFO_WHERE_OTHER.c_str() );
	m_pcInfoContactHome->SetValue( true );
	m_pcNotesText->Clear();

	cContactName = os::String( "" );
}


void MainWindow::CheckChanges()
{
	bContactChanged = false;

	/* Load Contact-data */
	os::String zPath = getenv( "HOME" );
	zPath += String("/Contacts/") += os::String(cContactName);
	try
	{
		os::Settings* pcSettings = new os::Settings(new File(zPath));
		pcSettings->Load();

		/* Compare the data */
		if( m_pcPriAddressZCSDropdown->GetSelection() != pcSettings->GetInt8("PriZCSsystem",0) )
			bContactChanged = true;
		if( m_pcSecAddressZCSDropdown->GetSelection() != pcSettings->GetInt8("SecZCSsystem",0) )
			bContactChanged = true;
		if( m_pcGenderDropdown->GetSelection() != pcSettings->GetInt8("Gender",0) )
			bContactChanged = true;
		if( m_pcNamePreFirstnameData->GetValue() != pcSettings->GetString("PreFirstName","") )
			bContactChanged = true;
		if( m_pcNameFirstnameData->GetValue() != pcSettings->GetString("FirstName","") )
			bContactChanged = true;
		if( m_pcNamePreMiddlenameData->GetValue() != pcSettings->GetString("PreMiddleName","") )
			bContactChanged = true;
		if( m_pcNameMiddlenameData->GetValue() != pcSettings->GetString("MiddleName","") )
			bContactChanged = true;
		if( m_pcNamePreLastnameData->GetValue() != pcSettings->GetString("PreLastName","") )
			bContactChanged = true;
		if( m_pcNameLastnameData->GetValue() != pcSettings->GetString("LastName","") )
			bContactChanged = true;
		if( m_pcNameNicknameData->GetValue() != pcSettings->GetString("NickName","") )
			bContactChanged = true;
		if( m_pcNameCompanyData->GetValue() != pcSettings->GetString("Company","") )
			bContactChanged = true;
		if( m_pcPriAddressAddress1Data->GetValue() != pcSettings->GetString("PriAddressAddress1","") )
			bContactChanged = true;
		if( m_pcPriAddressAddress2Data->GetValue() != pcSettings->GetString("PriAddressAddress2","") )
			bContactChanged = true;
		if( m_pcPriAddressAddress3Data->GetValue() != pcSettings->GetString("PriAddressAddress3","") )
			bContactChanged = true;
		if( m_pcPriAddressZCSDropdown->GetSelection() == 1 ) {
			if( m_pcPriAddressZCS1Data->GetValue() != pcSettings->GetString("PriAddressZip","") )
				bContactChanged = true;
			if( m_pcPriAddressZCS2Data->GetValue() != pcSettings->GetString("PriAddressCity","") )
				bContactChanged = true;
			if( m_pcPriAddressZCS3Data->GetValue() != pcSettings->GetString("PriAddressState","") )
				bContactChanged = true;
		} else if( m_pcPriAddressZCSDropdown->GetSelection() == 2 ) {
			if( m_pcPriAddressZCS1Data->GetValue() != pcSettings->GetString("PriAddressZip","") )
				bContactChanged = true;
			if( m_pcPriAddressZCS2Data->GetValue() != pcSettings->GetString("PriAddressState","") )
				bContactChanged = true;
			if( m_pcPriAddressZCS3Data->GetValue() != pcSettings->GetString("PriAddressCity","") )
				bContactChanged = true;
		} else if( m_pcPriAddressZCSDropdown->GetSelection() == 3 ) {
			if( m_pcPriAddressZCS1Data->GetValue() != pcSettings->GetString("PriAddressCity","") )
				bContactChanged = true;
			if( m_pcPriAddressZCS2Data->GetValue() != pcSettings->GetString("PriAddressZip","") )
				bContactChanged = true;
			if( m_pcPriAddressZCS3Data->GetValue() != pcSettings->GetString("PriAddressState","") )
				bContactChanged = true;
		} else if( m_pcPriAddressZCSDropdown->GetSelection() == 4 ) {
			if( m_pcPriAddressZCS1Data->GetValue() != pcSettings->GetString("PriAddressCity","") )
				bContactChanged = true;
			if( m_pcPriAddressZCS2Data->GetValue() != pcSettings->GetString("PriAddressState","") )
				bContactChanged = true;
			if( m_pcPriAddressZCS3Data->GetValue() != pcSettings->GetString("PriAddressZip","") )
				bContactChanged = true;
		} else if( m_pcPriAddressZCSDropdown->GetSelection() == 5 ) {
			if( m_pcPriAddressZCS1Data->GetValue() != pcSettings->GetString("PriAddressState","") )
				bContactChanged = true;
			if( m_pcPriAddressZCS2Data->GetValue() != pcSettings->GetString("PriAddressZip","") )
				bContactChanged = true;
			if( m_pcPriAddressZCS3Data->GetValue() != pcSettings->GetString("PriAddressCity","") )
				bContactChanged = true;
		} else if( m_pcPriAddressZCSDropdown->GetSelection() == 6 ) {
			if( m_pcPriAddressZCS1Data->GetValue() != pcSettings->GetString("PriAddressState","") )
				bContactChanged = true;
			if( m_pcPriAddressZCS2Data->GetValue() != pcSettings->GetString("PriAddressCity","") )
				bContactChanged = true;
			if( m_pcPriAddressZCS3Data->GetValue() != pcSettings->GetString("PriAddressZip","") )
				bContactChanged = true;		}
		if( m_pcPriAddressCountryData->GetValue() != pcSettings->GetString("PriAddressCountry","") )
			bContactChanged = true;
		if( m_pcSecAddressAddress1Data->GetValue() != pcSettings->GetString("SecAddressAddress1","") )
			bContactChanged = true;
		if( m_pcSecAddressAddress2Data->GetValue() != pcSettings->GetString("SecAddressAddress2","") )
			bContactChanged = true;
		if( m_pcSecAddressAddress3Data->GetValue() != pcSettings->GetString("SecAddressAddress3","") )
			bContactChanged = true;
		if( m_pcSecAddressZCSDropdown->GetSelection() == 1 ) {
			if( m_pcSecAddressZCS1Data->GetValue() != pcSettings->GetString("SecAddressZip","") )
				bContactChanged = true;
			if( m_pcSecAddressZCS2Data->GetValue() != pcSettings->GetString("SecAddressCity","") )
				bContactChanged = true;
			if( m_pcSecAddressZCS3Data->GetValue() != pcSettings->GetString("SecAddressState","") )
				bContactChanged = true;
		} else if( m_pcSecAddressZCSDropdown->GetSelection() == 2 ) {
			if( m_pcSecAddressZCS1Data->GetValue() != pcSettings->GetString("SecAddressZip","") )
				bContactChanged = true;
			if( m_pcSecAddressZCS2Data->GetValue() != pcSettings->GetString("SecAddressState","") )
				bContactChanged = true;
			if( m_pcSecAddressZCS3Data->GetValue() != pcSettings->GetString("SecAddressCity","") )
				bContactChanged = true;
		} else if( m_pcSecAddressZCSDropdown->GetSelection() == 3 ) {
			if( m_pcSecAddressZCS1Data->GetValue() != pcSettings->GetString("SecAddressCity","") )
				bContactChanged = true;
			if( m_pcSecAddressZCS2Data->GetValue() != pcSettings->GetString("SecAddressZip","") )
				bContactChanged = true;
			if( m_pcSecAddressZCS3Data->GetValue() != pcSettings->GetString("SecAddressState","") )
				bContactChanged = true;
		} else if( m_pcSecAddressZCSDropdown->GetSelection() == 4 ) {
			if( m_pcSecAddressZCS1Data->GetValue() != pcSettings->GetString("SecAddressCity","") )
				bContactChanged = true;
			if( m_pcSecAddressZCS2Data->GetValue() != pcSettings->GetString("SecAddressState","") )
				bContactChanged = true;
			if( m_pcSecAddressZCS3Data->GetValue() != pcSettings->GetString("SecAddressZip","") )
				bContactChanged = true;
		} else if( m_pcSecAddressZCSDropdown->GetSelection() == 5 ) {
			if( m_pcSecAddressZCS1Data->GetValue() != pcSettings->GetString("SecAddressState","") )
				bContactChanged = true;
			if( m_pcSecAddressZCS2Data->GetValue() != pcSettings->GetString("SecAddressZip","") )
				bContactChanged = true;
			if( m_pcSecAddressZCS3Data->GetValue() != pcSettings->GetString("SecAddressCity","") )
				bContactChanged = true;
		} else if( m_pcSecAddressZCSDropdown->GetSelection() == 6 ) {
			if( m_pcSecAddressZCS1Data->GetValue() != pcSettings->GetString("SecAddressState","") )
				bContactChanged = true;
			if( m_pcSecAddressZCS2Data->GetValue() != pcSettings->GetString("SecAddressCity","") )
				bContactChanged = true;
			if( m_pcSecAddressZCS3Data->GetValue() != pcSettings->GetString("SecAddressZip","") )
				bContactChanged = true;		}
		if( m_pcSecAddressCountryData->GetValue() != pcSettings->GetString("SecAddressCountry","") )
			bContactChanged = true;
		if( m_pcNotesText->GetValue() != pcSettings->GetString("Notes","") )
			bContactChanged = true;

		/* Get Other Contact Info */
		if( bContactChanged == false )
		{
			int nCount = m_pcInfoList->GetRowCount();
			if( nCount > 0 )
			{
				int counter;
				for( counter = 0; counter <= 999 ; counter = counter + 1)
				{
					char cEntry[3];
					int iEntry = counter;
					sprintf( cEntry, "%i", iEntry );

					if( nCount > counter )
					{
						ListViewStringRow* pcRow = static_cast<os::ListViewStringRow*>( m_pcInfoList->GetRow( counter ) );

						if( pcSettings->GetString(cEntry,"",0) != pcRow->GetString(0) || pcSettings->GetString(cEntry,"",1) != pcRow->GetString(1) || pcSettings->GetString(cEntry,"",2) != pcRow->GetString(2) )
						{
							bContactChanged = true;
							counter = 1000;
						}
					}
					else if( pcSettings->GetString(cEntry,"",0) != "" )
					{
						bContactChanged = true;
						counter = 1000;
					}
				}
			}
			else if( pcSettings->GetString("0","",0) != "" )
			{
				bContactChanged = true;
			}
		}

		delete( pcSettings );
	}
	catch(...)
	{
	}

	/* Ask the user if the data should be saved */
	if( bContactChanged == true )
	{
		os::Alert* pcAlert = new Alert( MSG_UNSAVEDALERT_TITLE, MSG_UNSAVEDALERT_TEXT, Alert::ALERT_WARNING,0, MSG_UNSAVEDALERT_SAVE.c_str(),MSG_UNSAVEDALERT_CONTINUE.c_str(),MSG_UNSAVEDALERT_CANCEL.c_str(),NULL);
		os::Invoker* pcInvoker = new os::Invoker( new os::Message( M_CONTACT_CHANGED_ALERT ), this );
		pcAlert->Go( pcInvoker );
		pcAlert->MakeFocus();
	}
}


void MainWindow::SaveContact()
{
	/* Create Contact-name */
	if( cContactName == "" )
	{
		os::String cPreFirstName = m_pcNamePreFirstnameData->GetValue();
		if( cPreFirstName != "" )
			cContactName = cPreFirstName;

		os::String cFirstName = m_pcNameFirstnameData->GetValue();
		if( cFirstName != "" && cContactName != "" )
			cContactName += String(" ") += cFirstName;
		if( cFirstName != "" && cContactName == "" )
			cContactName = cFirstName;

		os::String cPreMiddleName = m_pcNamePreMiddlenameData->GetValue();
		if( cPreMiddleName != "" && cContactName != "" )
			cContactName += String(" ") += cPreMiddleName;
		if( cPreMiddleName != "" && cContactName == "" )
			cContactName = cPreMiddleName;

		os::String cMiddleName = m_pcNameMiddlenameData->GetValue();
		if( cMiddleName != "" && cContactName != "" )
			cContactName += String(" ") += cMiddleName;
		if( cMiddleName != "" && cContactName == "" )
			cContactName = cMiddleName;

		os::String cPreLastName = m_pcNamePreLastnameData->GetValue();
		if( cPreLastName != "" && cContactName != "" )
			cContactName += String(" ") += cPreLastName;
		if( cPreLastName != "" && cContactName == "" )
			cContactName = cPreLastName;

		os::String cLastName = m_pcNameLastnameData->GetValue();
		if( cLastName != "" && cContactName != "" )
			cContactName += String(" ") += cLastName;
		if( cLastName != "" && cContactName == "" )
			cContactName = cLastName;

		os::String cNickName = m_pcNameNicknameData->GetValue();
		if( cContactName == "" )
			cContactName = cNickName;

		os::String cCompany = m_pcNameCompanyData->GetValue();
		if( cContactName == "" )
			cContactName = cCompany;
	}

	/* Save Contact */
	os::String zPath = getenv( "HOME" );
	zPath += os::String("/Contacts");
	mkdir( zPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO );
	try
	{
		mkdir( zPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO );
		zPath += os::String("/") += String(cContactName);
		os::File cFile( zPath, O_RDWR | O_CREAT );	
		os::Settings* pcSettings = new os::Settings(new File(zPath));

		/* Set Name and Address Info */
		pcSettings->AddInt8("Version", 2 );
		pcSettings->AddInt8("PriZCSsystem", m_pcPriAddressZCSDropdown->GetSelection() );
		pcSettings->AddInt8("SecZCSsystem", m_pcSecAddressZCSDropdown->GetSelection() );
		pcSettings->AddInt8("Gender", m_pcGenderDropdown->GetSelection() );
		pcSettings->AddString("PreFirstName", m_pcNamePreFirstnameData->GetValue() );
		pcSettings->AddString("FirstName", m_pcNameFirstnameData->GetValue() );
		pcSettings->AddString("PreMiddleName", m_pcNamePreMiddlenameData->GetValue() );
		pcSettings->AddString("MiddleName", m_pcNameMiddlenameData->GetValue() );
		pcSettings->AddString("PreLastName", m_pcNamePreLastnameData->GetValue() );
		pcSettings->AddString("LastName", m_pcNameLastnameData->GetValue() );
		pcSettings->AddString("NickName", m_pcNameNicknameData->GetValue() );
		pcSettings->AddString("Company", m_pcNameCompanyData->GetValue() );
		pcSettings->AddString("PriAddressAddress1", m_pcPriAddressAddress1Data->GetValue() );
		pcSettings->AddString("PriAddressAddress2", m_pcPriAddressAddress2Data->GetValue() );
		pcSettings->AddString("PriAddressAddress3", m_pcPriAddressAddress3Data->GetValue() );
		if( m_pcPriAddressZCSDropdown->GetSelection() == 1 ) {
			pcSettings->AddString("PriAddressZip", m_pcPriAddressZCS1Data->GetValue() );
			pcSettings->AddString("PriAddressCity", m_pcPriAddressZCS2Data->GetValue() );
			pcSettings->AddString("PriAddressState", m_pcPriAddressZCS3Data->GetValue() );
		} else if( m_pcPriAddressZCSDropdown->GetSelection() == 2 ) {
			pcSettings->AddString("PriAddressZip", m_pcPriAddressZCS1Data->GetValue() );
			pcSettings->AddString("PriAddressCity", m_pcPriAddressZCS3Data->GetValue() );
			pcSettings->AddString("PriAddressState", m_pcPriAddressZCS2Data->GetValue() );
		} else if( m_pcPriAddressZCSDropdown->GetSelection() == 3 ) {
			pcSettings->AddString("PriAddressZip", m_pcPriAddressZCS2Data->GetValue() );
			pcSettings->AddString("PriAddressCity", m_pcPriAddressZCS1Data->GetValue() );
			pcSettings->AddString("PriAddressState", m_pcPriAddressZCS3Data->GetValue() );
		} else if( m_pcPriAddressZCSDropdown->GetSelection() == 4 ) {
			pcSettings->AddString("PriAddressZip", m_pcPriAddressZCS3Data->GetValue() );
			pcSettings->AddString("PriAddressCity", m_pcPriAddressZCS1Data->GetValue() );
			pcSettings->AddString("PriAddressState", m_pcPriAddressZCS2Data->GetValue() );
		} else if( m_pcPriAddressZCSDropdown->GetSelection() == 5 ) {
			pcSettings->AddString("PriAddressZip", m_pcPriAddressZCS2Data->GetValue() );
			pcSettings->AddString("PriAddressCity", m_pcPriAddressZCS3Data->GetValue() );
			pcSettings->AddString("PriAddressState", m_pcPriAddressZCS1Data->GetValue() );
		} else if( m_pcPriAddressZCSDropdown->GetSelection() == 6 ) {
			pcSettings->AddString("PriAddressZip", m_pcPriAddressZCS3Data->GetValue() );
			pcSettings->AddString("PriAddressCity", m_pcPriAddressZCS2Data->GetValue() );
			pcSettings->AddString("PriAddressState", m_pcPriAddressZCS1Data->GetValue() );	}
		pcSettings->AddString("PriAddressCountry", m_pcPriAddressCountryData->GetValue() );
		pcSettings->AddString("SecAddressAddress1", m_pcSecAddressAddress1Data->GetValue() );
		pcSettings->AddString("SecAddressAddress2", m_pcSecAddressAddress2Data->GetValue() );
		pcSettings->AddString("SecAddressAddress3", m_pcSecAddressAddress3Data->GetValue() );
		if( m_pcSecAddressZCSDropdown->GetSelection() == 1 ) {
			pcSettings->AddString("SecAddressZip", m_pcSecAddressZCS1Data->GetValue() );
			pcSettings->AddString("SecAddressCity", m_pcSecAddressZCS2Data->GetValue() );
			pcSettings->AddString("SecAddressState", m_pcSecAddressZCS3Data->GetValue() );
		} else if( m_pcSecAddressZCSDropdown->GetSelection() == 2 ) {
			pcSettings->AddString("SecAddressZip", m_pcSecAddressZCS1Data->GetValue() );
			pcSettings->AddString("SecAddressCity", m_pcSecAddressZCS3Data->GetValue() );
			pcSettings->AddString("SecAddressState", m_pcSecAddressZCS2Data->GetValue() );
		} else if( m_pcSecAddressZCSDropdown->GetSelection() == 3 ) {
			pcSettings->AddString("SecAddressZip", m_pcSecAddressZCS2Data->GetValue() );
			pcSettings->AddString("SecAddressCity", m_pcSecAddressZCS1Data->GetValue() );
			pcSettings->AddString("SecAddressState", m_pcSecAddressZCS3Data->GetValue() );
		} else if( m_pcSecAddressZCSDropdown->GetSelection() == 4 ) {
			pcSettings->AddString("SecAddressZip", m_pcSecAddressZCS3Data->GetValue() );
			pcSettings->AddString("SecAddressCity", m_pcSecAddressZCS1Data->GetValue() );
			pcSettings->AddString("SecAddressState", m_pcSecAddressZCS2Data->GetValue() );
		} else if( m_pcSecAddressZCSDropdown->GetSelection() == 5 ) {
			pcSettings->AddString("SecAddressZip", m_pcSecAddressZCS2Data->GetValue() );
			pcSettings->AddString("SecAddressCity", m_pcSecAddressZCS3Data->GetValue() );
			pcSettings->AddString("SecAddressState", m_pcSecAddressZCS1Data->GetValue() );
		} else if( m_pcSecAddressZCSDropdown->GetSelection() == 6 ) {
			pcSettings->AddString("SecAddressZip", m_pcSecAddressZCS3Data->GetValue() );
			pcSettings->AddString("SecAddressCity", m_pcSecAddressZCS2Data->GetValue() );
			pcSettings->AddString("SecAddressState", m_pcSecAddressZCS1Data->GetValue() );	}
		pcSettings->AddString("SecAddressCountry", m_pcSecAddressCountryData->GetValue() );
		pcSettings->AddString("Notes", m_pcNotesText->GetValue() );

		/* Set Other Contact Info */
		int iCount = m_pcInfoList->GetRowCount();
		if( iCount > 0 )
		{
			int counter;
			for( counter = 0; counter <= iCount - 1; counter = counter + 1)
			{
				char cEntry[3];
				int iEntry = counter;
				sprintf( cEntry, "%i", iEntry );

				ListViewStringRow* pcRow = static_cast<os::ListViewStringRow*>( m_pcInfoList->GetRow( counter ) );
				os::String cTempType = pcRow->GetString(0);
				os::String cTempHWO = pcRow->GetString(1);
				os::String cTempData = pcRow->GetString(2);

				/* Get un-localized names */
				if( cTempHWO == MSG_MAINWND_INFO_WHERE_HOME )
					cTempHWO = os::String( "Home" );
				else if( cTempHWO == MSG_MAINWND_INFO_WHERE_WORK )
					cTempHWO = os::String( "Work" );
				if( cTempType == MSG_MAINWND_INFO_TYPE_PHONE )
					cTempType = os::String( "Phone" );
				else if( cTempType == MSG_MAINWND_INFO_TYPE_CELLPHONE )
					cTempType = os::String( "Cell Phone" );
				else if( cTempType == MSG_MAINWND_INFO_TYPE_FAX )
					cTempType = os::String( "Fax" );
				else if( cTempType == MSG_MAINWND_INFO_TYPE_PAGER )
					cTempType = os::String( "Pager" );
				else if( cTempType == MSG_MAINWND_INFO_TYPE_EMAIL )
					cTempType = os::String( "E-mail" );
				else if( cTempType == MSG_MAINWND_INFO_TYPE_ICQ )
					cTempType = os::String( "ICQ" );
				else if( cTempType == MSG_MAINWND_INFO_TYPE_MSN )
					cTempType = os::String( "MSN" );
				else if( cTempType == MSG_MAINWND_INFO_TYPE_JABBER )
					cTempType = os::String( "Jabber" );
				else if( cTempType == MSG_MAINWND_INFO_TYPE_YAHOO )
					cTempType = os::String( "Yahoo" );
				else if( cTempType == MSG_MAINWND_INFO_TYPE_AOL )
					cTempType = os::String( "AOL" );
				else if( cTempType == MSG_MAINWND_INFO_TYPE_SKYPE )
					cTempType = os::String( "Skype" );
				else if( cTempType == MSG_MAINWND_INFO_TYPE_WEBSITE )
					cTempType = os::String( "Website" );

				pcSettings->SetString(cEntry, cTempType, 0 );
				pcSettings->SetString(cEntry, cTempHWO, 1 );
				pcSettings->SetString(cEntry, cTempData, 2 );
			}
		}

		pcSettings->Save();
		delete( pcSettings );
	}
	catch(...)
	{
	}

	/* Get Contact-category */
	os::String cTempCategories;
	int nCategoryCount = m_pcCategoryList->GetRowCount();
	if( nCategoryCount > 0 )
	{
		int counter;
		for( counter = 0; counter <= nCategoryCount -1 ; counter = counter + 1)
		{
			ListViewStringRow* pcRow = static_cast<os::ListViewStringRow*>( m_pcCategoryList->GetRow( counter ) );
			if( counter == 0 )
			{
				if( pcRow->GetString( 0 ) == MSG_MAINWND_CATEGORY_FAMILY )
					cTempCategories = os::String( "Family" );
				else if( pcRow->GetString( 0 ) == MSG_MAINWND_CATEGORY_FRIENDS )
					cTempCategories = os::String( "Friends" );
				else if( pcRow->GetString( 0 ) == MSG_MAINWND_CATEGORY_COLLEAGUES )
					cTempCategories = os::String( "Colleagues" );
				else if( pcRow->GetString( 0 ) == MSG_MAINWND_CATEGORY_WORK )
					cTempCategories = os::String( "Work" );
				else if( pcRow->GetString( 0 ) == MSG_MAINWND_CATEGORY_MAILINGLISTS )
					cTempCategories = os::String( "Mailinglists" );
				else
					cTempCategories = pcRow->GetString( 0 );
			}
			else
			{
				os::String cTempCat;
				if( pcRow->GetString( 0 ) == MSG_MAINWND_CATEGORY_FAMILY )
					cTempCat = os::String( "Family" );
				else if( pcRow->GetString( 0 ) == MSG_MAINWND_CATEGORY_FRIENDS )
					cTempCat = os::String( "Friends" );
				else if( pcRow->GetString( 0 ) == MSG_MAINWND_CATEGORY_COLLEAGUES )
					cTempCat = os::String( "Colleagues" );
				else if( pcRow->GetString( 0 ) == MSG_MAINWND_CATEGORY_WORK )
					cTempCat = os::String( "Work" );
				else if( pcRow->GetString( 0 ) == MSG_MAINWND_CATEGORY_MAILINGLISTS )
					cTempCat = os::String( "Mailinglists" );
				else
					cTempCat = pcRow->GetString( 0 );

				cTempCategories = cTempCategories + os::String( ";;" ) + cTempCat;
			}
		}
	}

	/* Write the mimetype and categories to the Contact-file */
	int nFile = open( zPath.c_str(), O_RDWR | O_NOTRAVERSE );
	write_attr( nFile, "os::MimeType", O_TRUNC, ATTR_TYPE_STRING, "application/x-contact", 0, strlen( "application/x-contact" ) );
	write_attr( nFile, "Contact::Category", O_TRUNC, ATTR_TYPE_STRING, cTempCategories.c_str(), 0, strlen( cTempCategories.c_str() ) );
  	close( nFile );

	/* We just saved it, so it hasn't changed... */
	bContactChanged = false;
}


void MainWindow::LoadContact()
{
	/* Select Contact */
	int nSelected;
	if( bLoadContact == true )
	{
		nSelected = m_pcContactList->GetLastSelected();
		if( nSelected >= 0 )
		{
			ListViewStringRow* pcRow = static_cast<os::ListViewStringRow*>( m_pcContactList->GetRow( nSelected ) );
			cContactName = pcRow->GetString(0);
		}
	}
	else if( bLoadSearch == true )
	{
		nSelected = m_pcContactList->GetLastSelected();
		if( nSelected >= 0 )
		{
			ListViewStringRow* pcRow = static_cast<os::ListViewStringRow*>( m_pcContactList->GetRow( nSelected ) );
			cContactName = pcRow->GetString(0);
		}
	}

	/* Load Contact-data */
	os::String zPath = getenv( "HOME" );
	zPath += os::String( "/Contacts/" ) += os::String( cContactName );
	try
	{
		/* If we're loading a Contact at startup, we want a new zPath */
		if( bLoadStartup == true )
		{
			zPath = cContactPath;
		}

		os::Settings* pcSettings = new os::Settings( new File( zPath ) );
		pcSettings->Load();

		/* Get and Set Categories */
		char delims[] = ";;";
		char *result = NULL;
		os::FSNode cFileNode;
		cFileNode.SetTo( zPath );
		char zBuffer[PATH_MAX];
		memset( zBuffer, 0, PATH_MAX );
		cFileNode.ReadAttr( "Contact::Category", ATTR_TYPE_STRING, zBuffer, 0, PATH_MAX );
		result = strtok( zBuffer, delims );
		while( result != NULL )
		{
			ListViewStringRow *row = new ListViewStringRow();

			/* Get localized category-names */
			os::String cResult = result;
			if( cResult == "Family" )
				row->AppendString( MSG_MAINWND_CATEGORY_FAMILY );
			else if( cResult == "Friends" )
				row->AppendString( MSG_MAINWND_CATEGORY_FRIENDS );
			else if( cResult == "Colleagues" )
				row->AppendString( MSG_MAINWND_CATEGORY_COLLEAGUES );
			else if( cResult == "Work" )
				row->AppendString( MSG_MAINWND_CATEGORY_WORK );
			else if( cResult == "Mailinglists" )
				row->AppendString( MSG_MAINWND_CATEGORY_MAILINGLISTS );
			else
				row->AppendString( cResult );

			m_pcCategoryList->InsertRow( row );
			result = strtok( NULL, delims );
		}

		/* Get the ZCS system */
		m_pcPriAddressZCSDropdown->SetSelection( (int) pcSettings->GetInt8("PriZCSsystem",0) );
		m_pcSecAddressZCSDropdown->SetSelection( (int) pcSettings->GetInt8("SecZCSsystem",0) );

		/* Get Name and Address Info */
		m_pcGenderDropdown->SetSelection( (int) pcSettings->GetInt8("Gender",4) );
		m_pcNamePreFirstnameData->SetValue( pcSettings->GetString("PreFirstName","") );
		m_pcNameFirstnameData->SetValue( pcSettings->GetString("FirstName","") );
		m_pcNamePreMiddlenameData->SetValue( pcSettings->GetString("PreMiddleName","") );
		m_pcNameMiddlenameData->SetValue( pcSettings->GetString("MiddleName","") );
		m_pcNamePreLastnameData->SetValue( pcSettings->GetString("PreLastName","") );
		m_pcNameLastnameData->SetValue( pcSettings->GetString("LastName","") );
		m_pcNameNicknameData->SetValue( pcSettings->GetString("NickName","") );
		m_pcNameCompanyData->SetValue( pcSettings->GetString("Company","") );
		m_pcPriAddressAddress1Data->SetValue( pcSettings->GetString("PriAddressAddress1","") );
		m_pcPriAddressAddress2Data->SetValue( pcSettings->GetString("PriAddressAddress2","") );
		m_pcPriAddressAddress3Data->SetValue( pcSettings->GetString("PriAddressAddress3","") );
		if( m_pcPriAddressZCSDropdown->GetSelection() <= 1 ) {
			m_pcPriAddressZCS1Data->SetValue( pcSettings->GetString("PriAddressZip", "" ) );
			m_pcPriAddressZCS2Data->SetValue( pcSettings->GetString("PriAddressCity", "" ) );
			m_pcPriAddressZCS3Data->SetValue( pcSettings->GetString("PriAddressState", "" ) );
		} else if( m_pcPriAddressZCSDropdown->GetSelection() == 2 ) {
			m_pcPriAddressZCS1Data->SetValue( pcSettings->GetString("PriAddressZip", "" ) );
			m_pcPriAddressZCS3Data->SetValue( pcSettings->GetString("PriAddressCity", "" ) );
			m_pcPriAddressZCS2Data->SetValue( pcSettings->GetString("PriAddressState", "" ) );
		} else if( m_pcPriAddressZCSDropdown->GetSelection() == 3 ) {
			m_pcPriAddressZCS2Data->SetValue( pcSettings->GetString("PriAddressZip", "" ) );
			m_pcPriAddressZCS1Data->SetValue( pcSettings->GetString("PriAddressCity", "" ) );
			m_pcPriAddressZCS3Data->SetValue( pcSettings->GetString("PriAddressState", "" ) );
		} else if( m_pcPriAddressZCSDropdown->GetSelection() == 4 ) {
			m_pcPriAddressZCS3Data->SetValue( pcSettings->GetString("PriAddressZip", "" ) );
			m_pcPriAddressZCS1Data->SetValue( pcSettings->GetString("PriAddressCity", "" ) );
			m_pcPriAddressZCS2Data->SetValue( pcSettings->GetString("PriAddressState", "" ) );
		} else if( m_pcPriAddressZCSDropdown->GetSelection() == 5 ) {
			m_pcPriAddressZCS2Data->SetValue( pcSettings->GetString("PriAddressZip", "" ) );
			m_pcPriAddressZCS3Data->SetValue( pcSettings->GetString("PriAddressCity", "" ) );
			m_pcPriAddressZCS1Data->SetValue( pcSettings->GetString("PriAddressState", "" ) );
		} else if( m_pcPriAddressZCSDropdown->GetSelection() == 6 ) {
			m_pcPriAddressZCS3Data->SetValue( pcSettings->GetString("PriAddressZip", "" ) );
			m_pcPriAddressZCS2Data->SetValue( pcSettings->GetString("PriAddressCity", "" ) );
			m_pcPriAddressZCS1Data->SetValue( pcSettings->GetString("PriAddressState", "" ) );	}
		m_pcPriAddressCountryData->SetValue( pcSettings->GetString("PriAddressCountry","") );
		m_pcSecAddressAddress1Data->SetValue( pcSettings->GetString("SecAddressAddress1","") );
		m_pcSecAddressAddress2Data->SetValue( pcSettings->GetString("SecAddressAddress2","") );
		m_pcSecAddressAddress3Data->SetValue( pcSettings->GetString("SecAddressAddress3","") );
		if( m_pcSecAddressZCSDropdown->GetSelection() <= 1 ) {
			m_pcSecAddressZCS1Data->SetValue( pcSettings->GetString("SecAddressZip", "" ) );
			m_pcSecAddressZCS2Data->SetValue( pcSettings->GetString("SecAddressCity", "" ) );
			m_pcSecAddressZCS3Data->SetValue( pcSettings->GetString("SecAddressState", "" ) );
		} else if( m_pcSecAddressZCSDropdown->GetSelection() == 2 ) {
			m_pcSecAddressZCS1Data->SetValue( pcSettings->GetString("SecAddressZip", "" ) );
			m_pcSecAddressZCS3Data->SetValue( pcSettings->GetString("SecAddressCity", "" ) );
			m_pcSecAddressZCS2Data->SetValue( pcSettings->GetString("SecAddressState", "" ) );
		} else if( m_pcSecAddressZCSDropdown->GetSelection() == 3 ) {
			m_pcSecAddressZCS2Data->SetValue( pcSettings->GetString("SecAddressZip", "" ) );
			m_pcSecAddressZCS1Data->SetValue( pcSettings->GetString("SecAddressCity", "" ) );
			m_pcSecAddressZCS3Data->SetValue( pcSettings->GetString("SecAddressState", "" ) );
		} else if( m_pcSecAddressZCSDropdown->GetSelection() == 4 ) {
			m_pcSecAddressZCS3Data->SetValue( pcSettings->GetString("SecAddressZip", "" ) );
			m_pcSecAddressZCS1Data->SetValue( pcSettings->GetString("SecAddressCity", "" ) );
			m_pcSecAddressZCS2Data->SetValue( pcSettings->GetString("SecAddressState", "" ) );
		} else if( m_pcSecAddressZCSDropdown->GetSelection() == 5 ) {
			m_pcSecAddressZCS2Data->SetValue( pcSettings->GetString("SecAddressZip", "" ) );
			m_pcSecAddressZCS3Data->SetValue( pcSettings->GetString("SecAddressCity", "" ) );
			m_pcSecAddressZCS1Data->SetValue( pcSettings->GetString("SecAddressState", "" ) );
		} else if( m_pcSecAddressZCSDropdown->GetSelection() == 6 ) {
			m_pcSecAddressZCS3Data->SetValue( pcSettings->GetString("SecAddressZip", "" ) );
			m_pcSecAddressZCS2Data->SetValue( pcSettings->GetString("SecAddressCity", "" ) );
			m_pcSecAddressZCS1Data->SetValue( pcSettings->GetString("SecAddressState", "" ) );	}
		m_pcSecAddressCountryData->SetValue( pcSettings->GetString("SecAddressCountry","") );
		m_pcNotesText->SetValue( pcSettings->GetString("Notes","") );
		/* V1 data - start */
		if( pcSettings->GetString("PriAddressStreet","") != "" )
		{
			m_pcPriAddressAddress1Data->SetValue( pcSettings->GetString("PriAddressStreet","") );
			m_pcPriAddressZCSDropdown->SetSelection(1);
		}
		if( pcSettings->GetString("SecAddressStreet","") != "" )
		{
			m_pcSecAddressAddress1Data->SetValue( pcSettings->GetString("SecAddressStreet","") );
			m_pcSecAddressZCSDropdown->SetSelection(1);
		}
		/* V1 data - end */

		/* Get Other Contact Info */
		int counter;
		for( counter = 0; counter <= 999 ; counter = counter + 1)
		{
			char cEntry[3];
			int iEntry = counter;
			sprintf( cEntry, "%i", iEntry );

			os::String cTempType = pcSettings->GetString(cEntry,"",0);
			os::String cTempHWO = pcSettings->GetString(cEntry,"",1);
			os::String cTempData = pcSettings->GetString(cEntry,"",2);

			/* Get localized names */
			if( cTempHWO == "Home" )
				cTempHWO = MSG_MAINWND_INFO_WHERE_HOME;
			else if( cTempHWO == "Work" )
				cTempHWO = MSG_MAINWND_INFO_WHERE_WORK;
			if( cTempType == "Phone" )
				cTempType = MSG_MAINWND_INFO_TYPE_PHONE;
			else if( cTempType == "Cell Phone" )
				cTempType = MSG_MAINWND_INFO_TYPE_CELLPHONE;
			else if( cTempType == "Fax" )
				cTempType = MSG_MAINWND_INFO_TYPE_FAX;
			else if( cTempType == "Pager" )
				cTempType = MSG_MAINWND_INFO_TYPE_PAGER;
			else if( cTempType == "E-mail" )
				cTempType = MSG_MAINWND_INFO_TYPE_EMAIL;
			else if( cTempType == "ICQ" )
				cTempType = MSG_MAINWND_INFO_TYPE_ICQ;
			else if( cTempType == "MSN" )
				cTempType = MSG_MAINWND_INFO_TYPE_MSN;
			else if( cTempType == "Jabber" )
				cTempType = MSG_MAINWND_INFO_TYPE_JABBER;
			else if( cTempType == "Yahoo" )
				cTempType = MSG_MAINWND_INFO_TYPE_YAHOO;
			else if( cTempType == "AOL" )
				cTempType = MSG_MAINWND_INFO_TYPE_AOL;
			else if( cTempType == "Skype" )
				cTempType = MSG_MAINWND_INFO_TYPE_SKYPE;
			else if( cTempType == "Website" )
				cTempType = MSG_MAINWND_INFO_TYPE_WEBSITE;

			if( cTempType != "" )
			{
				ListViewStringRow *row = new ListViewStringRow();
				row->AppendString( cTempType );
				row->AppendString( cTempHWO );
				row->AppendString( cTempData );
				m_pcInfoList->InsertRow( row );
			}
			else
			{
				counter = 1000 ;
			}
		}

		delete( pcSettings );
	}
	catch(...)
	{
	}
}


void MainWindow::LoadCategoryList()
{
	/* Clear Category-dropdown */
	m_pcContactCategory->Clear();
	m_pcContactCategory->AppendItem( MSG_MAINWND_CATEGORY_ALLCATEGORIES );
	m_pcContactCategoryDropdown->Clear();
	m_pcContactCategoryDropdown->AppendItem( MSG_MAINWND_CATEGORY_CHOOSECATEGORY );
	m_pcContactCategoryDropdown->AppendItem( MSG_MAINWND_CATEGORY_NEWCATEGORY );
	m_pcContactCategoryDropdown->AppendItem( MSG_MAINWND_CATEGORY_FAMILY );
	m_pcContactCategoryDropdown->AppendItem( MSG_MAINWND_CATEGORY_FRIENDS );
	m_pcContactCategoryDropdown->AppendItem( MSG_MAINWND_CATEGORY_COLLEAGUES );
	m_pcContactCategoryDropdown->AppendItem( MSG_MAINWND_CATEGORY_WORK );
	m_pcContactCategoryDropdown->AppendItem( MSG_MAINWND_CATEGORY_MAILINGLISTS );
	m_pcContactCategoryDropdown->SetSelection( (int) 0 );

	/* Set Category-dir. */
	os::String cCatPath = getenv( "HOME" );
	cCatPath += os::String( "/Contacts" );

	/* Open up category directory and check it actually contains something */
	pDir = opendir( cCatPath.c_str() );
	if (pDir == NULL)
	{
		return;
	}

	int i = 0;
	while ( (psEntry = readdir( pDir )) != NULL)
	{
		/* If special directory (i.e. dot(.) and dotdot(..) then ignore */
		if (strcmp( psEntry->d_name, ".") == 0 || strcmp( psEntry->d_name, ".." ) == 0)
		{
			continue;
		}
		else
		{
			os::String cTempCatPath = cCatPath + os::String( "/" ) + psEntry->d_name;
			try
			{
				os::Settings* pcSettings = new os::Settings(new File( cTempCatPath ));
				pcSettings->Load();
				if( pcSettings->GetInt8("Version",0) != 10 )
				{
					/* Get the Categories */
					char delims[] = ";;";
					char *result = NULL;
					os::FSNode cFileNode;
					cFileNode.SetTo( cTempCatPath );
					char zBuffer[PATH_MAX];
					memset( zBuffer, 0, PATH_MAX );
					cFileNode.ReadAttr( "Contact::Category", ATTR_TYPE_STRING, zBuffer, 0, PATH_MAX );
					result = strtok( zBuffer, delims );
					while( result != NULL )
					{
						bool bDuplicateCS = false;
						bool bDuplicateC = false;

						/* Get localized category-names */
						os::String cResult = result;
						if( cResult == "Family" )
							cResult = MSG_MAINWND_CATEGORY_FAMILY;
						else if( cResult == "Friends" )
							cResult = MSG_MAINWND_CATEGORY_FRIENDS;
						else if( cResult == "Colleagues" )
							cResult = MSG_MAINWND_CATEGORY_COLLEAGUES;
						else if( cResult == "Work" )
							cResult = MSG_MAINWND_CATEGORY_WORK;
						else if( cResult == "Mailinglists" )
							cResult = MSG_MAINWND_CATEGORY_MAILINGLISTS;

						/* Check if it's a duplicate - Contact and Search Dropdownmenus */
						int nCountCS = m_pcContactCategory->GetItemCount();
						if( nCountCS > 0 )
						{
							for( int counter = 0; counter < nCountCS ; counter++ )
							{
								if( cResult == m_pcContactCategory->GetItem(counter) )
								{
									bDuplicateCS = true;
								}
							}
						}

						/* Check if it's a duplicate - UserCategories Dropdownmenu */
						int nCountC = m_pcContactCategoryDropdown->GetItemCount();
						if( nCountC > 0 )
						{
							for( int counter = 0; counter < nCountC ; counter++ )
							{
								if( cResult == m_pcContactCategoryDropdown->GetItem(counter) )
								{
									bDuplicateC = true;
								}
							}
						}

						/* Add it if it's not already there */
						if( bDuplicateCS == false )
						{
							m_pcContactCategory->AppendItem( cResult );
						}
						if( bDuplicateC == false )
						{
							m_pcContactCategoryDropdown->AppendItem( cResult );
						}

						result = strtok( NULL, delims );
					}
				}
				delete( pcSettings );
			}
			catch(...)
			{
			}
		}
		++i;
	}
	closedir( pDir );
}


void MainWindow::LoadContactList()
{
	m_pcContactList->Clear();

	/* Find out what category we want to work with */
	int nCategory = m_pcContactCategory->GetSelection();
	cCategory = os::String( m_pcContactCategory->GetItem(nCategory) );

	/* Set Category-dir. */
	os::String cCatPath = getenv( "HOME" );
	cCatPath += os::String( "/Contacts" );

	/* Open up category directory and check it actually contains something */
	pDir = opendir( cCatPath.c_str() );
	if (pDir == NULL)
	{
		return;
	}

	int i = 0;
	while( ( psEntry = readdir( pDir ) ) != NULL )
	{
		/* If special directory (i.e. dot(.) and dotdot(..) then ignore */
		if (strcmp( psEntry->d_name, ".") == 0 || strcmp( psEntry->d_name, ".." ) == 0)
		{
			continue;
		}
		else
		{
			bool bAddContact = false;
			os::String cTempCatPath = cCatPath + os::String( "/" ) + psEntry->d_name;
			try
			{
				os::Settings* pcSettings = new os::Settings(new File( cTempCatPath ));
				pcSettings->Load();
				if( pcSettings->GetInt8("Version",0) == 1 || pcSettings->GetInt8("Version",0) == 2 )
				{
					/* Get the Categories */
					char delims[] = ";;";
					char *result = NULL;
					os::FSNode cFileNode;
					cFileNode.SetTo( cTempCatPath );
					char zBuffer[PATH_MAX];
					memset( zBuffer, 0, PATH_MAX );
					cFileNode.ReadAttr( "Contact::Category", ATTR_TYPE_STRING, zBuffer, 0, PATH_MAX );
					result = strtok( zBuffer, delims );
					while( result != NULL )
					{
						os::String cResult = result;
						if( cResult == "Family" )
							cResult = MSG_MAINWND_CATEGORY_FAMILY;
						else if( cResult == "Friends" )
							cResult = MSG_MAINWND_CATEGORY_FRIENDS;
						else if( cResult == "Colleagues" )
							cResult = MSG_MAINWND_CATEGORY_COLLEAGUES;
						else if( cResult == "Work" )
							cResult = MSG_MAINWND_CATEGORY_WORK;
						else if( cResult == "Mailinglists" )
							cResult = MSG_MAINWND_CATEGORY_MAILINGLISTS;

						if( cCategory == cResult || nCategory == 0 )
						{
							bAddContact = true;
						}

						result = strtok( NULL, delims );
					}
				}
				delete( pcSettings );
			}
			catch(...)
			{
			}

			/* If it's valid, add it to the listview */
			if( bAddContact == true )
			{
				os::ListViewStringRow* pcRow = new os::ListViewStringRow();
				pcRow->AppendString( psEntry->d_name );
				m_pcContactList->InsertRow( pcRow );
			}
		}
		++i;
	}
	closedir( pDir );
}


void MainWindow::SaveSettings()
{
	os::String zPath = getenv( "HOME" );
	zPath += os::String("/Settings/Contact");
	mkdir( zPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO );
	try
	{
		mkdir( zPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO );
		zPath += os::String("/Settings");
		os::File cFile( zPath, O_RDWR | O_CREAT );	
		os::Settings* pcSettings = new os::Settings(new File(zPath));
		int nTempCategory = m_pcContactCategory->GetSelection();
		os::String cTempCategory = os::String( m_pcContactCategory->GetItem( nTempCategory ) );

		if( cTempCategory == MSG_MAINWND_CATEGORY_FAMILY )
			cTempCategory = os::String( "Family" );
		else if( cTempCategory == MSG_MAINWND_CATEGORY_FRIENDS )
			cTempCategory = os::String( "Friends" );
		else if( cTempCategory == MSG_MAINWND_CATEGORY_COLLEAGUES )
			cTempCategory = os::String( "Colleagues" );
		else if( cTempCategory == MSG_MAINWND_CATEGORY_WORK )
			cTempCategory = os::String( "Work" );
		else if( cTempCategory == MSG_MAINWND_CATEGORY_MAILINGLISTS )
			cTempCategory = os::String( "Mailinglists" );

		pcSettings->AddString("DefaultCategory", cTempCategory );
		pcSettings->Save();
		delete( pcSettings );
	}
	catch(...)
	{
	}
}


void MainWindow::LoadSettings()
{
	m_pcContactCategory->SetSelection( (int) 0 );
	os::String zPath = getenv( "HOME" );
	zPath += os::String( "/Settings/Contact/Settings" );
	try
	{
		os::Settings* pcSettings = new os::Settings( new File( zPath ) );
		pcSettings->Load();
		os::String cDefauleCategory = pcSettings->GetString("DefaultCategory","");
		for( int i = 0 ; i < m_pcContactCategory->GetItemCount() ; i++ )
		{
			if( cDefauleCategory == "Family" )
				cDefauleCategory = MSG_MAINWND_CATEGORY_FAMILY;
			else if( cDefauleCategory == "Friends" )
				cDefauleCategory = MSG_MAINWND_CATEGORY_FRIENDS;
			else if( cDefauleCategory == "Colleagues" )
				cDefauleCategory = MSG_MAINWND_CATEGORY_COLLEAGUES;
			else if( cDefauleCategory == "Work" )
				cDefauleCategory = MSG_MAINWND_CATEGORY_WORK;
			else if( cDefauleCategory == "Mailinglists" )
				cDefauleCategory = MSG_MAINWND_CATEGORY_MAILINGLISTS;

			if( m_pcContactCategory->GetItem( i ) == cDefauleCategory )
			{
				m_pcContactCategory->SetSelection( (int) i );
				i = 9999;
			}
		}
		delete( pcSettings );
	}
	catch(...)
	{
	}
}


bool MainWindow::OkToQuit()
{
	SaveSettings();
	delete( m_pcMonitor );
	os::Application::GetInstance()->PostMessage( os::M_QUIT );
	return( true );
}

