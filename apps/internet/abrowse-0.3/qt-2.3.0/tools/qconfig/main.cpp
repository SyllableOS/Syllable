#include <qapplication.h>
#include <stdlib.h>

#include "main.h"
#include <qfile.h>
#include <qtextstream.h>
#include <qlistview.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qmessagebox.h>
#include <qpopupmenu.h>
#include <qmenubar.h>
#include <qstatusbar.h>
#include <qsplitter.h>
#if defined(_OS_WIN32_)
#include <direct.h>
#else
#include <unistd.h>
#endif


#define FIXED_LAYOUT

class ChoiceItem : public QCheckListItem {
public:
    QString id;
    ChoiceItem(const QString& i, QListViewItem* parent) :
	QCheckListItem(parent,
	    i.mid(6), // strip "QT_NO_" as we reverse the logic
	    CheckBox),
	id(i)
    {
	setOpen(TRUE);
	label = text(0);
    }

    // We reverse the logic
    void setDefined(bool y) { setOn(!y); }
    bool isDefined() const { return !isOn(); }

    virtual void setOn(bool y)
    {
	QCheckListItem::setOn(y);
	setOpen(y);
	for (QListViewItem* i=firstChild(); i; i = i->nextSibling() ) {
	    ChoiceItem* ci = (ChoiceItem*)i; // all are ChoiceItem
	    if ( ci->isSelectable() != y ) {
		ci->setSelectable(y);
		listView()->repaintItem(ci);
	    }
	}
    }

    void paintBranches( QPainter * p, const QColorGroup & cg,
                            int w, int y, int h, GUIStyle s)
    {
	QListViewItem::paintBranches(p,cg,w,y,h,s);
    }

    void paintCell( QPainter * p, const QColorGroup & cg,
                               int column, int width, int align )
    {
	if ( !isSelectable() ) {
	    QColorGroup c = cg;
	    c.setColor(QColorGroup::Text, gray);
	    QCheckListItem::paintCell(p,c,column,width,align);
	} else {
	    QCheckListItem::paintCell(p,cg,column,width,align);
	}
    }

    void setInfo(const QString& l, const QString& d)
    {
	label = l;
	doc = d;
	setText(0,label);
    }

    QString label;

    QString info() const
    {
	return "<h2>"+label+"</h2>"+doc;
    }

private:
    QString doc;
};

Main::Main()
{
#ifdef FIXED_LAYOUT
    QHBox* horizontal = new QHBox(this);
#else
    QSplitter* horizontal = new QSplitter(this);
#endif

    lv = new QListView(horizontal);
    lv->setSorting(-1);
    lv->setRootIsDecorated(TRUE);
    lv->addColumn("ID");

    info = new QLabel(horizontal);
    info->setBackgroundMode(PaletteBase);
    info->setMargin(10);
    info->setFrameStyle(QFrame::WinPanel|QFrame::Sunken);
    info->setAlignment(AlignTop);

#ifdef FIXED_LAYOUT
    horizontal->setStretchFactor(info,2);
#endif

    connect(lv,SIGNAL(selectionChanged(QListViewItem*)),
	  this,SLOT(showInfo(QListViewItem*)));

    setCentralWidget(horizontal);

    QPopupMenu* file = new QPopupMenu( menuBar() );
    file->insertItem( "&Open",  this, SLOT(open()), CTRL+Key_O );
    file->insertItem( "&Save", this, SLOT(save()), CTRL+Key_S );
    file->insertSeparator();
    file->insertItem( "&Test all", this, SLOT(testAll()), CTRL+Key_T );
    file->insertSeparator();
    file->insertItem( "E&xit",  qApp, SLOT(quit()), CTRL+Key_Q );

    menuBar()->insertItem("&File",file);

    statusBar()->message("Ready");
}

void Main::open()
{
}

void Main::save()
{
}

void Main::testAll()
{
    QString qtdir = getenv("QTDIR");
    chdir((qtdir+"/src").ascii());
    QString c;
    for (QStringList::ConstIterator it = choices.begin(); it != choices.end(); ++it)
    {
	c += (*it);
	c += " ";
	c += dependencies[*it].join(" ");
	c += "\n";
    }
    QFile f("featurelist");
    f.open(IO_WriteOnly);
    f.writeBlock(c.ascii(),c.length());
    f.close();
    // system("./feature_size_calculator");

#if 0
    system("mv ../include/qconfig.h ../include/qconfig.h-orig");
    for (QStringList::ConstIterator it = choices.begin(); it != choices.end(); ++it)
    {
	QString choice = *it;
	QFile f("../include/qconfig.h");
	f.open(IO_WriteOnly);
	QCString s = "#define ";
	s += choice.latin1();
	s += "\n";
	f.writeBlock(s,s.length());
	f.close();
	int err = system("make");
	if ( err != 0 )
	    break;
    }
    system("mv ../include/qconfig.h-orig ../include/qconfig.h");
#endif
}


// ##### should be in QMap?
template <class K, class D>
QValueList<K> keys(QMap<K,D> map)
{
    QValueList<K> result;
    for (QMap<K,D>::ConstIterator it = map.begin(); it!=map.end(); ++it)
	result.append(it.key());
    return result;
}

void Main::loadFeatures(const QString& filename)
{
    QFile file(filename);
    if ( !file.open(IO_ReadOnly) ) {
	QMessageBox::warning(this,"Warning",
			     "Cannot open file " + filename);
	return;
    }
    QTextStream s(&file);
    QRegExp qt_no_xxx("QT_NO_[A-Z_0-9]*");
    QStringList deps;
    QString sec;
    QString lab;
    QString doc;
    bool on = FALSE;
    bool docmode = FALSE;
    QMap<QString,QString> label;
    QMap<QString,QString> documentation;
    QStringList sections;

    do {
	QString line = s.readLine();
	line.replace(QRegExp("# *define"),"#define");

	QStringList token = QStringList::split(QChar(' '),line);
	if ( on ) {
	    if ( docmode ) {
		if ( token[0] == "*/" )
		    docmode = FALSE;
		else if ( lab.isEmpty() )
		    lab = line.stripWhiteSpace();
		else
		    doc += line.simplifyWhiteSpace() + "\n";
	    } else if ( token[0] == "//#define" || token[0] == "#define" ) {
		dependencies[token[1]] = deps;
		for (QStringList::ConstIterator it = deps.begin(); it!=deps.end(); ++it)
		    rdependencies[*it].append(token[1]);
		section[token[1]] = sec;
		documentation[token[1]] = doc;
		label[token[1]] = lab;
		choices.append(token[1]);
		doc = "";
		lab = "";
	    } else if ( token[0] == "/*!" ) {
		docmode = TRUE;
	    } else if ( token[0] == "//" ) {
		token.remove(token.begin());
		sec = token.join(" ");
		sections.append(sec);
	    } else if ( token[0] == "#if" ) {
		ASSERT(deps.isEmpty());
		for (int i=1; i<(int)token.count(); i++) {
		    if ( token[i][0] == 'd' ) {
			int index;
			int len;
			index = qt_no_xxx.match(token[i],0,&len);
			if ( index >= 0 ) {
			    QString d = token[i].mid(index,len);
			    deps.append(d);
			}
		    }
		}
	    } else if ( token[0] == "#endif" ) {
		deps.clear();
	    } else if ( token[0].isEmpty() ) {
	    } else {
		qDebug("Cannot parse: %s",token.join(" ").ascii());
	    }
	} else if ( token[0] == "#include" ) {
	    on = TRUE;
	}
    } while (!s.atEnd());

    lv->clear();
    sections.sort();
    // ##### QListView default sort order is reverse of insertion order
    for (QStringList::Iterator se = sections.fromLast(); se != sections.end(); --se) {
	sectionitem[*se] = new QListViewItem(lv,*se);
    }  
    for (QStringList::Iterator ch = choices.begin(); ch != choices.end(); ++ch) {
	QStringList deps = dependencies[*ch];
	QListViewItem* parent = deps.isEmpty() ? 0 : item[deps[0]];
	if ( !parent ) {
	    parent = sectionitem[section[*ch]];
	}
	ChoiceItem* ci = new ChoiceItem(*ch,parent);
	item[*ch] = ci;
	if ( !label[*ch].isEmpty() )
	    ci->setInfo(label[*ch],documentation[*ch]);
    }

#ifdef FIXED_LAYOUT
    lv->setFixedWidth(lv->sizeHint().width());
#endif
}

void Main::loadConfig(const QString& filename)
{
    QFile file(filename);
    if ( !file.open(IO_ReadOnly) ) {
	QMessageBox::warning(this,"Warning",
			     "Cannot open file " + filename);
	return;
    }
    QTextStream s(&file);
    QRegExp qt_no_xxx("QT_NO_[A-Z_0-9]*");

    for (QStringList::Iterator ch = choices.begin(); ch != choices.end(); ++ch) {
	item[*ch]->setDefined(FALSE);
    }
    do {
	QString line = s.readLine();
	QStringList token = QStringList::split(QChar(' '),line);
	if ( token[0] == "#define" ) {
	    ChoiceItem* i = item[token[1]];
	    if ( i )
		i->setDefined(TRUE);
	    else {
		QMessageBox::warning(this,"Warning",
		    "The item " + token[1] + " is not used by qfeatures.h");
	    }
	}
    } while (!s.atEnd());
}

void Main::showInfo(QListViewItem* i)
{
    if ( !i->parent() ) {
	// section. do nothing for now
    } else {
	ChoiceItem* choice = (ChoiceItem*)i;
	QString i = choice->info();
	QStringList deps = dependencies[choice->id];
	if ( !deps.isEmpty() ) {
	    i += "<h3>Requires:</h3><ul>";
	    for (QStringList::ConstIterator it = deps.begin();
		    it != deps.end(); ++it)
	    {
		ChoiceItem* d = item[*it];
		if ( d )
		    i += "<li>"+d->label;
	    }
	    i += "</ul>";
	}
	QStringList rdeps = rdependencies[choice->id];
	if ( !rdeps.isEmpty() ) {
	    i += "<h3>Required for:</h3><ul>";
	    for (QStringList::ConstIterator it = rdeps.begin();
		    it != rdeps.end(); ++it)
	    {
		ChoiceItem* d = item[*it];
		if ( d )
		    i += "<li>"+d->label;
	    }
	    i += "</ul>";
	}
	info->setText(i);
    }
}


int main(int argc, char** argv)
{
    QApplication app(argc,argv);
    Main m;
    QString qtdir = getenv("QTDIR");
    QString qfeatures = qtdir + "/include/qfeatures.h";
    QString qconfig = qtdir + "/include/qconfig.h";
    for (int i=1; i<argc; i++) {
	QString arg = argv[i];
	if ( arg == "-f" && i+i<argc ) {
	    qfeatures = argv[++i];
	} else {
	    qconfig = argv[i];
	}
    }
    m.loadFeatures(qfeatures);
    m.loadConfig(qconfig);
    m.resize(m.sizeHint()+QSize(500,300));
    app.setMainWidget(&m);
    m.show();
    return app.exec();
}
