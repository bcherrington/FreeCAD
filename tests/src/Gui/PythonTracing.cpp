// SPDX-License-Identifier: LGPL-2.1-or-later

#include <functional>
#include <memory>
#include <stdexcept>
#include <QCoreApplication>
#include <QEvent>
#include <QObject>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

#include <Base/Interpreter.h>
#include <Gui/GuiApplication.h>
#include <Gui/PythonTracing.h>
#include <src/App/InitApplication.h>

namespace
{
// Many trace callbacks and elapsed time ensure a due tracer tick, without relying
// on a particular opcode or machine speed. The Qt event remains pending until
// Python tracing or the caller explicitly services it.
void runPython()
{
    Base::Interpreter().runString(
        "import time\n"
        "deadline = time.monotonic() + 0.03\n"
        "iterations = 0\n"
        "while time.monotonic() < deadline or iterations < 10000:\n"
        "    iterations += 1\n"
    );
}

class CallbackReceiver: public QObject
{
public:
    std::function<void()> callback;
    bool event(QEvent* event) override
    {
        if (event->type() == QEvent::User) {
            callback();
            return true;
        }
        return QObject::event(event);
    }
};

void deliver(CallbackReceiver& receiver)
{
    QEvent event(QEvent::User);
    QCoreApplication::sendEvent(&receiver, &event);
}
}  // namespace

class PythonTracingTest: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void nestedNotificationDoesNotPumpAndOuterCodeResumes()
    {
        Gui::PythonTracing trace;
        trace.setTimeout(1);
        Gui::PythonTracingLocker active(trace);
        runPython();
        auto fired = std::make_shared<bool>(false);
        QTimer::singleShot(0, this, [fired] { *fired = true; });
        CallbackReceiver receiver;
        receiver.callback = [&] {
            runPython();
        };
        deliver(receiver);
        QVERIFY(!*fired);
        runPython();
        QVERIFY(*fired);
    }

    void consoleScopeInsideNotificationStillPumps()
    {
        CallbackReceiver outer;
        outer.callback = [&] {
            Gui::PythonTracing trace;
            trace.setTimeout(1);
            Gui::PythonTracingLocker active(trace);
            runPython();
            auto fired = std::make_shared<bool>(false);
            QTimer::singleShot(0, this, [fired] { *fired = true; });
            CallbackReceiver inner;
            inner.callback = [&] {
                runPython();
            };
            deliver(inner);
            QVERIFY(!*fired);
            runPython();
            QVERIFY(*fired);
        };
        deliver(outer);
    }

    void exceptionUnwindsNotificationDepth()
    {
        Gui::PythonTracing trace;
        trace.setTimeout(1);
        Gui::PythonTracingLocker active(trace);
        runPython();
        auto fired = std::make_shared<bool>(false);
        QTimer::singleShot(0, this, [fired] { *fired = true; });
        CallbackReceiver receiver;
        receiver.callback = [&] {
            runPython();
            throw std::runtime_error("intentional tracing regression exception");
        };
        // GUIApplication's ordinary exception boundary handles the exception.
        deliver(receiver);
        QVERIFY(!*fired);
        runPython();
        QVERIFY(*fired);
    }
};

int main(int argc, char** argv)
{
    QTemporaryDir profile;
    if (!profile.isValid()) {
        return 1;
    }
    qputenv("FREECAD_USER_HOME", profile.path().toUtf8());
    Gui::GUIApplication app(argc, argv);
    tests::initApplication();
    PythonTracingTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "PythonTracing.moc"
