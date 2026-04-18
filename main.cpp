#include "GraphUserInterface.h"

#include <QApplication>


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    GraphUserInterface graphUserInterface;
    graphUserInterface.setWindowTitle(QStringLiteral("Graph Algorithm Visualizer"));
    graphUserInterface.showMaximized();
    return app.exec();
}
