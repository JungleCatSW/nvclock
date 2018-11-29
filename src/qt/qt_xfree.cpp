/*!
 ************************************************
 *
 * \file    qt_xfree.cpp
 * \brief   nvclock
 * \author  Jan Prokop <jprokop@ibl.sk>
 *
 ************************************************/

#include "qt_xfree.h"

#include <qlabel.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qtextedit.h>
#include <qgroupbox.h>
#include <qwhatsthis.h>
#include <qpushbutton.h>

//*****************************************************************************

static const int_item nvagp_list[] = {
    { 0, QT_TR_NOOP("Disable AGP") },
    { 1, QT_TR_NOOP("NVIDIA's internal AGP support") },
    { 2, QT_TR_NOOP("Use AGPGART if possible") },
    { 3, QT_TR_NOOP("Use any agp support") },
    { -1, 0 },
};

static const char *nvagp_help =
QT_TR_NOOP("Please note that NVIDIA's internal AGP support cannot "
           "work if AGPGART is either statically compiled into your "
           "kernel or is built as a module, but loaded into your "
           "kernel (some distributions load AGPGART into the kernel "
           "at boot up).  Default: 3 (the default was 1 until after "
           "1.0-1251).");


static const int_item nvenable_list[] = {
    { 0, QT_TR_NOOP("Disable") },
    { 1, QT_TR_NOOP("Enable") },
    { -1, 0 },
};

static const char *nvlogo_help =
QT_TR_NOOP("Disable drawing of the NVIDIA logo splash screen at "
           "X startup.  Default: the logo is drawn.");

static const char_item nvConnectedMonitor_list[] = {
    { "CRT", QT_TR_NOOP("Cathode ray tube (CRT)") },
    { "DFP", QT_TR_NOOP("Digital flat panel (DFP)") },
    { "TV" , QT_TR_NOOP("Television (TV)") },
    { NULL, NULL},
};

static const char *nvConnectedMonitor_help =
QT_TR_NOOP("Allows you to override what the NVIDIA kernel module "
           "detects is connected to your video card.  This may "
           "be useful, for example, if you use a KVM (keyboard, "
           "video, mouse) switch and you are switched away when "
           "X is started. In such a situation, the NVIDIA kernel "
           "module can't detect what display devices are connected, "
           "and the NVIDIA X driver assumes you have a single CRT "
           "connected. If, however, you use a digital flat panel "
           "instead of a CRT, use this option to explicitly tell the "
           "NVIDIA X driver what is connected. Valid values for this "
           "option are \"CRT\" (cathode ray tube), \"DFP\" (digital flat "
           "panel), or \"TV\" (television); if using TwinView, this "
           "option may be a comma-separated list of display devices; "
           "e.g.: \"CRT, CRT\" or \"CRT, DFP\".  Default: string is NULL.");

static const char *nvTwinView_help =
QT_TR_NOOP("Enable or disable TwinView.  Please see APPENDIX I for "
           "details. Default: TwinView is disabled.");


static const char_item nvTwinViewOrient_list[] = {
    { "RightOf", QT_TR_NOOP("RightOf") },
    { "LeftOf", QT_TR_NOOP("LeftOf") },
    { "Above", QT_TR_NOOP("Above") },
    { "Below", QT_TR_NOOP("Below") },
    { "Clone", QT_TR_NOOP("Clone") },
    { NULL, NULL },
};

static const char *nvTwinViewOrient_help =
QT_TR_NOOP("Controls the relationship between the two display devices "
           "when using TwinView.  Takes one of the following values: "
           "\"RightOf\" \"LeftOf\" \"Above\" \"Below\" \"Clone\".  Please see "
           "APPENDIX I for details. Default: string is NULL.");

static const char_item nvTwinHSync_list[] = {
    { "30-50", QT_TR_NOOP("30kHz-50kHz (TV)") },
    { "30-90", QT_TR_NOOP("30kHz-90kHz") },
    { NULL, NULL },
};

static const char *nvTwinHSync_help =
QT_TR_NOOP("This option is like the HorizSync entry in the Monitor "
           "section, but is for the second monitor when using "
           "TwinView.  Please see APPENDIX I for details. Default: "
           "none. ");

static const char_item nvTwinVRefresh_list[] = {
    { "50",     QT_TR_NOOP("50Hz PAL-TV") },
    { "60",     QT_TR_NOOP("60Hz NTSC-TV") },
    { "50-75",  QT_TR_NOOP("50Hz-75Hz") },
    { "50-100", QT_TR_NOOP("50Hz-100Hz") },
    { "50-150", QT_TR_NOOP("50Hz-150Hz") },
    { NULL, NULL },
};

static const char *nvTwinVRefresh_help =
QT_TR_NOOP("This option is like the VertRefresh entry in the Monitor "
           "section, but is for the second monitor when using "
           "TwinView.  Please see APPENDIX I for details. Default: none.");

static const char_item nvTVStandard_list[] = {
    { "PAL-B", QT_TR_NOOP("PAL-B") },
    { "PAL-D", QT_TR_NOOP("PAL-D (China and North Korea)") },
    { "PAL-G", QT_TR_NOOP("PAL-G") },
    { "PAL-H", QT_TR_NOOP("PAL-H (Belgium)") },
    { "PAL-I", QT_TR_NOOP("PAL-I (Hong Kong and The United Kingdom)") },
    { "PAL-K1", QT_TR_NOOP("PAL-K1 (Guinea)") },
    { "PAL-M", QT_TR_NOOP("PAL-M (Brazil)") },
    { "PAL-N", QT_TR_NOOP("PAL-N (France, Paraguay, and Uruguay)") },
    { "PAL-NC", QT_TR_NOOP("PAL-NC (Argentina)") },
    { "NTSC-J", QT_TR_NOOP("NTSC-J (Japan)") },
    { "NTSC-M", QT_TR_NOOP("NTSC-M (Canada, USA)") },
    { NULL, NULL},
};

static const char *nvTVStandard_help =
QT_TR_NOOP("");

static const char_item nvTVFormat_list[] = {
    { "SVIDEO", QT_TR_NOOP("S-Video") },
    { "COMPOSITE", QT_TR_NOOP("Composite") },
    { NULL, NULL},
};

static const char *nvTVFormat_help =
QT_TR_NOOP("");

//*****************************************************************************

CTabXFree::CTabXFree(QWidget *parent, char *name)
    : QWidget(parent, name)
{
    QGridLayout *l = new QGridLayout(this, 2, 3, 4, 4);

    int yy = 0;

    cAGP = addCombo(tr("AGP"), yy++, this, l);
    initCombo(cAGP, nvagp_list, nvagp_help);

    cLogo = addCombo(tr("Logo"), yy++, this, l);
    initCombo(cLogo, nvenable_list, nvlogo_help);

    cMon = addCombo(tr("Connected monitor"), yy++, this, l);
    initCombo(cMon, nvConnectedMonitor_list, nvConnectedMonitor_help);

    //*** Twin View ***
    checkTwinView = new QCheckBox(tr("TwinView"), this);
    l->addWidget(checkTwinView, yy++, 0);

    QGroupBox *box = initTwinView();
    box->setEnabled(FALSE);
    l->addMultiCellWidget(box, yy, yy, 0, 1);
    connect(checkTwinView, SIGNAL(toggled(bool)), box, SLOT(setEnabled(bool)));
    yy++;

    //*** TV Out ***
    checkTVOut = new QCheckBox(tr("TV Out"), this);
    l->addWidget(checkTVOut, yy++, 0);

    box = initTVOut();
    box->setEnabled(FALSE);
    l->addMultiCellWidget(box, yy, yy, 0, 1);
    connect(checkTVOut, SIGNAL(toggled(bool)), box, SLOT(setEnabled(bool)));
    yy++;

    QPushButton *b = new QPushButton(tr("Generate file"), this);
    l->addMultiCellWidget(b, yy, yy, 0, 1);
    connect(b, SIGNAL(clicked()), SLOT(slotGenerate()));
}

//*****************************************************************************

QGroupBox*
CTabXFree::initTwinView()
{
    QGroupBox *g = new QGroupBox(tr("TwinView Setup"), this);
    QGridLayout *l = new QGridLayout(g, 4, 2, 4, 2);

    int yy = 0;
    l->addRowSpacing(yy++, 15);

    cTwinOrient = addCombo(tr("Orientation"), yy++, g, l);
    initCombo(cTwinOrient, nvTwinViewOrient_list, nvTwinViewOrient_help);

    cTwinHSync = addCombo(tr("Horizontal sync"), yy++, g, l);
    initCombo(cTwinHSync, nvTwinHSync_list, nvTwinHSync_help);

    cTwinVRefresh = addCombo(tr("Vertical refresh"), yy++, g, l);
    initCombo(cTwinVRefresh, nvTwinVRefresh_list, nvTwinVRefresh_help);

    cTwinMon = addCombo(tr("Connected monitor"), yy++, g, l);
    initCombo(cTwinMon, nvConnectedMonitor_list, nvConnectedMonitor_help);

    return(g);
}

//*****************************************************************************

QGroupBox*
CTabXFree::initTVOut()
{
    QGroupBox *g = new QGroupBox(tr("TV Out Setup"), this);
    QGridLayout *l = new QGridLayout(g, 4, 2, 4, 2);

    int yy = 0;
    l->addRowSpacing(yy++, 15);

    cTVStandard = addCombo(tr("Standard"), yy++, g, l);
    initCombo(cTVStandard, nvTVStandard_list, nvTVStandard_help);

    cTVFormat = addCombo(tr("Format"), yy++, g, l);
    initCombo(cTVFormat, nvTVFormat_list, nvTVFormat_help);

    return(g);
}

//*****************************************************************************

void
CTabXFree::initCombo(QComboBox *combo, const int_item *list, const char *txt)
{
    for(int i=0; list[i].text; i++)
        combo->insertItem(list[i].text);

    QWhatsThis::add(combo, txt);
}

//*****************************************************************************

void
CTabXFree::initCombo(QComboBox *combo, const char_item *list, const char *txt)
{
    for(int i=0; list[i].text; i++)
        combo->insertItem(list[i].text);

    QWhatsThis::add(combo, txt);
}

//*****************************************************************************

QComboBox*
CTabXFree::addCombo(const QString &label, int yy,
                    QWidget *parent, QGridLayout *gl)
{
    gl->addWidget(new QLabel(label, parent), yy, 0, AlignRight | AlignVCenter);

    QComboBox *combo = new QComboBox(parent);
    gl->addWidget(combo, yy, 1);
    return(combo);
}

//*****************************************************************************

QString
CTabXFree::line(const char *txt, const QString &value) const
{
    QString s;
    s.sprintf("Option \"%s\" \"", txt);
    return(s + value + "\"\n");
}

//*****************************************************************************

QString
CTabXFree::line(const char *txt, int value) const
{
    QString s;
    s.sprintf("Option \"%s\" ", txt);
    return(s + '"' + QString::number(value) + "\"\n");
}

//*****************************************************************************

void
CTabXFree::slotGenerate()
{
    QString output;
    output += line("NvAGP", nvagp_list[cAGP->currentItem()].value);
    output += line("NoLogo", nvenable_list[cLogo->currentItem()].value);
    output += line("ConnectedMonitor", nvConnectedMonitor_list[cMon->currentItem()].value);

    if(checkTwinView->isChecked()) {
        output += line("TwinView", "TRUE");
        output += line("TwinViewOrientation", nvTwinViewOrient_list[cTwinOrient->currentItem()].value);
        output += line("SecondMonitorHorizSync", nvTwinHSync_list[cTwinHSync->currentItem()].value);
        output += line("SecondMonitorVertRefresh", nvTwinVRefresh_list[cTwinVRefresh->currentItem()].value);
        output += line("ConnectedMonitor", nvConnectedMonitor_list[cTwinMon->currentItem()].value);
    }

    if(checkTVOut->isChecked()) {
        output += line("TVStandard", nvTVStandard_list[cTVStandard->currentItem()].value);
        output += line("TVFormat", nvTVFormat_list[cTVFormat->currentItem()].value);
    }

    static QTextEdit *textOutput = NULL;
    if(!textOutput) {
        textOutput = new QTextEdit(0);
        textOutput->setReadOnly(TRUE);
    }
    textOutput->setText(output);
    textOutput->show();
}
