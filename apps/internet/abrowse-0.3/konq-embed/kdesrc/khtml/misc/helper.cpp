/*
 * This file is part of the CSS implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * $Id$
 */
#include "helper.h"
#include <qmap.h>
#include <qpainter.h>
#include <dom/dom_string.h>
#include <xml/dom_stringimpl.h>
#include <qlist.h>
#include <khtmllayout.h>
#include <kstaticdeleter.h>
#include <kapp.h>
#include <qtooltip.h>

using namespace DOM;
using namespace khtml;

// ### make it const if possible...
 struct HTMLColors {
    QMap<QString,QColor> map;
    HTMLColors()
    {
        map["black"] = "#000000";
        map["green"] = "#008000";
        map["silver"] = "#c0c0c0";
        map["lime"] = "#00ff00";
        map["gray"] = "#808080";
        map["olive"] = "#808000";
        map["white"] = "#ffffff";
        map["yellow"] = "#ffff00";
        map["maroon"] = "#800000";
        map["navy"] = "#000080";
        map["red"] = "#ff0000";
        map["blue"] = "#0000ff";
        map["purple"] = "#800080";
        map["teal"] = "#008080";
        map["fuchsia"] = "#ff00ff";
        map["aqua"] = "#00ffff";
	map["crimson"] = "#dc143c";
	map["indigo"] = "#4b0082";
#ifdef __ATHEOS__
	printf( "Warning: HTMLColors::HTMLColors() not initiating all colors\n" );
#else	
        // ### react to style changes
        // see http://www.richinstyle.com for details
        QColorGroup cg = kapp->palette().active();
        map["activeborder"] = cg.light(); // bordercolor of an active window
        map["activecaption"] = cg.text(); // caption color of an active window
        map["appworkspace"] = cg.background(); // background color of an MDI interface
        map["highlight"] = cg.highlight();
        map["highlighttext"] = cg.highlightedText();
        cg = kapp->palette().inactive();
        map["background"] = cg.background(); // desktop background color
        map["buttonface"] = cg.button(); // Button background color
        map["buttonhighlight"] =  cg.light();
        map["buttonshadow"] = cg.shadow();
        map["buttontext"] = cg.buttonText();
        map["captiontext"] = cg.text();
        map["infobackground"] = QToolTip::palette().inactive().background();
        map["menu"] = cg.background();
        map["menutext"] = cg.foreground();
        map["scrollbar"] = cg.background();
        map["threeddarkshadow"] = cg.dark();
        map["threedface"] = cg.button();
        map["threedhighlight"] = cg.light();
        map["threedlightshadow"] = cg.midlight();
        map["window"] = cg.background();
        map["windowframe"] = cg.background();
        map["text"] = cg.text();
        cg = kapp->palette().disabled();
        map["inactiveborder"] = cg.background();
        map["inactivecaption"] = cg.background();
        map["inactivecaptiontext"] = cg.text();
        map["graytext"] = cg.text();
#endif	
    };
};

static HTMLColors *htmlColors = 0L;

static KStaticDeleter<HTMLColors> hcsd;

void khtml::setNamedColor(QColor &color, const QString &_name)
{
    if( !htmlColors )
        htmlColors = hcsd.setObject( new HTMLColors );

    int pos;
    QString name = _name;
    // remove white spaces for those broken websites out there :-(
    while ( ( pos = name.find( ' ' ) ) != -1 )  name.remove( pos, 1 );

    int len = name.length();
    char ch = name[0].latin1();

    if(len == 0 || (len == 11 && name.find("transparent", 0, false) == 0) )
    {
        color = QColor(); // invalid color == transparent
        return;
    }

    // also recognize "color=ffffff"
    if (len == 6)
    {
        bool ok;
        int val = name.toInt(&ok, 16);
        if(ok)
        {
            color.setRgb((0xff << 24) | val);
            return;
        }
        // recognize #12345 (duplicate the last character)
        if(name[0] == '#') {
            bool ok;
            int val = name.right(5).toInt(&ok, 16);
            if(ok)
            {
                color.setRgb((0xff << 24) | (val * 16 + val&0xf));
                return;
            }
        }
        if ( !name[0].isLetter() ) {
	    color = QColor();
	    return;
	}
    }

    if ( len > 4 && ch == 'r' && name[1].lower() == 'g' &&
         name[2].lower() == 'b' && name[3].cell() == '(' &&
         name[len-1].cell() == ')')
    {
        // CSS like rgb(r, g, b) style
        DOMString rgb = name.mid(4, name.length()-5);
        QList<Length> *l = rgb.implementation()->toLengthList();
        if(l->count() != 3)
        {
	    // transparent in case of an invalid color.
            color = QColor();
            delete l;
            return;
        }
        int r = l->at(0)->isUndefined() ? 0 : l->at(0)->width(255);
        if(r < 0) r = 0;
        if(r > 255) r = 255;
        int g = l->at(1)->isUndefined() ? 0 : l->at(1)->width(255);
        if(g < 0) g = 0;
        if(g > 255) g = 255;
        int b = l->at(2)->isUndefined() ? 0 : l->at(2)->width(255);
        if(b < 0) b = 0;
        if(b > 255) b = 255;

        color.setRgb(r, g, b);
        delete l;
    }
    else
    {
        QColor tc = htmlColors->map[name];
        if ( !tc.isValid() )
            tc = htmlColors->map[name.lower()];

        if (tc.isValid())
            color = tc;
        else {
            color.setNamedColor(name);
            if ( !color.isValid() )  color.setNamedColor( name.lower() );
            if(!color.isValid()) {
                bool hasalpha = false;
                for(unsigned int i = 0; i < name.length(); i++)
                    if(name[i].isLetterOrNumber()) {
                        hasalpha = true;
                        break;
                    }

                if(!hasalpha)
                  color = Qt::black;
            }
        }
    }
}

QPainter *khtml::printpainter = 0;

void khtml::setPrintPainter( QPainter *printer )
{
    printpainter = printer;
}

QFontMetrics khtml::fontMetrics( const QFont &f )
{
    if(printpainter) {
	printpainter->setFont( f );
	return printpainter->fontMetrics();
    }
    return QFontMetrics( f );
}
