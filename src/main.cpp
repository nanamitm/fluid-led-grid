#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>
#include "fluid_model.h"
#include "gravity_source.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    auto *model = new FluidModel(&app);

#ifdef Q_OS_ANDROID
    auto *gravity = new RealGravitySource(&app);
#else
    auto *gravity = new MouseGravitySource(&app);
#endif
    gravity->start();

    QObject::connect(gravity, &GravitySource::gravityChanged,
                     model,   &FluidModel::onGravityChanged);

    // 60 FPS シミュレーションループ
    QTimer timer;
    timer.setInterval(16);
    QObject::connect(&timer, &QTimer::timeout, model, &FluidModel::tick);
    timer.start();

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("FluidSim", model);

    using namespace Qt::StringLiterals;
    const QUrl url(u"qrc:/FluidLedGrid/qml/Main.qml"_s);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
