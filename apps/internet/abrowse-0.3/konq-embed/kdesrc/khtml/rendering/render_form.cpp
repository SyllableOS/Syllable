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

#include <kdebug.h>
#include <kurl.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kapp.h>

#include <qcombobox.h>
#include "misc/helper.h"

#include "dom_nodeimpl.h"
#include "dom_textimpl.h"
#include "dom_docimpl.h"

#include "html/html_formimpl.h"
#include "html/html_documentimpl.h"
#include "misc/htmlhashes.h"

#include "rendering/render_form.h"
#include "rendering/render_style.h"
#include "rendering/render_root.h"
#include <assert.h>

#include "khtmlview.h"
#include "khtml_part.h"
#include "khtml_ext.h"

using namespace khtml;

RenderFormElement::RenderFormElement(QScrollView *view,
                                     HTMLGenericFormElementImpl *element)
    : RenderWidget(view)
{
    // init RenderObject attributes
    setInline(true);   // our object is Inline

    m_element = element;
}

RenderFormElement::~RenderFormElement()
{
}

void RenderFormElement::applyLayout(int iWidth, int iHeight)
{
    if(!style()->width().isVariable())
        iWidth = style()->width().width(containingBlock()->contentWidth());

    if(!style()->height().isVariable())
        iHeight = style()->height().width(containingBlock()->contentHeight());

    if(m_widget) {
        m_widget->resize(iWidth, iHeight);
        m_widget->setEnabled(!m_element->disabled());
    }

    m_width  = iWidth  + borderLeft() + paddingLeft() + paddingRight() + borderRight();
    m_height = iHeight + borderTop() + paddingTop() + paddingBottom() + borderBottom();
}

void RenderFormElement::calcMinMaxWidth()
{
    layout();

    //kdDebug( 6040 ) << "inside RenderFormElement::calcMinMaxWidth()" << endl;

    m_minWidth = m_width;
    m_maxWidth = m_width;
}

void RenderFormElement::blur()
{
    disconnect(m_widget,SIGNAL(blurred()),this,SLOT(slotBlurred()));
    RenderWidget::blur(); // calls m_widget->clearFocus();
    connect(m_widget,SIGNAL(blurred()),this,SLOT(slotBlurred()));
}

void RenderFormElement::focus()
{
    disconnect(m_widget,SIGNAL(focused()),this,SLOT(slotFocused()));
    RenderWidget::focus(); // calls m_widget->setFocus();
    connect(m_widget,SIGNAL(focused()),this,SLOT(slotFocused()));
}

void RenderFormElement::slotBlurred()
{
    m_element->onBlur();
}

void RenderFormElement::slotFocused()
{
    m_element->onFocus();
}

void RenderFormElement::slotSelected()
{
}

void RenderFormElement::slotClicked()
{
    DOM::NodeImpl::MouseEvent dummy( 0, DOM::NodeImpl::MouseClick );
    m_element->mouseEventHandler( &dummy, true);
}

void RenderFormElement::editableWidgetFocused( QWidget *widget )
{
#ifdef __ATHEOS__
    if ( widget == NULL ) widget = NULL;
//    printf( "Warning: RenderFormElement::editableWidgetFocused() not implemented\n" );
#else
    KHTMLPartBrowserExtension *ext = browserExt();
    if ( ext )
        ext->editableWidgetFocused( widget );
#endif
}

void RenderFormElement::editableWidgetBlurred( QWidget *widget )
{
#ifdef __ATHEOS__
    if ( widget == NULL ) widget = NULL;
//    printf( "Warning: RenderFormElement::editableWidgetBlurred() not implemented\n" );
#else
    KHTMLPartBrowserExtension *ext = browserExt();
    if ( ext )
        ext->editableWidgetBlurred( widget );
#endif
}

KHTMLPartBrowserExtension *RenderFormElement::browserExt() const
{
    RenderRoot *renderRoot = root();

    if ( !renderRoot )
        return 0;

    KHTMLPart *part = static_cast<KHTMLView *>( renderRoot->view() )->part();
    return static_cast<KHTMLPartBrowserExtension *>( part->browserExtension() );
}

// -------------------------------------------------------------------------

RenderButton::RenderButton(QScrollView *view,
                           HTMLGenericFormElementImpl *element)
    : RenderFormElement(view, element)
{
}

void RenderButton::layout()
{
    QSize s(0, 0);

    if(m_widget)
        s = m_widget->sizeHint();

    applyLayout(s.width(), s.height());
    if (isPositioned()) {
	calcAbsoluteHorizontal();
	calcAbsoluteVertical();
    }

    setLayouted();
}

// ------------------------------------------------------------------------------

RenderHiddenButton::RenderHiddenButton(QScrollView *view,
                                       HTMLInputElementImpl *element)
    : RenderButton(view, element)
{
}

// -------------------------------------------------------------------------------

CheckBoxWidget::CheckBoxWidget(QWidget *parent)
    : QCheckBox(parent)
{
    setAutoMask(true);
}

void CheckBoxWidget::focusInEvent(QFocusEvent* e)
{
    QCheckBox::focusInEvent(e);
    emit focused();
}

void CheckBoxWidget::focusOutEvent(QFocusEvent* e)
{
    QCheckBox::focusOutEvent(e);
    emit blurred();
}

// -------------------------------------------------------------------------------

RenderCheckBox::RenderCheckBox(QScrollView *view,
                               HTMLInputElementImpl *element)
    : RenderButton(view, element)
{
    CheckBoxWidget *b = new CheckBoxWidget(view->viewport());
    setQWidget(b, false);
    connect(b,SIGNAL(focused()),this,SLOT(slotFocused()));
    connect(b,SIGNAL(blurred()),this,SLOT(slotBlurred()));
    connect(b,SIGNAL(stateChanged(int)),this,SLOT(slotStateChanged(int)));
    connect(b,SIGNAL(clicked()), this, SLOT(slotClicked()));
}

void RenderCheckBox::layout()
{
    static_cast<QCheckBox*>(m_widget)->setChecked(static_cast<HTMLInputElementImpl*>(m_element)->checked());
    RenderButton::layout();
}

void RenderCheckBox::slotStateChanged(int state)
{
    m_element->setAttribute(ATTR_CHECKED,state == 2 ? "" : 0);
}

// -------------------------------------------------------------------------------

RadioButtonWidget::RadioButtonWidget(QWidget *parent)
    : QRadioButton(parent)
{
    setAutoMask(true);
}

void RadioButtonWidget::focusInEvent(QFocusEvent* e)
{
    QRadioButton::focusInEvent(e);
    emit focused();
}

void RadioButtonWidget::focusOutEvent(QFocusEvent* e)
{
    QRadioButton::focusOutEvent(e);
    emit blurred();
}

// -------------------------------------------------------------------------------

RenderRadioButton::RenderRadioButton(QScrollView *view,
                                     HTMLInputElementImpl *element)
    : RenderButton(view, element)
{
    RadioButtonWidget *b = new RadioButtonWidget(view->viewport());

    setQWidget(b, false);
    connect(b,SIGNAL(focused()),this,SLOT(slotFocused()));
    connect(b,SIGNAL(blurred()),this,SLOT(slotBlurred()));
    connect(b, SIGNAL(clicked()), this, SLOT(slotClicked()));
}

void RenderRadioButton::setChecked(bool checked)
{
    static_cast<QRadioButton *>(m_widget)->setChecked(checked);
}

void RenderRadioButton::slotClicked()
{
    // emit mouseClick event etc
    RenderButton::slotClicked();

    m_element->setAttribute(ATTR_CHECKED,"");

    if (m_element->ownerDocument()->isHTMLDocument())
        static_cast<HTMLDocumentImpl*>(m_element->ownerDocument())->updateRendering();
}

void RenderRadioButton::layout()
{
    static_cast<QRadioButton*>(m_widget)->setChecked(static_cast<HTMLInputElementImpl*>(m_element)->checked());
    RenderButton::layout();
}

// -------------------------------------------------------------------------------

void PushButtonWidget::focusInEvent(QFocusEvent* e)
{
    QPushButton::focusInEvent(e);
    emit focused();
}

void PushButtonWidget::focusOutEvent(QFocusEvent* e)
{
    QPushButton::focusOutEvent(e);
    emit blurred();
}

// -------------------------------------------------------------------------------


RenderSubmitButton::RenderSubmitButton(QScrollView *view, HTMLInputElementImpl *element)
    : RenderButton(view, element)
{
    PushButtonWidget *p = new PushButtonWidget(view->viewport());
    setQWidget(p, false);
    connect(m_widget,SIGNAL(focused()),this,SLOT(slotFocused()));
    connect(m_widget,SIGNAL(blurred()),this,SLOT(slotBlurred()));

    connect(p, SIGNAL(clicked()), this, SLOT(slotClicked()));
    m_clicked = false;
}

RenderSubmitButton::~RenderSubmitButton()
{
}

void RenderSubmitButton::slotClicked()
{
    m_clicked = true;
    RenderButton::slotClicked();

    // ### if the above line calls some javascript which deletes us we will probably crash here
    if (m_element->form())
        m_element->form()->prepareSubmit();
}

void RenderSubmitButton::layout()
{
    QString value = static_cast<HTMLInputElementImpl*>(m_element)->value().isEmpty() ?
        defaultLabel() : static_cast<HTMLInputElementImpl*>(m_element)->value().string();
    value = value.visual();
    value = value.stripWhiteSpace();
    QString raw;
    for(unsigned int i = 0; i < value.length(); i++) {
        raw += value[i];
        if(value[i] == '&')
            raw += '&';
    }

    static_cast<PushButtonWidget*>(m_widget)->setText(raw);
    static_cast<PushButtonWidget*>(m_widget)->setFont(style()->font());

    RenderButton::layout();
}

QString RenderSubmitButton::defaultLabel() {
    return i18n("Submit");
}


// -------------------------------------------------------------------------------

RenderImageButton::RenderImageButton(HTMLInputElementImpl *element)
    : RenderImage()
{
    m_element = element;
}

RenderImageButton::~RenderImageButton()
{
}

// -------------------------------------------------------------------------------

RenderResetButton::RenderResetButton(QScrollView *view, HTMLInputElementImpl *element)
    : RenderSubmitButton(view, element)
{
}

RenderResetButton::~RenderResetButton()
{
}

void RenderResetButton::slotClicked()
{
    m_clicked = true;
    // don't call RenderSubmitButton:: here!
    RenderButton::slotClicked();
    if (m_element->form())
        m_element->form()->prepareReset();
}

QString RenderResetButton::defaultLabel() {
    return i18n("Reset");
}


// -------------------------------------------------------------------------------

RenderPushButton::RenderPushButton(QScrollView *view, HTMLInputElementImpl *element)
    : RenderSubmitButton(view, element)
{
}

RenderPushButton::~RenderPushButton()
{
}

void RenderPushButton::slotClicked()
{
    // DON't call RenderSubmitButton::slotClicked here!
    RenderButton::slotClicked();
}


QString RenderPushButton::defaultLabel() {
    QString s;
    return s;
}

// -------------------------------------------------------------------------------

bool LineEditWidget::event( QEvent *e )
{
#ifndef __ATHEOS__
        if ( e->type() == QEvent::AccelAvailable && isReadOnly() ) {
        QKeyEvent* ke = (QKeyEvent*) e;
        if ( ke->state() & ControlButton ) {
            switch ( ke->key() ) {
                case Key_Left:
                case Key_Right:
                case Key_Up:
                case Key_Down:
                case Key_Home:
                case Key_End:
                    ke->accept();
                default:
                break;
            }
        }
    }
#endif // __ATHEOS__
    return QLineEdit::event( e );
}

void LineEditWidget::focusInEvent(QFocusEvent* e)
{
    QLineEdit::focusInEvent(e);
    emit focused();
}

void LineEditWidget::focusOutEvent(QFocusEvent* e)
{
    QLineEdit::focusOutEvent(e);
    emit blurred();
}

void LineEditWidget::keyPressEvent(QKeyEvent* e)
{
#ifdef __ATHEOS__
    if ( e == NULL ) e = NULL;
    printf( "Warning: LineEditWidget::focusOutEvent() not emiting\n" );
#else	
    QLineEdit::keyPressEvent(e);
    emit onKeyDown();
#endif
}

void LineEditWidget::keyReleaseEvent(QKeyEvent* e)
{
#ifdef __ATHEOS__
    if ( e == NULL ) e = NULL;
    printf( "Warning: LineEditWidget::keyReleaseEvent() not emiting\n" );
#else	
    QLineEdit::keyReleaseEvent(e);
    emit onKeyUp();
#endif    
}

// -----------------------------------------------------------------------------

RenderLineEdit::RenderLineEdit(QScrollView *view, HTMLInputElementImpl *element)
    : RenderFormElement(view, element)
{
    LineEditWidget *edit = new LineEditWidget(view);
    connect(edit,SIGNAL(focused()),this,SLOT(slotFocused()));
    connect(edit,SIGNAL(blurred()),this,SLOT(slotBlurred()));
    connect(edit,SIGNAL(returnPressed()), this, SLOT(slotReturnPressed()));
    connect(edit,SIGNAL(textChanged(const QString &)),this,SLOT(slotTextChanged(const QString &)));

    if(element->inputType() == HTMLInputElementImpl::PASSWORD)
        edit->setEchoMode( QLineEdit::Password );

    setQWidget(edit, false);
}

void RenderLineEdit::slotReturnPressed()
{
    if (m_element->form())
        m_element->form()->prepareSubmit();
}

#ifdef __ATHEOS__

void RenderLineEdit::layout()
{
//    QFontMetrics fm = fontMetrics( m_widget->font() );
    QSize s;

    QLineEdit *edit = static_cast<QLineEdit*>(m_widget);
    HTMLInputElementImpl *input = static_cast<HTMLInputElementImpl*>(m_element);
    int size = input->size();
/*

    edit->constPolish();

    int h = fm.height();
    int w = fm.width( 'x' ) * (size > 0 ? size : 17); // "some"
    if ( edit->frame() ) {
        h += 8;
        // ### this is not really portable between all styles.
        // I think one should try to find a generic solution which
        // works with all possible styles. Lars.
        // ### well, it is. it's the 1:1 copy of QLineEdit::sizeHint()
        // the only reason that made me including this thingie is
        // that I cannot get a sizehint for a specific number of characters
        // in the lineedit from it. It's not my fault, it's Qt's. Dirk
        if ( m_widget->style().guiStyle() == Qt::WindowsStyle && h < 26 )
            h = 22;
        s = QSize( w + 8, h ).expandedTo( QApplication::globalStrut() );
    } else
	s = QSize( w + 4, h + 4 ).expandedTo( QApplication::globalStrut() );

    edit->blockSignals(true);
    int pos = edit->cursorPosition();*/


    edit->setText(static_cast<HTMLInputElementImpl*>(m_element)->value().string().visual());

    
/*    edit->setCursorPosition(pos);
    edit->blockSignals(false);
    */

    edit->SetMinPreferredSize( size > 0 ? size : 17 );
    s = edit->sizeHint();
    
    // ### what if maxlength goes back to 0? can we unset maxlength in the widget?
    if (input->maxLength() > 0)
        edit->setMaxLength(input->maxLength());
    edit->setReadOnly(m_element->readOnly());

    applyLayout(s.width(), s.height());
    if (isPositioned()) {
	calcAbsoluteHorizontal();
	calcAbsoluteVertical();
    }
    setLayouted();
}

#else

void RenderLineEdit::layout()
{
    QFontMetrics fm = fontMetrics( m_widget->font() );
    QSize s;

    QLineEdit *edit = static_cast<QLineEdit*>(m_widget);
    HTMLInputElementImpl *input = static_cast<HTMLInputElementImpl*>(m_element);

    int size = input->size();

    edit->constPolish();

    int h = fm.height();
    int w = fm.width( 'x' ) * (size > 0 ? size : 17); // "some"
    if ( edit->frame() ) {
        h += 8;
        // ### this is not really portable between all styles.
        // I think one should try to find a generic solution which
        // works with all possible styles. Lars.
        // ### well, it is. it's the 1:1 copy of QLineEdit::sizeHint()
        // the only reason that made me including this thingie is
        // that I cannot get a sizehint for a specific number of characters
        // in the lineedit from it. It's not my fault, it's Qt's. Dirk
        if ( m_widget->style().guiStyle() == Qt::WindowsStyle && h < 26 )
            h = 22;
        s = QSize( w + 8, h ).expandedTo( QApplication::globalStrut() );
    } else
	s = QSize( w + 4, h + 4 ).expandedTo( QApplication::globalStrut() );

    edit->blockSignals(true);
    int pos = edit->cursorPosition();
    edit->setText(static_cast<HTMLInputElementImpl*>(m_element)->value().string().visual());
    edit->setCursorPosition(pos);
    edit->blockSignals(false);

    // ### what if maxlength goes back to 0? can we unset maxlength in the widget?
    if (input->maxLength() > 0)
        edit->setMaxLength(input->maxLength());
    edit->setReadOnly(m_element->readOnly());

    applyLayout(s.width(), s.height());
    if (isPositioned()) {
	calcAbsoluteHorizontal();
	calcAbsoluteVertical();
    }
    setLayouted();
}
#endif // __ATHEOS__


void RenderLineEdit::slotTextChanged(const QString &string)
{
    HTMLInputElementImpl * e = static_cast<HTMLInputElementImpl*>(m_element);
    // No need to set changed to true, this change comes from the rendering widget
    bool wasChanged = e->changed();
    e->setValue(DOMString(string));
    e->setChanged( wasChanged );
}

void RenderLineEdit::select()
{
    static_cast<LineEditWidget*>(m_widget)->selectAll();
}

void RenderLineEdit::slotFocused()
{
    editableWidgetFocused( m_widget );
    RenderFormElement::slotFocused();
}

void RenderLineEdit::slotBlurred()
{
    editableWidgetBlurred( m_widget );
    RenderFormElement::slotBlurred();
}


// ---------------------------------------------------------------------------

RenderFieldset::RenderFieldset(QScrollView *view,
                               HTMLGenericFormElementImpl *element)
    : RenderFormElement(view, element)
{
}

RenderFieldset::~RenderFieldset()
{
}

// -----------------------------------------------------------------------------

#ifndef __ATHEOS__
FileHBoxWidget::FileHBoxWidget(QWidget* parent)
    : QHBox(parent)
{
}

// -------------------------------------------------------------------------


RenderFileButton::RenderFileButton(QScrollView *view, HTMLInputElementImpl *element)
    : RenderFormElement(view, element)
{
    QHBox *w = new FileHBoxWidget(view->viewport());

    m_edit = new LineEditWidget(w);
    m_edit->setFocusPolicy(QWidget::ClickFocus);
    connect(m_edit, SIGNAL(returnPressed()), this, SLOT(slotReturnPressed()));
    connect(m_edit, SIGNAL(textChanged(const QString &)),this,SLOT(slotTextChanged(const QString &)));
    connect(m_edit,SIGNAL(focused()),this,SLOT(slotFocused()));
    connect(m_edit,SIGNAL(blurred()),this,SLOT(slotBlurred()));
    m_button = new PushButtonWidget(i18n("Browse..."), w);

    connect(m_button,SIGNAL(clicked()),w,SIGNAL(clicked()));
    connect(m_button,SIGNAL(focused()),w,SIGNAL(focused()));
    connect(m_button,SIGNAL(blurred()),w,SIGNAL(blurred()));
    connect(w,SIGNAL(clicked()),this,SLOT(slotClicked()));
    connect(w,SIGNAL(focused()),this,SLOT(slotFocused()));
    connect(w,SIGNAL(blurred()),this,SLOT(slotBlurred()));

    if (element->maxLength() > 0) m_edit->setMaxLength(element->maxLength());

    w->setStretchFactor(m_edit, 2);
    w->setFocusProxy(m_button);

    setQWidget(w, false);
    m_haveFocus = false;
}

RenderFileButton::~RenderFileButton()
{
}

void RenderFileButton::slotClicked()
{
    QString file_name = KFileDialog::getOpenFileName(QString::null, QString::null, 0, i18n("Browse..."));
    if (!file_name.isNull()) {
        // ### truncate if > maxLength
        static_cast<HTMLInputElementImpl*>(m_element)->setFilename(DOMString(file_name));
        m_edit->setText(file_name);
    }
}

void RenderFileButton::layout( )
{
    // this is largely taken from the RenderLineEdit layout
    QFontMetrics fm = fontMetrics( m_edit->font() );
    QSize s;
    HTMLInputElementImpl *input = static_cast<HTMLInputElementImpl*>(m_element);
    int size = input->size();

    int h = fm.height();
    int w = fm.width( 'x' ) * (size > 0 ? size : 17);
    w += m_button->sizeHint().width();

    if ( m_edit->frame() ) {
        h += 8;
        if ( m_widget->style().guiStyle() == Qt::WindowsStyle && h < 26 )
            h = 22;
        s = QSize( w + 8, h );
    } else
        s = QSize( w + 4, h + 4 );

    m_edit->blockSignals(true);
    m_edit->setText(static_cast<HTMLInputElementImpl*>(m_element)->filename().string());
    m_edit->blockSignals(false);
    m_edit->setMaxLength(input->maxLength());

    m_edit->setReadOnly(m_element->readOnly());

    applyLayout(s.width(), s.height());
    if (isPositioned()) {
	calcAbsoluteHorizontal();
	calcAbsoluteVertical();
    }
    setLayouted();
}

void RenderFileButton::slotReturnPressed()
{
    if (m_element->form())
        m_element->form()->prepareSubmit();
}

void RenderFileButton::slotTextChanged(const QString &string)
{
    static_cast<HTMLInputElementImpl*>(m_element)->setFilename(DOMString(string));
}

void RenderFileButton::slotBlurred()
{
    const QObject *senderObj = sender();

    if (senderObj != m_edit && senderObj != m_button)
        return;

    if ((senderObj == m_edit && m_button->hasFocus()) ||
        (senderObj == m_button && m_edit->hasFocus()))
    {
        if ( senderObj == m_edit )
            editableWidgetBlurred( m_edit );

        m_haveFocus = true;
    }
    else {
        m_haveFocus = false;
        RenderFormElement::slotBlurred();
    }
}

void RenderFileButton::slotFocused()
{
    const QObject *senderObj = sender();

    if (senderObj != m_edit && senderObj != m_button)
        return;

    if (!m_haveFocus)
        RenderFormElement::slotFocused();

    if ( senderObj == m_edit )
        editableWidgetFocused( m_edit );

    m_haveFocus = true;
}

void RenderFileButton::select()
{
    m_edit->selectAll();
}
#endif // __ATHEOS__


// -------------------------------------------------------------------------

RenderLabel::RenderLabel(QScrollView *view,
                         HTMLGenericFormElementImpl *element)
    : RenderFormElement(view, element)
{

}

RenderLabel::~RenderLabel()
{
}


// -------------------------------------------------------------------------

RenderLegend::RenderLegend(QScrollView *view,
                           HTMLGenericFormElementImpl *element)
    : RenderFormElement(view, element)
{
}

RenderLegend::~RenderLegend()
{
}


// -------------------------------------------------------------------------------

ListBoxWidget::ListBoxWidget(QWidget *parent)
    : KListBox(parent)
{
    // ### looks broken
    //setAutoMask(true);
    connect(this, SIGNAL(pressed(QListBoxItem*)), this, SLOT(slotPressed(QListBoxItem*)));
}

void ListBoxWidget::focusInEvent(QFocusEvent* e)
{
    KListBox::focusInEvent(e);
    emit focused();
}

void ListBoxWidget::focusOutEvent(QFocusEvent* e)
{
    KListBox::focusOutEvent(e);
    emit blurred();
}

void ListBoxWidget::slotPressed(QListBoxItem* item)
{
    if(item)
        emit activated(index(item));
}
// -------------------------------------------------------------------------------

ComboBoxWidget::ComboBoxWidget(QWidget *parent)
    : KComboBox(false, parent)
{
    setAutoMask(true);
#ifndef __ATHEOS__    
    if (listBox()) listBox()->installEventFilter(this);
#endif    
}

void ComboBoxWidget::focusInEvent(QFocusEvent* e)
{
    KComboBox::focusInEvent(e);
    emit focused();
}

void ComboBoxWidget::focusOutEvent(QFocusEvent* e)
{
    KComboBox::focusOutEvent(e);
    emit blurred();
}

#ifndef __ATHEOS__    

bool ComboBoxWidget::event(QEvent *e)
{
    if (e->type()==QEvent::KeyPress)
    {
	QKeyEvent *ke = static_cast<QKeyEvent *>(e);
	switch(ke->key())
	{
	case Key_Return:
	case Key_Enter:
	    popup();
	    ke->accept();
	    return true;
	default:
	    return KComboBox::event(e);
	}
    }
    return KComboBox::event(e);
}

bool ComboBoxWidget::eventFilter(QObject *dest, QEvent *e)
{
    if (dest==listBox() &&  e->type()==QEvent::KeyPress)
    {
	QKeyEvent *ke = static_cast<QKeyEvent *>(e);
	bool forward = false;
	switch(ke->key())
	{
	case Key_Tab:
	    forward=true;
	case Key_BackTab:
	    // ugly hack. emulate popdownlistbox() (private in QComboBox)
	    // we re-use ke here to store the reference to the generated event.
	    ke = new QKeyEvent(QEvent::KeyPress, Key_Escape, 0, 0);
	    QApplication::sendEvent(dest,ke);
	    focusNextPrevChild(forward);
	    delete ke;
	    return true;
	default:
	    return KComboBox::eventFilter(dest, e);
	}
    }
    return KComboBox::eventFilter(dest, e);
}
#endif // __ATHEOS__

// -------------------------------------------------------------------------

RenderSelect::RenderSelect(QScrollView *view, HTMLSelectElementImpl *element)
    : RenderFormElement(view, element)
{
    m_multiple = element->multiple();
    m_size = QMAX(element->size(), 1);
    m_useListBox = (m_multiple || m_size > 1);

    if(m_useListBox)
        setQWidget(createListBox(), false);
    else
	setQWidget(createComboBox(), false);

    connect(m_widget, SIGNAL(activated(int)), this, SLOT(slotActivated(int)));
    m_ignoreSelectEvents = false;
}

#ifdef __ATHEOS__
void RenderSelect::layout( )
{
    // ### maintain selection properly between type/size changes, and work
    // out how to handle multiselect->singleselect (probably just select
    // first selected one)

    HTMLSelectElementImpl* f = static_cast<HTMLSelectElementImpl*>(m_element);
    m_ignoreSelectEvents = true;

    // change widget type

    bool oldMultiple = m_multiple;
    unsigned oldSize = m_size;
    bool oldListbox = m_useListBox;

    m_multiple = f->multiple();
    m_size = QMAX(f->size(), 1);
    m_useListBox = (m_multiple || m_size > 1);

    if (oldMultiple != m_multiple || oldSize != m_size) {
        if (m_useListBox != oldListbox) {
            // type of select has changed
            delete m_widget;

            if(m_useListBox)
                setQWidget(createListBox(), false);
            else
                setQWidget(createComboBox(), false);

//	    m_widget->Connect( this, ID_KHTML_FORM_SELECTION_CHANGED, ID_FORM_SELECTION_CHANGED );
            connect(m_widget,SIGNAL(activated(int)),this,SLOT(slotActivated(int)));
        }

        if (m_useListBox && oldMultiple != m_multiple) {
            static_cast<KListBox*>(m_widget)->setSelectionMode(m_multiple ? QListBox::Multi : QListBox::Single);
        }
        m_selectionChanged = true;
    }

    HTMLSelectElementImpl *select = static_cast<HTMLSelectElementImpl*>(m_element);

    // update contents listbox/combobox based on options in m_element
    // ### check if we actually need to do this
    QArray<HTMLGenericFormElementImpl*> listItems = select->listItems();
    int listIndex;

    if(m_useListBox)
        static_cast<KListBox*>(m_widget)->clear();
    else
        static_cast<KComboBox*>(m_widget)->clear();

    for (listIndex = 0; listIndex < int(listItems.size()); listIndex++) {
        if (listItems[listIndex]->id() == ID_OPTGROUP) {
            DOMString text = listItems[listIndex]->getAttribute(ATTR_LABEL);
            if (text.isNull())
                text = "";

            if(m_useListBox) {
                QListBoxText *item = new QListBoxText(QString(text.implementation()->s, text.implementation()->l).visual());
                static_cast<KListBox*>(m_widget)
                    ->insertItem(item, listIndex);
                item->setSelectable(false);
            }
            else
                static_cast<KComboBox*>(m_widget)
                    ->insertItem(QString(text.implementation()->s, text.implementation()->l).visual(), listIndex);
        }
        else if (listItems[listIndex]->id() == ID_OPTION) {
            DOMString text = static_cast<HTMLOptionElementImpl*>(listItems[listIndex])->text();
            if (text.isNull())
                text = "";
            if (listItems[listIndex]->parentNode()->id() == ID_OPTGROUP)
                text = DOMString("    ")+text;

            if(m_useListBox)
                static_cast<KListBox*>(m_widget)
                    ->insertItem(QString(text.implementation()->s, text.implementation()->l).visual(), listIndex);
            else
                static_cast<KComboBox*>(m_widget)
                    ->insertItem(QString(text.implementation()->s, text.implementation()->l).visual(), listIndex);
        }
        else
            assert(false);
        m_selectionChanged = true;
    }

    // update selection
    if (m_selectionChanged)
        updateSelection();

    if (m_useListBox) {
        // check if multiple and size was not given or invalid
        // Internet Exploder sets size to QMIN(number of elements, 4)
        // Netscape seems to simply set it to "number of elements"
        // the average of that is IMHO QMIN(number of elements, 15)
        // so I did that ;-)
        if(m_multiple && m_size < 1)
            m_size = QMIN(static_cast<KListBox*>(m_widget)->count(), 15);
    }
    else {
        // and now disable the widget in case there is no <option> given
        // ### do the same if there is only optgroups
        KComboBox* w = static_cast<KComboBox*>(m_widget);
        if(!w->count())
            w->setEnabled(false);
        // ### select the first option (unless another specified), in case the first item is an optgroup
    }

    // calculate size

    if(m_useListBox) {
        KListBox* w = static_cast<KListBox*>(m_widget);
/*
        QListBoxItem* p = w->firstItem();
        int width = 0;
        int height = 0;
        while(p) {
            width = QMAX(width, p->width(p->listBox()));
            height = QMAX(height, p->height(p->listBox()));
            p = p->next();
        }

        width += 2*w->frameWidth() + w->verticalScrollBar()->sizeHint().width();
        height = QMAX(m_size, 1)*height + 2*w->frameWidth();

        applyLayout(width, height);
	*/
        QSize s(w->sizeHint());

        applyLayout(s.width(), s.height());
    }
    else
    {
        QSize s(m_widget->sizeHint());

        applyLayout(s.width(), s.height());
    }

    setLayouted();

    m_ignoreSelectEvents = false;
}


#else // __ATHEOS__


void RenderSelect::layout( )
{
    // ### maintain selection properly between type/size changes, and work
    // out how to handle multiselect->singleselect (probably just select
    // first selected one)

    HTMLSelectElementImpl* f = static_cast<HTMLSelectElementImpl*>(m_element);
    m_ignoreSelectEvents = true;

    // change widget type

    bool oldMultiple = m_multiple;
    unsigned oldSize = m_size;
    bool oldListbox = m_useListBox;

    m_multiple = f->multiple();
    m_size = QMAX(f->size(), 1);
    m_useListBox = (m_multiple || m_size > 1);

    if (oldMultiple != m_multiple || oldSize != m_size) {
        if (m_useListBox != oldListbox) {
            // type of select has changed
            delete m_widget;

            if(m_useListBox)
                setQWidget(createListBox(), false);
            else
                setQWidget(createComboBox(), false);

            connect(m_widget,SIGNAL(activated(int)),this,SLOT(slotActivated(int)));
        }

        if (m_useListBox && oldMultiple != m_multiple) {
            static_cast<KListBox*>(m_widget)->setSelectionMode(m_multiple ? QListBox::Multi : QListBox::Single);
        }
        m_selectionChanged = true;
    }

    HTMLSelectElementImpl *select = static_cast<HTMLSelectElementImpl*>(m_element);

    // update contents listbox/combobox based on options in m_element
    // ### check if we actually need to do this
    QArray<HTMLGenericFormElementImpl*> listItems = select->listItems();
    int listIndex;

    if(m_useListBox)
        static_cast<KListBox*>(m_widget)->clear();
    else
        static_cast<KComboBox*>(m_widget)->clear();

    for (listIndex = 0; listIndex < int(listItems.size()); listIndex++) {
        if (listItems[listIndex]->id() == ID_OPTGROUP) {
            DOMString text = listItems[listIndex]->getAttribute(ATTR_LABEL);
            if (text.isNull())
                text = "";

            if(m_useListBox) {
                QListBoxText *item = new QListBoxText(QString(text.implementation()->s, text.implementation()->l).visual());
                static_cast<KListBox*>(m_widget)
                    ->insertItem(item, listIndex);
                item->setSelectable(false);
            }
            else
                static_cast<KComboBox*>(m_widget)
                    ->insertItem(QString(text.implementation()->s, text.implementation()->l).visual(), listIndex);
        }
        else if (listItems[listIndex]->id() == ID_OPTION) {
            DOMString text = static_cast<HTMLOptionElementImpl*>(listItems[listIndex])->text();
            if (text.isNull())
                text = "";
            if (listItems[listIndex]->parentNode()->id() == ID_OPTGROUP)
                text = DOMString("    ")+text;

            if(m_useListBox)
                static_cast<KListBox*>(m_widget)
                    ->insertItem(QString(text.implementation()->s, text.implementation()->l).visual(), listIndex);
            else
                static_cast<KComboBox*>(m_widget)
                    ->insertItem(QString(text.implementation()->s, text.implementation()->l).visual(), listIndex);
        }
        else
            assert(false);
        m_selectionChanged = true;
    }

    // update selection
    if (m_selectionChanged)
        updateSelection();

    if (m_useListBox) {
        // check if multiple and size was not given or invalid
        // Internet Exploder sets size to QMIN(number of elements, 4)
        // Netscape seems to simply set it to "number of elements"
        // the average of that is IMHO QMIN(number of elements, 15)
        // so I did that ;-)
        if(m_multiple && m_size < 1)
            m_size = QMIN(static_cast<KListBox*>(m_widget)->count(), 15);
    }
    else {
        // and now disable the widget in case there is no <option> given
        // ### do the same if there is only optgroups
        KComboBox* w = static_cast<KComboBox*>(m_widget);
        if(!w->count())
            w->setEnabled(false);
        // ### select the first option (unless another specified), in case the first item is an optgroup
    }

    // calculate size

    if(m_useListBox) {
        KListBox* w = static_cast<KListBox*>(m_widget);

        QListBoxItem* p = w->firstItem();
        int width = 0;
        int height = 0;
        while(p) {
            width = QMAX(width, p->width(p->listBox()));
            height = QMAX(height, p->height(p->listBox()));
            p = p->next();
        }

        width += 2*w->frameWidth() + w->verticalScrollBar()->sizeHint().width();
        height = QMAX(m_size, 1)*height + 2*w->frameWidth();

        applyLayout(width, height);
    }
    else
    {
        QSize s(m_widget->sizeHint());

        applyLayout(s.width(), s.height());
    }

    setLayouted();

    m_ignoreSelectEvents = false;

}
#endif // __ATHEOS__

void RenderSelect::close()
{
    HTMLSelectElementImpl* f = static_cast<HTMLSelectElementImpl*>(m_element);

    // Restore state
    QString state = f->ownerDocument()->registerElement(f);
    if ( !state.isEmpty())
        static_cast<HTMLSelectElementImpl*>(m_element)->restoreState( state );

    setLayouted(false);
    static_cast<HTMLSelectElementImpl*>(m_element)->recalcListItems();

    RenderFormElement::close();
}
void RenderSelect::slotActivated(int index)
{
    if (m_ignoreSelectEvents)
        return;

    m_ignoreSelectEvents = true;

    QArray<HTMLGenericFormElementImpl*> listItems = static_cast<HTMLSelectElementImpl*>(m_element)->listItems();
    index = static_cast<HTMLSelectElementImpl*>(m_element)->listToOptionIndex(index);
    if(index >= 0 && index < int(listItems.size()))
    {
        if (listItems[index]->id() == ID_OPTION) {
            if (m_useListBox)
                static_cast<HTMLOptionElementImpl*>(listItems[index])->setSelected(static_cast<KListBox*>(m_widget)->isSelected(index));
            else
                static_cast<HTMLOptionElementImpl*>(listItems[index])->setSelected(true);
        }
        else {
            // nope, sorry, can't select this
            if (m_useListBox)
                static_cast<KListBox*>(m_widget)->setCurrentItem(-1);
            else {
                // ###
            }
        }
    }

    m_ignoreSelectEvents = false;

    static_cast<HTMLSelectElementImpl*>(m_element)->onChange();
}

void RenderSelect::setOptionsChanged(bool /*ptionsChanged*/)
{
    // ###
//    m_optionsChanged = _optionsChanged;
}
ListBoxWidget *RenderSelect::createListBox()
{
    ListBoxWidget *lb = new ListBoxWidget(m_view);
    connect(lb,SIGNAL(focused()),this,SLOT(slotFocused()));
    connect(lb,SIGNAL(blurred()),this,SLOT(slotBlurred()));
    lb->setSelectionMode(m_multiple ? QListBox::Multi : QListBox::Single);
    return lb;
}

ComboBoxWidget *RenderSelect::createComboBox()
{
    ComboBoxWidget *cb = new ComboBoxWidget(m_view);
    connect(cb,SIGNAL(focused()),this,SLOT(slotFocused()));
    connect(cb,SIGNAL(blurred()),this,SLOT(slotBlurred()));
    return cb;
}

void RenderSelect::updateSelection()
{
    QArray<HTMLGenericFormElementImpl*> listItems = static_cast<HTMLSelectElementImpl*>(m_element)->listItems();
    int i;
    if (m_useListBox) {
        // if multi-select, we select only the new selected index
        KListBox *listBox = static_cast<KListBox*>(m_widget);
        for (i = 0; i < int(listItems.size()); i++)
            listBox->setSelected(i,listItems[i]->id() == ID_OPTION &&
                                static_cast<HTMLOptionElementImpl*>(listItems[i])->selected());
    }
    else {
        for (i = 0; i < int(listItems.size()); i++)
            if (listItems[i]->id() == ID_OPTION && static_cast<HTMLOptionElementImpl*>(listItems[i])->selected()) {
                static_cast<KComboBox*>(m_widget)->setCurrentItem(i);
                return;
            }
        static_cast<KComboBox*>(m_widget)->setCurrentItem(0); // ### wrong if item 0 is an optgroup
    }

    m_selectionChanged = false;
}

// -------------------------------------------------------------------------

TextAreaWidget::TextAreaWidget(int wrap, QWidget* parent)
    : QMultiLineEdit(parent)
{
#ifdef __ATHEOS__
//    printf( "Warning: TextAreaWidget::TextAreaWidget() does not set wrapping mode\n" );
#else
    if(wrap != DOM::HTMLTextAreaElementImpl::ta_NoWrap) {
        setWordWrap(QMultiLineEdit::WidgetWidth);
        clearTableFlags(Tbl_autoScrollBars);
        setTableFlags(Tbl_vScrollBar);
    }
    else {
        clearTableFlags(Tbl_autoScrollBars);
        setTableFlags(Tbl_vScrollBar | Tbl_hScrollBar);
    }
#endif // __ATHEOS__
    setAutoMask(true);
}

void TextAreaWidget::focusInEvent(QFocusEvent* e)
{
    QMultiLineEdit::focusInEvent(e);
    emit focused();
}

void TextAreaWidget::focusOutEvent(QFocusEvent* e)
{
    QMultiLineEdit::focusOutEvent(e);
    emit blurred();
}

bool TextAreaWidget::event( QEvent *e )
{
#ifndef __ATHEOS__
    if ( e->type() == QEvent::AccelAvailable && isReadOnly() ) {
        QKeyEvent* ke = (QKeyEvent*) e;
        if ( ke->state() & ControlButton ) {
            switch ( ke->key() ) {
                case Key_Left:
                case Key_Right:
                case Key_Up:
                case Key_Down:
                case Key_Home:
                case Key_End:
                    ke->accept();
                default:
                break;
            }
        }
    }
#endif // __ATHEOS__
    return QMultiLineEdit::event( e );
}

// -------------------------------------------------------------------------

// ### allow contents to be manipulated via DOM - will require updating
// of text node child

RenderTextArea::RenderTextArea(QScrollView *view, HTMLTextAreaElementImpl *element)
    : RenderFormElement(view, element)
{
    TextAreaWidget *edit = new TextAreaWidget(element->wrap(), view);
    setQWidget(edit, false);
    connect(edit,SIGNAL(textChanged()),this,SLOT(slotTextChanged()));
    connect(edit,SIGNAL(blurred()),this,SLOT(slotBlurred()));
    connect(edit,SIGNAL(focused()),this,SLOT(slotFocused()));
}




#ifdef __ATHEOS__

void RenderTextArea::layout()
{
    TextAreaWidget* w = static_cast<TextAreaWidget*>(m_widget);
    HTMLTextAreaElementImpl* f = static_cast<HTMLTextAreaElementImpl*>(m_element);

    if (!layouted()) {
	w->setReadOnly(m_element->readOnly());
//	w->blockSignals(true);
	int line, col;
	w->getCursorPosition( &line, &col );
	w->setText(f->value().string().visual());
	w->setCursorPosition( line, col );
//	w->blockSignals(false);
    }

//    QFontMetrics m = fontMetrics(w->font());

    w->SetMinPreferredSize( QMAX(f->cols(), 1), QMAX(f->rows(), 1) );
    QSize size = w->sizeHint();
    
/*    QSize size( QMAX(f->cols(), 1)*m.width('x') + w->frameWidth()*5 +
                w->verticalScrollBar()->sizeHint().width(),
                QMAX(f->rows(), 1)*m.height() + w->frameWidth()*3 +
                (w->wordWrap() == QMultiLineEdit::NoWrap ?
                 w->horizontalScrollBar()->sizeHint().height() : 0)
        );*/
    applyLayout( size.width() + 20 + 4, size.height() + 20/*(w->wordWrap() == QMultiLineEdit::NoWrap ? 20 : 0)*/ + 4 );

    setLayouted();
}

#else


void RenderTextArea::layout( )
{
    TextAreaWidget* w = static_cast<TextAreaWidget*>(m_widget);
    HTMLTextAreaElementImpl* f = static_cast<HTMLTextAreaElementImpl*>(m_element);

    if (!layouted()) {
	w->setReadOnly(m_element->readOnly());
	w->blockSignals(true);
	int line, col;
	w->getCursorPosition( &line, &col );
	w->setText(f->value().string().visual());
	w->setCursorPosition( line, col );
	w->blockSignals(false);
    }

    QFontMetrics m = fontMetrics(w->font());
    QSize size( QMAX(f->cols(), 1)*m.width('x') + w->frameWidth()*5 +
                w->verticalScrollBar()->sizeHint().width(),
                QMAX(f->rows(), 1)*m.height() + w->frameWidth()*3 +
                (w->wordWrap() == QMultiLineEdit::NoWrap ?
                 w->horizontalScrollBar()->sizeHint().height() : 0)
        );

    applyLayout(size.width(), size.height());

    setLayouted();
}
#endif // __ATHEOS__

void RenderTextArea::close( )
{
    HTMLTextAreaElementImpl *f = static_cast<HTMLTextAreaElementImpl*>(m_element);

    QString state = f->ownerDocument()->registerElement( f );
    if ( !state.isEmpty() )
        f->restoreState( state );

    if(f->firstChild() && f->firstChild()->id() == ID_TEXT && state.isEmpty())
        f->setValue(static_cast<TextImpl*>(f->firstChild())->string());

    layout();

    RenderFormElement::close();
}

QString RenderTextArea::text()
{
    return static_cast<TextAreaWidget *>(m_widget)->text();
}

void RenderTextArea::slotTextChanged()
{
    QString txt;
    HTMLTextAreaElementImpl* e = static_cast<HTMLTextAreaElementImpl*>(m_element);
    TextAreaWidget* w = static_cast<TextAreaWidget*>(m_widget);
    if(e->wrap() == DOM::HTMLTextAreaElementImpl::ta_Physical) {
        for(int i=0; i < w->numLines(); i++)
            txt += w->textLine(i) + QString::fromLatin1("\n");
    }
    else
        txt = w->text();

    // No need to set changed to true, this change comes from the rendering widget
    bool wasChanged = e->changed();
    e->setValue(txt);
    e->setChanged( wasChanged );
}

void RenderTextArea::select()
{
    static_cast<TextAreaWidget *>(m_widget)->selectAll();
}

void RenderTextArea::slotFocused()
{
    editableWidgetFocused( m_widget );
    RenderFormElement::slotFocused();
}

void RenderTextArea::slotBlurred()
{
    editableWidgetBlurred( m_widget );
    RenderFormElement::slotBlurred();
}

// ---------------------------------------------------------------------------

#include "render_form.moc"
