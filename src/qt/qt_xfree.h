/*!
 ************************************************
 *
 * \file    qt_xfree.h
 * \brief   nvclock
 * \author  Jan Prokop <jprokop@ibl.sk>
 *
 ************************************************/

#ifndef QT_XFREE
#define QT_XFREE

#include <qwidget.h>

class QCheckBox;
class QGroupBox;
class QComboBox;
class QGridLayout;

//----------------------------------------------------------------------

struct int_item {
    int value;
    const char *text;
};

struct char_item {
    const char *value;
    const char *text;
};

//----------------------------------------------------------------------

class CTabXFree : public QWidget {
    Q_OBJECT

public:
    CTabXFree(QWidget *parent = NULL, char *name = NULL);

protected slots:
    void slotGenerate();

private:
    void initCombo(QComboBox *combo, const int_item *list, const char *txt);
    void initCombo(QComboBox *combo, const char_item *list, const char *txt);

    QComboBox *addCombo(const QString &label, int yy,
                        QWidget *parent, QGridLayout *gl);

    QGroupBox *initTwinView();
    QGroupBox *initTVOut();

    QString line(const char *txt, int value) const;
    QString line(const char *txt, const QString &value) const;

    QCheckBox *checkTwinView, *checkTVOut;
    QComboBox *cAGP, *cLogo, *cMon;
    QComboBox *cTwinOrient, *cTwinHSync, *cTwinVRefresh, *cTwinMon;
    QComboBox *cTVStandard, *cTVFormat;
};
#endif
