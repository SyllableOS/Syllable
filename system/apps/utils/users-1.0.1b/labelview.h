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
#ifndef WR_UI_LABELVIEW_H
#define WR_UI_LABELVIEW_H

#include <gui/view.h>
#include <vector>

namespace ui {
  class LabelView : public os::View {
    public:
      LabelView( const os::Rect& cFrame, const std::string& cName,
                 const std::string& cText,
                 uint32 nResizeMask = os::CF_FOLLOW_LEFT | os::CF_FOLLOW_TOP,
                 uint32 nFlags = os::WID_WILL_DRAW | os::WID_CLEAR_BACKGROUND );

      LabelView( const os::Rect& cFrame, const std::string& cName,
                 const char *pzText,
                 uint32 nResizeMask = os::CF_FOLLOW_LEFT | os::CF_FOLLOW_TOP,
                 uint32 nFlags = os::WID_WILL_DRAW | os::WID_CLEAR_BACKGROUND );

      virtual ~LabelView();

      void SetText( const std::string& cText )
        { SetText( cText.c_str() ); }
      
      virtual void AttachedToWindow();

      virtual void SetText( const char *pzText );

      virtual os::Point GetPreferredSize( bool bLargest );

      virtual void FrameSized( const os::Point& cDelta );

      const char *GetText( ) const;

      virtual void Paint( const os::Rect& cDamaged );
      
    private:
      char *m_pzText;
      float m_vLongestWord;
      unsigned m_nWordCount, m_nTextLength;
      std::vector<const char *> m_cLines;
  };
};

#endif
