/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
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

//#define FORMS_DEBUG
//#under FORMS_DEBUG

#include "html_formimpl.h"

#include "khtmlview.h"
#include "khtml_part.h"
#include "html_documentimpl.h"
#include "khtml_settings.h"
#include "htmlhashes.h"

#include "css/cssstyleselector.h"
#include "css/cssproperties.h"
#include "css/csshelper.h"
#include "xml/dom_textimpl.h"

#include "rendering/render_form.h"

#include <kcharsets.h>
#include <kdebug.h>
#include <kmimetype.h>
#include <kmessagebox.h>
#include <klocale.h>
#ifndef __ATHEOS__
#include <netaccess.h>
#include <kfileitem.h>
#endif
#include <qfile.h>
#include <qtextcodec.h>

#include <assert.h>

using namespace DOM;
using namespace khtml;

//template class QList<khtml::RenderFormElement>;

HTMLFormElementImpl::HTMLFormElementImpl(DocumentImpl *doc)
    : HTMLElementImpl(doc)
{
    m_post = false;
    m_multipart = false;
    m_enctype = "application/x-www-form-urlencoded";
    m_boundary = "----------0xKhTmLbOuNdArY";
    m_acceptcharset = "UNKNOWN";
}

HTMLFormElementImpl::~HTMLFormElementImpl()
{
    QListIterator<HTMLGenericFormElementImpl> it(formElements);
    for (; it.current(); ++it)
        it.current()->setForm(0);
}

ushort HTMLFormElementImpl::id() const
{
    return ID_FORM;
}

long HTMLFormElementImpl::length() const
{
    return formElements.count();
}

static QCString encodeCString(const QCString& e)
{
    // http://www.w3.org/TR/html4/interact/forms.html#h-17.13.4.1
    // safe characters like NS handles them for compatibility
    static const char *safe = "-._*";
    QCString encoded(e.length()*3+1);
    int enclen = 0;

    QCString orig(e.data(), e.size());

    for(unsigned pos = 0; pos < e.length(); pos++) {
        unsigned char c = e[pos];

        if ( (( c >= 'A') && ( c <= 'Z')) ||
             (( c >= 'a') && ( c <= 'z')) ||
             (( c >= '0') && ( c <= '9')) ||
             (strchr(safe, c))
            )
            encoded[enclen++] = c;
        else if ( c == ' ' )
            encoded[enclen++] = '+';
        else if ( c == '\n' )
        {
            encoded[enclen++] = '%';
            encoded[enclen++] = '0';
            encoded[enclen++] = 'D';
            encoded[enclen++] = '%';
            encoded[enclen++] = '0';
            encoded[enclen++] = 'A';
        }
        else if ( c != '\r' )
        {
            encoded[enclen++] = '%';
            unsigned int h = c / 16;
            h += (h > 9) ? ('A' - 10) : '0';
            encoded[enclen++] = h;

            unsigned int l = c % 16;
            l += (l > 9) ? ('A' - 10) : '0';
            encoded[enclen++] = l;
        }
    }
    encoded[enclen++] = '\0';
    encoded.truncate(enclen);

    return encoded;
}

inline static QCString fixUpfromUnicode(const QTextCodec* codec, const QString& s)
{
    QCString str = codec->fromUnicode(s);
    str.truncate(str.length());
    return str;
}

void HTMLFormElementImpl::i18nData()
{
    QString foo1 = i18n( "You're about to send data to the Internet\n"
                         "via an unencrypted connection.\nIt might be possible "
                         "for others to see this information.\n\n"
                         "Do you want to continue?");
    QString foo2 = i18n("KDE Web browser");
    QString foo3 = i18n("When you send a password unencrypted to the Internet,\n"
                        "it might be possible for others to capture it as plain text.\n\n"
                        "Do you want to continue?");
    QString foo4 = i18n("You're about to transfer the following files from\n"
                        "your local computer to the internet.\n\n"
                        "Do you really want to continue?");
    QString foo5 = i18n("Your data submission is redirected to\n"
                        "an insecure site. The data is sent unencrypted.\n\n"
                        "Do you want to continue?");
}


QByteArray HTMLFormElementImpl::formData()
{
#ifdef FORMS_DEBUG
    kdDebug( 6030 ) << "form: formData()" << endl;
#endif

    QByteArray form_data(0);
    QCString enc_string = ""; // used for non-multipart data

    // find out the QTextcodec to use
    QString str = m_acceptcharset.string();
    QChar space(' ');
    for(unsigned int i=0; i < str.length(); i++) if(str[i].latin1() == ',') str[i] = space;
    QStringList charsets = QStringList::split(' ', str);
    QTextCodec* codec = 0;
    for ( QStringList::Iterator it = charsets.begin(); it != charsets.end(); ++it )
    {
        QString enc = (*it);
        if(enc.contains("UNKNOWN"))
        {
            // use standard document encoding
            enc = "ISO 8859-1";
            if(view && view->part())
                enc = view->part()->encoding();
        }
        if((codec = KGlobal::charsets()->codecForName(enc.latin1())))
            break;
    }
    if(!codec)
    {
        QString n = KGlobal::charsets()->name(view->part()->settings()->charset());
        if(n != "any")
            codec = KGlobal::charsets()->codecForName(n);
    }

    if(!codec)
        codec = QTextCodec::codecForLocale();

    m_encCharset = codec->name();
    for(unsigned int i=0; i < m_encCharset.length(); i++)
        m_encCharset[i] = m_encCharset[i].latin1() == ' ' ? QChar('-') : m_encCharset[i].lower();

#if 0
        // users with UTF8 locale should be killed immediately
        if(!codec || codec->mibEnum() == 106)
            codec = QTextCodec::codecForMib(4);
#endif

    for(HTMLGenericFormElementImpl *current = formElements.first(); current; current = formElements.next())
    {
        khtml::encodingList lst;

        if (!current->disabled() && current->encoding(codec, lst, m_multipart))
        {
            //kdDebug(6030) << "adding name " << current->name().string() << endl;
	    if ( lst.count() == 1 ) {
		// hack for isindex element
		enc_string += encodeCString(*lst.begin());
	    } else {
                ASSERT(!(lst.count()&1));

                khtml::encodingList::Iterator it;
                for( it = lst.begin(); it != lst.end(); ++it )
                {
                    if (!m_multipart)
                    {
                        if(enc_string.length())
                            enc_string += '&';

                        enc_string += encodeCString(*it);
                        enc_string += "=";
                        ++it;
                        enc_string += encodeCString(*it);
                    }
                    else
                    {
                        QCString hstr("--");
                        hstr += m_boundary.string().latin1();
                        hstr += "\r\n";
                        hstr += "Content-Disposition: form-data; name=\"";
                        hstr += (*it).data();
                        hstr += "\"";

                        // if the current type is FILE, then we also need to
                        // include the filename
                        if (current->nodeType() == Node::ELEMENT_NODE && current->id() == ID_INPUT &&
                            static_cast<HTMLInputElementImpl*>(current)->inputType() == HTMLInputElementImpl::FILE)
                        {
                            QString path = static_cast<HTMLInputElementImpl*>(current)->filename().string();
                            QString onlyfilename = path.mid(path.findRev('/')+1);

                            hstr += ("; filename=\"" + onlyfilename + "\"\r\n").ascii();
                            if(!static_cast<HTMLInputElementImpl*>(current)->filename().isEmpty())
                            {
                                hstr += "Content-Type: ";
                                KMimeType::Ptr ptr = KMimeType::findByURL(KURL(path));
                                hstr += ptr->name().ascii();
                            }
                        }

                        hstr += "\r\n\r\n";
                        ++it;

                        // append body
                        unsigned int old_size = form_data.size();
                        form_data.resize( old_size + hstr.length() + (*it).size() + 1);
                        memcpy(form_data.data() + old_size, hstr.data(), hstr.length());
                        memcpy(form_data.data() + old_size + hstr.length(), *it, (*it).size());
                        form_data[form_data.size()-2] = '\r';
                        form_data[form_data.size()-1] = '\n';
                    }
                }
	    }
        }
    }
    if (m_multipart)
        enc_string = ("--" + m_boundary.string() + "--\r\n").ascii();

    int old_size = form_data.size();
    form_data.resize( form_data.size() + enc_string.length() );
    memcpy(form_data.data() + old_size, enc_string.data(), enc_string.length() );

    return form_data;
}

void HTMLFormElementImpl::setEnctype( const DOMString& type )
{
    if(type.string().find("multipart", 0, false) != -1 || type.string().find("form-data", 0, false) != -1)
    {
        m_enctype = "multipart/form-data";
        m_multipart = true;
        m_post = true;
    }
    else
    {
        m_enctype = "application/x-www-form-urlencoded";
        m_multipart = false;
    }
    m_encCharset = QString::null;
}

void HTMLFormElementImpl::setBoundary( const DOMString& bound )
{
    m_boundary = bound;
}

void HTMLFormElementImpl::prepareSubmit()
{
    if(!view) return;

    DOMString script = getAttribute(ATTR_ONSUBMIT);
    if (!script.isNull() && view->part()->jScriptEnabled()) {
        QVariant ret = view->part()->executeScript(Node(this), script.string());
	if (ret.type() == QVariant::Bool && ret.toBool() == false)
            return; // don't submit if script returns false
    }

    submit();
}

void HTMLFormElementImpl::submit(  )
{
#ifdef FORMS_DEBUG
    kdDebug( 6030 ) << "submit pressed!" << endl;
#endif
    if(!view) return;

    QByteArray form_data = formData();
    if(m_post)
    {

        view->part()->submitForm( "post", m_url.string(), form_data,
                                  m_target.string(),
                                  enctype().string(),
//                                   m_encCharset.isEmpty() ? enctype().string()
//                                   : QString(enctype().string() + "; charset=" + m_encCharset),
                                  boundary().string() );
    }
    else
        view->part()->submitForm( "get", m_url.string(), form_data,
                                  m_target.string() );
}

void HTMLFormElementImpl::prepareReset()
{
#ifdef FORMS_DEBUG
    kdDebug( 6030 ) << "reset pressed!" << endl;
#endif

    DOMString script = getAttribute(ATTR_ONRESET);
    if (!script.isNull() && view->part()->jScriptEnabled())
	view->part()->executeScript(Node(this), script.string());

    reset();
}

void HTMLFormElementImpl::reset(  )
{

    HTMLGenericFormElementImpl *current = formElements.first();
    while(current)
    {
        current->reset();
        current = formElements.next();
    }
    if (document->isHTMLDocument())
        static_cast<HTMLDocumentImpl*>(document)->updateRendering();
}

void HTMLFormElementImpl::parseAttribute(AttrImpl *attr)
{
    switch(attr->attrId)
    {
    case ATTR_ACTION:
        m_url = khtml::parseURL(attr->value());
        break;
    case ATTR_TARGET:
        m_target = attr->value();
        break;
    case ATTR_METHOD:
        if ( strcasecmp( attr->value(), "post" ) == 0 )
            m_post = true;
        break;
    case ATTR_ENCTYPE:
        setEnctype( attr->value() );
        break;
    case ATTR_ACCEPT_CHARSET:
        // space separated list of charsets the server
        // accepts - see rfc2045
        m_acceptcharset = attr->value();
        break;
    case ATTR_ACCEPT:
        // ignore this one for the moment...
        break;
    default:
        HTMLElementImpl::parseAttribute(attr);
    }
}

void HTMLFormElementImpl::attach(KHTMLView *_view)
{
    view = _view;
    HTMLElementImpl::attach(_view);
}

void HTMLFormElementImpl::detach()
{
    view = 0;
    HTMLElementImpl::detach();
}

void HTMLFormElementImpl::radioClicked( HTMLGenericFormElementImpl *caller )
{
    HTMLGenericFormElementImpl *current;
    for (current = formElements.first(); current; current = formElements.next())
    {
        if (current->id() == ID_INPUT &&
            static_cast<HTMLInputElementImpl*>(current)->inputType() == HTMLInputElementImpl::RADIO &&
            current != caller && current->name() == caller->name()) {
            static_cast<HTMLInputElementImpl*>(current)->setChecked(false);
        }
    }
}

void HTMLFormElementImpl::registerFormElement(HTMLGenericFormElementImpl *e)
{
    formElements.append(e);
}

void HTMLFormElementImpl::removeFormElement(HTMLGenericFormElementImpl *e)
{
    // ### make sure this get's called, when formElements get removed from
    // the document tree
    formElements.remove(e);
}


// -------------------------------------------------------------------------

HTMLGenericFormElementImpl::HTMLGenericFormElementImpl(DocumentImpl *doc, HTMLFormElementImpl *f)
    : HTMLElementImpl(doc)
{
    m_form = f;
    if (m_form)
        m_form->registerFormElement(this);

    view = 0;
    m_disabled = m_readOnly = false;

}

HTMLGenericFormElementImpl::HTMLGenericFormElementImpl(DocumentImpl *doc)
    : HTMLElementImpl(doc)
{
    m_form = getForm();
    if (m_form)
        m_form->registerFormElement(this);

    view = 0;
    m_disabled = m_readOnly = false;
}

HTMLGenericFormElementImpl::~HTMLGenericFormElementImpl()
{
    if (m_form)
        m_form->removeFormElement(this);
}

void HTMLGenericFormElementImpl::parseAttribute(AttrImpl *attr)
{
    switch(attr->attrId)
    {
    case ATTR_NAME:
        _name = attr->value();
        break;
    case ATTR_DISABLED:
        m_disabled = attr->val() != 0;
        break;
    case ATTR_READONLY:
        m_readOnly = attr->val() != 0;
        break;
    default:
        HTMLElementImpl::parseAttribute(attr);
    }
}

HTMLFormElementImpl *HTMLGenericFormElementImpl::getForm() const
{
    NodeImpl *p = parentNode();
    while(p)
    {
        if( p->id() == ID_FORM )
            return static_cast<HTMLFormElementImpl *>(p);
        p = p->parentNode();
    }
#ifdef FORMS_DEBUG
    kdDebug( 6030 ) << "couldn't find form!" << endl;
#endif
    return 0;
}


void HTMLGenericFormElementImpl::attach(KHTMLView *_view)
{
    view = _view;
    HTMLElementImpl::attach(_view);
}

void HTMLGenericFormElementImpl::detach()
{
    view = 0;
    HTMLElementImpl::detach();
}

void HTMLGenericFormElementImpl::onBlur()
{
    DOMString script = getAttribute(ATTR_ONBLUR);
    if (!script.isEmpty() && view->part()->jScriptEnabled())
        view->part()->executeScript(Node(this), script.string());
}

void HTMLGenericFormElementImpl::onFocus()
{
    // ###:-| this is called from JS _and_ from event handlers.
    // Split into two functions (BIC)
    ownerDocument()->setFocusNode(this);
    DOMString script = getAttribute(ATTR_ONFOCUS);
    if (!script.isEmpty() && view->part()->jScriptEnabled())
        view->part()->executeScript(Node(this), script.string());
}

void HTMLGenericFormElementImpl::onSelect()
{
    DOMString script = getAttribute(ATTR_ONSELECT);
    if (!script.isEmpty() && view->part()->jScriptEnabled())
        view->part()->executeScript(Node(this), script.string());
}

void HTMLGenericFormElementImpl::onChange()
{
    DOMString script = getAttribute(ATTR_ONCHANGE);
    if (!script.isEmpty() && view->part()->jScriptEnabled())
        view->part()->executeScript(Node(this), script.string());
}

void HTMLGenericFormElementImpl::setFocus(bool received)
{
    if (received)
    {
	if(m_render)
	    static_cast<RenderFormElement*>(m_render)->focus();
	onFocus();
    }
    else
    {
	if(m_render)
	    static_cast<RenderFormElement*>(m_render)->blur();
	onBlur();
    }
}

bool HTMLGenericFormElementImpl::isSelectable() const
{
    if (m_disabled)
	return false;
    return renderer()!=0;
}

// -------------------------------------------------------------------------

HTMLButtonElementImpl::HTMLButtonElementImpl(DocumentImpl *doc, HTMLFormElementImpl *f)
    : HTMLGenericFormElementImpl(doc, f)
{
    m_clicked = false;
    m_type = SUBMIT;
    m_dirty = true;
}

HTMLButtonElementImpl::HTMLButtonElementImpl(DocumentImpl *doc)
    : HTMLGenericFormElementImpl(doc)
{
    m_clicked = false;
    m_type = BUTTON;
    m_dirty = true;
}

HTMLButtonElementImpl::~HTMLButtonElementImpl()
{
}

const DOMString HTMLButtonElementImpl::nodeName() const
{
    return "BUTTON";
}

ushort HTMLButtonElementImpl::id() const
{
    return ID_BUTTON;
}

DOMString HTMLButtonElementImpl::type() const
{
    return getAttribute(ATTR_TYPE);
}

void HTMLButtonElementImpl::parseAttribute(AttrImpl *attr)
{
    switch(attr->attrId)
    {
    case ATTR_TYPE:
        if ( strcasecmp( attr->value(), "submit" ) == 0 )
            m_type = SUBMIT;
        else if ( strcasecmp( attr->value(), "reset" ) == 0 )
            m_type = RESET;
        else if ( strcasecmp( attr->value(), "button" ) == 0 )
            m_type = BUTTON;
        break;
    case ATTR_VALUE:
        m_value = attr->value();
        m_currValue = m_value.string();
        break;
    case ATTR_ACCESSKEY:
    case ATTR_ONFOCUS:
    case ATTR_ONBLUR:
        // ignore for the moment
        break;
    default:
        HTMLGenericFormElementImpl::parseAttribute(attr);
    }
}

void HTMLButtonElementImpl::mouseEventHandler( MouseEvent *ev, bool /*inside*/)
{
    if (m_type != BUTTON && (ev->type == MouseClick) || (ev->type == MouseRelease)) {
        m_clicked = true;

        if(m_form && m_type == SUBMIT) m_form->prepareSubmit();
        if(m_form && m_type == RESET) m_form->prepareReset();
    }
}

void HTMLButtonElementImpl::attach(KHTMLView *_view)
{
    HTMLElementImpl::attach(_view);
}

// -------------------------------------------------------------------------

HTMLFieldSetElementImpl::HTMLFieldSetElementImpl(DocumentImpl *doc, HTMLFormElementImpl *f)
    : HTMLGenericFormElementImpl(doc, f)
{
}

HTMLFieldSetElementImpl::HTMLFieldSetElementImpl(DocumentImpl *doc)
    : HTMLGenericFormElementImpl(doc)
{
}

HTMLFieldSetElementImpl::~HTMLFieldSetElementImpl()
{
}

const DOMString HTMLFieldSetElementImpl::nodeName() const
{
    return "FIELDSET";
}

ushort HTMLFieldSetElementImpl::id() const
{
    return ID_FIELDSET;
}

// -------------------------------------------------------------------------

HTMLInputElementImpl::HTMLInputElementImpl(DocumentImpl *doc)
    : HTMLGenericFormElementImpl(doc)
{
    init();
}

HTMLInputElementImpl::HTMLInputElementImpl(DocumentImpl *doc, HTMLFormElementImpl *f)
    : HTMLGenericFormElementImpl(doc, f)
{
    init();
}

void HTMLInputElementImpl::init()
{
    m_type = TEXT;
    m_maxLen = -1;
    m_size = 20;
    m_clicked = false;
    m_defaultChecked = false;
    m_checked = false;
    m_filename = "";
    m_haveType = false;
    m_firstAttach = true;

    view = 0;
}

HTMLInputElementImpl::~HTMLInputElementImpl()
{
}

const DOMString HTMLInputElementImpl::nodeName() const
{
    return "INPUT";
}

ushort HTMLInputElementImpl::id() const
{
    return ID_INPUT;
}

DOMString HTMLInputElementImpl::type() const
{
    // needs to be lowercase according to DOM spec
    switch (m_type) {
    case TEXT: return "text";
    case PASSWORD: return "password";
    case CHECKBOX: return "checkbox";
    case RADIO: return "radio";
    case SUBMIT: return "submit";
    case RESET: return "reset";
    case FILE: return "file";
    case HIDDEN: return "hidden";
    case IMAGE: return "image";
    case BUTTON: return "button";
    default: return "";
    }
}

QString HTMLInputElementImpl::state( )
{
    switch (m_type) {
    case TEXT:
    case PASSWORD:
        return m_value.string()+'.'; // Make sure the string is not empty!
    case CHECKBOX:
        return QString::fromLatin1(m_checked ? "on" : "off");
    case RADIO:
        return QString::fromLatin1(m_checked ? "on" : "off");
    case FILE:
        return m_filename.string()+'.';
    default:
        return QString::null;
    }
}



void HTMLInputElementImpl::restoreState(const QString &state)
{
    switch (m_type) {
    case TEXT:
    case PASSWORD:
        m_value = DOMString(state.left(state.length()-1));
        break;
    case CHECKBOX:
    case RADIO:
        m_checked = state == QString::fromLatin1("on");
        break;
    case FILE:
        m_value = m_filename = DOMString(state.left(state.length()-1));
        break;
    default:
        break;
    }
    setChanged(true);
}

void HTMLInputElementImpl::setFocus(bool received)
{
    if (m_type != IMAGE)
        HTMLGenericFormElementImpl::setFocus(received);
    else
	HTMLElementImpl::setFocus(received);
}

void HTMLInputElementImpl::select(  )
{
    if(!m_render) return;

    if (m_type == TEXT || m_type == PASSWORD)
        static_cast<RenderLineEdit*>(m_render)->select();
    else if (m_type == FILE) {
#ifdef __ATHEOS__
	printf( "Warning: HTMLInputElementImpl::select() not calling RenderFileButton::select()\n" );
#else	
        static_cast<RenderFileButton*>(m_render)->select();
#endif	
    }
}

void HTMLInputElementImpl::click(  )
{
    // ###
#ifdef FORMS_DEBUG
    kdDebug( 6030 ) << " HTMLInputElementImpl::click(  )" << endl;
#endif
}

void HTMLInputElementImpl::parseAttribute(AttrImpl *attr)
{
    // ### IMPORTANT: check that the type can't be changed after the first time
    // otherwise a javascript programmer may be able to set a text field's value
    // to something like /etc/passwd and then change it to a file field
    // ### also check that input defaults to something - text perhaps?
    switch(attr->attrId)
    {
    case ATTR_TYPE: {
            typeEnum newType;

            if ( strcasecmp( attr->value(), "text" ) == 0 )
                newType = TEXT;
            else if ( strcasecmp( attr->value(), "password" ) == 0 )
                newType = PASSWORD;
            else if ( strcasecmp( attr->value(), "checkbox" ) == 0 )
                newType = CHECKBOX;
            else if ( strcasecmp( attr->value(), "radio" ) == 0 )
                newType = RADIO;
            else if ( strcasecmp( attr->value(), "submit" ) == 0 )
                newType = SUBMIT;
            else if ( strcasecmp( attr->value(), "reset" ) == 0 )
                newType = RESET;
            else if ( strcasecmp( attr->value(), "file" ) == 0 )
                newType = FILE;
            else if ( strcasecmp( attr->value(), "hidden" ) == 0 )
                newType = HIDDEN;
            else if ( strcasecmp( attr->value(), "image" ) == 0 )
                newType = IMAGE;
            else if ( strcasecmp( attr->value(), "button" ) == 0 )
                newType = BUTTON;
            else if ( strcasecmp( attr->value(), "khtml_isindex" ) == 0 )
                newType = ISINDEX;
            else
                newType = TEXT;

            if (!m_haveType) {
                m_type = newType;
                m_haveType = true;
            }
            else if (m_type != newType) {
                setAttribute(ATTR_TYPE,type());
            }
        }
        break;
    case ATTR_VALUE:
        m_value = attr->value();
        break;
    case ATTR_CHECKED:
        setChecked(attr->val() != 0);
        break;
    case ATTR_MAXLENGTH:
        m_maxLen = attr->val() ? attr->val()->toInt() : -1;
        break;
    case ATTR_SIZE:
        m_size = attr->val() ? attr->val()->toInt() : 20;
        break;
    case ATTR_SRC:
        m_src = khtml::parseURL(attr->value());
        break;
    case ATTR_ALT:
    case ATTR_USEMAP:
    case ATTR_ACCESSKEY:
        // ### ignore for the moment
        break;
    case ATTR_ALIGN:
        addCSSProperty(CSS_PROP_TEXT_ALIGN, attr->value() );
        break;
    case ATTR_WIDTH:
        if(getAttribute(ATTR_SIZE).isEmpty())
            addCSSLength(CSS_PROP_WIDTH, attr->value() );
        break;
    case ATTR_HEIGHT:
        addCSSLength(CSS_PROP_HEIGHT, attr->value() );
        break;

    default:
        HTMLGenericFormElementImpl::parseAttribute(attr);
    }
}

void HTMLInputElementImpl::attach(KHTMLView *_view)
{
    setStyle(document->styleSelector()->styleForElement(this));
    view = _view;

    if(m_firstAttach) {
        m_defaultChecked = m_checked;
        m_defaultValue = m_value;
        m_firstAttach = false;
    }

    khtml::RenderObject *r = _parent ? _parent->renderer() : 0;
    if(r)
    {
        switch(m_type)
        {
        case TEXT:
        case PASSWORD:
	    case ISINDEX:
            m_render = new RenderLineEdit(view, this);
            break;
        case CHECKBOX:
            m_render = new RenderCheckBox(view, this);
            break;
        case RADIO:
            m_render = new RenderRadioButton(view, this);
            break;
        case SUBMIT:
            m_render = new RenderSubmitButton(view, this);
            break;
        case IMAGE:
        {
            m_render = new RenderImageButton(this);
            setHasEvents();
            break;
        }
        case RESET:
            m_render = new RenderResetButton(view, this);
            break;
#ifndef __ATHEOS__	    
        case FILE:
            m_render = new RenderFileButton(view, this);
            break;
#endif	    
        case HIDDEN:
            m_render = 0;
            break;
        case BUTTON:
            m_render = new RenderPushButton(view, this);
            break;
        }

        if (m_render)
        {
            m_render->setStyle(m_style);
#ifdef FORMS_DEBUG
            kdDebug( 6030 ) << "adding " << m_render->renderName() << " as child of " << r->renderName() << endl;
#endif
            QString state = document->registerElement(this);
            if ( !state.isEmpty())
            {
#ifdef FORMS_DEBUG
                kdDebug( 6030 ) << "Restoring InputElem name=" << _name.string() <<
                    " state=" << state << endl;
#endif
                restoreState( state );
            }

            r->addChild(m_render, _next ? _next->renderer() : 0);
        }
    }
    NodeBaseImpl::attach(_view);

    if (m_render && m_type == IMAGE) {
        static_cast<RenderImageButton*>
            (m_render)->setImageUrl(m_src,
                                    static_cast<HTMLDocumentImpl *>(document)->baseURL(),
                                    static_cast<HTMLDocumentImpl *>(document)->docLoader());

    }
}


bool HTMLInputElementImpl::encoding(const QTextCodec* codec, khtml::encodingList& encoding, bool multipart)
{
    // image generates its own name's
    if (_name.isEmpty() && m_type != IMAGE && m_type != ISINDEX) return false;

    // IMAGE needs special handling later
    if(m_type != IMAGE && m_type != ISINDEX)
        encoding += fixUpfromUnicode(codec, _name.string().stripWhiteSpace());

    switch (m_type) {
        case HIDDEN:
        case TEXT:
        case PASSWORD:
            // always successful
            encoding += fixUpfromUnicode(codec, m_value.string());
            return true;
        case CHECKBOX:

            if( checked() )
            {
                encoding += ( m_value.isNull() ? QCString("on") : fixUpfromUnicode(codec, m_value.string()));
                return true;
            }
            break;

        case RADIO:

            if( checked() )
            {
                encoding += fixUpfromUnicode(codec, m_value.string());
                return true;
            }
            break;

        case BUTTON:
        case RESET:
            // those buttons are never successful
            return false;

        case IMAGE:

            if(m_clicked && clickX() != -1)
            {
                m_clicked = false;
                QString astr(_name.isEmpty() ? QString::fromLatin1("x") : _name.string().stripWhiteSpace() + ".x");

                encoding += fixUpfromUnicode(codec, astr);
                astr.setNum(clickX());
                encoding += fixUpfromUnicode(codec, astr);
                astr = _name.isEmpty() ? QString::fromLatin1("y") : _name.string().stripWhiteSpace() + ".y";
                encoding += fixUpfromUnicode(codec, astr);
                astr.setNum(clickY());
                encoding += fixUpfromUnicode(codec, astr);

                return true;
            }
            break;

        case SUBMIT:

            if (m_render && static_cast<RenderSubmitButton*>(m_render)->clicked())
            {
                QString enc_str = m_value.isNull() ?
                    static_cast<RenderSubmitButton*>(m_render)->defaultLabel() : value().string();

                if (m_render)
                    static_cast<RenderSubmitButton*>(m_render)->setClicked(false);

                if(!enc_str.isEmpty())
                {
                    encoding += fixUpfromUnicode(codec, enc_str);
                    return true;
                }
            }
            break;

#ifndef __ATHEOS__
        case FILE:
        {
            // can't submit file on GET
            // don't submit if display: none or display: hidden
            if(!multipart || !renderer() || !renderer()->isVisible())
                return false;

            QString local;
            QCString dummy("");

            // if no filename at all is entered, return successful, however empty
            // null would be more logical but netscape posts an empty file. argh.
            if(m_filename.isEmpty()) {
                encoding += dummy;
                return true;
            }

            KURL fileurl(m_filename.string());
            KIO::UDSEntry filestat;

            if (!KIO::NetAccess::stat(fileurl, filestat)) {
                KMessageBox::sorry(0L, i18n("Error fetching file for submission:\n%1").arg(KIO::NetAccess::lastErrorString()));
                return false;
            }

            KFileItem fileitem(filestat, fileurl, true, false);
            if(fileitem.isDir()) {
                encoding += dummy;
                return false;
            }

            if ( KIO::NetAccess::download(KURL(m_filename.string()), local) )
            {
                QFile file(local);
                if (file.open(IO_ReadOnly))
                {
                    QCString filearray(file.size()+1);
                    file.readBlock( filearray.data(), file.size());
                    filearray[filearray.size()-1] = '\0';
                    file.close();

                    encoding += filearray;
                    KIO::NetAccess::removeTempFile( local );

                    return true;
                }
                else
                {
                    KMessageBox::error(0L, i18n("Cannot open downloaded file.\nSubmit a bugreport"));
                    KIO::NetAccess::removeTempFile( local );
                    return false;
                }
            }
            else {
                KMessageBox::sorry(0L, i18n("Error fetching file for submission:\n%1").arg(KIO::NetAccess::lastErrorString()));
                return false;
            }
            break;
        }
#endif	 // __ATHEOS__
	case ISINDEX:
	    encoding += fixUpfromUnicode(codec, m_value.string());
	    return true;
    }
    return false;
}

void HTMLInputElementImpl::reset()
{
    setValue(m_defaultValue);
    setChecked(m_defaultChecked);
    if ((m_type == SUBMIT || m_type == RESET || m_type == BUTTON || m_type == IMAGE) && m_render)
        static_cast<RenderSubmitButton*>(m_render)->setClicked(false);
}

void HTMLInputElementImpl::setChecked(bool _checked)
{
    m_checked = _checked;
    if (m_type == RADIO && m_form && m_checked)
        m_form->radioClicked(this);
    setChanged(true);
}


DOMString HTMLInputElementImpl::value() const
{
    if(m_value.isNull())
        return DOMString(""); // some JS sites obviously need this

    // Readonly support for type=file
    return m_type == FILE ? m_filename : m_value;
}


void HTMLInputElementImpl::setValue(DOMString val)
{
    switch (m_type) {
    case TEXT:
    case PASSWORD:
        m_value = (val.isNull() ? DOMString("") : val);
        setChanged(true);
        break;
    case FILE:
        // sorry, can't change this!
        m_value = m_filename;
        setChanged(true);
        break;
    default:
        setAttribute(ATTR_VALUE,val);
    }
}

bool HTMLInputElementImpl::mouseEvent( int _x, int _y,
                                       int _tx, int _ty,
                                       MouseEvent *ev )
{
    bool wasPressed = pressed();
    bool ret = HTMLGenericFormElementImpl::mouseEvent(_x,_y,_tx,_ty,ev);
    if  ( m_render && m_render->style() && !m_render->style()->visiblity() == khtml::HIDDEN) {
        if (m_type == IMAGE && (ev->type == MouseClick || ((ev->type == MouseRelease) && wasPressed))) {
            xPos = _x - _tx - m_render->xPos();
            yPos = _y - _ty - m_render->yPos();
            m_clicked = true;
            if(m_form) m_form->prepareSubmit();
            return true;
        }
    }
    return ret;
}

void HTMLInputElementImpl::setOwnerDocument(DocumentImpl *_document)
{
    if (ownerDocument())
	ownerDocument()->removeElement(this);
    HTMLGenericFormElementImpl::setOwnerDocument(_document);
}

// -------------------------------------------------------------------------

HTMLLabelElementImpl::HTMLLabelElementImpl(DocumentImpl *doc, HTMLFormElementImpl *f)
    : HTMLGenericFormElementImpl(doc, f)
{
}

HTMLLabelElementImpl::HTMLLabelElementImpl(DocumentImpl *doc)
    : HTMLGenericFormElementImpl(doc)
{
}

HTMLLabelElementImpl::~HTMLLabelElementImpl()
{
}

const DOMString HTMLLabelElementImpl::nodeName() const
{
    return "LABEL";
}

ushort HTMLLabelElementImpl::id() const
{
    return ID_LABEL;
}

// -------------------------------------------------------------------------

HTMLLegendElementImpl::HTMLLegendElementImpl(DocumentImpl *doc, HTMLFormElementImpl *f)
    : HTMLGenericFormElementImpl(doc, f)
{
}

HTMLLegendElementImpl::HTMLLegendElementImpl(DocumentImpl *doc)
    : HTMLGenericFormElementImpl(doc)
{
}

HTMLLegendElementImpl::~HTMLLegendElementImpl()
{
}

const DOMString HTMLLegendElementImpl::nodeName() const
{
    return "LEGEND";
}

ushort HTMLLegendElementImpl::id() const
{
    return ID_LEGEND;
}

// -------------------------------------------------------------------------

HTMLSelectElementImpl::HTMLSelectElementImpl(DocumentImpl *doc)
    : HTMLGenericFormElementImpl(doc)
{
    m_multiple = false;
    view = 0;
    // 0 means invalid (i.e. not set)
    m_size = 0;
}

HTMLSelectElementImpl::HTMLSelectElementImpl(DocumentImpl *doc, HTMLFormElementImpl *f)
    : HTMLGenericFormElementImpl(doc, f)
{
    m_multiple = false;
    view = 0;
    // 0 means invalid (i.e. not set)
    m_size = 0;
}

ushort HTMLSelectElementImpl::id() const
{
    return ID_SELECT;
}

DOMString HTMLSelectElementImpl::type() const
{
    return (m_multiple ? "select-multiple" : "select-one");
}

long HTMLSelectElementImpl::selectedIndex() const
{
    uint i;
    for (i = 0; i < m_listItems.size(); i++) {
        if (m_listItems[i]->id() == ID_OPTION
            && static_cast<HTMLOptionElementImpl*>(m_listItems[i])->selected())
            return listToOptionIndex(int(i)); // selectedIndex is the *first* selected item; there may be others
    }
    return -1;
}

void HTMLSelectElementImpl::setSelectedIndex( long  index )
{
    // deselect all other options and select only the new one
    int listIndex;
    for (listIndex = 0; listIndex < int(m_listItems.size()); listIndex++) {
        if (m_listItems[listIndex]->id() == ID_OPTION)
            static_cast<HTMLOptionElementImpl*>(m_listItems[listIndex])->setSelected(false);
    }
    listIndex = optionToListIndex(index);
    if (listIndex >= 0)
        static_cast<HTMLOptionElementImpl*>(m_listItems[listIndex])->setSelected(true);

    setChanged(true);
}

long HTMLSelectElementImpl::length() const
{
    int len = 0;
    uint i;
    for (i = 0; i < m_listItems.size(); i++) {
        if (m_listItems[i]->id() == ID_OPTION)
            len++;
    }
    return len;
}

void HTMLSelectElementImpl::add( const HTMLElement &element, const HTMLElement &before )
{
    if(element.isNull() || element.id() != ID_OPTION)
        return;

    int exceptioncode = 0;
    insertBefore(element.handle(), before.handle(), exceptioncode );
    if (!exceptioncode)
	recalcListItems();
}

void HTMLSelectElementImpl::remove( long index )
{
    int exceptioncode = 0;
    int listIndex = optionToListIndex(index);

    if(listIndex < 0 || index >= int(m_listItems.size()))
        return; // ### what should we do ? remove the last item?

    removeChild(m_listItems[listIndex], exceptioncode);
    if( !exceptioncode )
	recalcListItems();
}

DOMString HTMLSelectElementImpl::value( )
{
    uint i;
    for (i = 0; i < m_listItems.size(); i++) {
        if (m_listItems[i]->id() == ID_OPTION
            && static_cast<HTMLOptionElementImpl*>(m_listItems[i])->selected())
            return static_cast<HTMLOptionElementImpl*>(m_listItems[i])->value();
    }
    return 0;
}

void HTMLSelectElementImpl::setValue(DOMStringImpl* value)
{
    // ### find the option with value() matching the given parameter
    // and make it the current selection.
}

QString HTMLSelectElementImpl::state( )
{
    QString state;
    QArray<HTMLGenericFormElementImpl*> items = listItems();

    int l = items.count();

    state.fill('.', l);
    for(int i = 0; i < l; i++)
        if(items[i]->id() == ID_OPTION && static_cast<HTMLOptionElementImpl*>(items[i])->selected())
            state[i] = 'X';

    return state;
}

void HTMLSelectElementImpl::restoreState(const QString &_state)
{
    recalcListItems();

    QString state = _state;
    if(!state.isEmpty() && !state.contains('X') && !m_multiple) {
        ASSERT("should not happen in restoreState!");
        state[0] = 'X';
    }

    QArray<HTMLGenericFormElementImpl*> items = listItems();

    int l = items.count();
    for(int i = 0; i < l; i++) {
        if(items[i]->id() == ID_OPTION) {
            HTMLOptionElementImpl* oe = static_cast<HTMLOptionElementImpl*>(items[i]);
            oe->setSelected(state[i] == 'X');
        }
    }

    setChanged(true);
}

NodeImpl *HTMLSelectElementImpl::insertBefore ( NodeImpl *newChild, NodeImpl *refChild, int &exceptioncode )
{
    NodeImpl *result = HTMLGenericFormElementImpl::insertBefore(newChild,refChild, exceptioncode );
    if (!exceptioncode)
	recalcListItems();
    return result;
}

NodeImpl *HTMLSelectElementImpl::replaceChild ( NodeImpl *newChild, NodeImpl *oldChild, int &exceptioncode )
{
    NodeImpl *result = HTMLGenericFormElementImpl::replaceChild(newChild,oldChild, exceptioncode);
    if( !exceptioncode )
	recalcListItems();
    return result;
}

NodeImpl *HTMLSelectElementImpl::removeChild ( NodeImpl *oldChild, int &exceptioncode )
{
    NodeImpl *result = HTMLGenericFormElementImpl::removeChild(oldChild, exceptioncode);
    if( !exceptioncode )
	recalcListItems();
    return result;
}

NodeImpl *HTMLSelectElementImpl::appendChild ( NodeImpl *newChild, int &exceptioncode )
{
    NodeImpl *result = HTMLGenericFormElementImpl::appendChild(newChild, exceptioncode);
    if( !exceptioncode )
	recalcListItems();
    setChanged(true);
    return result;
}

void HTMLSelectElementImpl::parseAttribute(AttrImpl *attr)
{
    switch(attr->attrId)
    {
    case ATTR_SIZE:
        m_size = attr->val()->toInt();
        break;
    case ATTR_MULTIPLE:
        m_multiple = (attr->val() != 0);
        break;
    case ATTR_ACCESSKEY:
        // ### ignore for the moment
        break;
    case ATTR_ONFOCUS:
    case ATTR_ONBLUR:
    case ATTR_ONSELECT:
    case ATTR_ONCHANGE:
        // ###
        break;
    default:
        HTMLGenericFormElementImpl::parseAttribute(attr);
    }
}

void HTMLSelectElementImpl::attach(KHTMLView *_view)
{
    setStyle(document->styleSelector()->styleForElement(this));
    view = _view;

    khtml::RenderObject *r = _parent->renderer();
    if(r)
    {
        RenderSelect *f = new RenderSelect(view, this);
        if (f)
        {
            m_render = f;
            m_render->setStyle(m_style);
            r->addChild(m_render, _next ? _next->renderer() : 0);
        }
    }
    NodeBaseImpl::attach(_view);
}


bool HTMLSelectElementImpl::encoding(const QTextCodec* codec, khtml::encodingList& encoded_values, bool)
{
    bool successful = false;
    QCString enc_name = fixUpfromUnicode(codec, _name.string().stripWhiteSpace());

    uint i;
    for (i = 0; i < m_listItems.size(); i++) {
        if (m_listItems[i]->id() == ID_OPTION) {
            HTMLOptionElementImpl *option = static_cast<HTMLOptionElementImpl*>(m_listItems[i]);
            if (option->selected()) {
                encoded_values += enc_name;
                if (option->value().isNull())
                    encoded_values += fixUpfromUnicode(codec, option->text().string().stripWhiteSpace());
                else
                    encoded_values += fixUpfromUnicode(codec, option->value().string());
                successful = true;
            }
        }
    }

    // ### this case should not happen. make sure that we select the first option
    // in any case. otherwise we have no consistency with the DOM interface. FIXME!
    // we return the first one if it was a combobox select
    if (!successful && !m_multiple && m_size <= 1 && m_listItems.size() &&
        (m_listItems[0]->id() == ID_OPTION) ) {
        HTMLOptionElementImpl *option = static_cast<HTMLOptionElementImpl*>(m_listItems[0]);
        encoded_values += enc_name;
        if (option->value().isNull())
            encoded_values += fixUpfromUnicode(codec, option->text().string().stripWhiteSpace());
        else
            encoded_values += fixUpfromUnicode(codec, option->value().string());
        successful = true;
    }

    return successful;
}

int HTMLSelectElementImpl::optionToListIndex(int optionIndex) const
{
    if (optionIndex < 0 || optionIndex >= int(m_listItems.size()))
        return -1;

    int listIndex = 0;
    int optionIndex2 = 0;
    for (;
         optionIndex2 < int(m_listItems.size()) && optionIndex2 <= optionIndex;
         listIndex++) { // not a typo!
        if (m_listItems[listIndex]->id() == ID_OPTION)
            optionIndex2++;
    }
    listIndex--;
    return listIndex;
}

int HTMLSelectElementImpl::listToOptionIndex(int listIndex) const
{
    if (listIndex < 0 || listIndex >= int(m_listItems.size()) ||
        m_listItems[listIndex]->id() != ID_OPTION)
        return -1;

    int optionIndex = 0; // actual index of option not counting OPTGROUP entries that may be in list
    int i;
    for (i = 0; i < listIndex; i++)
        if (m_listItems[i]->id() == ID_OPTION)
            optionIndex++;
    return optionIndex;
}

void HTMLSelectElementImpl::recalcListItems()
{
    NodeImpl* current = firstChild();
    bool inOptGroup = false;
    m_listItems.resize(0);
    bool foundSelected = false;
    while(current) {
        if (!inOptGroup && current->id() == ID_OPTGROUP && current->firstChild()) {
            // ### what if optgroup contains just comments? don't want one of no options in it...
            m_listItems.resize(m_listItems.size()+1);
            m_listItems[m_listItems.size()-1] = static_cast<HTMLGenericFormElementImpl*>(current);
            current = current->firstChild();
            inOptGroup = true;
        }
        if (current->id() == ID_OPTION) {
            m_listItems.resize(m_listItems.size()+1);
            m_listItems[m_listItems.size()-1] = static_cast<HTMLGenericFormElementImpl*>(current);
            if (foundSelected && !m_multiple && static_cast<HTMLOptionElementImpl*>(current)->selected())
                static_cast<HTMLOptionElementImpl*>(current)->setSelected(false);
            foundSelected = static_cast<HTMLOptionElementImpl*>(current)->selected();
        }
        NodeImpl *parent = current->parentNode();
        current = current->nextSibling();
        if (!current) {
            if (inOptGroup) {
                current = parent->nextSibling();
                inOptGroup = false;
            }
        }
    }
    setChanged(true);
}

void HTMLSelectElementImpl::reset()
{
    uint i;
    for (i = 0; i < m_listItems.size(); i++) {
        if (m_listItems[i]->id() == ID_OPTION) {
            HTMLOptionElementImpl *option = static_cast<HTMLOptionElementImpl*>(m_listItems[i]);
            bool selected = (!option->getAttribute(ATTR_SELECTED).isNull());
            option->setSelected(selected);
            if (!m_multiple && selected)
                return;
        }
    }
}

void HTMLSelectElementImpl::notifyOptionSelected(HTMLOptionElementImpl *selectedOption, bool selected)
{
    if (selected && !m_multiple) {
        // deselect all other options
        uint i;
        for (i = 0; i < m_listItems.size(); i++) {
            if (m_listItems[i]->id() == ID_OPTION && m_listItems[i] != selectedOption)
                static_cast<HTMLOptionElementImpl*>(m_listItems[i])->setSelected(false);
        }
    }
    if (m_render && m_render->layouted())
    {
        static_cast<RenderSelect*>(m_render)->setSelectionChanged(true);
        if(m_render->layouted())
            static_cast<RenderSelect*>(m_render)->updateSelection();
    }
    setChanged(true);
}

// -------------------------------------------------------------------------

HTMLOptGroupElementImpl::HTMLOptGroupElementImpl(DocumentImpl *doc, HTMLFormElementImpl *f)
    : HTMLGenericFormElementImpl(doc, f)
{
}

HTMLOptGroupElementImpl::HTMLOptGroupElementImpl(DocumentImpl *doc)
    : HTMLGenericFormElementImpl(doc)
{
}

HTMLOptGroupElementImpl::~HTMLOptGroupElementImpl()
{
}

ushort HTMLOptGroupElementImpl::id() const
{
    return ID_OPTGROUP;
}

NodeImpl *HTMLOptGroupElementImpl::insertBefore ( NodeImpl *newChild, NodeImpl *refChild, int &exceptioncode )
{
    NodeImpl *result = HTMLGenericFormElementImpl::insertBefore(newChild,refChild, exceptioncode);
    if ( !exceptioncode )
	recalcSelectOptions();
    return result;
}

NodeImpl *HTMLOptGroupElementImpl::replaceChild ( NodeImpl *newChild, NodeImpl *oldChild, int &exceptioncode )
{
    NodeImpl *result = HTMLGenericFormElementImpl::replaceChild(newChild,oldChild, exceptioncode);
    if(!exceptioncode)
	recalcSelectOptions();
    return result;
}

NodeImpl *HTMLOptGroupElementImpl::removeChild ( NodeImpl *oldChild, int &exceptioncode )
{
    NodeImpl *result = HTMLGenericFormElementImpl::removeChild(oldChild, exceptioncode);
    if( !exceptioncode )
	recalcSelectOptions();
    return result;
}

NodeImpl *HTMLOptGroupElementImpl::appendChild ( NodeImpl *newChild, int &exceptioncode )
{
    NodeImpl *result = HTMLGenericFormElementImpl::appendChild(newChild, exceptioncode);
    if( !exceptioncode )
	recalcSelectOptions();
    return result;
}

void HTMLOptGroupElementImpl::parseAttribute(AttrImpl *attr)
{
    HTMLGenericFormElementImpl::parseAttribute(attr);
    recalcSelectOptions();
}

void HTMLOptGroupElementImpl::recalcSelectOptions()
{
    NodeImpl *select = parentNode();
    while (select && select->id() != ID_SELECT)
        select = select->parentNode();
    if (select)
        static_cast<HTMLSelectElementImpl*>(select)->recalcListItems();
}

// -------------------------------------------------------------------------

HTMLOptionElementImpl::HTMLOptionElementImpl(DocumentImpl *doc, HTMLFormElementImpl *f)
    : HTMLGenericFormElementImpl(doc, f)
{
    m_selected = false;
}

HTMLOptionElementImpl::HTMLOptionElementImpl(DocumentImpl *doc)
    : HTMLGenericFormElementImpl(doc)
{
    m_selected = false;
}

const DOMString HTMLOptionElementImpl::nodeName() const
{
    return "OPTION";
}

ushort HTMLOptionElementImpl::id() const
{
    return ID_OPTION;
}

DOMString HTMLOptionElementImpl::text()
{
    DOMString label = getAttribute(ATTR_LABEL);
    if (label.isEmpty() && firstChild() && firstChild()->nodeType() == Node::TEXT_NODE) {
        // ### allow for comments and multiple textnodes in our children
        return firstChild()->nodeValue();
    }
    else
        return label;
}

long HTMLOptionElementImpl::index() const
{
    // ###
    return 0;
}

void HTMLOptionElementImpl::setIndex( long  )
{
    // ###
}

void HTMLOptionElementImpl::parseAttribute(AttrImpl *attr)
{
    switch(attr->attrId)
    {
    case ATTR_SELECTED:
        m_selected = (attr->val() != 0);
        break;
    case ATTR_VALUE:
        m_value = attr->value();
        break;
    default:
        HTMLGenericFormElementImpl::parseAttribute(attr);
    }
}

void HTMLOptionElementImpl::setSelected(bool _selected)
{
    if(m_selected == _selected)
        return;

    m_selected = _selected;
    HTMLSelectElementImpl *select = getSelect();
    if (select)
        select->notifyOptionSelected(this,_selected);
}

HTMLSelectElementImpl *HTMLOptionElementImpl::getSelect()
{
    NodeImpl *select = parentNode();
    while (select && select->id() != ID_SELECT)
        select = select->parentNode();
    return static_cast<HTMLSelectElementImpl*>(select);
}


// -------------------------------------------------------------------------

HTMLTextAreaElementImpl::HTMLTextAreaElementImpl(DocumentImpl *doc)
    : HTMLGenericFormElementImpl(doc)
{
    // DTD requires rows & cols be specified, but we will provide reasonable defaults
    m_rows = 3;
    m_cols = 60;
    m_wrap = ta_Virtual;
    m_value = 0;
}


HTMLTextAreaElementImpl::HTMLTextAreaElementImpl(DocumentImpl *doc, HTMLFormElementImpl *f)
    : HTMLGenericFormElementImpl(doc, f)
{
    // DTD requires rows & cols be specified, but we will provide reasonable defaults
    m_rows = 3;
    m_cols = 60;
    m_wrap = ta_Virtual;
    m_value = 0;
}

ushort HTMLTextAreaElementImpl::id() const
{
    return ID_TEXTAREA;
}

DOMString HTMLTextAreaElementImpl::type() const
{
    return "textarea";
}

QString HTMLTextAreaElementImpl::state( )
{
    // Make sure the string is not empty!
    return value().string()+'.';
}

void HTMLTextAreaElementImpl::restoreState(const QString &state)
{
    m_value = DOMString(state.left(state.length()-1));
    setChanged(true);
}

void HTMLTextAreaElementImpl::select(  )
{
    if (m_render)
        static_cast<RenderTextArea*>(m_render)->select();
    onSelect();
}

void HTMLTextAreaElementImpl::parseAttribute(AttrImpl *attr)
{
    switch(attr->attrId)
    {
    case ATTR_ROWS:
        m_rows = attr->val() ? attr->val()->toInt() : 3;
        break;
    case ATTR_COLS:
        m_cols = attr->val() ? attr->val()->toInt() : 60;
        break;
    case ATTR_WRAP:
        // virtual / physical is Netscape extension of HTML 3.0, now deprecated
        // soft/ hard / off is recommendation for HTML 4 extension by IE and NS 4
        if ( strcasecmp( attr->value(), "virtual" ) == 0  || strcasecmp( attr->value(), "soft") == 0)
            m_wrap = ta_Virtual;
        else if ( strcasecmp ( attr->value(), "physical" ) == 0 || strcasecmp( attr->value(), "hard") == 0)
            m_wrap = ta_Physical;
        else if(strcasecmp( attr->value(), "on" ) == 0)
            m_wrap = ta_Physical;
        else if(strcasecmp( attr->value(), "off") == 0)
            m_wrap = ta_NoWrap;
        break;
    case ATTR_ACCESSKEY:
        // ignore for the moment
        break;
    case ATTR_ONFOCUS:
    case ATTR_ONBLUR:
    case ATTR_ONSELECT:
    case ATTR_ONCHANGE:
        // no need to parse
        break;
    default:
        HTMLGenericFormElementImpl::parseAttribute(attr);
    }
}

void HTMLTextAreaElementImpl::attach(KHTMLView *_view)
{
    setStyle(document->styleSelector()->styleForElement(this));
    view = _view;

    khtml::RenderObject *r = _parent->renderer();
    if(r)
    {
        RenderTextArea *f = new RenderTextArea(view, this);

        if (f)
        {
            m_render = f;
            m_render->setStyle(m_style);
            r->addChild(m_render, _next ? _next->renderer() : 0);

            // registerElement and restoreState calls are in RenderTextArea::close

        }
    }
    NodeBaseImpl::attach(_view);
}

bool HTMLTextAreaElementImpl::encoding(const QTextCodec* codec, encodingList& encoding, bool)
{
    if (_name.isEmpty() || !m_render) return false;

    encoding += fixUpfromUnicode(codec, _name.string().stripWhiteSpace());
    encoding += fixUpfromUnicode(codec, value().string());

    return true;
}

void HTMLTextAreaElementImpl::reset()
{
    setValue(defaultValue());
}

DOMString HTMLTextAreaElementImpl::value()
{
    if (m_value.isNull())
        m_value = defaultValue();
    return m_value;
}

void HTMLTextAreaElementImpl::setValue(DOMString _value)
{
    m_value = _value;
    setChanged(true);
}


DOMString HTMLTextAreaElementImpl::defaultValue()
{
    DOMString val = "";
    // there may be comments - just grab the text nodes
    NodeImpl *n;
    for (n = firstChild(); n; n = n->nextSibling())
        if (n->isTextNode())
            val += static_cast<TextImpl*>(n)->data();
    return val;
}

void HTMLTextAreaElementImpl::setDefaultValue(DOMString _defaultValue)
{
    // there may be comments - remove all the text nodes and replace them with one
    QList<NodeImpl> toRemove;
    NodeImpl *n;
    for (n = firstChild(); n; n = n->nextSibling())
        if (n->isTextNode())
            toRemove.append(n);
    QListIterator<NodeImpl> it(toRemove);
    int exceptioncode;
    for (; it.current(); ++it) {
        removeChild(it.current(), exceptioncode);
    }
    insertBefore(document->createTextNode(_defaultValue),firstChild(), exceptioncode);
    setValue(_defaultValue);
}

// -------------------------------------------------------------------------

HTMLIsIndexElementImpl::HTMLIsIndexElementImpl(DocumentImpl *doc)
    : HTMLInputElementImpl(doc)
{
    m_type = TEXT;
}

HTMLIsIndexElementImpl::HTMLIsIndexElementImpl(DocumentImpl *doc, HTMLFormElementImpl *f)
    : HTMLInputElementImpl(doc, f)
{
    m_type = TEXT;
}

HTMLIsIndexElementImpl::~HTMLIsIndexElementImpl()
{
}

const DOMString HTMLIsIndexElementImpl::nodeName() const
{
    return "ISINDEX";
}

ushort HTMLIsIndexElementImpl::id() const
{
    return ID_ISINDEX;
}

void HTMLIsIndexElementImpl::parseAttribute(AttrImpl* attr)
{
    switch(attr->attrId)
    {
    case ATTR_PROMPT:
        m_prompt = attr->value();
    default:
        // don't call HTMLInputElement::parseAttribute here, as it would
        // accept attributes this element does not support
        HTMLGenericFormElementImpl::parseAttribute(attr);
    }
}

void HTMLIsIndexElementImpl::attach(KHTMLView* v)
{
    HTMLInputElementImpl::attach(v);
    // ### fix this, this is just a crude hack
    setValue(m_prompt);
}

// -------------------------------------------------------------------------

