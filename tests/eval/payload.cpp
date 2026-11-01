// Prints the reading request the tablet would send for a page image: payload PAGE.png MODEL [EFFORT]
#include "../../src/reading.h"
#include <QCoreApplication>
#include <QFile>
#include <iostream>
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    if (argc < 3) { std::cerr << "Usage: payload PAGE.png MODEL [EFFORT]\n"; return 1; }
    QFile page(argv[1]);
    if (!page.open(QIODevice::ReadOnly)) { std::cerr << "Cannot read " << argv[1] << "\n"; return 1; }
    QJsonObject config{{"model", QString(argv[2])}, {"timezone", "Europe/Stockholm"}};
    if (argc > 3) config["effort"] = QString(argv[3]);
    std::cout << QJsonDocument(diaryWithEffort(diaryReadingPayload(page.readAll(), config), config)).toJson(QJsonDocument::Compact).toStdString();
}
