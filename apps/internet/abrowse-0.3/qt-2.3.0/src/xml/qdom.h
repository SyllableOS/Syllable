/****************************************************************************
** $Id$
**
** Definition of QDomDocument and related classes.
**
** Created : 000518
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the XML module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition licenses may use this
** file in accordance with the Qt Commercial License Agreement provided
** with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef QDOM_H
#define QDOM_H

#ifndef QT_H
#include <qstring.h>
#include <qdict.h>
#include <qrect.h>
#include <qfont.h>
#include <qpen.h>
#include <qpoint.h>
#include <qsize.h>
#include <qvariant.h>
#include <qmime.h>
#endif // QT_H

#include <qmodules.h>

#if !defined(QT_MODULE_XML)
#define QM_EXPORT
#else
#define QM_EXPORT Q_EXPORT
#endif

#ifndef QT_NO_DOM
class QWidget;
class QLayout;
class QIODevice;
class QTextStream;

class QDOM_DocumentPrivate;
class QDOM_DocumentTypePrivate;
class QDOM_DocumentFragmentPrivate;
class QDOM_NodePrivate;
class QDOM_NodeListPrivate;
class QDOM_ImplementationPrivate;
class QDOM_ElementPrivate;
class QDOM_NotationPrivate;
class QDOM_EntityPrivate;
class QDOM_EntityReferencePrivate;
class QDOM_ProcessingInstructionPrivate;
class QDOM_AttrPrivate;
class QDOM_CharacterDataPrivate;
class QDOM_TextPrivate;
class QDOM_CommentPrivate;
class QDOM_CDATASectionPrivate;
class QDOM_NamedNodeMapPrivate;
class QDOM_ImplementationPrivate;

class QDomNodeList;
class QDomElement;
class QDomText;
class QDomComment;
class QDomCDATASection;
class QDomProcessingInstruction;
class QDomAttr;
class QDomEntityReference;
class QDomDocument;
class QDomNamedNodeMap;
class QDomDocument;
class QDomDocumentFragment;
class QDomDocumentType;
class QDomImplementation;
class QDomNode;
class QDomEntity;
class QDomNotation;
class QDomCharacterData;

class QM_EXPORT QDomImplementation
{
public:
    QDomImplementation();
    QDomImplementation( const QDomImplementation& );
    virtual ~QDomImplementation();
    QDomImplementation& operator= ( const QDomImplementation& );
    bool operator== ( const QDomImplementation& ) const;
    bool operator!= ( const QDomImplementation& ) const;

    virtual bool hasFeature( const QString& feature, const QString& version );

    bool isNull();

private:
    QDomImplementation( QDOM_ImplementationPrivate* );

    QDOM_ImplementationPrivate* impl;

    friend class QDomDocument;
};

class QM_EXPORT QDomNode // Ok
{
public:
    enum NodeType {
	BaseNode                  = 0,
	ElementNode               = 1,
	AttributeNode             = 2,
	TextNode                  = 3,
	CDATASectionNode          = 4,
	EntityReferenceNode       = 5,
	EntityNode                = 6,
	ProcessingInstructionNode = 7,
	CommentNode               = 8,
	DocumentNode              = 9,
	DocumentTypeNode          = 10,
	DocumentFragmentNode      = 11,
	NotationNode              = 12,
	CharacterDataNode         = 13
    };

    QDomNode();
    QDomNode( const QDomNode& );
    QDomNode& operator= ( const QDomNode& );
    bool operator== ( const QDomNode& ) const;
    bool operator!= ( const QDomNode& ) const;
    virtual ~QDomNode();

    virtual QString nodeName() const;
    virtual QString nodeValue() const;
    virtual void setNodeValue( const QString& );
    virtual QDomNode::NodeType nodeType() const;

    virtual QDomNode         parentNode() const;
    virtual QDomNodeList     childNodes() const;
    virtual QDomNode         firstChild() const;
    virtual QDomNode         lastChild() const;
    virtual QDomNode         previousSibling() const;
    virtual QDomNode         nextSibling() const;
    virtual QDomNamedNodeMap attributes() const;
    virtual QDomDocument     ownerDocument() const;

    virtual QDomNode insertBefore( const QDomNode& newChild, const QDomNode& refChild );
    virtual QDomNode insertAfter( const QDomNode& newChild, const QDomNode& refChild );
    virtual QDomNode replaceChild( const QDomNode& newChild, const QDomNode& oldChild );
    virtual QDomNode removeChild( const QDomNode& oldChild );
    virtual QDomNode appendChild( const QDomNode& newChild );
    virtual QDomNode cloneNode( bool deep = TRUE ) const;

    // Qt extension
    virtual bool isAttr() const;
    virtual bool isCDATASection() const;
    virtual bool isDocumentFragment() const;
    virtual bool isDocument() const;
    virtual bool isDocumentType() const;
    virtual bool isElement() const;
    virtual bool isEntityReference() const;
    virtual bool isText() const;
    virtual bool isEntity() const;
    virtual bool isNotation() const;
    virtual bool isProcessingInstruction() const;
    virtual bool isCharacterData() const;
    virtual bool isComment() const;

    /**
     * Shortcut to avoid dealing with QDomNodeList
     * all the time.
     */
    QDomNode namedItem( const QString& name ) const;

    bool isNull() const;
    void clear();

    QDomAttr toAttr();
    QDomCDATASection toCDATASection();
    QDomDocumentFragment toDocumentFragment();
    QDomDocument toDocument();
    QDomDocumentType toDocumentType();
    QDomElement toElement();
    QDomEntityReference toEntityReference();
    QDomText toText();
    QDomEntity toEntity();
    QDomNotation toNotation();
    QDomProcessingInstruction toProcessingInstruction();
    QDomCharacterData toCharacterData();
    QDomComment toComment();

    void save( QTextStream&, int ) const;

protected:
    QDOM_NodePrivate* impl;
    QDomNode( QDOM_NodePrivate* );

private:
    friend class QDomDocument;
    friend class QDomDocumentType;
    friend class QDomNodeList;
    friend class QDomNamedNodeMap;
};

class QM_EXPORT QDomNodeList // Ok
{
public:
    QDomNodeList();
    QDomNodeList( const QDomNodeList& );
    QDomNodeList& operator= ( const QDomNodeList& );
    bool operator== ( const QDomNodeList& ) const;
    bool operator!= ( const QDomNodeList& ) const;
    virtual ~QDomNodeList();

    virtual QDomNode item( int index ) const;
    virtual uint length() const;
    uint count() const { return length(); } // Qt API consitancy

    QDomNodeList( QDOM_NodeListPrivate* );
private:
    QDOM_NodeListPrivate* impl;
};

class QM_EXPORT QDomDocumentType : public QDomNode
{
public:
    QDomDocumentType();
    QDomDocumentType( const QDomDocumentType& x );
    QDomDocumentType& operator= ( const QDomDocumentType& );
    ~QDomDocumentType();

    virtual QString name() const;
    virtual QDomNamedNodeMap entities() const;
    virtual QDomNamedNodeMap notations() const;

    // Reimplemented from QDomNode
    QDomNode::NodeType nodeType() const;
    bool isDocumentType() const;

private:
    QDomDocumentType( QDOM_DocumentTypePrivate* );

    friend class QDomDocument;
    friend class QDomNode;
};

class QM_EXPORT QDomDocument : public QDomNode
{
public:
    QDomDocument();
    QDomDocument( const QString& name );
    QDomDocument( const QDomDocument& x );
    QDomDocument& operator= ( const QDomDocument& );
    ~QDomDocument();

    // Qt extensions
    bool setContent( const QCString& text );
    bool setContent( const QByteArray& text );
    bool setContent( const QString& text );
    bool setContent( QIODevice* dev );

    // QDomAttributes
    QDomDocumentType doctype() const;
    QDomImplementation implementation() const;
    QDomElement documentElement() const;

    // Factories
    QDomElement               createElement( const QString& tagName );
    QDomDocumentFragment      createDocumentFragment();
    QDomText                  createTextNode( const QString& data );
    QDomComment               createComment( const QString& data );
    QDomCDATASection          createCDATASection( const QString& data );
    QDomProcessingInstruction createProcessingInstruction( const QString& target, const QString& data );
    QDomAttr                  createAttribute( const QString& name );
    QDomEntityReference       createEntityReference( const QString& name );
    QDomNodeList              elementsByTagName( const QString& tagname ) const;

    // Reimplemented from QDomNode
    QDomNode::NodeType nodeType() const;
    bool isDocument() const;

    // Qt extensions
    QString toString() const;
    QCString toCString() const { return toString().utf8(); }

private:
    QDomDocument( QDOM_DocumentPrivate* );

    friend class QDomNode;
};

class QM_EXPORT QDomNamedNodeMap
{
public:
    QDomNamedNodeMap();
    QDomNamedNodeMap( const QDomNamedNodeMap& );
    QDomNamedNodeMap& operator= ( const QDomNamedNodeMap& );
    bool operator== ( const QDomNamedNodeMap& ) const;
    bool operator!= ( const QDomNamedNodeMap& ) const;
    ~QDomNamedNodeMap();

    QDomNode namedItem( const QString& name ) const;
    QDomNode setNamedItem( const QDomNode& arg );
    QDomNode removeNamedItem( const QString& name );
    QDomNode item( int index ) const;
    uint length() const;
    bool contains( const QString& name ) const;

private:
    friend class QDomNode;
    friend class QDomDocumentType;
    friend class QDomElement;

    QDomNamedNodeMap( QDOM_NamedNodeMapPrivate* );

    QDOM_NamedNodeMapPrivate* impl;
};

class QM_EXPORT QDomDocumentFragment : public QDomNode
{
public:
    QDomDocumentFragment();
    QDomDocumentFragment( const QDomDocumentFragment& x );
    QDomDocumentFragment& operator= ( const QDomDocumentFragment& );
    ~QDomDocumentFragment();

    // Reimplemented from QDomNode
    QDomNode::NodeType nodeType() const;
    bool isDocumentFragment() const;

private:
    QDomDocumentFragment( QDOM_DocumentFragmentPrivate* );

    friend class QDomDocument;
    friend class QDomNode;
};

class QM_EXPORT QDomCharacterData : public QDomNode
{
public:
    QDomCharacterData();
    QDomCharacterData( const QDomCharacterData& x );
    QDomCharacterData& operator= ( const QDomCharacterData& );
    ~QDomCharacterData();

    virtual QString data() const;
    virtual void setData( const QString& );
    virtual uint length() const;

    virtual QString substringData( unsigned long offset, unsigned long count );
    virtual void    appendData( const QString& arg );
    virtual void    insertData( unsigned long offset, const QString& arg );
    virtual void    deleteData( unsigned long offset, unsigned long count );
    virtual void    replaceData( unsigned long offset, unsigned long count, const QString& arg );

    // Reimplemented from QDomNode
    QDomNode::NodeType nodeType() const;
    bool isCharacterData() const;

private:
    QDomCharacterData( QDOM_CharacterDataPrivate* );

    friend class QDomDocument;
    friend class QDomText;
    friend class QDomComment;
    friend class QDomNode;
};

class QM_EXPORT QDomAttr : public QDomNode
{
public:
    QDomAttr();
    QDomAttr( const QDomAttr& x );
    QDomAttr& operator= ( const QDomAttr& );
    ~QDomAttr();

    virtual QString  name() const;
    virtual bool     specified() const;
    virtual QString  value() const;
    virtual void setValue( const QString& );

    // Reimplemented from QDomNode
    QDomNode::NodeType nodeType() const;
    bool isAttr() const;

private:
    QDomAttr( QDOM_AttrPrivate* );

    friend class QDomDocument;
    friend class QDomElement;
    friend class QDomNode;
};

class QM_EXPORT QDomElement : public QDomNode
{
public:
    QDomElement();
    QDomElement( const QDomElement& x );
    QDomElement& operator= ( const QDomElement& );
    ~QDomElement();

    void setTagName( const QString& name );
    QString  tagName() const;
    QString  attribute( const QString& name, const QString& defValue = QString::null ) const;
    void     setAttribute( const QString& name, const QString& value );
    void     setAttribute( const QString& name, int value );
    void     setAttribute( const QString& name, uint value );
    void     setAttribute( const QString& name, double value );
    void     removeAttribute( const QString& name );
    QDomAttr     attributeNode( const QString& name);
    QDomAttr     setAttributeNode( const QDomAttr& newAttr );
    QDomAttr     removeAttributeNode( const QDomAttr& oldAttr );
    bool     hasAttribute( const QString& name ) const;
    virtual QDomNodeList elementsByTagName( const QString& tagname ) const;
    void     normalize();

    // Reimplemented from QDomNode
    QDomNamedNodeMap attributes() const;
    QDomNode::NodeType nodeType() const;
    bool isElement() const;

    QString text() const;

private:
    QDomElement( QDOM_ElementPrivate* );

    friend class QDomDocument;
    friend class QDomNode;
};

class QM_EXPORT QDomText : public QDomCharacterData
{
public:
    QDomText();
    QDomText( const QDomText& x );
    QDomText& operator= ( const QDomText& );
    ~QDomText();

    QDomText splitText( int offset );

    // Reimplemented from QDomNode
    QDomNode::NodeType nodeType() const;
    bool isText() const;

private:
    QDomText( QDOM_TextPrivate* );

    friend class QDomCDATASection;
    friend class QDomDocument;
    friend class QDomNode;
};

class QM_EXPORT QDomComment : public QDomCharacterData
{
public:
    QDomComment();
    QDomComment( const QDomComment& x );
    QDomComment& operator= ( const QDomComment& );
    ~QDomComment();

    // Reimplemented from QDomNode
    QDomNode::NodeType nodeType() const;
    bool isComment() const;

private:
    QDomComment( QDOM_CommentPrivate* );

    friend class QDomDocument;
    friend class QDomNode;
};

class QM_EXPORT QDomCDATASection : public QDomText
{
public:
    QDomCDATASection();
    QDomCDATASection( const QDomCDATASection& x );
    QDomCDATASection& operator= ( const QDomCDATASection& );
    ~QDomCDATASection();

    // Reimplemented from QDomNode
    QDomNode::NodeType nodeType() const;
    bool isCDATASection() const;

private:
    QDomCDATASection( QDOM_CDATASectionPrivate* );

    friend class QDomDocument;
    friend class QDomNode;
};

class QM_EXPORT QDomNotation : public QDomNode
{
public:
    QDomNotation();
    QDomNotation( const QDomNotation& x );
    QDomNotation& operator= ( const QDomNotation& );
    ~QDomNotation();

    QString publicId() const;
    QString systemId() const;

    // Reimplemented from QDomNode
    QDomNode::NodeType nodeType() const;
    bool isNotation() const;

private:
    QDomNotation( QDOM_NotationPrivate* );

    friend class QDomDocument;
    friend class QDomNode;
};

class QM_EXPORT QDomEntity : public QDomNode
{
public:
    QDomEntity();
    QDomEntity( const QDomEntity& x );
    QDomEntity& operator= ( const QDomEntity& );
    ~QDomEntity();

    virtual QString publicId() const;
    virtual QString systemId() const;
    virtual QString notationName() const;

    // Reimplemented from QDomNode
    QDomNode::NodeType nodeType() const;
    bool isEntity() const;

private:
    QDomEntity( QDOM_EntityPrivate* );

    friend class QDomNode;
};

class QM_EXPORT QDomEntityReference : public QDomNode
{
public:
    QDomEntityReference();
    QDomEntityReference( const QDomEntityReference& x );
    QDomEntityReference& operator= ( const QDomEntityReference& );
    ~QDomEntityReference();

    // Reimplemented from QDomNode
    QDomNode::NodeType nodeType() const;
    bool isEntityReference() const;

private:
    QDomEntityReference( QDOM_EntityReferencePrivate* );

    friend class QDomDocument;
    friend class QDomNode;
};

class QM_EXPORT QDomProcessingInstruction : public QDomNode
{
public:
    QDomProcessingInstruction();
    QDomProcessingInstruction( const QDomProcessingInstruction& x );
    QDomProcessingInstruction& operator= ( const QDomProcessingInstruction& );
    ~QDomProcessingInstruction();

    virtual QString target() const;
    virtual QString data() const;
    virtual void setData( const QString& d );

    // Reimplemented from QDomNode
    QDomNode::NodeType nodeType() const;
    bool isProcessingInstruction() const;

private:
    QDomProcessingInstruction( QDOM_ProcessingInstructionPrivate* );

    friend class QDomDocument;
    friend class QDomNode;
};


QTextStream& operator<<( QTextStream&, const QDomNode& );

#endif //QT_NO_DOM
#endif // QDOM_H
