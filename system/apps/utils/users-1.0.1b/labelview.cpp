/*
 *  Users and Passwords Manager for AtheOS
 *  Copyright (C) 2002 William Rose
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
#include <gui/font.h>

#include "labelview.h"

namespace ui {
  LabelView::LabelView( const os::Rect& cFrame, const std::string& cName,
                        const std::string& cText,
                        uint32 nResizeMask,
                        uint32 nFlags )
    : View( cFrame, cName, nResizeMask, nFlags ), m_pzText( NULL ),
      m_vLongestWord( 0.0f ), m_nWordCount( 0 ), m_nTextLength( 0 ) {
    SetText( cText.c_str() );

    SetFgColor( 0, 0, 0 );
  }

  LabelView::LabelView( const os::Rect& cFrame, const std::string& cName,
                        const char *pzText,
                        uint32 nResizeMask,
                        uint32 nFlags )
    : View( cFrame, cName, nResizeMask, nFlags ), m_pzText( NULL ),
      m_vLongestWord( 0.0f ), m_nWordCount( 0 ), m_nTextLength( 0 ) {
    SetText( pzText );

    SetFgColor( 0, 0, 0 );
  }

  LabelView::~LabelView() {
  }

  void LabelView::SetText( const char *pzText ) {
    if( m_pzText != NULL )
      delete[] m_pzText;

    if( pzText == NULL ) {
      m_nTextLength = 1;
      m_pzText = new char[1];
      *m_pzText = 0;
    } else {
      m_nTextLength = strlen( pzText ) + 1;
      m_pzText = new char[ m_nTextLength ];
      strcpy( m_pzText, pzText );
    }
    
    FrameSized( os::Point( 0, 0 ) );
  }

  void LabelView::FrameSized( const os::Point& cDelta ) {
    m_cLines.clear();

    // Create a list of line-break points.
    float width = GetBounds().Width();
    float line = 0.0;
    bool  inword = false;
    char *pzLastLine = m_pzText;
    char *pzLastWordEnd = m_pzText;
    
    m_vLongestWord = 0.0f;
    m_nWordCount = 0;
    
    for( char *p = m_pzText; p - m_pzText < m_nTextLength; ++p ) {
      switch( *p ) {
        case ' ':
        case '\t':
        case 0:
          if( inword ) {
            // New word break;
            inword = false;

            ++m_nWordCount;
            
            float vWordLength
                    = GetStringWidth( pzLastWordEnd, p - pzLastWordEnd );
            
            if( vWordLength > m_vLongestWord )
              m_vLongestWord = vWordLength;

            // We want to execute the following once usually, but twice if
            // we happen to break before a word that is itself > the width.
            while( p > pzLastLine && 
                GetStringWidth( pzLastLine, p - pzLastLine ) > width ) {
              // The last word will not fit on the line.  Is it the only word
              // on the line?
              if( pzLastWordEnd == pzLastLine ) {
                // Have to break a long word in two and back up a bit (bugger)
                // Do a binary search for the character to break at
                unsigned l = 0, r = (p - pzLastLine), m = r >> 1;

                while( m > l ) {
                  if( GetStringWidth( pzLastLine, m + 1 ) < (width - 2.0f) )
                    l = m;
                  else
                    r = m;

                  m = l + ((r - l) >> 1);
                }
                
                pzLastLine += m + 1;
                pzLastWordEnd = pzLastLine;
                // pzLastWordEnd will be set to p + 1 (i.e. pzLastLine) below
                m_cLines.insert( m_cLines.end(), pzLastLine );
              } else {
                // Okay, start a new line after the last word.
                pzLastLine = pzLastWordEnd;
                m_cLines.insert( m_cLines.end(), pzLastLine );
              }
            }
          }
          
          // Run of spaces/tabs ... add them on to the end of the last word
          pzLastWordEnd = p + 1;
          break;
          
        case '\n':
          // Forced line break.
          pzLastLine = p + 1;
          pzLastWordEnd = pzLastLine;
          m_cLines.insert( m_cLines.end(), pzLastLine );
          ++m_nWordCount;
          break;
        
        default:
          inword = true;
          break;
      }
    }

    Invalidate();
    Flush();
  }
  
  os::Point LabelView::GetPreferredSize( bool bLargest ) {
    os::font_height cMetrics;

    GetFontHeight( &cMetrics );
    
    float line = cMetrics.ascender + cMetrics.descender + cMetrics.line_gap;
    
    if( bLargest ) {
      return os::Point( GetStringWidth( m_pzText ), line );
    } else {
      return os::Point( m_vLongestWord + 1, line * m_nWordCount );
    }
  }
  
  const char *LabelView::GetText( ) const {
    return m_pzText;
  }

  void LabelView::AttachedToWindow() {
    View *pcParent = GetParent();

    if( pcParent != NULL ) {
      SetBgColor( pcParent->GetBgColor() );
    }
  }

  void LabelView::Paint( const os::Rect& cDamaged ) {
    os::Rect cBounds = GetBounds();

    FillRect( cBounds, get_default_color( os::COL_NORMAL ) );
    
    os::font_height cMetrics;

    GetFontHeight( &cMetrics );
    
    const char *pzCurrent = m_pzText;
    float line = cMetrics.ascender + cMetrics.descender + cMetrics.line_gap;
    os::Point pos( 0, cMetrics.ascender );
    
    
    for( std::vector<const char *>::iterator ppzNext = m_cLines.begin();
         ppzNext != m_cLines.end();
         ++ppzNext, pos.y += line ) {
      
      DrawString( pzCurrent, (*ppzNext) - pzCurrent, pos );

      pzCurrent = *ppzNext;
    }

    // Draw last line
    DrawString( pzCurrent, pos );
  }
};
