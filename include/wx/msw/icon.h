/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/icon.h
// Purpose:     wxIcon class
// Author:      Julian Smart
// Created:     01/02/97
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_ICON_H_
#define _WX_ICON_H_

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/msw/gdiimage.h"

// ---------------------------------------------------------------------------
// icon data
// ---------------------------------------------------------------------------

// notice that although wxIconRefData inherits from wxBitmapRefData, it is not
// a valid wxBitmapRefData
class WXDLLIMPEXP_CORE wxIconRefData : public wxGDIImageRefData
{
public:
    wxIconRefData() = default;
    virtual ~wxIconRefData() { Free(); }

    virtual void Free() override;
};

// ---------------------------------------------------------------------------
// Icon
// ---------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxIcon : public wxGDIImage
{
public:
    // ctors
        // default
    wxIcon() = default;

        // from raw data
    wxIcon(const char bits[], int width, int height);

        // from XPM data
    wxIcon(const char* const* data) { CreateIconFromXpm(data); }
        // from resource/file
    wxIcon(const wxString& name,
           wxBitmapType type = wxICON_DEFAULT_TYPE,
           int desiredWidth = -1, int desiredHeight = -1);

    wxIcon(const wxIconLocation& loc);

    virtual bool LoadFile(const wxString& name,
                          wxBitmapType type = wxICON_DEFAULT_TYPE,
                          int desiredWidth = -1, int desiredHeight = -1);

    bool CreateFromHICON(WXHICON icon);

    // implementation only from now on
    wxIconRefData *GetIconData() const { return (wxIconRefData *)m_refData; }

#if WXWIN_COMPATIBILITY_3_0
    wxDEPRECATED_INLINE(void SetHICON(WXHICON icon), SetHandle((WXHANDLE)icon); )
#endif // WXWIN_COMPATIBILITY_3_0

    WXHICON GetHICON() const { return (WXHICON)GetHandle(); }
    bool InitFromHICON(WXHICON icon, int width, int height, double scale = 1.0);

    // create from bitmap (which should have a mask unless it's monochrome):
    // there shouldn't be any implicit bitmap -> icon conversion (i.e. no
    // ctors, assignment operators...), but it's ok to have such function
    void CopyFromBitmap(const wxBitmap& bmp);

    wxDECLARE_VARIANT_OBJECT_EXPORTED(wxIcon, WXDLLIMPEXP_CORE);

protected:
    virtual wxGDIImageRefData *CreateData() const override
    {
        return new wxIconRefData;
    }

    wxNODISCARD virtual wxObjectRefData *CloneRefData(const wxObjectRefData *data) const override;

    // create from XPM data
    void CreateIconFromXpm(const char* const* data);

private:
    wxDECLARE_DYNAMIC_CLASS(wxIcon);
};

#endif
    // _WX_ICON_H_
