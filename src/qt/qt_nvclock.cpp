/*!
 ************************************************
 *
 * \file    qt_nvclock.cpp
 * \brief   nvclock
 * \author  Jan Prokop <jprokop@ibl.sk>
 *
 ************************************************/

#include "qt_xfree.h"
#include "qt_nvclock.h"
#include "../../config.h"

#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qspinbox.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qmessagebox.h>

#include "x.xpm"
#include "nv.xpm"
#include "links.xpm"
#include "people.xpm"

//********************************************************************************

static const char *txtLinks = {
    "<qt>"
    "<b>NVClock</b><br>"
    "<ul>"
    "<li> http://www.linuxhardware.org/nvclock</li>"
    "<li> http://nvclock.sourceforge.net</li>"
    "<li> http://projects.uid0.sk/nvclock/index.html</li>"
    "</ul>"

    "<b>NVidia</b><br>"
    "<ul>"
    "<li>home page http://www.nvidia.com</li>"
    "<li>linux drivers http://www.nvidia.com/view.asp?IO=linux_display_archive</li>"
    "</ul>"

    "<b>Accelerated OpenGL games</b><ul>"
    "<li>PrBoom http://prboom.sourceforge.net/about.html</li>"
    "<li>Quake2 http://www.icculus.org/quake2</li>"
    "<li>Quake2 http://www.idsoftware.com/games/quake/quake2</li>"
    "<li>Quake3 Arena http://www.idsoftware.com/games/quake/quake3-arena</li>"
    "<li>Return To Castle Wolfrestein http://www.idsoftware.com/games/wolfenstein/rtcw</li>"
    "<li>Tux Racer http://www.tuxracer.com</li>"
    "</ul>"
    "</qt>"
};

static const char *txtAbout = {
    "<qt><b>Roderick Colenbrander thunderbird2k@gmx.nospam.net</b>"
    "<ul>"
    "<li>author of nvclock</li>"
    "<li>overclocking/pci code</li>"
    "<li>support for multiple cards</li>"
    "<li>GTK gui</li>"
    "<li>documentation</li>"
    "</ul>"
    "<b>Jan Prokop jprokop@ibl.nospam.sk</b>"
    "<ul>"
    "<li>Qt dialog</li>"
    "</ul>"
    "<b>Christian Zander phoenix@minion.nospam.de</b>"
    "<br>(Christian only worked on nvclock 0.1 and 0.2)"
    "<ul>"
    "<li>cleanups</li>"
    "<li>autoconf/automake</li>"
    "<li>restructuring</li>"
    "</ul>"
    "</qt>"
};

//********************************************************************************

CTabNVidia::CTabNVidia(QWidget *parent, const char *name)
    : QWidget(parent, name)
{
    l = new QGridLayout(this, 2, 3, 4, 4);
    l->setColStretch(1, 20);

    int yy = 0;
    addLabel(tr("Card number"), yy);
    comboCardNo = new QComboBox(this, "comboCardNo");
    l->addWidget(comboCardNo, yy++, 1);

    addLabel(tr("Memory speed"), yy);
    spinMemory = new QSpinBox(100, 1200, 1, this, "spinMemory");
    spinMemory->setSuffix("MHz");
    l->addWidget(spinMemory, yy++, 1);

    addLabel(tr("Core speed"), yy);
    spinCore = new QSpinBox(100, 1200, 1, this, "spinCore");
    spinCore->setSuffix("MHz");
    l->addWidget(spinCore, yy++, 1);

    checkDebug = new QCheckBox(tr("Debug info"), this, "checkDebug");
    l->addWidget(checkDebug, yy++, 1);
    
    QPushButton *button = new QPushButton(tr("GO"), this);
    l->addMultiCellWidget(button, 0, 2, 2, 2);
    connect(button, SIGNAL(clicked()), SLOT(slotGo()));

    connect(comboCardNo, SIGNAL(activated(int)),
            SLOT(slotLoad(int)));

    initCardInfo(yy++);
    initAGPInfo(yy++);
    initVideoBiosInfo(yy++);

    if(getCards()) slotLoad(0);
}

//********************************************************************************

void
CTabNVidia::addLabel(const QString &text, int yy)
{ l->addWidget(new QLabel(text, this), yy, 0, AlignRight | AlignVCenter); }

//********************************************************************************

void
CTabNVidia::initCardInfo(int posy)
{
#define ADD(lname, txt) \
    layout->addWidget(new QLabel(txt, cardInfo), yy, 0, Qt::AlignRight | Qt::AlignVCenter); \
    lname = new QLabel(cardInfo); \
    lname->setFont(f); \
    layout->addWidget(lname, yy, 1); yy++;

    cardInfo = new QGroupBox(tr("Card info"), this);
    l->addMultiCellWidget(cardInfo, posy, posy, 0, 2);

    int yy = 0;
    QGridLayout *layout = new QGridLayout(cardInfo, 5, 2, 4, 2);
    layout->addRowSpacing(yy++, 10);

    QFont f(font());
    f.setBold(TRUE);

    ADD(labelGPUName, tr("GPU Name"));
    ADD(labelGPUArch, tr("GPU Architecture"));
    ADD(labelType, tr("Type"));
    ADD(labelBustype, tr("Bustype"));
    ADD(labelMemorySize, tr("Memory size"));
    ADD(labelMemoryType, tr("Memory type"));
#undef ADD
}

//********************************************************************************

void
CTabNVidia::initAGPInfo(int posy)
{
#define ADD(lname, txt) \
    layout->addWidget(new QLabel(txt, agpInfo), yy, 0, Qt::AlignRight | Qt::AlignVCenter); \
    lname = new QLabel(agpInfo); \
    lname->setFont(f); \
    layout->addWidget(lname, yy, 1); yy++;

    agpInfo = new QGroupBox(tr("AGP info"), this);
    l->addMultiCellWidget(agpInfo, posy, posy, 0, 2);

    int yy = 0;
    QGridLayout *layout = new QGridLayout(agpInfo, 6, 2, 4, 2);
    layout->addRowSpacing(yy++, 10);

    QFont f(font());
    f.setBold(TRUE);

    ADD(labelAGPstatus, tr("AGP status"));
    ADD(labelSupAGPrates, tr("Supported AGP rates"));
    ADD(labelAGPrate, tr("AGP rate"));
    ADD(labelFWstatus, tr("Fast Writes"));
    ADD(labelSBAstatus, tr("Sideband Addressing"));
#undef ADD
}

//********************************************************************************

void
CTabNVidia::initVideoBiosInfo(int posy)
{
#define ADD(lname, txt) \
    layout->addWidget(new QLabel(txt, biosInfo), yy, 0, Qt::AlignRight | Qt::AlignVCenter); \
    lname = new QLabel(biosInfo); \
    lname->setFont(f); \
    layout->addWidget(lname, yy, 1); yy++;

    biosInfo = new QGroupBox(tr("BIOS Info"), this);
    l->addMultiCellWidget(biosInfo, posy, posy, 0, 2);

    int yy = 0;
    QGridLayout *layout = new QGridLayout(biosInfo, 6, 2, 4, 2);
    layout->addRowSpacing(yy++, 10);

    QFont f(font());
    f.setBold(TRUE);

    ADD(labelBiosMsg, tr("Signon message"));
    ADD(labelBiosVersion, tr("Version"));
    ADD(labelBiosPerf, tr("Performance level"));
    ADD(labelBiosMask, tr("VID mask"));
    ADD(labelBiosVLevel, tr("Voltage level"));
    //ADD(labelBios, tr(""));
#undef ADD
}

//********************************************************************************

int
CTabNVidia::getCards()
{
    QString s;
    setEnabled(nvclock.num_cards >= 0);

    if(!s.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), s);
        return(FALSE);
    }

    for(int i=0; i < nvclock.num_cards; i++)
        comboCardNo->insertItem(nvclock.card[i].card_name);

    return(TRUE);
}

//********************************************************************************

QString
CTabNVidia::getType(int type) const
{
    QString s;
    switch(type) {
    case DESKTOP: s = tr("Desktop"); break;
    case NFORCE: s = tr("NForce"); break;
    case MOBILE: s = tr("Mobile"); break;
    default: s = tr("Unknown");
    }
    return(s);
}

//********************************************************************************

void
CTabNVidia::loadCardInfo()
{
    labelGPUName->setText(nv_card->card_name);

    QString s;
    s.sprintf("NV%X %X", nv_card->get_gpu_architecture(), nv_card->get_gpu_revision());
    labelGPUArch->setText(s);

    labelType->setText(getType(nv_card->gpu));
    labelBustype->setText(nv_card->get_bus_type());
    labelMemorySize->setText(QString("%1 MB").arg(nv_card->get_memory_size()));
    labelMemoryType->setText(QString("%1 bit %2")
                             .arg(nv_card->get_memory_width())
                             .arg(nv_card->get_memory_type()));

    QToolTip::add(spinMemory, tr("%1 MHz - %2 MHz")
                  .arg(nv_card->memclk_min)
                  .arg(nv_card->memclk_max));

    QToolTip::add(spinCore, tr("%1 MHz - %2 MHz")
                  .arg(nv_card->nvclk_min)
                  .arg(nv_card->nvclk_max));
}

//********************************************************************************

void
CTabNVidia::loadAGPInfo()
{
    labelAGPstatus->setText(nv_card->get_agp_status());
    labelSupAGPrates->setText(nv_card->get_agp_supported_rates());

    QString s;
    if(nv_card->get_bus_rate() == 0) s = "-";
    else s = QString("%1X").arg(nv_card->get_bus_rate());
    labelAGPrate->setText(s);

    labelFWstatus->setText(QString("%1").arg(nv_card->get_agp_fw_status()));
    labelSBAstatus->setText(QString("%1").arg(nv_card->get_agp_sba_status()));
}

//********************************************************************************

void
CTabNVidia::loadVideoBIOSInfo()
{
    nvbios *b = nv_card->bios;

    labelBiosMsg->setText(b->signon_msg);
    labelBiosVersion->setText(b->version);

    QString s, tmp;
    for(int i=0; i<b->perf_entries; i++) {
        performance &p = b->perf_lst[i];

        if(nv_card->bios->volt_entries)
            tmp.sprintf("%d: gpu %dMHz/memory %dMHz/%.2fV\n",
                        i, p.nvclk, p.memclk, p.voltage);
        else
            tmp.sprintf("%d: %dMHz / %dMHz / %.2fV",
                        i, p.nvclk, p.memclk, p.voltage);

        s += tmp;
    }
    labelBiosPerf->setText(s);

    //*** VID Mask ***
    if(nv_card->bios->volt_entries)
        s.sprintf("VID mask: %x\n", nv_card->bios->volt_mask);
    else
        s = QString::null;
    labelBiosMask->setText(s);

    //*** Voltage level ***
    s = QString::null;
    for(int i=0; i<nv_card->bios->volt_entries; i++) {
        voltage &v = nv_card->bios->volt_lst[i];
        tmp.sprintf("%d: %.2fV, VID: %x\n", i, v.voltage, v.VID);
        s += tmp;
    }
    labelBiosVLevel->setText(s);

    //labelBios->setText(s);
}

//********************************************************************************

int
CTabNVidia::slotLoad(int card_number)
{
    float memclk, nvclk;
    int id = comboCardNo->currentItem();

    /* Set the card object to the new card */
    set_card(id);

    memclk = nv_card->get_memory_speed();
    nvclk  = nv_card->get_gpu_speed();

    spinMemory->setRange(nv_card->memclk_min, nv_card->memclk_max);
    spinCore->setRange(nv_card->nvclk_min, nv_card->nvclk_max);

    spinMemory->setValue((int)memclk);
    spinCore->setValue((int)nvclk);

    loadCardInfo();
	
	/* For now a dirty AGP check .. */
    if(qstrcmp(nv_card->get_bus_type(), "AGP") == 0)
        loadAGPInfo();
    
    if(nv_card->bios)
	loadVideoBIOSInfo();
    return(TRUE);
}

//********************************************************************************

int
CTabNVidia::slotGo()
{
    int id = comboCardNo->currentItem();

    nv_card->debug = checkDebug->isChecked();
    nv_card->set_gpu_speed(spinCore->value());
    nv_card->set_memory_speed(spinMemory->value());

    return(slotLoad(id));
}

//********************************************************************************
//********************************************************************************

CNVclock::CNVclock(QWidget *parent, const char *name)
: QTabDialog(parent, name)
{
    setCaption(tr("Linux Overclocker - NVclock QT "));
    
    setOkButton(0);
    setCancelButton(tr("&Quit"));

    CTabNVidia *tabNVidia = new CTabNVidia(this);
    addTab(tabNVidia, QPixmap(nv_xpm), tr("NVidia"));

    CTabXFree *tabXFree = new CTabXFree(this);
    addTab(tabXFree, QPixmap(x_xpm), tr("XFree"));

    QTextBrowser *browser = new QTextBrowser(this);
    browser->setText(txtLinks);
    addTab(browser, QPixmap(links_xpm), tr("Links"));

    browser = new QTextBrowser(this);
    browser->setText(txtAbout);
    addTab(browser, QPixmap(people_xpm), tr("About"));
}
