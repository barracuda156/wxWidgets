/////////////////////////////////////////////////////////////////////////////
// Name:        src/osx/cocoa/colour.mm
// Purpose:     Conversions between NSColor and wxColour
// Author:      Vadim Zeitlin
// Created:     2015-11-26 (completely replacing the old version of the file)
// Copyright:   (c) 2015 Vadim Zeitlin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#include "wx/colour.h"

#include "wx/osx/private.h"
#include "wx/osx/private/available.h"

class wxNSColorRefData : public wxColourRefData
{
public:
    wxNSColorRefData(WX_NSColor color);
    
    wxNSColorRefData(const wxNSColorRefData& other);

    virtual ~wxNSColorRefData();
    
    virtual CGFloat Red() const override;
    virtual CGFloat Green() const override;
    virtual CGFloat Blue() const override;
    virtual CGFloat Alpha() const override;
    
    virtual bool IsSolid() const override;

    CGColorRef GetCGColor() const override;
    
    virtual wxColourRefData* Clone() const override { return new wxNSColorRefData(*this); }
    
    virtual WX_NSColor GetNSColor() const override;
    virtual WX_NSImage GetNSPatternImage() const override;
private:
    static CGFloat GetCGColorComponent(CGColorRef col, int rgbaIndex);
    WX_NSColor m_nsColour;
    
    wxDECLARE_NO_ASSIGN_CLASS(wxNSColorRefData);
};

wxNSColorRefData::wxNSColorRefData(WX_NSColor color)
{
    m_nsColour = [color retain];
}

wxNSColorRefData::wxNSColorRefData(const wxNSColorRefData& other)
{
    m_nsColour = [other.m_nsColour retain];
}

wxNSColorRefData::~wxNSColorRefData()
{
    [m_nsColour release];
}

WX_NSColor wxNSColorRefData::GetNSColor() const
{
    return m_nsColour;
}

CGFloat wxNSColorRefData::GetCGColorComponent(CGColorRef col, int rgbaIndex)
{
    CGFloat value = 0.0;
    
    if ( col )
    {
        wxCFRef<CGColorRef> rgbacol;
        CGColorSpaceModel model = CGColorSpaceGetModel(CGColorGetColorSpace(col));
        size_t noComp = CGColorGetNumberOfComponents(col);
        const CGFloat* components = CGColorGetComponents(col);
        
        bool isRGB = true;
        
        if (model == kCGColorSpaceModelMonochrome)
        {
            wxASSERT_MSG(1 <= noComp && noComp <= 2, "Monochrome Color unexpected components");
            // is alpha is requested look into component if available
            if ( rgbaIndex == 3 )
                value = noComp > 1 ? components[1] : 1.0;
            else
                value = components[0];
            isRGB = false;
        }
        else if (model != kCGColorSpaceModelRGB)
        {
            if ( WX_IS_MACOS_OR_IOS_AVAILABLE(10, 11, 9, 0) )
            {
                rgbacol = CGColorCreateCopyByMatchingToColorSpace(wxMacGetGenericRGBColorSpace(), kCGRenderingIntentDefault, col, nullptr);
                noComp = CGColorGetNumberOfComponents(rgbacol);
                components = CGColorGetComponents(rgbacol);
            }
            else
            {
                isRGB = false;
            }
        }
        
        if (isRGB)
        {
            wxASSERT_MSG(3 <= noComp && noComp <= 4, "RGB Color unexpected components");
            // is alpha is requested look into component if available
            if ( rgbaIndex == 3 )
                value = noComp == 4 ? components[3] : 1.0;
            else
                value = components[rgbaIndex];
        }
    }
    
    return value;
}

CGFloat wxNSColorRefData::Red() const
{
    wxOSXEffectiveAppearanceSetter helper;
    if ( NSColor* colRGBA = [m_nsColour colorUsingColorSpaceName:NSCalibratedRGBColorSpace] )
        return [colRGBA redComponent];
    
    return GetCGColorComponent([m_nsColour CGColor], 0);
}

CGFloat wxNSColorRefData::Green() const
{
    wxOSXEffectiveAppearanceSetter helper;
    if ( NSColor* colRGBA = [m_nsColour colorUsingColorSpaceName:NSCalibratedRGBColorSpace] )
        return [colRGBA greenComponent];
    
    return GetCGColorComponent([m_nsColour CGColor], 1);
}

CGFloat wxNSColorRefData::Blue() const
{
    wxOSXEffectiveAppearanceSetter helper;
    if ( NSColor* colRGBA = [m_nsColour colorUsingColorSpaceName:NSCalibratedRGBColorSpace] )
        return [colRGBA blueComponent];
    
    return GetCGColorComponent([m_nsColour CGColor], 2);
}

CGFloat wxNSColorRefData::Alpha() const
{
    wxOSXEffectiveAppearanceSetter helper;
    if ( NSColor* colRGBA = [m_nsColour colorUsingColorSpaceName:NSCalibratedRGBColorSpace] )
        return [colRGBA alphaComponent];
    
    return GetCGColorComponent([m_nsColour CGColor], 3);
}

bool wxNSColorRefData::IsSolid() const
{
    return [m_nsColour colorUsingColorSpaceName:NSCalibratedRGBColorSpace] != nil;
}

CGColorRef wxNSColorRefData::GetCGColor() const
{
    wxOSXEffectiveAppearanceSetter helper;
    return [m_nsColour CGColor];
}

WX_NSImage wxNSColorRefData::GetNSPatternImage() const
{
    NSColor* colPat = [m_nsColour colorUsingColorSpaceName:NSPatternColorSpace];
    if ( colPat )
    {
        NSImage* nsimage = [colPat patternImage];
        if ( nsimage )
        {
            return nsimage;
        }
    }

    return nullptr;
}

WX_NSColor wxColourRefData::GetNSColor() const
{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1080
    if ( WX_IS_MACOS_AVAILABLE(10, 8) ) {
        wxOSXEffectiveAppearanceSetter helper;
        return [m_nsColour CGColor];
    }
#endif
    CGColorRef cgcolor = NULL;

    // Simplest case is when we can directly get the RGBA components:
    if (NSColor* colRGBA = [m_nsColour colorUsingColorSpaceName:NSCalibratedRGBColorSpace])
    {
        CGFloat components[4];
        [colRGBA getRed:&components[0] green:&components[1] blue:&components[2] alpha:&components[3]];

        cgcolor = CGColorCreate(wxMacGetGenericRGBColorSpace(), components);
    }
    // Some colours use patterns, we can handle them with the help of CGColorRef
    else if (NSColor* colPat = [m_nsColour colorUsingColorSpaceName:NSPatternColorSpace])
    {
        NSImage* const nsimage = [colPat patternImage];
        if (nsimage)
        {
            NSSize size = [nsimage size];
            NSRect r = NSMakeRect(0, 0, size.width, size.height);
            CGImageRef cgimage = [nsimage CGImageForProposedRect:&r context:nil hints:nil];
            if (cgimage)
            {
                // Callbacks for CGPatternCreate()
                struct PatternCreateCallbacks
                {
                    static void Draw(void* info, CGContextRef ctx)
                    {
                        CGImageRef image = (CGImageRef)info;
                        CGContextDrawImage(
                            ctx,
                            CGRectMake(0, 0, CGImageGetWidth(image), CGImageGetHeight(image)),
                            image);
                    }

                    static void Release(void* WXUNUSED(info))
                    {
                        // Do not release the image here, we don't own it as it
                        // comes from NSImage.
                    }
                };

                const CGPatternCallbacks callbacks = {
                    /* version: */ 0,
                    &PatternCreateCallbacks::Draw,
                    &PatternCreateCallbacks::Release
                };

                CGPatternRef pattern = CGPatternCreate(
                                            cgimage,
                                            CGRectMake(0, 0, size.width, size.height),
                                            CGAffineTransformMake(1, 0, 0, 1, 0, 0),
                                            size.width,
                                            size.height,
                                            kCGPatternTilingConstantSpacing,
                                            /* isColored: */ true,
                                            &callbacks
                                       );
                CGColorSpaceRef space = CGColorSpaceCreatePattern(NULL);
                CGFloat components[1] = { 1.0 };
                cgcolor = CGColorCreateWithPattern(space, pattern, components);
                CGColorSpaceRelease(space);
                CGPatternRelease(pattern);
            }
        }
    }

    if (cgcolor == NULL)
    {
        // Don't assert here, this will more likely than not result in a crash as
        // colours are often created in drawing code which will be called again
        // when the assert dialog is shown, resulting in a recursive assertion
        // failure and, hence, a crash.
        NSLog(@"Failed to convert NSColor \"%@\" to CGColorRef.", m_nsColour);
    }
    return cgcolor;
}

WX_NSImage wxColourRefData::GetNSPatternImage() const
{
    return nullptr;
}

wxColour::wxColour(WX_NSColor col)
{
    m_refData = new wxNSColorRefData(col);
}
