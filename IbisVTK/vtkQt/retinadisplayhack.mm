#include <Cocoa/Cocoa.h>
#include "retinadisplayhack.h"

void disableGLHiDPI( long a_id )
{
    NSView * view = reinterpret_cast<NSView*>( a_id );
    [view setWantsBestResolutionOpenGLSurface:NO];
}
