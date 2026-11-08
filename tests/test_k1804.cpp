#include "src/core/projectstorage.h"
#include "src/elements/masterBlockElement/k1804microcode.h"

#include <QTemporaryDir>
#include <QtTest>

using namespace editor;

class K1804Test : public QObject {
    Q_OBJECT

private slots:
    void toOctal_data();
    void toOctal();
    void fromOctal_data();
    void fromOctal();
    void binaryWords();
    void projectRoundTrip();
};

void K1804Test::toOctal_data()
{
    QTest::addColumn<quint16>("value");
    QTest::addColumn<int>("width");
    QTest::addColumn<QString>("expected");
    QTest::newRow("zero") << quint16(0) << 1 << QStringLiteral("0");
    QTest::newRow("padding") << quint16(8) << 4 << QStringLiteral("0010");
    QTest::newRow("no-truncation") << quint16(64) << 1 << QStringLiteral("100");
    QTest::newRow("maximum") << quint16(65535) << 1 << QStringLiteral("177777");
}

void K1804Test::toOctal()
{
    QFETCH(quint16, value);
    QFETCH(int, width);
    QFETCH(QString, expected);
    QCOMPARE(K1804Microcode::toOctal(value, width), expected);
}

void K1804Test::fromOctal_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<quint16>("expected");
    QTest::addColumn<bool>("valid");
    QTest::newRow("empty") << QString() << quint16(0) << true;
    QTest::newRow("whitespace") << QStringLiteral("  17  ") << quint16(15) << true;
    QTest::newRow("maximum") << QStringLiteral("177777") << quint16(65535) << true;
    QTest::newRow("overflow") << QStringLiteral("200000") << quint16(0) << false;
    QTest::newRow("invalid-digit") << QStringLiteral("128") << quint16(0) << false;
    QTest::newRow("invalid-text") << QStringLiteral("abc") << quint16(0) << false;
}

void K1804Test::fromOctal()
{
    QFETCH(QString, text);
    QFETCH(quint16, expected);
    QFETCH(bool, valid);
    bool ok = !valid;
    QCOMPARE(K1804Microcode::fromOctal(text, &ok), expected);
    QCOMPARE(ok, valid);
}

void K1804Test::binaryWords()
{
    std::array<quint16, 16> fields{};
    fields[1] = 0x1234;
    fields[2] = 3;
    fields[3] = 15;
    fields[5] = 31;
    fields[6] = 31;
    fields[4] = 15;
    fields[7] = 3;
    fields[8] = 3;
    fields[9] = 7;
    fields[10] = 7;
    fields[13] = 3;
    fields[11] = 7;
    fields[12] = 7;
    fields[14] = 3;
    fields[15] = 255;
    const auto words = K1804Microcode::packBinaryWords(0x4321, fields);
    QCOMPARE(words[0], quint16(0x4321));
    QCOMPARE(words[1], quint16(0x1234));
    QCOMPARE(words[2], quint16(0xffff));
    QCOMPARE(words[3], quint16(0xffff));
    QCOMPARE(words[4], quint16(0xffff));
}

void K1804Test::projectRoundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("project.xml"));
    CanvasScene firstScene;
    CanvasScene secondScene;
    QList<PageInfo> original{{QStringLiteral("First page"), &firstScene},
                             {QStringLiteral("Second page"), &secondScene}};
    QList<QUuid> firstIds;
    QList<QUuid> secondIds;
    for (int i = 0; i < original.size(); ++i) {
        auto *scene = original[i].scene;
        auto *first = new MasterBlockElement(1, QStringLiteral("First"), QUuid::createUuid());
        auto *second = new MasterBlockElement(2, QStringLiteral("Second"), QUuid::createUuid());
        first->setPos(40, 60);
        second->setPos(400, 60);
        first->setAddress(quint16(10 + i));
        first->setStepName(QStringLiteral("step %1").arg(i));
        first->setMField(1, quint16(100 + i));
        first->regenerateLinesFromMFields();
        scene->addItem(first);
        scene->addItem(second);
        auto *link = new ConnectorElement(3, QStringLiteral("Link"), QUuid::createUuid());
        link->setStartConnection(first, PortPosition::OutMid);
        link->setEndConnection(second, PortPosition::InMid);
        link->setPoints({QPointF(250, 100), QPointF(400, 100)});
        scene->addItem(link);
        firstIds.append(first->uid());
        secondIds.append(second->uid());
    }

    QString error;
    QVERIFY2(ProjectStorage::saveToFile(path, QStringLiteral("Project"), original, &error), qPrintable(error));
    QObject owner;
    QList<PageInfo> loaded;
    QString projectName;
    QVERIFY2(ProjectStorage::loadFromFile(path, projectName, loaded, &error, &owner), qPrintable(error));
    QCOMPARE(projectName, QStringLiteral("Project"));
    QCOMPARE(loaded.size(), 2);
    for (int i = 0; i < loaded.size(); ++i) {
        QCOMPARE(loaded[i].name, original[i].name);
        QVERIFY(loaded[i].scene);
        QCOMPARE(loaded[i].scene->allElements().size(), 3);
        MasterBlockElement *first = nullptr;
        MasterBlockElement *second = nullptr;
        ConnectorElement *link = nullptr;
        for (ShapeElement *element : loaded[i].scene->allElements()) {
            if (auto *block = dynamic_cast<MasterBlockElement *>(element)) {
                if (block->uid() == firstIds[i]) first = block;
                if (block->uid() == secondIds[i]) second = block;
            } else if (auto *connector = dynamic_cast<ConnectorElement *>(element)) {
                link = connector;
            }
        }
        QVERIFY(first);
        QVERIFY(second);
        QVERIFY(link);
        QCOMPARE(first->address(), quint16(10 + i));
        QCOMPARE(first->stepName(), QStringLiteral("step %1").arg(i));
        QCOMPARE(first->mField(1), quint16(100 + i));
        QCOMPARE(link->startBlock(), first);
        QCOMPARE(link->endBlock(), second);
        QVERIFY(link->startPort() == PortPosition::OutMid);
        QVERIFY(link->endPort() == PortPosition::InMid);
    }
}

QTEST_MAIN(K1804Test)
#include "test_k1804.moc"
