#include "service/session.h"
#include "ui/dialog.h"
#include <QApplication>
#include <QCoreApplication>
#include <cstring>
int main(int argc, char **argv)
{
    if (argc == 2 && std::strcmp(argv[1],"--face-auth-worker") == 0) {
        QCoreApplication application(argc,argv);
        return face::runWorker();
    }
    QApplication application(argc,argv);
    face::FaceEnrollmentDialog dialog("test-origin","synthetic-account","1",nullptr);
    return dialog.exec();
}
