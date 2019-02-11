///
/// \file RDSParser.cpp
/// \brief RDS Parser class implementation.
///
/// \author Matthias Hertel, http://www.mathertel.de
/// \copyright Copyright (c) 2014 by Matthias Hertel.\n
/// This work is licensed under a BSD style license.\n
/// See http://www.mathertel.de/License.aspx
///
/// \details
///
/// More documentation and source code is available at http://www.mathertel.de/Arduino
///
/// ChangeLog see RDSParser.h.

#include "RDSParser.h"

#define DEBUG_FUNC0(fn)          { Serial.print(fn); Serial.println("()"); }

/// Setup the RDS object and initialize private variables to 0.
RDSParser::RDSParser() {
  memset(this, 0, sizeof(RDSParser));
  memset(ps0, '-', sizeof(ps0));
  ps0[8] = '\0';
  memset(rt0, 0, sizeof(rt0));
} // RDSParser()

void RDSParser::clearRDS() {
  _sendServiceName(ps0);
  _sendText(rt0);
  PIcode = 0x0000;
  memset(afList, 0, sizeof(afList));
}

void RDSParser::init() {
  strcpy(_PSName1, "--------");
  strcpy(_PSName2, _PSName1);
  strcpy(programServiceName, "        ");
  memset(_RDSText, 0, sizeof(_RDSText));
  _lastTextIDX = 0;
} // init()


void RDSParser::attachServicenNameCallback(receiveServicenNameFunction newFunction) {
  _sendServiceName = newFunction;
} // attachServicenNameCallback

void RDSParser::attachTextCallback(receiveTextFunction newFunction) {
  _sendText = newFunction;
} // attachTextCallback


void RDSParser::attachTimeCallback(receiveTimeFunction newFunction) {
  _sendTime = newFunction;
} // attachTimeCallback


void RDSParser::processData(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4) {
  // DEBUG_FUNC0("process");
  PIcode = block1;
  uint8_t  idx; // index of rdsText
  char c1, c2;
  char *p;
  static uint16_t ab3 = 0;
  uint16_t mins; ///< RDS time in minutes
  uint8_t off;   ///< RDS time offset and sign
  uint16_t mjd;  ///< modified julian day
  int16_t mi;
  uint8_t af1, af2, af;
  bool inlist = false;
  // Serial.print('('); Serial.print(block1, HEX); Serial.print(' '); Serial.print(block2, HEX); Serial.print(' '); Serial.print(block3, HEX); Serial.print(' '); Serial.println(block4, HEX);

  if (block1 == 0) {
    // reset all the RDS info.
    init();

    memset(_RDSText, 0, sizeof(_RDSText));
    // Send out empty data
    if (_sendServiceName) _sendServiceName(programServiceName);
    if (_sendText)        _sendText(_RDSText);
    return;
  } // if

  // analyzing Block 2
  rdsGroupType = 0x0A | ((block2 & 0xF000) >> 8) | ((block2 & 0x0800) >> 11);
  rdsPTY = (block2 & 0x03E0) >> 5;
  rdsTP = (block2 & 0x0400) >> 10;

  switch (rdsGroupType) {
    case 0x0A:
      af1 = (block3 & 0xFF00) >> 8;
      af2 = (block3 & 0x00FF);
      if ((block3 != ab3) && ((af1 == tunedF) || (af2 == tunedF))) {
        Serial.print("AF: ");
        Serial.print(af1 + 875);
        Serial.print(" ");
        Serial.print(af2 + 875);
        Serial.print(" tuned: "); Serial.print(tunedF + 875); Serial.print(" ");
        Serial.println((af2 > af1));
        af = (af1 ^ tunedF)^af2;
        for (ab3 = 0; ab3 < 40; ab3++) {
          if (afList[ab3] == af) {
            inlist = true;
            break;
          }
          if (afList[ab3] == 0) break;
        }
        if (!inlist) afList[ab3] = af;
        ab3 = block3;
      }
    case 0x0B:
      rdsMusic = (block2 & 0x0008);
      rdsTA = (block2 & 0x0010) >> 4;

      // The data received is part of the Service Station Name
      idx = 2 * (block2 & 0x0003);
      if (idx > 8) break;
      // new data is 2 chars from block 4
      c1 = block4 >> 8;
      c2 = block4 & 0x00FF;

      // check that the data was received successfully twice
      // before publishing the station name

      if ((_PSName1[idx] == c1) && (_PSName1[idx + 1] == c2)) {
        // retrieved the text a second time: store to _PSName2
        _PSName2[idx] = c1;
        _PSName2[idx + 1] = c2;
        _PSName2[8] = '\0';

        if ((idx == 6) && strcmp(_PSName1, _PSName2) == 0) {
          if (strcmp(_PSName2, programServiceName) != 0) {
            // publish station name
            strcpy(programServiceName, _PSName2);
            if (_sendServiceName)
              _sendServiceName(programServiceName);
          } // if
        } // if
      } // if

      if ((_PSName1[idx] != c1) || (_PSName1[idx + 1] != c2)) {
        _PSName1[idx] = c1;
        _PSName1[idx + 1] = c2;
        _PSName1[8] = '\0';
        // Serial.println(_PSName1);
      } // if
      break;

    case 0x2A:
      // The data received is part of the RDS Text.
      _textAB = (block2 & 0x0010);
      idx = 4 * (block2 & 0x000F);

      if (idx > 64) break;

      if (idx < _lastTextIDX) {
        // the existing text might be complete because the index is starting at the beginning again.
        // now send it to the possible listener.
        if (_sendText)
          _sendText(_RDSText);
      }
      _lastTextIDX = idx;

      if (_textAB != _last_textAB) {
        // when this bit is toggled the whole buffer should be cleared.
        _last_textAB = _textAB;
        memset(_RDSText, 0, sizeof(_RDSText));
        // Serial.println("T>CLEAR");
      } // if


      // new data is 2 chars from block 3
      _RDSText[idx] = (block3 >> 8);     idx++;
      _RDSText[idx] = (block3 & 0x00FF); idx++;

      // new data is 2 chars from block 4
      _RDSText[idx] = (block4 >> 8);     idx++;
      _RDSText[idx] = (block4 & 0x00FF); idx++;

      // Serial.print(' '); Serial.println(_RDSText);
      // Serial.print("T>"); Serial.println(_RDSText);
      break;

    case 0x4A:
      // Clock time and date
      off = (block4) & 0x3F; // 6 bits
      mi = (block4 >> 6) & 0x3F; // 6 bits
      mi += 60 * (((block3 & 0x0001) << 4) | ((block4 >> 12) & 0x0F));

      // adjust offset
      if (off & 0x20) {
        mi -= 30 * (off & 0x1F);
      } else {
        mi += 30 * (off & 0x1F);
      }
      mjd = ((block2 & 0x0003) << 15) | ((block3 & 0xFFFE) >> 1);

      if (mi < 0) {
        mins = (uint16_t) (1440 + mi);
        //printf("yesterday?\n");
        mjd--;
      }
      else if (mi / 1440) {
        //printf("tomorrow?\n");
        mjd++;

      } else mins = (uint16_t)mi;

      if ((_sendTime) && (mins != _lastRDSMinutes)) {
        _lastRDSMinutes = mins;
        _sendTime(mjd, mins);
      } // if
      break;

    case 0x6A:
      // IH
      break;

    case 0x8A:
      // TMC
      break;

    case 0xAA:
      // TMC
      break;

    case 0xCA:
      // TMC
      break;

    case 0xEA:
      // IH
      break;
    default:
      // Serial.print("RDS_GRP:"); Serial.println(rdsGroupType, HEX);
      break;
  }
} // processData()

// End.

