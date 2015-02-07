

#include "config.h"
#include "FontCache.h"
#include "FontPlatformData.h"
#include "Font.h"
#include "NotImplemented.h"
#include "SimpleFontData.h"
#include "SkPaint.h"
#include "SkTypeface.h"
#include "SkUtils.h"

namespace WebCore {

void FontCache::platformInit()
{
}

const SimpleFontData* FontCache::getFontDataForCharacters(const Font& font, const UChar* characters, int length)
{
    // since all of our fonts logically map to the fallback, we can always claim
    // that each font supports all characters.
    return font.primaryFont();
}

SimpleFontData* FontCache::getSimilarFontPlatformData(const Font& font)
{
    return 0;
}

SimpleFontData* FontCache::getLastResortFallbackFont(const FontDescription& font)
{
    static AtomicString str("sans-serif");
    return getCachedFontData(font, str);
}

static char* AtomicStringToUTF8String(const AtomicString& utf16)
{
    SkASSERT(sizeof(uint16_t) == sizeof(utf16.characters()[0]));
    const uint16_t* uni = (uint16_t*)utf16.characters();

    size_t bytes = SkUTF16_ToUTF8(uni, utf16.length(), NULL);
    char*  utf8 = (char*)sk_malloc_throw(bytes + 1);

    (void)SkUTF16_ToUTF8(uni, utf16.length(), utf8);
    utf8[bytes] = 0;
    return utf8;
}

FontPlatformData* FontCache::createFontPlatformData(const FontDescription& fontDescription, const AtomicString& family)
{
    char*       storage = 0;
    const char* name = 0;
    
    if (family.length() == 0) {
        static const struct {
            FontDescription::GenericFamilyType  mType;
            const char*                         mName;
        } gNames[] = {
            { FontDescription::SerifFamily,     "serif" },
            { FontDescription::SansSerifFamily, "sans-serif" },
            { FontDescription::MonospaceFamily, "monospace" },
            { FontDescription::CursiveFamily,   "cursive" },
            { FontDescription::FantasyFamily,   "fantasy" }
        };

        FontDescription::GenericFamilyType type = fontDescription.genericFamily();
        for (unsigned i = 0; i < SK_ARRAY_COUNT(gNames); i++)
        {
            if (type == gNames[i].mType)
            {
                name = gNames[i].mName;
                break;
            }
        }
        // if we fall out of the loop, its ok for name to still be 0
    }
    else {    // convert the name to utf8
        storage = AtomicStringToUTF8String(family);
        name = storage;
    }
    
    int style = SkTypeface::kNormal;
    if (fontDescription.weight() >= FontWeightBold)
        style |= SkTypeface::kBold;
    if (fontDescription.italic())
        style |= SkTypeface::kItalic;

    SkTypeface* tf = SkTypeface::CreateFromName(name, (SkTypeface::Style)style);
    
    FontPlatformData* result = new FontPlatformData(tf,
                                                    fontDescription.computedSize(),
                                                    (style & SkTypeface::kBold) && !tf->isBold(),
                                                    (style & SkTypeface::kItalic) && !tf->isItalic());
    tf->unref();
    sk_free(storage);
    return result;
}

    // new as of SVN change 36269, Sept 8, 2008
void FontCache::getTraitsInFamily(const AtomicString& familyName, Vector<unsigned>& traitsMasks)
{
    // Don't understand this yet, but it seems safe to leave unimplemented
}

}
