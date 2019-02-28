#ifndef MYTHVTBINTEROP_H
#define MYTHVTBINTEROP_H

// MythTV
#include "mythopenglinterop.h"

// OSX
#include <CoreVideo/CoreVideo.h>

class MythVTBInterop : public MythOpenGLInterop
{
  public:
    static MythVTBInterop* Create(MythRenderOpenGL *Context, MythOpenGLInterop::Type Type);
    static Type GetInteropType(MythCodecID CodecId, MythRenderOpenGL *Context = nullptr);

    MythVTBInterop(MythRenderOpenGL *Context, MythOpenGLInterop::Type Type);
   ~MythVTBInterop() override;

    vector<MythGLTexture*> Acquire(MythRenderOpenGL *Context,
                                   VideoColourSpace *ColourSpace,
                                   VideoFrame       *Frame,
                                   FrameScanType    Scan) override;
};

class MythVTBSurfaceInterop : public MythVTBInterop
{
  public:
    MythVTBSurfaceInterop(MythRenderOpenGL *Context);
   ~MythVTBSurfaceInterop() override;

    vector<MythGLTexture*> Acquire(MythRenderOpenGL *Context,
                                   VideoColourSpace *ColourSpace,
                                   VideoFrame       *Frame,
                                   FrameScanType     Scan) override;
};

#endif
