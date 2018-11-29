/*************************************************
**      To: main.cpp
** Project: nvclock
**  Author: Jan Prokop
**  e-mail: jprokop@ibl.sk
*************************************************/

/* for va_list.... */
//#include <stdarg.h>
//#include <stdlib.h>

//#include <unistd.h>
//#include <sys/types.h>

#include "qt_nvclock.h"

#include <qmessagebox.h>
#include <qapplication.h>

//***********************************************************************

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    if(!init_nvclock()) {
        QMessageBox::critical(0, QObject::tr("Error"),
                              QObject::tr("Can't initialize !"));
        return(-1);
    }

    nvclock.dpy = qt_xdisplay(); /* small hack, to keep the QT version working */

    CNVclock nv(0, "nv");

    app.setMainWidget(&nv);
    nv.show();
    
    return(app.exec());
}
