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
#ifndef RENDER_FORM_H
#define RENDER_FORM_H

#include "render_replaced.h"
#include "render_image.h"
#include "render_flow.h"

class QWidget;
class QMultiLineEdit;
class QScrollView;
class QLineEdit;
class QListboxItem;

#ifdef __ATHEOS__
class QKeyEvent;
#endif
#include <qmultilineedit.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qpushbutton.h>

#ifdef __ATHEOS__

#include <qlistbox.h>
#include <qcombobox.h>

#else // __ATHEOS__
#include <qhbox.h>
#include <klistbox.h>
#include <kcombobox.h>
#include "html_formimpl.h"
#endif

class KHTMLPartBrowserExtension;

namespace DOM {
    class HTMLFormElementImpl;
    class HTMLInputElementImpl;
    class HTMLSelectElementImpl;
    class HTMLGenericFormElementImpl;
    class HTMLTextAreaElementImpl;
};

namespace khtml {

class DocLoader;

// -------------------------------------------------------------------------

class RenderFormElement : public khtml::RenderWidget
{
    Q_OBJECT
public:
    RenderFormElement(QScrollView *view, DOM::HTMLGenericFormElementImpl *element);
    virtual ~RenderFormElement();

    virtual const char *renderName() const { return "RenderForm"; }

    virtual bool isRendered() const  { return true; }

    virtual void calcMinMaxWidth();

    DOM::HTMLGenericFormElementImpl *element() { return m_element; }

    virtual void blur();
    virtual void focus();

public slots:
    virtual void slotBlurred();
    virtual void slotFocused();
    virtual void slotSelected();
    virtual void slotClicked();

protected:

    void applyLayout(int iWidth, int iHeight);

    void editableWidgetFocused( QWidget *widget );
    void editableWidgetBlurred( QWidget *widget );

    DOM::HTMLGenericFormElementImpl *m_element;

private:
    KHTMLPartBrowserExtension *browserExt() const;
};


// -------------------------------------------------------------------------

// generic class for all buttons
class RenderButton : public RenderFormElement
{
    Q_OBJECT
public:
    RenderButton(QScrollView *view, DOM::HTMLGenericFormElementImpl *element);

    virtual const char *renderName() const { return "RenderButton"; }

    virtual void layout();
};

// -------------------------------------------------------------------------

class PushButtonWidget : public QPushButton
{
    Q_OBJECT
public:
    PushButtonWidget(QWidget *parent) : QPushButton(parent) {}
    PushButtonWidget(const QString &text, QWidget *parent) : QPushButton(text,parent) {}

protected:
    virtual void focusInEvent(QFocusEvent *);
    virtual void focusOutEvent(QFocusEvent *);

signals:
    void focused();
    void blurred();
};



// -------------------------------------------------------------------------

class RenderHiddenButton : public RenderButton
{
public:
    RenderHiddenButton(QScrollView *view, DOM::HTMLInputElementImpl *element);

    virtual const char *renderName() const { return "RenderHiddenButton"; }
};


// -------------------------------------------------------------------------

class RenderCheckBox : public RenderButton
{
    Q_OBJECT
public:
    RenderCheckBox(QScrollView *view, DOM::HTMLInputElementImpl *element);

    virtual const char *renderName() const { return "RenderCheckBox"; }

    virtual void layout( );
public slots:
    virtual void slotStateChanged(int state);
};

// -------------------------------------------------------------------------

class CheckBoxWidget : public QCheckBox
{
    Q_OBJECT
public:
    CheckBoxWidget(QWidget *parent);

protected:
    virtual void focusInEvent(QFocusEvent *);
    virtual void focusOutEvent(QFocusEvent *);

signals:
    void focused();
    void blurred();
};



// -------------------------------------------------------------------------

class RenderRadioButton : public RenderButton
{
    Q_OBJECT
public:
    RenderRadioButton(QScrollView *view, DOM::HTMLInputElementImpl *element);

    virtual const char *renderName() const { return "RenderRadioButton"; }

    virtual void setChecked(bool);

    virtual void layout();

 public slots:
    void slotClicked();
};

// -------------------------------------------------------------------------

class RadioButtonWidget : public QRadioButton
{
    Q_OBJECT
public:
    RadioButtonWidget(QWidget *parent);

protected:
    virtual void focusInEvent(QFocusEvent *);
    virtual void focusOutEvent(QFocusEvent *);

signals:
    void focused();
    void blurred();
};


// -------------------------------------------------------------------------

class RenderSubmitButton : public RenderButton
{
    Q_OBJECT
public:
    RenderSubmitButton(QScrollView *view, DOM::HTMLInputElementImpl *element);
    virtual ~RenderSubmitButton();

    virtual const char *renderName() const { return "RenderButton"; }

    virtual QString defaultLabel();

    virtual void layout();
    bool clicked() { return m_clicked; }
    void setClicked(bool _clicked) { m_clicked = _clicked; }

public slots:
    virtual void slotClicked();

protected:
    bool m_clicked;
};

// -------------------------------------------------------------------------

class RenderImageButton : public RenderImage
{
public:
    RenderImageButton(DOM::HTMLInputElementImpl *element);
    virtual ~RenderImageButton();

    virtual const char *renderName() const { return "RenderImageButton"; }

    DOM::HTMLInputElementImpl *m_element;
};


// -------------------------------------------------------------------------

class RenderResetButton : public RenderSubmitButton
{
public:
    RenderResetButton(QScrollView *view, DOM::HTMLInputElementImpl *element);
    virtual ~RenderResetButton();

    virtual QString defaultLabel();
    virtual void slotClicked();
};

// -------------------------------------------------------------------------

// these define <Input type=button>, and can only work with scripts

class RenderPushButton : public RenderSubmitButton
{
public:
    RenderPushButton(QScrollView *view, DOM::HTMLInputElementImpl *element);
    virtual ~RenderPushButton();

    virtual QString defaultLabel();

    virtual void slotClicked();
};

// -------------------------------------------------------------------------

class RenderLineEdit : public RenderFormElement
{
    Q_OBJECT
public:
    RenderLineEdit(QScrollView *view, DOM::HTMLInputElementImpl *element);

    virtual void layout();

    virtual const char *renderName() const { return "RenderLineEdit"; }
    void select();

public slots:
    void slotReturnPressed();
    void slotTextChanged(const QString &string);
    virtual void slotFocused();
    virtual void slotBlurred();
};

// -------------------------------------------------------------------------

class LineEditWidget : public QLineEdit
{
    Q_OBJECT
public:
    LineEditWidget(QWidget *parent) : QLineEdit(parent) {}

protected:
    virtual void focusInEvent(QFocusEvent *);
    virtual void focusOutEvent(QFocusEvent *);
    virtual void keyPressEvent(QKeyEvent *);
    virtual void keyReleaseEvent(QKeyEvent *);
    virtual bool event( QEvent *e );
signals:
    void focused();
    void blurred();
    void onKeyDown();
    void onKeyUp();
};

// -------------------------------------------------------------------------

class RenderFieldset : public RenderFormElement
{
public:
    RenderFieldset(QScrollView *view, DOM::HTMLGenericFormElementImpl *element);
    virtual ~RenderFieldset();

    virtual const char *renderName() const { return "RenderFieldSet"; }
};


// -------------------------------------------------------------------------
#ifndef __ATHEOS__
/*
class RenderFileButton : public RenderFormElement
{
    Q_OBJECT
public:
    RenderFileButton(QScrollView *view, DOM::HTMLInputElementImpl *element);
    virtual ~RenderFileButton();

    virtual const char *renderName() const { return "RenderFileButton"; }
    virtual void layout();
    void select();

public slots:
    virtual void slotClicked();
    virtual void slotReturnPressed();
    virtual void slotTextChanged(const QString &string);
    virtual void slotBlurred();
    virtual void slotFocused();

protected:
    bool m_clicked;
    bool m_haveFocus;
    QLineEdit   *m_edit;
    QPushButton *m_button;
};
*/
#endif

// -------------------------------------------------------------------------

class RenderLabel : public RenderFormElement
{
public:
    RenderLabel(QScrollView *view, DOM::HTMLGenericFormElementImpl *element);
    virtual ~RenderLabel();

    virtual const char *renderName() const { return "RenderLabel"; }
};


// -------------------------------------------------------------------------

class RenderLegend : public RenderFormElement
{
public:
    RenderLegend(QScrollView *view, DOM::HTMLGenericFormElementImpl *element);
    virtual ~RenderLegend();

    virtual const char *renderName() const { return "RenderLegend"; }
};

// -------------------------------------------------------------------------
class ListBoxWidget : public KListBox
{
    Q_OBJECT
public:
    ListBoxWidget(QWidget *parent);

protected:
    virtual void focusInEvent(QFocusEvent *);
    virtual void focusOutEvent(QFocusEvent *);

signals:
    void focused();
    void blurred();
    void activated(int);
private slots:
    void slotPressed(QListBoxItem*);
};

// -------------------------------------------------------------------------

class ComboBoxWidget : public KComboBox
{
    Q_OBJECT
public:
    ComboBoxWidget(QWidget *parent);

protected:
    virtual void focusInEvent(QFocusEvent *);
    virtual void focusOutEvent(QFocusEvent *);
#ifndef __ATHEOS__    
    virtual bool event(QEvent *);
    virtual bool eventFilter(QObject *dest, QEvent *e);
#endif
signals:
    virtual void focused();
    virtual void blurred();
};
#ifndef __ATHEOS__
/*
class FileHBoxWidget : public QHBox
{
    Q_OBJECT
public:
    FileHBoxWidget(QWidget* parent);

signals:
    void focused();
    void blurred();
    void clicked();
};
*/
#endif // __ATHEOS__

// -------------------------------------------------------------------------

class RenderSelect : public RenderFormElement
{
    Q_OBJECT
public:
    RenderSelect(QScrollView *view, DOM::HTMLSelectElementImpl *element);

    virtual const char *renderName() const { return "RenderSelect"; }

    virtual void layout();
    virtual void close( );

    void setOptionsChanged(bool _optionsChanged);

    bool selectionChanged() { return m_selectionChanged; }
    void setSelectionChanged(bool _selectionChanged) { m_selectionChanged = _selectionChanged; }

    void updateSelection();

protected:
    ListBoxWidget *createListBox();
    ComboBoxWidget *createComboBox();

    unsigned  m_size;
    bool m_multiple;
    bool m_useListBox;
    bool m_selectionChanged;
    bool m_ignoreSelectEvents;
protected slots:
    void slotActivated(int index);
};

// -------------------------------------------------------------------------

class TextAreaWidget : public QMultiLineEdit
{
    Q_OBJECT
public:
    TextAreaWidget(int wrap, QWidget* parent);
#ifndef __ATHEOS__
    QScrollBar* verticalScrollBar () const
        { return QTableView::verticalScrollBar(); };
    QScrollBar* horizontalScrollBar () const
        { return QTableView::horizontalScrollBar(); };
#endif // __ATHEOS__
    bool hasMarkedText() const { return QMultiLineEdit::hasMarkedText(); }

protected:
    virtual void focusInEvent(QFocusEvent *);
    virtual void focusOutEvent(QFocusEvent *);
    virtual bool event (QEvent *e );

signals:
    void focused();
    void blurred();
};


// -------------------------------------------------------------------------

class RenderTextArea : public RenderFormElement
{
    Q_OBJECT
public:
    RenderTextArea(QScrollView *view, DOM::HTMLTextAreaElementImpl *element);

    virtual const char *renderName() const { return "RenderTextArea"; }

    virtual void layout();
    virtual void close ( );

    QString text(); // ### remove

    void select();

protected slots:
    void slotTextChanged();
    virtual void slotFocused();
    virtual void slotBlurred();
};

// -------------------------------------------------------------------------

}; //namespace

#endif
