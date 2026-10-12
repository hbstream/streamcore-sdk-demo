#pragma once

#include "streamcore/streamcore_sdk.h"

#include <QElapsedTimer>
#include <QString>
#include <QWidget>

#include <memory>

namespace demo_processor { class ProcessorFrameAdapter; }

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSlider;
class QTimer;
class QPaintEngine;

class ProcessorVideoWidget : public QWidget
{
public:
    explicit ProcessorVideoWidget(QWidget* parent = nullptr);
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    QPaintEngine* paintEngine() const override;
#endif

protected:
    void showEvent(QShowEvent* event) override;
};

// UI composition and session orchestration for the independent Processor tab.
// Algorithms and model dependencies live outside the Demo. The page owns SDK
// Capture/Publisher handles and the frame adapter; Capture is destroyed before
// the adapter, so no SDK callback can reference a freed C++ object. Existing
// feature pages do not depend on this class.
class ProcessorDemoPage : public QWidget
{
public:
    explicit ProcessorDemoPage(bool chinese, QWidget* parent = nullptr);
    ~ProcessorDemoPage() override;

    void StartFromEnvironment();
    bool Start();
    void Stop();

private:
    QString Tr(const char* english, const char* chinese) const;
    void UpdateStats();
    void SetStatus(const QString& message);
    void RefreshCameraDevices();
    void UpdateSourceAudioUi();
    void LoadEffectPack();
    void SelectAlgorithm();

    bool chinese_ = false;
    QLineEdit* file_edit_ = nullptr;
    QComboBox* source_combo_ = nullptr;
    QComboBox* camera_combo_ = nullptr;
    QLabel* source_detail_label_ = nullptr;
    QCheckBox* microphone_check_ = nullptr;
    QLabel* audio_hint_label_ = nullptr;
    QWidget* file_row_ = nullptr;
    QWidget* camera_row_ = nullptr;
    QLineEdit* module_edit_ = nullptr;
    QWidget* module_row_ = nullptr;
    QLabel* module_label_ = nullptr;
    QLineEdit* reference_edit_ = nullptr;
    QWidget* reference_row_ = nullptr;
    QLabel* reference_label_ = nullptr;
    QWidget* advanced_row_ = nullptr;
    QSlider* strength_slider_ = nullptr;
    QLabel* strength_value_ = nullptr;
    QString sample_path_;
    QString pack_module_path_;
    QLineEdit* url_edit_ = nullptr;
    QComboBox* algorithm_combo_ = nullptr;
    QCheckBox* publish_check_ = nullptr;
    QPushButton* start_button_ = nullptr;
    QPushButton* stop_button_ = nullptr;
    QLabel* status_label_ = nullptr;
    QLabel* pack_hint_label_ = nullptr;
    QLabel* stats_label_ = nullptr;
    ProcessorVideoWidget* original_video_ = nullptr;
    ProcessorVideoWidget* processed_video_ = nullptr;
    QTimer* stats_timer_ = nullptr;
    QElapsedTimer stats_clock_;
    qint64 stats_last_replaced_ = 0;
    streamcore_capture_handle capture_ = nullptr;
    streamcore_publisher_handle publisher_ = nullptr;
    std::unique_ptr<demo_processor::ProcessorFrameAdapter> frame_adapter_;
};
