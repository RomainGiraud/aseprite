// SHE library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_SKIA_SKIA_SYSTEM_INCLUDED
#define SHE_SKIA_SKIA_SYSTEM_INCLUDED
#pragma once

#include "base/base.h"
#include "base/file_handle.h"

#include "SkImageDecoder.h"
#include "SkPixelRef.h"
#include "SkStream.h"

#include "she/skia/skia_display.h"
#include "she/skia/skia_surface.h"

#ifdef _WIN32
  #include "she/win/event_queue.h"
#elif __APPLE__
  #include "she/osx/app.h"
  #include "she/osx/event_queue.h"
#else
  #include "she/x11/event_queue.h"
#endif

namespace she {

EventQueueImpl g_queue;

class SkiaSystem : public CommonSystem {
public:
  SkiaSystem()
    : m_defaultDisplay(nullptr)
    , m_gpuAcceleration(false) {
#if !defined(__APPLE__) && !defined(_WIN32)
    // Create one decoder on Linux to load .png files with
    // libpng. Without this, SkImageDecoder::Factory() returns null
    // for .png files.
    SkAutoTDelete<SkImageDecoder> decoder(
      CreatePNGImageDecoder());
#endif
  }

  ~SkiaSystem() {
  }

  void dispose() override {
    delete this;
  }

  void finishLaunching() override {
#if __APPLE__
    // Start processing NSApplicationDelegate events. (E.g. after
    // calling this we'll receive application:openFiles: and we'll
    // generate DropFiles events.)  events
    OSXApp::instance()->finishLaunching();
#endif
  }

  Capabilities capabilities() const override {
    return Capabilities(
      int(Capabilities::MultipleDisplays) |
      int(Capabilities::CanResizeDisplay) |
      int(Capabilities::DisplayScale)
#if SK_SUPPORT_GPU
      | int(Capabilities::GpuAccelerationSwitch)
#endif
      );
  }

  EventQueue* eventQueue() override {
    return &g_queue;
  }

  bool gpuAcceleration() const override {
    return m_gpuAcceleration;
  }

  void setGpuAcceleration(bool state) override {
    m_gpuAcceleration = state;
  }

  gfx::Size defaultNewDisplaySize() override {
    gfx::Size sz;
#ifdef _WIN32
    sz.w = GetSystemMetrics(SM_CXMAXIMIZED);
    sz.h = GetSystemMetrics(SM_CYMAXIMIZED);
    sz.w -= GetSystemMetrics(SM_CXSIZEFRAME)*4;
    sz.h -= GetSystemMetrics(SM_CYSIZEFRAME)*4;
    sz.w = MAX(0, sz.w);
    sz.h = MAX(0, sz.h);
#endif
    return sz;
  }

  Display* defaultDisplay() override {
    return m_defaultDisplay;
  }

  Display* createDisplay(int width, int height, int scale) override {
    SkiaDisplay* display = new SkiaDisplay(width, height, scale);
    if (!m_defaultDisplay)
      m_defaultDisplay = display;
    return display;
  }

  Surface* createSurface(int width, int height) override {
    SkiaSurface* sur = new SkiaSurface;
    sur->create(width, height);
    return sur;
  }

  Surface* createRgbaSurface(int width, int height) override {
    SkiaSurface* sur = new SkiaSurface;
    sur->createRgba(width, height);
    return sur;
  }

  Surface* loadSurface(const char* filename) override {
    base::FileHandle fp(base::open_file_with_exception(filename, "rb"));
    SkAutoTDelete<SkStreamAsset> stream(new SkFILEStream(fp.get(), SkFILEStream::kCallerRetains_Ownership));

    SkAutoTDelete<SkImageDecoder> decoder(SkImageDecoder::Factory(stream));
    if (decoder) {
      stream->rewind();
      SkBitmap bm;
      SkImageDecoder::Result res = decoder->decode(stream, &bm,
        kN32_SkColorType, SkImageDecoder::kDecodePixels_Mode);

      if (res == SkImageDecoder::kSuccess) {
        bm.pixelRef()->setURI(filename);

        SkiaSurface* sur = new SkiaSurface();
        sur->swapBitmap(bm);
        return sur;
      }
    }
    return nullptr;
  }

  Surface* loadRgbaSurface(const char* filename) override {
    return loadSurface(filename);
  }

private:
  SkiaDisplay* m_defaultDisplay;
  bool m_gpuAcceleration;
};

EventQueue* EventQueue::instance() {
  return &g_queue;
}

} // namespace she

#endif
