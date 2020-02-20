/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AEBitstreamPacker.h"

#include "AEPackIEC61937.h"
#include "AEStreamInfo.h"
#include "utils/log.h"
#include "Util.h"

#include <algorithm>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define BURST_HEADER_SIZE       8
#define MAT_PKT_OFFSET          61440
#define MAT_FRAME_SIZE          61424
#define EAC3_MAX_BURST_PAYLOAD_SIZE (24576 - BURST_HEADER_SIZE)

CAEBitstreamPacker::CAEBitstreamPacker() :
  m_trueHD   (NULL),
  m_dtsHD    (NULL),
  m_eac3     (NULL)
{
  Reset();
}

CAEBitstreamPacker::~CAEBitstreamPacker()
{
  delete[] m_trueHD;
  delete[] m_dtsHD;
  delete[] m_eac3;
}

void CAEBitstreamPacker::Pack(CAEStreamInfo &info, uint8_t* data, int size)
{
  m_pauseDuration = 0;
  switch (info.m_type)
  {
    case CAEStreamInfo::STREAM_TYPE_TRUEHD:
      PackTrueHD(info, data, size);
      break;

    case CAEStreamInfo::STREAM_TYPE_DTSHD:
    case CAEStreamInfo::STREAM_TYPE_DTSHD_MA:
      PackDTSHD (info, data, size);
      break;

    case CAEStreamInfo::STREAM_TYPE_AC3:
      m_dataSize = CAEPackIEC61937::PackAC3(data, size, m_packedBuffer);
      break;

    case CAEStreamInfo::STREAM_TYPE_EAC3:
      PackEAC3 (info, data, size);
      break;

    case CAEStreamInfo::STREAM_TYPE_DTSHD_CORE:
    case CAEStreamInfo::STREAM_TYPE_DTS_512:
      m_dataSize = CAEPackIEC61937::PackDTS_512(data, size, m_packedBuffer, info.m_dataIsLE);
      break;

    case CAEStreamInfo::STREAM_TYPE_DTS_1024:
      m_dataSize = CAEPackIEC61937::PackDTS_1024(data, size, m_packedBuffer, info.m_dataIsLE);
      break;

    case CAEStreamInfo::STREAM_TYPE_DTS_2048:
      m_dataSize = CAEPackIEC61937::PackDTS_2048(data, size, m_packedBuffer, info.m_dataIsLE);
      break;

    default:
      CLog::Log(LOGERROR, "CAEBitstreamPacker::Pack - no pack function");
  }
}

bool CAEBitstreamPacker::PackPause(CAEStreamInfo &info, unsigned int millis, bool iecBursts)
{
  // re-use last buffer
  if (m_pauseDuration == millis)
    return false;

  switch (info.m_type)
  {
    case CAEStreamInfo::STREAM_TYPE_TRUEHD:
    case CAEStreamInfo::STREAM_TYPE_EAC3:
      m_dataSize = CAEPackIEC61937::PackPause(m_packedBuffer, millis, GetOutputChannelMap(info).Count() * 2, GetOutputRate(info), 4, info.m_sampleRate);
      m_pauseDuration = millis;
      break;

    case CAEStreamInfo::STREAM_TYPE_AC3:
    case CAEStreamInfo::STREAM_TYPE_DTSHD:
    case CAEStreamInfo::STREAM_TYPE_DTSHD_MA:
    case CAEStreamInfo::STREAM_TYPE_DTSHD_CORE:
    case CAEStreamInfo::STREAM_TYPE_DTS_512:
    case CAEStreamInfo::STREAM_TYPE_DTS_1024:
    case CAEStreamInfo::STREAM_TYPE_DTS_2048:
      m_dataSize = CAEPackIEC61937::PackPause(m_packedBuffer, millis, GetOutputChannelMap(info).Count() * 2, GetOutputRate(info), 3, info.m_sampleRate);
      m_pauseDuration = millis;
      break;

    default:
      CLog::Log(LOGERROR, "CAEBitstreamPacker::Pack - no pack function");
  }

  if (!iecBursts)
  {
    memset(m_packedBuffer, 0, m_dataSize);
  }

  return true;
}

unsigned int CAEBitstreamPacker::GetSize() const
{
  return m_dataSize;
}

uint8_t* CAEBitstreamPacker::GetBuffer()
{
  return m_packedBuffer;
}

void CAEBitstreamPacker::Reset()
{
  m_dataSize = 0;
  m_pauseDuration = 0;
  m_packedBuffer[0] = 0;
}

/* we need to pack 24 TrueHD audio units into the unknown MAT format before packing into IEC61937 */
void CAEBitstreamPacker::PackTrueHD(CAEStreamInfo &info, uint8_t* data, int size)
{
  /* magic MAT format values, meaning is unknown at this point */
  static const uint8_t mat_start_code [20] = { 0x07, 0x9E, 0x00, 0x03, 0x84, 0x01, 0x01, 0x01, 0x80, 0x00, 0x56, 0xA5, 0x3B, 0xF4, 0x81, 0x83, 0x49, 0x80, 0x77, 0xE0 };
  static const uint8_t mat_middle_code[12] = { 0xC3, 0xC1, 0x42, 0x49, 0x3B, 0xFA, 0x82, 0x83, 0x49, 0x80, 0x77, 0xE0 };
  static const uint8_t mat_end_code   [16] = { 0xC3, 0xC2, 0xC0, 0xC4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x97, 0x11 };

  static const struct {
      unsigned int pos;
      const uint8_t *code;
      unsigned int len;
  } matCodes[] = {
      { 0, mat_start_code, sizeof(mat_start_code) },
      { 30708, mat_middle_code, sizeof(mat_middle_code) },
      { MAT_FRAME_SIZE - sizeof(mat_end_code), mat_end_code, sizeof(mat_end_code) },
  };

  /* create the buffer if it doesnt already exist */
  if (!m_trueHD)
  {
    m_trueHD    = new uint8_t[MAT_FRAME_SIZE];
    m_trueHDPos = 0;
  }

  if (size < 4 || info.m_sampleRate < 44100)
    return;

  unsigned int paddingRemaining = 0;
  unsigned int totalFrameSize = size;

  uint16_t inputTiming = (data[2] << 8) | data[3];
  if (m_trueHDPrevSize)
  {
    unsigned int samplesPerFrame = 40 * info.m_sampleRate / ((info.m_sampleRate % 48000) ? 44100 : 48000);
    uint16_t deltaSamples = inputTiming - m_trueHDPrevTime;
    /*
     * One multiple-of-48kHz frame is 1/1200 sec and the IEC 61937 rate
     * is 768kHz = 768000*4 bytes/sec.
     * The nominal space per frame is therefore
     * (768000*4 bytes/sec) * (1/1200 sec) = 2560 bytes.
     * For multiple-of-44.1kHz frames: 1/1102.5 sec, 705.6kHz, 2560 bytes.
     *
     * 2560 is divisible by samplesPerFrame.
     */
    unsigned int deltaBytes = deltaSamples * 2560 / samplesPerFrame;

    /* padding needed before this frame */
    paddingRemaining = deltaBytes - m_trueHDPrevSize;

    /* sanity check */
    if (paddingRemaining < 0 || paddingRemaining >= MAT_FRAME_SIZE / 2)
        paddingRemaining = 0;
  }

  unsigned int nextCodeIdx = 0;
  for (nextCodeIdx = 0; nextCodeIdx < ARRAY_SIZE(matCodes); nextCodeIdx++)
    if (m_trueHDPos <= matCodes[nextCodeIdx].pos)
      break;

  if (nextCodeIdx >= ARRAY_SIZE(matCodes))
  {
    CLog::Log(LOGERROR, "CAEBitstreamPacker - Invalid m_trueHDPos {}", m_trueHDPos);
    return;
  }

  unsigned int dataRemaining = size;
  const uint8_t *dataPtr = data;

  while (paddingRemaining || dataRemaining || matCodes[nextCodeIdx].pos == m_trueHDPos)
  {
    if (matCodes[nextCodeIdx].pos == m_trueHDPos)
    {
      /* time to insert MAT code */
      unsigned int codeLen = matCodes[nextCodeIdx].len;
      unsigned int codeLenRemaining = codeLen;
      memcpy(m_trueHD + matCodes[nextCodeIdx].pos, matCodes[nextCodeIdx].code, codeLen);
      m_trueHDPos += codeLen;

      nextCodeIdx++;
      if (nextCodeIdx == ARRAY_SIZE(matCodes))
      {
        /* this was the last code, move to the next MAT frame */
        m_dataSize  = CAEPackIEC61937::PackTrueHD(m_trueHD, MAT_FRAME_SIZE, m_packedBuffer);

        nextCodeIdx = 0;
        m_trueHDPos = 0;

        /* inter-frame gap has to be counted as well, add it */
        codeLenRemaining += MAT_PKT_OFFSET - MAT_FRAME_SIZE;
      }

      if (paddingRemaining)
      {
        /* consider the MAT code as padding */
        unsigned int countedAsPadding = std::min(paddingRemaining, codeLenRemaining);
        paddingRemaining -= countedAsPadding;
        codeLenRemaining -= countedAsPadding;
      }
      /* count the remainder of the code as part of frame size */
      if (codeLenRemaining)
        totalFrameSize += codeLenRemaining;
    }

    if (paddingRemaining)
    {
      unsigned int paddingToInsert = std::min(matCodes[nextCodeIdx].pos - m_trueHDPos,
                                              paddingRemaining);

      memset(m_trueHD + m_trueHDPos, 0, paddingToInsert);
      m_trueHDPos += paddingToInsert;
      paddingRemaining -= paddingToInsert;

      if (paddingRemaining)
        continue; /* time to insert MAT code */
    }

    if (dataRemaining)
    {
      unsigned int dataToInsert = std::min(matCodes[nextCodeIdx].pos - m_trueHDPos,
                                           dataRemaining);

      memcpy(m_trueHD + m_trueHDPos, dataPtr, dataToInsert);
      m_trueHDPos += dataToInsert;
      dataPtr += dataToInsert;
      dataRemaining -= dataToInsert;
    }
  }

  m_trueHDPrevSize = totalFrameSize;
  m_trueHDPrevTime = inputTiming;
}

void CAEBitstreamPacker::PackDTSHD(CAEStreamInfo &info, uint8_t* data, int size)
{
  static const uint8_t dtshd_start_code[10] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xfe };
  unsigned int dataSize = sizeof(dtshd_start_code) + 2 + size;

  if (dataSize > m_dtsHDSize)
  {
    delete[] m_dtsHD;
    m_dtsHDSize = dataSize;
    m_dtsHD     = new uint8_t[dataSize];
    memcpy(m_dtsHD, dtshd_start_code, sizeof(dtshd_start_code));
  }

  m_dtsHD[sizeof(dtshd_start_code) + 0] = ((uint16_t)size & 0xFF00) >> 8;
  m_dtsHD[sizeof(dtshd_start_code) + 1] = ((uint16_t)size & 0x00FF);
  memcpy(m_dtsHD + sizeof(dtshd_start_code) + 2, data, size);

  m_dataSize = CAEPackIEC61937::PackDTSHD(m_dtsHD, dataSize, m_packedBuffer, info.m_dtsPeriod);
}

void CAEBitstreamPacker::PackEAC3(CAEStreamInfo &info, uint8_t* data, int size)
{
  unsigned int framesPerBurst = info.m_repeat;

  if (m_eac3FramesPerBurst != framesPerBurst)
  {
    /* switched streams, discard partial burst */
    m_eac3Size = 0;
    m_eac3FramesPerBurst = framesPerBurst;
  }

  if (m_eac3FramesPerBurst == 1)
  {
    /* simple case, just pass through */
    m_dataSize = CAEPackIEC61937::PackEAC3(data, size, m_packedBuffer);
  }
  else
  {
    /* multiple frames needed to achieve 6 blocks as required by IEC 61937-3:2007 */

    if (m_eac3 == NULL)
      m_eac3 = new uint8_t[EAC3_MAX_BURST_PAYLOAD_SIZE];

    unsigned int newsize = m_eac3Size + size;
    bool overrun = newsize > EAC3_MAX_BURST_PAYLOAD_SIZE;

    if (!overrun)
    {
      memcpy(m_eac3 + m_eac3Size, data, size);
      m_eac3Size = newsize;
      m_eac3FramesCount++;
    }

    if (m_eac3FramesCount >= m_eac3FramesPerBurst || overrun)
    {
      m_dataSize = CAEPackIEC61937::PackEAC3(m_eac3, m_eac3Size, m_packedBuffer);
      m_eac3Size = 0;
      m_eac3FramesCount = 0;
    }
  }
}

unsigned int CAEBitstreamPacker::GetOutputRate(CAEStreamInfo &info)
{
  unsigned int rate;
  switch (info.m_type)
  {
    case CAEStreamInfo::STREAM_TYPE_AC3:
      rate = info.m_sampleRate;
      break;
    case CAEStreamInfo::STREAM_TYPE_EAC3:
      rate = info.m_sampleRate * 4;
      break;
    case CAEStreamInfo::STREAM_TYPE_TRUEHD:
      if (info.m_sampleRate == 48000 ||
          info.m_sampleRate == 96000 ||
          info.m_sampleRate == 192000)
        rate = 192000;
      else
        rate = 176400;
      break;
    case CAEStreamInfo::STREAM_TYPE_DTS_512:
    case CAEStreamInfo::STREAM_TYPE_DTS_1024:
    case CAEStreamInfo::STREAM_TYPE_DTS_2048:
    case CAEStreamInfo::STREAM_TYPE_DTSHD_CORE:
      rate = info.m_sampleRate;
      break;
    case CAEStreamInfo::STREAM_TYPE_DTSHD:
    case CAEStreamInfo::STREAM_TYPE_DTSHD_MA:
      rate = 192000;
      break;
    default:
      rate = 48000;
      break;
  }
  return rate;
}

CAEChannelInfo CAEBitstreamPacker::GetOutputChannelMap(CAEStreamInfo &info)
{
  int channels = 2;
  switch (info.m_type)
  {
    case CAEStreamInfo::STREAM_TYPE_AC3:
    case CAEStreamInfo::STREAM_TYPE_EAC3:
    case CAEStreamInfo::STREAM_TYPE_DTS_512:
    case CAEStreamInfo::STREAM_TYPE_DTS_1024:
    case CAEStreamInfo::STREAM_TYPE_DTS_2048:
    case CAEStreamInfo::STREAM_TYPE_DTSHD_CORE:
    case CAEStreamInfo::STREAM_TYPE_DTSHD:
      channels = 2;
      break;

    case CAEStreamInfo::STREAM_TYPE_TRUEHD:
    case CAEStreamInfo::STREAM_TYPE_DTSHD_MA:
      channels = 8;
      break;

    default:
      break;
  }

  CAEChannelInfo channelMap;
  for (int i=0; i<channels; i++)
  {
    channelMap += AE_CH_RAW;
  }

  return channelMap;
}
