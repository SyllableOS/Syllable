#ifndef __kdialogbase_h__
#define __kdialogbase_h__

#include <kdialog.h>

// compatibility
#include <qpixmap.h>

class QVBoxLayout;
class QHBoxLayout;
class QPushButton;

class KDialogBase : public QDialog
{
    Q_OBJECT
public:
    /**
     *  @li @p Help -    Show Help-button.
     *  @li @p Default - Show Default-button.
     *  @li @p Ok -      Show Ok-button.
     *  @li @p Apply -   Show Apply-button.
     *  @li @p Try -     Show Try-button.
     *  @li @p Cancel -  Show Cancel-button.
     *  @li @p Close -   Show Close-button.
     *  @li @p User1 -   Show User define-button 1.
     *  @li @p User2 -   Show User define-button 2.
     *  @li @p User3 -   Show User define-button 3.
     *  @li @p No -      Show No-button.
     *  @li @p Yes -     Show Yes-button.
     *  @li @p Stretch - Used internally. Ignored when used in a constructor.
     */
    enum ButtonCode
    {
      Help    = 0x00000001,
      Default = 0x00000002,
      Ok      = 0x00000004,
      Apply   = 0x00000008,
      Try     = 0x00000010,
      Cancel  = 0x00000020,
      Close   = 0x00000040,
      User1   = 0x00000080,
      User2   = 0x00000100,
      User3   = 0x00000200,
      No      = 0x00000080,
      Yes     = 0x00000100,
      Stretch = 0x80000000
    };

    KDialogBase( const QString &caption, int buttonMask, ButtonCode defaultButton,
                 ButtonCode escapeButton, QWidget *parent, const char *name,
                 bool modal, bool separator,
                 const QString &user1 = QString::null,
                 const QString &user2 = QString::null,
                 const QString &user3 = QString::null
                 );

    virtual ~KDialogBase();

    void setMainWidget( QWidget *widget );
    QWidget *mainWidget() const { return m_mainWidget; }

    void enableButtonSeparator( bool state );
    void enableButtonCancel( bool ) {}

private slots:
    void slotOk();
    void slotCancel();

private:
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_buttonLayout;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;
    QWidget *m_mainWidget;
};

#endif
