// Only interesting for Qt/Embedded

#include <qapplication.h>
#include <qscrollview.h>
#include <qfile.h>
#include <qfont.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qstringlist.h>
#include <qlistview.h>
#include <qmainwindow.h>
#include <qmessagebox.h>
#ifdef _WS_QWS_
#include <qmemorymanager_qws.h>
#endif

#include <stdlib.h>


class FontViewItem : public QListViewItem {
    QString family;
    int pointSize;
    int weight;
    bool italic;
    QFont font;

public:
    FontViewItem(const QString& f, int pt, int w, bool ital, QListView* parent) :
	QListViewItem(parent),
	family(f), pointSize(pt), weight(w), italic(ital)
    {
    }

    void renderAndSave()
    {
	font = QFont(family,pointSize,weight,italic);
#ifdef _WS_QWS_
	memorymanager->savePrerenderedFont((QMemoryManager::FontID)font.handle());
#endif
	setHeight(QFontMetrics(font).lineSpacing());
	repaint();
    }

    void paintCell( QPainter *p, const QColorGroup & cg,
                            int column, int width, int alignment )
    {
	p->setFont(font);
	QListViewItem::paintCell(p,cg,column,width,alignment);
    }

    int width( const QFontMetrics&,
                       const QListView* lv, int column) const
    {
	QFontMetrics fm(font);
	return QListViewItem::width(fm,lv,column);
    }

    QString text(int col) const
    {
	switch (col) {
	    case 0:
		return family;
	    case 1:
		return QString::number(pointSize)+"pt";
	    case 2:
		if ( weight < QFont::Normal ) {
		    return "Light";
		} else if ( weight >= QFont::Black ) {
		    return "Black";
		} else if ( weight >= QFont::Bold ) {
		    return "Bold";
		} else if ( weight >= QFont::DemiBold ) {
		    return "DemiBold";
		} else {
		    return "Normal";
		}
	    case 3:
		return italic ? "Italic" : "Roman";
	}
	return QString::null;
    }
};

class MakeQPF : public QMainWindow
{
    Q_OBJECT
    QListView* view;
public:
    MakeQPF()
    {
	view = new QListView(this);
	view->addColumn("Family");
	view->addColumn("Size");
	view->addColumn("Weight");
	view->addColumn("Style");
	setCentralWidget(view);
	QString fontdir = QString(getenv("QTDIR")) + "/etc/fonts";
	readFontDir(fontdir);

	connect(view,SIGNAL(selectionChanged(QListViewItem*)),
	    this,SLOT(renderAndSave(QListViewItem*)));
    }

    void readFontDir(const QString& fntd)
    {
	QString fontdir = fntd + "/fontdir";
	QFile fd(fontdir);
	if ( !fd.open(IO_ReadOnly) ) {
	    QMessageBox::warning(this, "Read Error",
		"<p>Cannot read "+fontdir);
	    return;
	}
	while ( !fd.atEnd() ) {
	    QString line;
	    fd.readLine(line,9999);
	    if ( line[0] != '#' ) {
		QStringList attr = QStringList::split(" ",line);
		if ( attr.count() >= 7 ) {
		    QString family = attr[0];
		    int weight = QString(attr[4]).toInt();
		    bool italic = QString(attr[3]) == "y";
		    QStringList sizes = attr[5];
		    if ( sizes[0] == "0" ) {
			if ( attr[7].isNull() )
			    sizes = QStringList::split(',',attr[6]);
			else
			    sizes = QStringList::split(',',attr[7]);
		    }
		    for (QStringList::Iterator it = sizes.begin(); it != sizes.end(); ++it) {
			int pointSize = (*it).toInt()/10;
			if ( pointSize )
			    new FontViewItem(
				family, pointSize, weight, italic, view);
		    }
		}
	    }
	}
    }

    void renderAndSave(const QString& family)
    {
	QListViewItem* c = view->firstChild();
	while ( c ) {
	    if ( c->text(0).lower() == family.lower() )
		renderAndSave(c);
	    c = c->nextSibling();
	}
    }

private slots:
    void renderAndSave(QListViewItem* i)
    {
	((FontViewItem*)i)->renderAndSave();
    }
};

int main(int argc, char** argv)
{
    QApplication app(argc, argv, QApplication::GuiServer);
    MakeQPF m;
    if ( argc > 1 ) {
	while (*++argv)
	    m.renderAndSave(*argv);
    } else {
	// Interactive
	app.setMainWidget(&m);
	m.show();
	return app.exec();
    }
}

#include "main.moc"
