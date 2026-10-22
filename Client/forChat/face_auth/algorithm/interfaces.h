#pragma once
#include "core/types.h"
#include <QRect>
#include <QPointF>
#include <memory>

namespace face {
struct Detection { QRect bounds; QVector<QPointF> landmarks; };
class IFaceDetector { public: virtual ~IFaceDetector() {};
    virtual QVector<Detection> detect(const QImage &frame) = 0; };
class IFaceAligner { public: virtual ~IFaceAligner() {};
    virtual QImage align(const QImage &frame, const Detection &face) = 0; };
class IFaceEmbedder { public: virtual ~IFaceEmbedder() {};
    virtual QVector<float> embed(const QImage &aligned) = 0; };
class IFaceMatcher { public: virtual ~IFaceMatcher() {};
    virtual double match(const QVector<float> &probe, const QVector<float> &saved) = 0; };
struct LivenessResult { bool passed = false; QString message; };
class ILivenessChecker { public: virtual ~ILivenessChecker() {};
    virtual LivenessResult check(const QImage &frame) = 0; };
class ICamera { public: virtual ~ICamera() {};
    virtual void open(int index) = 0;
    virtual QImage read() = 0;
    virtual void close() = 0; };
struct Algorithm {
    std::unique_ptr<ICamera> camera;
    std::unique_ptr<IFaceDetector> detector;
    std::unique_ptr<IFaceAligner> aligner;
    std::unique_ptr<IFaceEmbedder> embedder;
};
Algorithm makeAlgorithm(const FaceAuthConfig &config);
FaceQualityResult inspectQuality(const QImage &frame, const QVector<Detection> &faces,
                                 const FaceAuthConfig &config);
}
