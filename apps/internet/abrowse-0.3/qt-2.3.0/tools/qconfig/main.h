#include <qmainwindow.h>
#include <qmap.h>
#include <qstringlist.h>

class ChoiceItem;
class QListViewItem;
class QListView;
class QLabel;

class Main : public QMainWindow {
    Q_OBJECT
public:
    Main();
    void loadFeatures(const QString& filename);
    void loadConfig(const QString& filename);
private:
    QMap<QString,QStringList> dependencies;
    QMap<QString,QStringList> rdependencies;
    QMap<QString,QString> section;
    QMap<QString,ChoiceItem*> item;
    QMap<QString,QListViewItem*> sectionitem;
    QStringList choices;
    QListView* lv;
    QLabel* info;

private slots:
    void showInfo(QListViewItem* i);
    void open();
    void save();
    void testAll();
};
