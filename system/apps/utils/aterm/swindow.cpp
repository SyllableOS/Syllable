
#include <stdio.h>
#include "swindow.h"
#include "messages.h"
#include <util/application.h>
#include <util/message.h>


/**
 * Show settings window
 *
 * The window will show the provided \a cSettings values.
 * If user choose to save changes, \a cSettings will be updated to reflect changes.
 *
 */
SettingsWindow::SettingsWindow( const Rect & cFrame,
    TermSettings * pcSettings, const String &cName, const String &cTitle,
    uint32 nFlags )
:Window( cFrame, cName, cTitle, nFlags )
{
    m_pcDestSettings = pcSettings;
    /* Make a local copy of setings */
    m_cTempSettings = *pcSettings;

    /*
     * a first table : 1 col, 3 rows :
     *    - first row contains colors table
     *    - second row contains ibeam check box
     *    - third row contains buttons
     */
    m_pcLayoutTable =
        new TableView( Rect( 0, 0, 1, 1 ), "LayoutTable", "", 1, 4,
        CF_FOLLOW_LEFT | CF_FOLLOW_TOP );

    /*
     * The colors table
     */
    m_pcColorsTable =
        new TableView( Rect( 0, 0, 1, 1 ), "ColorsTable", "", 3, 3,
        CF_FOLLOW_LEFT | CF_FOLLOW_TOP );

    m_pcNormalLabel =
        new StringView( Rect( 0, 0, 1, 1 ), "NormalLabel", "Normal text" );
    m_pcSelectedLabel =
        new StringView( Rect( 0, 0, 1, 1 ), "SelectedLabel",
        "Selected text" );
    m_pcTextLabel = new StringView( Rect( 0, 0, 1, 1 ), "TextLabel", "Text" );
    m_pcBackgroundLabel =
        new StringView( Rect( 0, 0, 1, 1 ), "BackgroundLabel", "Background" );
    m_pcNormalLabel->ResizeTo( m_pcNormalLabel->GetPreferredSize( false ) );
    m_pcSelectedLabel->ResizeTo( m_pcSelectedLabel->
        GetPreferredSize( false ) );
    m_pcTextLabel->ResizeTo( m_pcTextLabel->GetPreferredSize( false ) );
    m_pcBackgroundLabel->ResizeTo( m_pcBackgroundLabel->
        GetPreferredSize( false ) );

    m_pcNormalTextList =
        new DropdownMenu( Rect( 0, 0, 1, 1 ), "NormalTextList",
        CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
    m_pcNormalBackgroundList =
        new DropdownMenu( Rect( 0, 0, 1, 1 ), "NormalBackgroundList",
        CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
    m_pcSelectedTextList =
        new DropdownMenu( Rect( 0, 0, 1, 1 ), "SelectedTextList",
        CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
    m_pcSelectedBackgroundList =
        new DropdownMenu( Rect( 0, 0, 1, 1 ), "SelectedBackgroundList",
        CF_FOLLOW_LEFT | CF_FOLLOW_TOP );

    m_pcNormalTextList->SetReadOnly( true );
    m_pcNormalBackgroundList->SetReadOnly( true );
    m_pcSelectedTextList->SetReadOnly( true );
    m_pcSelectedBackgroundList->SetReadOnly( true );

    for( int i = 0; i < 16; i++ ) {
        m_pcNormalTextList->AppendItem( Colors[i].name );
        m_pcNormalBackgroundList->AppendItem( Colors[i].name );
        m_pcSelectedTextList->AppendItem( Colors[i].name );
        m_pcSelectedBackgroundList->AppendItem( Colors[i].name );
    }

    m_pcNormalTextList->ResizeTo( m_pcNormalTextList->
        GetPreferredSize( false ) );
    m_pcNormalBackgroundList->ResizeTo( m_pcNormalBackgroundList->
        GetPreferredSize( false ) );
    m_pcSelectedTextList->ResizeTo( m_pcSelectedTextList->
        GetPreferredSize( false ) );
    m_pcSelectedBackgroundList->ResizeTo( m_pcSelectedBackgroundList->
        GetPreferredSize( false ) );

    m_pcColorsTable->SetChild( m_pcNormalLabel, 0, 1 );
    m_pcColorsTable->SetChild( m_pcSelectedLabel, 0, 2 );
    m_pcColorsTable->SetChild( m_pcTextLabel, 1, 0 );
    m_pcColorsTable->SetChild( m_pcBackgroundLabel, 2, 0 );
    m_pcColorsTable->SetChild( m_pcNormalTextList, 1, 1 );
    m_pcColorsTable->SetChild( m_pcNormalBackgroundList, 2, 1 );
    m_pcColorsTable->SetChild( m_pcSelectedTextList, 1, 2 );
    m_pcColorsTable->SetChild( m_pcSelectedBackgroundList, 2, 2 );

    //m_pcColorsTable->ResizeTo( m_pcColorsTable->GetPreferredSize( false ) );

    /*
     * The ibeam checkbox
     */
    m_pcIBeamCheck =
        new CheckBox( Rect( 0, 0, 1, 1 ), "IBeamCheck",
        "White halo around the I-beam cursor", NULL,
        CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
    m_pcIBeamCheck->ResizeTo( m_pcIBeamCheck->GetPreferredSize( false ) );

    /*
     * The buttons
     */
    m_pcButtonsTable =
        new TableView( Rect( 0, 0, 1, 1 ), "ButtonsTable", "", 3, 2,
        CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
    m_pcApplyButton =
        new Button( Rect( 0, 0, 1, 1 ), "ApplyButton", "Apply and close",
        new Message( M_APPLY ), CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
    m_pcSaveButton =
        new Button( Rect( 0, 0, 1, 1 ), "SaveButton",
        "Apply, Save as defaults and close", new Message( M_SAVE ),
        CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
    m_pcCancelButton =
        new Button( Rect( 0, 0, 1, 1 ), "CancelButton", "Cancel",
        new Message( M_CANCEL ), CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
    m_pcRevertButton =
        new Button( Rect( 0, 0, 1, 1 ), "RevertButton",
        "Revert to factory settings", new Message( M_REVERT ),
        CF_FOLLOW_LEFT | CF_FOLLOW_TOP );

    m_pcApplyButton->ResizeTo( m_pcApplyButton->GetPreferredSize( false ) );
    m_pcSaveButton->ResizeTo( m_pcSaveButton->GetPreferredSize( false ) );
    m_pcCancelButton->ResizeTo( m_pcCancelButton->GetPreferredSize( false ) );
    m_pcRevertButton->ResizeTo( m_pcRevertButton->GetPreferredSize( false ) );

    m_pcButtonsTable->SetChild( m_pcSaveButton, 2, 1 );
    m_pcButtonsTable->SetChild( m_pcApplyButton, 2, 0 );
    m_pcButtonsTable->SetChild( m_pcCancelButton, 1, 0 );
    m_pcButtonsTable->SetChild( m_pcRevertButton, 0, 0 );

    m_pcButtonsTable->ResizeTo( m_pcButtonsTable->GetPreferredSize( false ) );

    /*
     * The notice
     */
    m_pcNoticeLabel =
        new StringView( Rect( 0, 0, 1, 1 ), "NoticeLabel", "Note: "
        "clicking \"Save as defaults\" will also save current terminal size"
        " and position." );

    /*
     * Final layout
     */
    m_pcLayoutTable->SetChild( m_pcColorsTable, 0, 0 );
    m_pcLayoutTable->SetCellAlignment( 0, 0, ALIGN_CENTER, ALIGN_CENTER );
    m_pcLayoutTable->SetCellAlignment( 0, 1, ALIGN_CENTER, ALIGN_CENTER );
    m_pcLayoutTable->SetChild( m_pcIBeamCheck, 0, 1 );
    m_pcLayoutTable->SetChild( m_pcButtonsTable, 0, 2 );
    m_pcLayoutTable->SetChild( m_pcNoticeLabel, 0, 3 );
    m_pcLayoutTable->ResizeTo( m_pcLayoutTable->GetPreferredSize( false ) );

    AddChild( m_pcLayoutTable );
    SetFrame( m_pcLayoutTable->GetFrame() );

    ShowSettings();
}


SettingsWindow::~SettingsWindow()
{
    delete m_pcNormalLabel;
    delete m_pcSelectedLabel;
    delete m_pcTextLabel;
    delete m_pcBackgroundLabel;
    delete m_pcNormalTextList;
    delete m_pcNormalBackgroundList;
    delete m_pcSelectedTextList;
    delete m_pcSelectedBackgroundList;
    delete m_pcColorsTable;
}


bool SettingsWindow::OkToQuit()
{
    return true;
}


/**
 * Generic message handler
 *
 * When user clicks the save button, the terminal window settings are updated
 * and a message is send to the terminal to reflect changes. Changes are also
 * saved to disk.
 *
 * When user clicks the apply button, the terminal window settings are updated
 * and a message is send to the terminal to reflect changes.
 *
 * When user clicks the revert button, no change is made to terminal window
 * until user click save button.
 * 
 * Cancel button closes this window, discarding any changes.
 */
void SettingsWindow::HandleMessage( Message * pcMsg )
{
    switch ( pcMsg->GetCode() ) {
    case M_APPLY:
        ApplySettings();
        Application::GetInstance()->
            HandleMessage( new Message( M_REFRESH ) );
        Quit();
        break;
    case M_SAVE:
        ApplySettings();
        m_pcDestSettings->Save( false );
        Application::GetInstance()->
            HandleMessage( new Message( M_REFRESH ) );
        Quit();
        break;
    case M_CANCEL:
        Quit();
        break;
    case M_REVERT:
        m_cTempSettings.RevertToDefaults();
        ShowSettings();
        break;
    default:
        Window::HandleMessage( pcMsg );
    }
}


/**
 * Update controls in the window to reflect settings values
 *
 * Settings are displayed from the window local copy of original settings.
 */
void SettingsWindow::ShowSettings()
{
    m_pcNormalTextList->
        SetSelection( TermSettings::GetPaletteIndex( m_cTempSettings.
                GetNormalTextColor() ) );
    m_pcNormalBackgroundList->
        SetSelection( TermSettings::GetPaletteIndex( m_cTempSettings.
                GetNormalBackgroundColor() ) );
    m_pcSelectedTextList->
        SetSelection( TermSettings::GetPaletteIndex( m_cTempSettings.
                GetSelectedTextColor() ) );
    m_pcSelectedBackgroundList->
        SetSelection( TermSettings::GetPaletteIndex( m_cTempSettings.
                GetSelectedBackgroundColor() ) );
    m_pcIBeamCheck->SetValue( m_cTempSettings.GetIBeam() );
}


/**
 * Copy current settings into the original structure.
 */
void SettingsWindow::ApplySettings()
{
    m_cTempSettings.
        SetNormalTextColor( TermSettings::
        GetPaletteColor( ( uint8 )m_pcNormalTextList->GetSelection() ) );
    m_cTempSettings.
        SetNormalBackgroundColor( TermSettings::
        GetPaletteColor( ( uint8 )m_pcNormalBackgroundList->
                GetSelection() ) );
    m_cTempSettings.
        SetSelectedTextColor( TermSettings::
        GetPaletteColor( ( uint8 )m_pcSelectedTextList->GetSelection() ) );
    m_cTempSettings.
        SetSelectedBackgroundColor( TermSettings::
        GetPaletteColor( ( uint8 )m_pcSelectedBackgroundList->
                GetSelection() ) );

    m_pcDestSettings->SetNormalTextColor( m_cTempSettings.
        GetNormalTextColor() );
    m_pcDestSettings->SetNormalBackgroundColor( m_cTempSettings.
        GetNormalBackgroundColor() );
    m_pcDestSettings->SetSelectedTextColor( m_cTempSettings.
        GetSelectedTextColor() );
    m_pcDestSettings->SetSelectedBackgroundColor( m_cTempSettings.
        GetSelectedBackgroundColor() );
    m_pcDestSettings->SetIBeam( m_pcIBeamCheck->GetValue() );
}
