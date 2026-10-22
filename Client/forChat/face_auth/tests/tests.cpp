#include <QtTest>
#include "algorithm/interfaces.h"
using namespace face;
class FaceTests : public QObject {
    Q_OBJECT
private slots:
    void configRejectsInvalid() {
        FaceAuthConfig config;
        config.matchThreshold = -1;
        QVERIFY_EXCEPTION_THROWN(config.validate(), Failure);
    }
    void qualityRejects() {
        FaceAuthConfig config;
        QImage frame(320,240,QImage::Format_RGB32);
        frame.fill(Qt::gray);
        QCOMPARE(inspectQuality(frame, {}, config).error, FaceAuthError::NoFace);
        Detection detection;
        detection.bounds = QRect(20,20,180,180);
        detection.landmarks = {QPointF(60,70), QPointF(150,70), QPointF(105,110), QPointF(70,150), QPointF(140,150)};
        QCOMPARE(inspectQuality(frame, {detection,detection}, config).error, FaceAuthError::MultipleFaces);
        Detection small = detection; small.bounds = QRect(20,20,30,30);
        QCOMPARE(inspectQuality(frame, {small}, config).error, FaceAuthError::TooSmall);
        QCOMPARE(inspectQuality(frame, {detection}, config).error, FaceAuthError::Blurry);
        frame.fill(Qt::black);
        QCOMPARE(inspectQuality(frame, {detection}, config).error, FaceAuthError::Exposure);
    }
    void matcherRejectsInvalid() {
        QVector<float> probe(128,0), same(128,0), other(128,0);
        probe[0] = same[0] = 1; other[1] = 1;
        QVERIFY(cosine(probe,same) > 0.99);
        QVERIFY(cosine(probe,other) < 0.55);
        QVERIFY(cosine(probe,{}) < 0);
    }
    void modelUnavailableIsFailure() {
        FaceAuthConfig config;
        config.modelDirectory = "/nonexistent/forchat-face-test";
        QVERIFY_EXCEPTION_THROWN(makeAlgorithm(config), Failure);
    }
};
QTEST_GUILESS_MAIN(FaceTests)
#include "tests.moc"
