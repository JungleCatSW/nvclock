/*!
 ************************************************
 *
 * \file    qt_nvclock.h
 * \brief   nvclock
 * \author  Jan Prokop <jprokop@ibl.sk>
 *
 ************************************************/

#ifndef QT_NVCLOCK
#define QT_NVCLOCK

#include <qtabdialog.h>
#include <qtextbrowser.h>

#include "nvclock.h"

class QLabel;
class QSpinBox;
class QCheckBox;
class QComboBox;
class QGroupBox;
class QGridLayout;

//------------------------------------------------------------------

class CTabNVidia : public QWidget {
    Q_OBJECT
        
public:
    CTabNVidia(QWidget *parent = NULL, const char *name = NULL);

public slots:
    int  slotGo();
    int  slotLoad(int id);
    
private:
    QComboBox   *comboCardNo;
    QSpinBox    *spinCore, *spinMemory;
    QCheckBox   *checkDebug;
    QGridLayout *l;
    QGroupBox   *cardInfo, *agpInfo, *biosInfo;

    QLabel *labelGPUName, *labelBustype, *labelMemorySize, *labelMemoryType;
    QLabel *labelAGPstatus, *labelSupAGPrates, *labelAGPrate;
    QLabel *labelFWstatus, *labelSBAstatus, *labelType, *labelGPUArch;

    QLabel *labelBiosMsg, *labelBiosVersion, *labelBiosPerf;
    QLabel *labelBiosMask, *labelBiosVLevel;

    void addLabel(const QString &text, int yy);
    void initCardInfo(int posy);
    void initAGPInfo(int posy);
    void initVideoBiosInfo(int posy);

    void loadCardInfo();
    void loadAGPInfo();
    void loadVideoBIOSInfo();

    int  getCards();
    QString getType(int type) const;
};

//------------------------------------------------------------------
              
class CNVclock : public QTabDialog {
public:
    CNVclock(QWidget *parent = NULL, const char *name = NULL);
};
#endif
