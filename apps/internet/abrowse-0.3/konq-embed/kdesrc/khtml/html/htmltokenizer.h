/*
    This file is part of the KDE libraries

    Copyright (C) 1997 Martin Jones (mjones@kde.org)
              (C) 1997 Torben Weis (weis@kde.org)
              (C) 1998 Waldo Bastian (bastian@kde.org)
              (C) 2001 Dirk Mueller (mueller@kde.org)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/
//----------------------------------------------------------------------------
//
// KDE HTML Widget -- Tokenizers
// $Id$

#ifndef HTMLTOKENIZER_H
#define HTMLTOKENIZER_H


//
// External Classes
//
///////////////////

class KCharsets;

//
// Internal Classes
//
///////////////////

class StringTokenizer;
class HTMLTokenizer;

#include <qstring.h>
#include <qobject.h>

#include "stringit.h"
#include "loader_client.h"

class KHTMLParser;
class KHTMLView;

namespace DOM {
    class HTMLDocumentImpl;
    class DocumentFragmentImpl;
}

namespace khtml {
    class CachedScript;
    class Token;
}

// The count of spaces used for each tab.
#define TAB_SIZE 8


class Tokenizer : public QObject
{
    Q_OBJECT
public:
    virtual void begin() = 0;
    virtual void write( const QString &str ) = 0;
    virtual void end() = 0;
    virtual void finish() = 0;
    virtual void setOnHold(bool /*_onHold*/) {}

signals:
    void finishedParsing();

};

//-----------------------------------------------------------------------------

/**
 * @internal
 * This class takes QStrings as input, and splits up the input streams into
 * tokens, which are passed on to the @ref KHTMLParser.
 */
class HTMLTokenizer : public Tokenizer, public khtml::CachedObjectClient
{
    Q_OBJECT
public:
    HTMLTokenizer(DOM::HTMLDocumentImpl *, KHTMLView * = 0);
    HTMLTokenizer(DOM::HTMLDocumentImpl *, DOM::DocumentFragmentImpl *frag);
    virtual ~HTMLTokenizer();

    void begin();
    void setPlainText();
    void write( const QString &str );
    void end();
    void finish();
    virtual void setOnHold(bool _onHold);

protected:
    void reset();
    void addPending();
    void processToken();

    void addListing(khtml::DOMStringIt list);

    void parseComment(khtml::DOMStringIt &str);
    void parseText(khtml::DOMStringIt &str);
    void parseStyle(khtml::DOMStringIt &str);
    void parseScript(khtml::DOMStringIt &str);
    void parseListing(khtml::DOMStringIt &str);
    void parseTag(khtml::DOMStringIt &str);
    void parseEntity(khtml::DOMStringIt &str, QChar *&dest, bool start = false);
    void parseProcessingInstruction(khtml::DOMStringIt &str);
    void addScriptOutput();

    // check if we have enough space in the buffer.
    // if not enlarge it
    inline void checkBuffer(int len = 10)
    {
        if ( (dest - buffer) > size-len )
            enlargeBuffer();
    }
    inline void checkScriptBuffer(int len = 10)
    {
        if ( scriptCodeSize + len >= scriptCodeMaxSize )
            enlargeScriptBuffer();
    }

    void enlargeBuffer();
    void enlargeScriptBuffer();

    // from CachedObjectClient
    void notifyFinished(khtml::CachedObject *finishedObj);
protected:
    // Internal buffers
    ///////////////////
    QChar *buffer;
    QChar *dest;

    khtml::Token *currToken;

    // the size of buffer
    int size;

    // Tokenizer flags
    //////////////////
    // are we in quotes within a html tag
    typedef enum
    {
        NoQuote = 0,
        SingleQuote,
        DoubleQuote
    } HTMLQuote;

    HTMLQuote tquote;

    typedef enum
    {
        NonePending = 0,
        SpacePending,
        LFPending,
        TabPending
    } HTMLPendingType;

    // To avoid multiple spaces
    HTMLPendingType pending;

    typedef enum
    {
        NoneDiscard = 0,
        SpaceDiscard,
        LFDiscard,
        AllDiscard  // discard all spaces, LF's etc until next non white char
    } HTMLDiscardType;

    // Discard line breaks immediately after start-tags
    // Discard spaces after '=' within tags
    HTMLDiscardType discard;

    // Discard the LF part of CRLF sequence
    bool skipLF;

    // Flag to say that we have the '<' but not the character following it.
    bool startTag;

    typedef enum {
        NoTag = 0,
        TagName,
        SearchAttribute,
        AttributeName,
        SearchEqual,
        SearchValue,
        QuotedValue,
        Value,
        SearchEnd
    } HTMLTagParse;

    // Flag to say, we are just parsing a tag, meaning, we are in the middle
    // of <tag...
    HTMLTagParse tag;

    // Flag to say that we are just parsing an attribute
    bool parseAttr;

    // Are we in a <pre> ... </pre> block
    bool pre;

    // if 'pre == true' we track in which column we are
    int prePos;

    // Are we in a <script> ... </script> block
    bool script;

    // Are we in a <style> ... </style> block
    bool style;

    // Are we in a <select> ... </select> block
    bool select;

    // Are we in a <listing> ... </listing> block
    bool listing;

    // Are we in plain textmode ?
    bool plaintext;

    // Are we in a &... character entity description?
    bool charEntity;

    // XML processing instructions. Ignored at the moment
    bool processingInstruction;

    // Area we in a <!-- comment --> block
    bool comment;

    // Are we in a <textarea> ... </textarea> block
    bool textarea;

    // was the previous character escaped ?
    bool escaped;

    // name of an unknown attribute
    QString attrName;

    // Used to store the code of a srcipting sequence
    QChar *scriptCode;
    // Size of the script sequenze stored in @ref #scriptCode
    int scriptCodeSize;
    // Maximal size that can be stored in @ref #scriptCode
    int scriptCodeMaxSize;

    // Stores characters if we are scanning for a string like "</script>"
    QChar searchBuffer[ 10 ];
    // Counts where we are in the string we are scanning for
    int searchCount;
    // The string we are searching for
    const QChar *searchFor;
    // true if we are waiting for an external script (<SCRIPT SRC=...) to load, i.e.
    // we don't do any parsing while this is true
    bool loadingExtScript;
    // if no more data is coming, just parse what we have (including ext scripts that
    // may be still downloading) and finish
    bool noMoreData;
    // URL to get source code of script from
    QString scriptSrc;
    bool javascript;
    // the HTML code we will parse after the external script we are waiting for has loaded
    QString pendingSrc;
    // true if we are executing a script while parsing a document. This causes the parsing of
    // the output of the script to be postponed until after the script has finished executing
    bool executingScript;
    khtml::CachedScript *cachedScript;
    // you can pause the tokenizer if you need to display a dialog or something
    bool onHold;

    QString scriptOutput;

    QChar entityBuffer[10];
    uint entityPos;

    QString _src;
    khtml::DOMStringIt src;

    KCharsets *charsets;
    KHTMLParser *parser;

    KHTMLView *view;
};

#endif // HTMLTOKENIZER

