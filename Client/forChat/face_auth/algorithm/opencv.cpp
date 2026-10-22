#include "interfaces.h"
#include <QFile>
#include <QDir>
#include <QCryptographicHash>
#include <cmath>
#ifdef FACE_WITH_OPENCV
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect/face.hpp>
#include <opencv2/videoio.hpp>
#endif

namespace face {
FaceQualityResult inspectQuality(const QImage &frame, const QVector<Detection> &faces,
                                 const FaceAuthConfig &config)
{
    FaceQualityResult result;
    auto reject = [&](FaceAuthError error, const QString &message) {
        result.error = error; result.message = message; return result;
    };
    if (faces.isEmpty()) return reject(FaceAuthError::NoFace, QStringLiteral("未检测到人脸"));
    if (faces.size() != 1) return reject(FaceAuthError::MultipleFaces, QStringLiteral("只允许一张人脸"));
    const Detection &detected = faces.first();
    const QRect bounds = detected.bounds.intersected(frame.rect());
    if (bounds != detected.bounds || bounds.width() < config.minFacePixels || bounds.height() < config.minFacePixels)
        return reject(FaceAuthError::TooSmall, QStringLiteral("请靠近摄像头并保持完整人脸"));
    if (detected.landmarks.size() != 5)
        return reject(FaceAuthError::Tilted, QStringLiteral("无法定位人脸关键点"));
    const QPointF eyes = detected.landmarks[1] - detected.landmarks[0];
    result.roll = std::atan2(eyes.y(), std::abs(eyes.x())) * 180 / 3.141592653589793;
    const double distance = std::abs(eyes.x());
    if (distance < 1 || std::abs(result.roll) > config.maxRoll)
        return reject(FaceAuthError::Tilted, QStringLiteral("请保持头部水平"));
    result.yaw = (detected.landmarks[2].x() -
        (detected.landmarks[0].x() + detected.landmarks[1].x()) / 2) / distance;
    if (std::abs(result.yaw) > 0.45)
        return reject(FaceAuthError::Tilted, QStringLiteral("转头角度过大"));
    QImage gray = frame.copy(bounds).convertToFormat(QImage::Format_Grayscale8);
    double sum = 0, lapSum = 0, lapSquared = 0;
    int count = 0;
    for (int row = 1; row < gray.height() - 1; ++row) {
        const uchar *previous = gray.constScanLine(row - 1);
        const uchar *current = gray.constScanLine(row);
        const uchar *next = gray.constScanLine(row + 1);
        for (int column = 1; column < gray.width() - 1; ++column) {
            const double lap = previous[column] + next[column] + current[column - 1] +
                current[column + 1] - 4.0 * current[column];
            sum += current[column]; lapSum += lap; lapSquared += lap * lap; ++count;
        }
    }
    result.brightness = sum / qMax(1, count);
    result.sharpness = lapSquared / qMax(1, count) - std::pow(lapSum / qMax(1, count), 2);
    gray.fill(0);
    if (result.brightness < config.minBrightness || result.brightness > config.maxBrightness)
        return reject(FaceAuthError::Exposure, QStringLiteral("光线过暗或过亮"));
    if (result.sharpness < config.minSharpness)
        return reject(FaceAuthError::Blurry, QStringLiteral("画面模糊，请保持稳定"));
    result.message = QStringLiteral("质量合格");
    return result;
}

#ifdef FACE_WITH_OPENCV
static QString checkedModel(const FaceAuthConfig &config, const QString &name, const QString &digest)
{
    QFile file(QDir(config.modelDirectory).filePath(name));
    if (!file.exists()) throw Failure(FaceAuthError::ModelMissing, QStringLiteral("模型缺失，请按 models/README.md 安装"));
    if (!file.open(QIODevice::ReadOnly)) throw Failure(FaceAuthError::ModelInvalid, QStringLiteral("模型不可读"));
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (digest.size() != 64 || !hash.addData(&file) || hash.result().toHex() != digest.toLatin1().toLower())
        throw Failure(FaceAuthError::ModelInvalid, QStringLiteral("模型 SHA-256 未配置或不匹配"));
    return file.fileName();
}
static cv::Mat bgrImage(const QImage &frame)
{
    QImage rgb = frame.convertToFormat(QImage::Format_RGB888);
    cv::Mat view(rgb.height(), rgb.width(), CV_8UC3, rgb.bits(), rgb.bytesPerLine());
    cv::Mat bgr;
    cv::cvtColor(view, bgr, cv::COLOR_RGB2BGR);
    rgb.fill(0);
    return bgr;
}
static QImage ownedImage(const cv::Mat &bgr)
{
    cv::Mat rgb;
    cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
    QImage result = QImage(rgb.data, rgb.cols, rgb.rows, int(rgb.step), QImage::Format_RGB888).copy();
    rgb.setTo(0);
    return result;
}
class Camera : public ICamera {
    cv::VideoCapture capture;
public:
    ~Camera() override { close(); }
    void open(int index) override {
        if (!capture.open(index))
            throw Failure(FaceAuthError::CameraUnavailable, QStringLiteral("摄像头不存在、被占用或系统未授权"));
        capture.set(cv::CAP_PROP_FRAME_WIDTH, 640);
        capture.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
        capture.set(cv::CAP_PROP_BUFFERSIZE, 1);
    }
    QImage read() override {
        cv::Mat frame;
        if (!capture.read(frame) || frame.empty())
            throw Failure(FaceAuthError::CameraUnavailable, QStringLiteral("摄像头读取失败，请关闭占用设备的程序后重试"));
        QImage result = ownedImage(frame);
        frame.setTo(0);
        return result.scaled(640, 480, Qt::KeepAspectRatio);
    }
    void close() override { capture.release(); }
};
class Detector : public IFaceDetector {
    cv::Ptr<cv::FaceDetectorYN> detector;
public:
    Detector(const QString &path, double threshold) {
        detector = cv::FaceDetectorYN::create(path.toStdString(), "", cv::Size(640,480), float(threshold));
    }
    QVector<Detection> detect(const QImage &frame) override {
        cv::Mat bgr = bgrImage(frame), found;
        detector->setInputSize(bgr.size());
        detector->detect(bgr, found);
        bgr.setTo(0);
        QVector<Detection> result;
        for (int row = 0; row < found.rows; ++row) {
            Detection detection;
            const float *values = found.ptr<float>(row);
            bool finite = true;
            for (int column = 0; column < 15; ++column) finite = finite && std::isfinite(values[column]);
            if (!finite) throw Failure(FaceAuthError::ModelInvalid, QStringLiteral("检测模型输出无效"));
            detection.bounds = QRect(int(values[0]), int(values[1]), int(values[2]), int(values[3]));
            for (int index = 0; index < 5; ++index)
                detection.landmarks.append(QPointF(values[4 + index * 2], values[5 + index * 2]));
            result.append(detection);
        }
        return result;
    }
};
class Recognizer : public IFaceAligner, public IFaceEmbedder {
    cv::Ptr<cv::FaceRecognizerSF> recognizer;
public:
    explicit Recognizer(const QString &path) {
        recognizer = cv::FaceRecognizerSF::create(path.toStdString(), "");
    }
    QImage align(const QImage &frame, const Detection &face) override {
        cv::Mat landmarks = cv::Mat::zeros(1,15,CV_32F);
        for (int index = 0; index < 5; ++index) {
            landmarks.at<float>(0,4 + index * 2) = float(face.landmarks[index].x());
            landmarks.at<float>(0,5 + index * 2) = float(face.landmarks[index].y());
        }
        cv::Mat bgr = bgrImage(frame), aligned;
        recognizer->alignCrop(bgr, landmarks, aligned);
        QImage result = ownedImage(aligned);
        bgr.setTo(0); aligned.setTo(0);
        return result;
    }
    QVector<float> embed(const QImage &aligned) override {
        cv::Mat bgr = bgrImage(aligned), feature;
        recognizer->feature(bgr, feature);
        bgr.setTo(0);
        if (feature.total() != 128 || feature.type() != CV_32F)
            throw Failure(FaceAuthError::ModelInvalid, QStringLiteral("识别模型维度无效"));
        QVector<float> result(128);
        double norm = 0;
        for (int index = 0; index < 128; ++index) {
            result[index] = feature.ptr<float>()[index];
            norm += double(result[index]) * result[index];
        }
        feature.setTo(0);
        if (!std::isfinite(norm) || norm < 1e-12)
            throw Failure(FaceAuthError::ModelInvalid, QStringLiteral("识别模型输出无效"));
        for (float &value : result) value /= float(std::sqrt(norm));
        return result;
    }
};
#endif
Algorithm makeAlgorithm(const FaceAuthConfig &config)
{
#ifdef FACE_WITH_OPENCV
    const QString detectorPath = checkedModel(config, config.detectorFile, config.detectorSha256);
    const QString embedderPath = checkedModel(config, config.embedderFile, config.embedderSha256);
    try {
        Algorithm algorithm;
        algorithm.camera.reset(new Camera);
        algorithm.detector.reset(new Detector(detectorPath, config.detectionThreshold));
        algorithm.aligner.reset(new Recognizer(embedderPath));
        algorithm.embedder.reset(new Recognizer(embedderPath));
        return algorithm;
    } catch (const cv::Exception &) {
        throw Failure(FaceAuthError::ModelInvalid, QStringLiteral("模型加载失败，请检查 OpenCV 与模型版本兼容性"));
    }
#else
    Q_UNUSED(config);
    throw Failure(FaceAuthError::ModelMissing, QStringLiteral("此构建未启用 OpenCV 人脸模块"));
#endif
}
}
