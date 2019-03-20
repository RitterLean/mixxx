/**
  * @file controllermanager.h
  * @author Sean Pappalardo spappalardo@mixxx.org
  * @date Sat Apr 30 2011
  * @brief Manages creation/enumeration/deletion of hardware controllers.
  */

#ifndef CONTROLLERMANAGER_H
#define CONTROLLERMANAGER_H

#include <QSharedPointer>

#include <unistd.h> // udp.hh needs this file
#include "../lib/oscpkt-1.2/udp.hh"
#include "../lib/oscpkt-1.2/oscpkt.hh"

#include "controllers/controllerenumerator.h"
#include "controllers/controllerpreset.h"
#include "controllers/controllerpresetinfo.h"
#include "controllers/controllerpresetinfoenumerator.h"
#include "preferences/usersettings.h"
#include "mixer/playermanager.h"

//Forward declaration(s)
class Controller;
class ControllerLearningEventFilter;

// Function to sort controllers by name
bool controllerCompare(Controller *a, Controller *b);

/** Manages enumeration/operation/deletion of hardware controllers. */
class ControllerManager : public QObject {
    Q_OBJECT
  public:
    // Should oscPort be passed as a command-line arg from mixxx.cpp
    // or should it be added as a setting somewhere else?
    ControllerManager(UserSettingsPointer pConfig, int oscPort);
    virtual ~ControllerManager();

    QList<Controller*> getControllers() const;
    QList<Controller*> getControllerList(bool outputDevices=true, bool inputDevices=true);
    ControllerLearningEventFilter* getControllerLearningEventFilter() const;
    QSharedPointer<PresetInfoEnumerator> getMainThreadPresetEnumerator() {
        return m_pMainThreadPresetEnumerator;
    }

    // Prevent other parts of Mixxx from having to manually connect to our slots
    void setUpDevices() { emit(requestSetUpDevices()); };
    void savePresets(bool onlyActive=false) { emit(requestSave(onlyActive)); };

    static QList<QString> getPresetPaths(UserSettingsPointer pConfig);

    // If pathOrFilename is an absolute path, returns it. If it is a relative
    // path and it is contained within any of the directories in presetPaths,
    // returns the path to the first file in the path that exists.
    static QString getAbsolutePath(const QString& pathOrFilename,
                                   const QStringList& presetPaths);

    bool importScript(const QString& scriptPath, QString* newScriptFileName);
    static bool checksumFile(const QString& filename, quint16* pChecksum);

  signals:
    void devicesChanged();
    void requestSetUpDevices();
    void requestShutdown();
    void requestSave(bool onlyActive);
    void requestInitialize();
    void loadTrackToGroup(QString location, QString group);

  public slots:
    void updateControllerList();

    void openController(Controller* pController);
    void closeController(Controller* pController);

    // Writes out presets for currently connected input devices
    void slotSavePresets(bool onlyActive=false);

    void locationLoadedToPlayer(QString location, QString group);

  private slots:
    // Perform initialization that should be delayed until the ControllerManager
    // thread is started.
    void slotInitialize();
    // Open whatever controllers are selected in the preferences. This currently
    // only runs on start-up but maybe should instead be signaled by the
    // preferences dialog on apply, and only open/close changed devices
    void slotSetUpDevices();
    void slotShutdown();
    bool loadPreset(Controller* pController,
                    ControllerPresetPointer preset);
    // Calls poll() on all devices that have isPolling() true.
    void pollDevices();
    void pollOsc();
    void startPolling();
    void stopPolling();
    void startPollingOsc();
    void stopPollingOsc();
    void maybeStartOrStopPolling();

    static QString presetFilenameFromName(QString name) {
        return name.replace(" ", "_").replace("/", "_").replace("\\", "_");
    }

  private:
    void setUpOsc();

    void handleOscGet(const std::string& group, const std::string& key);
    void handleOscSet(const std::string& group, const std::string& key, double value);
    void handleOscLoad(const std::string& group, const std::string& location);

    oscpkt::UdpSocket m_sock;
    oscpkt::PacketReader m_pr;
    oscpkt::PacketWriter m_pw;
    UserSettingsPointer m_pConfig;
    ControllerLearningEventFilter* m_pControllerLearningEventFilter;
    QTimer m_pollTimer, m_oscPollTimer;
    mutable QMutex m_mutex;
    QList<ControllerEnumerator*> m_enumerators;
    QList<Controller*> m_controllers;
    QThread* m_pThread;
    QSharedPointer<PresetInfoEnumerator> m_pMainThreadPresetEnumerator;
    bool m_skipPoll;
    int m_oscPort;
    bool m_pollingOsc;
};

#endif  // CONTROLLERMANAGER_H
