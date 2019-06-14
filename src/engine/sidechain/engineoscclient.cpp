/***************************************************************************
                          EngineOscClient.cpp  -  class to record the mix
                             -------------------
    copyright            : (C) 2007 by John Sully
    copyright            : (C) 2010 by Tobias Rafreider
    email                :
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "engine/sidechain/engineoscclient.h"

#include "control/controlobject.h"
#include "control/controlproxy.h"
#include "oscclient/defs_oscclient.h"
#include "preferences/usersettings.h"

#include "errordialoghandler.h"
#include "mixer/playerinfo.h"
#include "util/event.h"

#include "mixer/playermanager.h"
#include <QDebug>

EngineOscClient::EngineOscClient(UserSettingsPointer &pConfig)
    : m_pConfig(pConfig), m_prefUpdate(ConfigKey("[Preferences]", "updated")) {

  connectServer();

  ControlProxy *xfader =
      new ControlProxy(ConfigKey("[Master]", "crossfader"), this);
  xfader->connectValueChanged(this, &EngineOscClient::sendVolume);
  m_connectedControls.append(xfader);

  // connect play buttons
  for (int deckNr = 0; deckNr < (int)PlayerManager::numDecks(); deckNr++) {

    ControlProxy *play = new ControlProxy(
        ConfigKey(PlayerManager::groupForDeck(deckNr), "play"), this);
    play->connectValueChanged(this, &EngineOscClient::playing);
    m_connectedControls.append(play);

    PlayerInfo &playerInfo = PlayerInfo::instance();

    ControlProxy *cue = new ControlProxy(
        ConfigKey(PlayerManager::groupForDeck(deckNr), "cue_default"), this);
    cue->connectValueChanged(this, &EngineOscClient::loadData);
    m_connectedControls.append(cue);

    ControlProxy *rate = new ControlProxy(ConfigKey(PlayerManager::groupForDeck(deckNr), "rate"), this);
    rate->connectValueChanged(this, &EngineOscClient::speed);
    m_connectedControls.append(rate);

    ControlProxy *playpos = new ControlProxy(ConfigKey(PlayerManager::groupForDeck(deckNr), "playposition"), this);
    playpos->connectValueChanged(this, &EngineOscClient::position);
    m_connectedControls.append(playpos);

    ControlProxy *volume = new ControlProxy(
        ConfigKey(PlayerManager::groupForDeck(deckNr), "volume"), this);
    volume->connectValueChanged(this, &EngineOscClient::sendVolume);
    m_connectedControls.append(volume);

    QString equalizerRackString= "[EqualizerRack1_";
    QString effectString= "_Effect1]";
    QString eqChannel = equalizerRackString + PlayerManager::groupForDeck(deckNr) + effectString;

    ControlProxy *highs = new ControlProxy(
        ConfigKey(eqChannel, "parameter3"), this);
    highs->connectValueChanged(this, &EngineOscClient::sendHighs);
    m_connectedControls.append(highs);

    ControlProxy *mids = new ControlProxy(
        ConfigKey(eqChannel, "parameter2"), this);
    mids->connectValueChanged(this, &EngineOscClient::sendMids);
    m_connectedControls.append(mids);

    ControlProxy *lows = new ControlProxy(
        ConfigKey(eqChannel, "parameter1"), this);
    lows->connectValueChanged(this, &EngineOscClient::sendLows);
    m_connectedControls.append(lows);
  }

  // connect to settings changes
  connect(&m_prefUpdate, SIGNAL(valueChanged(double)), this,
          SLOT(connectServer()));
}

EngineOscClient::~EngineOscClient() {}

void EngineOscClient::process(const CSAMPLE *pBuffer, const int iBufferSize) {
  // if (m_time.elapsed() < 10)
  //   return;
  // sendState();
}

// void EngineOscClient::sendState() {
//   // PlayerInfo &playerInfo = PlayerInfo::instance();
//   // int numDecks = (int)PlayerManager::numDecks();
//   // lo_send(m_serverAddress, "/mixxx/numDecks", "i", numDecks);

//   // for (int deckNr = 0; deckNr < numDecks; deckNr++) {

//   //   lo_send(m_serverAddress, "/mixxx/deck/playing", "ii", deckNr,
//   //           (int)playerInfo.isDeckPlaying(deckNr));
//   //   lo_send(m_serverAddress, "/mixxx/deck/volume", "if", deckNr,
//   //           playerInfo.getDeckVolume(deckNr));

//   //   // speed
//   //   ControlProxy rate(ConfigKey(PlayerManager::groupForDeck(deckNr), "rate"));
//   //   ControlProxy rateRange(
//   //       ConfigKey(PlayerManager::groupForDeck(deckNr), "rateRange"));
//   //   ControlProxy rev(ConfigKey(PlayerManager::groupForDeck(deckNr), "reverse"));
//   //   float speed = 1 + float(rate.get()) * float(rateRange.get());
//   //   if (rev.get())
//   //     speed *= -1;
//   //   lo_send(m_serverAddress, "/mixxx/deck/speed", "if", deckNr, speed);

//   // }
//   // m_time.restart();
// }

void EngineOscClient::sendEQ(QString input)
{
  // qWarning() << "Input" << input;
  // PlayerInfo &playerInfo = PlayerInfo::instance();
  int numDecks = (int)PlayerManager::numDecks();
  QString equalizerRackString= "[EqualizerRack1_";
  QString effectString= "_Effect1]";
  
  for (int deckNr = 0; deckNr < numDecks; deckNr++) {
    QString eqChannel = equalizerRackString + PlayerManager::groupForDeck(deckNr) + effectString;
    // qWarning() << EngineOscClient::highs;
    if (deckNr < 2) {
          ControlProxy eq(ConfigKey(eqChannel, input));
    // qWarning() << "Sending EQ value" << highs.get();
    // qWarning() << "Sending EQ value default" << highs.getDefault();
    // qWarning() << "Sending EQ on Deck" << deckNr << input << float(eq.getParameter());
    // qWarning() << m_serverAddress;
    QString address = "/mixxx/deck/eq/" + input;
    // qWarning() << "adress" << address;
    lo_send(m_serverAddress, address.toUtf8().constData(), "if", deckNr, float(eq.getParameter()));
    // lo_send(m_serverAddress, "/mixxx/deck/eq", "is", deckNr, float(eq.getParameter()));
    //qWarning() << "Sending EQ Parameter" << deckNr << highs.getKey();
    }
  }
}


void EngineOscClient::sendHighs() 
{
  sendEQ("parameter3");
}
void EngineOscClient::sendMids() 
{
  sendEQ("parameter2");
}
void EngineOscClient::sendLows() 
{
  sendEQ("parameter1");
}
 
  void EngineOscClient::position() {

    PlayerInfo &playerInfo = PlayerInfo::instance();
    int numDecks = (int)PlayerManager::numDecks();
    lo_send(m_serverAddress, "/mixxx/numDecks", "i", numDecks);

    for (int deckNr = 0; deckNr < numDecks; deckNr++) {
        ControlProxy posRel(ConfigKey(PlayerManager::groupForDeck(deckNr), "playposition"));
        lo_send(m_serverAddress, "/mixxx/deck/pos", "if", deckNr, float(posRel.get()));
    }

  }

  void EngineOscClient::loadData() {
        
    PlayerInfo &playerInfo = PlayerInfo::instance();
    int numDecks = (int)PlayerManager::numDecks();
    // lo_send(m_serverAddress, "/mixxx/numDecks", "i", numDecks);

    for (int deckNr = 0; deckNr < numDecks; deckNr++) {

        ControlProxy dur(ConfigKey(PlayerManager::groupForDeck(deckNr), "duration"));
        lo_send(m_serverAddress, "/mixxx/deck/duration", "if", deckNr, float(dur.get()));

        lo_send(m_serverAddress, "/mixxx/deck/playing", "ii", deckNr,
                (int)playerInfo.isDeckPlaying(deckNr));
        // lo_send(m_serverAddress, "/mixxx/deck/volume", "if", deckNr,
        //         playerInfo.getDeckVolume(deckNr));
        TrackPointer pTrack = playerInfo.getTrackInfo(PlayerManager::groupForDeck(deckNr));
        QString title = "";
        QString artist ="";
            if (pTrack) {  
              // qWarning() << "new Song loaded";
              title = pTrack->getTitle();
              artist = pTrack->getArtist();
            }
        lo_send(m_serverAddress, "/mixxx/deck/title", "is", deckNr, title.toUtf8().data());
        lo_send(m_serverAddress, "/mixxx/deck/artist", "is", deckNr, artist.toUtf8().data());

        ControlProxy posRel(ConfigKey(PlayerManager::groupForDeck(deckNr), "playposition"));
        // lo_send(m_serverAddress, "/mixxx/deck/pos", "if", deckNr, float(posRel.get()));
    }

  }

  void EngineOscClient::playing() {

    PlayerInfo &playerInfo = PlayerInfo::instance();
    int numDecks = (int)PlayerManager::numDecks();
    // lo_send(m_serverAddress, "/mixxx/numDecks", "i", numDecks);

    for (int deckNr = 0; deckNr < numDecks; deckNr++) {

       
        lo_send(m_serverAddress, "/mixxx/deck/playing", "ii", deckNr,
                (int)playerInfo.isDeckPlaying(deckNr));
        // lo_send(m_serverAddress, "/mixxx/deck/volume", "if", deckNr,
        //         playerInfo.getDeckVolume(deckNr));
        // lo_send(m_serverAddress, "/mixxx/deck/pos", "if", deckNr, float(posRel.get()));
    }
  }

  void EngineOscClient::speed() {
      PlayerInfo &playerInfo = PlayerInfo::instance();
      int numDecks = (int)PlayerManager::numDecks();
      for (int deckNr = 0; deckNr < numDecks; deckNr++) {
          ControlProxy rate(ConfigKey(PlayerManager::groupForDeck(deckNr), "rate"));
          ControlProxy rateRange(
              ConfigKey(PlayerManager::groupForDeck(deckNr), "rateRange"));
          ControlProxy rev(ConfigKey(PlayerManager::groupForDeck(deckNr), "reverse"));
          float speed = 1 + float(rate.get()) * float(rateRange.get());
          TrackPointer pTrack = playerInfo.getTrackInfo(PlayerManager::groupForDeck(deckNr));
          if (pTrack) {
              double bpm = pTrack ->getBpm();
              // qWarning() << speed * bpm;
              lo_send(m_serverAddress, "/mixxx/deck/bpm", "if", deckNr, speed * bpm);
              lo_send(m_serverAddress, "/mixxx/deck/speed", "if", deckNr, speed);
          }
      }
  }

  void EngineOscClient::sendVolume() {
      PlayerInfo &playerInfo = PlayerInfo::instance();
      int numDecks = (int)PlayerManager::numDecks();
      for (int deckNr = 0; deckNr < numDecks; deckNr++) {
          lo_send(m_serverAddress, "/mixxx/deck/volume", "if", deckNr, playerInfo.getDeckVolume(deckNr));
      }
  }

// void EngineOscClient::maybeSendState() {
//   // if (m_time.elapsed() < 10)
//   //   return;
//   sendState();
// }

void EngineOscClient::connectServer() {
  QString server =
      m_pConfig->getValueString(ConfigKey(OSC_CLIENT_PREF_KEY, "Server"));
  QString port =
      m_pConfig->getValueString(ConfigKey(OSC_CLIENT_PREF_KEY, "Port"));
  m_serverAddress =
      lo_address_new(server.toLatin1().data(), port.toLatin1().data());
}
