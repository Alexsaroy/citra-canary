// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <QOpenGLWidget>
#include <QThread>
#include "common/thread.h"
#include "core/core.h"
#include "core/frontend/emu_window.h"

class QKeyEvent;
class QScreen;
class QTouchEvent;
class QOffscreenSurface;
class QOpenGLContext;

class GMainWindow;
class GRenderWindow;

class GGLContext : public Frontend::GraphicsContext {
public:
    explicit GGLContext(QOpenGLContext* shared_context);

    void MakeCurrent() override;

    void DoneCurrent() override;

private:
    QOpenGLContext* context;
    QOffscreenSurface* surface;
};

class EmuThread final : public QThread {
    Q_OBJECT

public:
    explicit EmuThread(Frontend::GraphicsContext& context);
    ~EmuThread() override;

    /**
     * Start emulation (on new thread)
     * @warning Only call when not running!
     */
    void run() override;

    /**
     * Steps the emulation thread by a single CPU instruction (if the CPU is not already running)
     * @note This function is thread-safe
     */
    void ExecStep() {
        exec_step = true;
        running_cv.notify_all();
    }

    /**
     * Sets whether the emulation thread is running or not
     * @param running Boolean value, set the emulation thread to running if true
     * @note This function is thread-safe
     */
    void SetRunning(bool running) {
        std::unique_lock lock{running_mutex};
        this->running = running;
        lock.unlock();
        running_cv.notify_all();
    }

    /**
     * Check if the emulation thread is running or not
     * @return True if the emulation thread is running, otherwise false
     * @note This function is thread-safe
     */
    bool IsRunning() const {
        return running;
    }

    /**
     * Requests for the emulation thread to stop running
     */
    void RequestStop() {
        stop_run = true;
        SetRunning(false);
    };

private:
    bool exec_step = false;
    bool running = false;
    std::atomic<bool> stop_run{false};
    std::mutex running_mutex;
    std::condition_variable running_cv;

    Frontend::GraphicsContext& core_context;

signals:
    /**
     * Emitted when the CPU has halted execution
     *
     * @warning When connecting to this signal from other threads, make sure to specify either
     * Qt::QueuedConnection (invoke slot within the destination object's message thread) or even
     * Qt::BlockingQueuedConnection (additionally block source thread until slot returns)
     */
    void DebugModeEntered();

    /**
     * Emitted right before the CPU continues execution
     *
     * @warning When connecting to this signal from other threads, make sure to specify either
     * Qt::QueuedConnection (invoke slot within the destination object's message thread) or even
     * Qt::BlockingQueuedConnection (additionally block source thread until slot returns)
     */
    void DebugModeLeft();

    void ErrorThrown(Core::System::ResultStatus, std::string);
};

class GRenderWindow : public QOpenGLWidget, public Frontend::EmuWindow {
    Q_OBJECT

public:
    GRenderWindow(QWidget* parent, EmuThread* emu_thread);
    ~GRenderWindow() override;

    // EmuWindow implementation.
    void MakeCurrent() override;
    void DoneCurrent() override;
    void PollEvents() override;
    std::unique_ptr<Frontend::GraphicsContext> CreateSharedContext() const override;

    void paintGL() override;

    void BackupGeometry();
    void RestoreGeometry();
    void restoreGeometry(const QByteArray& geometry); // overridden
    QByteArray saveGeometry();                        // overridden

    qreal windowPixelRatio() const;

    void closeEvent(QCloseEvent* event) override;

    void resizeEvent(QResizeEvent* event) override;

    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    bool event(QEvent* event) override;

    void focusOutEvent(QFocusEvent* event) override;

    void InitRenderTarget();

    void CaptureScreenshot(u32 res_scale, const QString& screenshot_path);

public slots:

    void OnEmulationStarting(EmuThread* emu_thread);
    void OnEmulationStopping();
    void OnFramebufferSizeChanged();

signals:
    /// Emitted when the window is closed
    void Closed();

private:
    std::pair<u32, u32> ScaleTouch(QPointF pos) const;
    void TouchBeginEvent(const QTouchEvent* event);
    void TouchUpdateEvent(const QTouchEvent* event);
    void TouchEndEvent();

    void OnMinimalClientAreaChangeRequest(std::pair<u32, u32> minimal_size) override;

    std::unique_ptr<GraphicsContext> core_context;

    QByteArray geometry;

    EmuThread* emu_thread;

    /// Temporary storage of the screenshot taken
    QImage screenshot_image;

protected:
    void showEvent(QShowEvent* event) override;
};
