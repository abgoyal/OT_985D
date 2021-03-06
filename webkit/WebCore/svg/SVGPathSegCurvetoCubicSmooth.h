

#ifndef SVGPathSegCurvetoCubicSmooth_h
#define SVGPathSegCurvetoCubicSmooth_h

#if ENABLE(SVG)

#include "SVGPathSeg.h"

namespace WebCore {

    class SVGPathSegCurvetoCubicSmooth : public SVGPathSeg {
    public:
        SVGPathSegCurvetoCubicSmooth(float x, float y, float x2, float y2)
        : m_x(x), m_y(y), m_x2(x2), m_y2(y2) { }

        virtual String toString() const { return pathSegTypeAsLetter() + String::format(" %.6lg %.6lg %.6lg %.6lg", m_x2, m_y2, m_x, m_y); }

        void setX(float x) { m_x = x; }
        float x() const { return m_x; }

        void setY(float y) { m_y = y; }
        float y() const { return m_y; }

        void setX2(float x2) { m_x2 = x2; }
        float x2() const { return m_x2; }

        void setY2(float y2) { m_y2 = y2; }
        float y2() const { return m_y2; }

    private:
        float m_x;
        float m_y;
        float m_x2;
        float m_y2;
    };

    class SVGPathSegCurvetoCubicSmoothAbs : public SVGPathSegCurvetoCubicSmooth { 
    public:
        static PassRefPtr<SVGPathSegCurvetoCubicSmoothAbs> create(float x, float y, float x2, float y2) { return adoptRef(new SVGPathSegCurvetoCubicSmoothAbs(x, y, x2, y2)); }

        virtual unsigned short pathSegType() const { return PATHSEG_CURVETO_CUBIC_SMOOTH_ABS; }
        virtual String pathSegTypeAsLetter() const { return "S"; }

    private:
        SVGPathSegCurvetoCubicSmoothAbs(float x, float y, float x2, float y2);
    };

    class SVGPathSegCurvetoCubicSmoothRel : public SVGPathSegCurvetoCubicSmooth { 
    public:
        static PassRefPtr<SVGPathSegCurvetoCubicSmoothRel> create(float x, float y, float x2, float y2) { return adoptRef(new SVGPathSegCurvetoCubicSmoothRel(x, y, x2, y2)); }

        virtual unsigned short pathSegType() const { return PATHSEG_CURVETO_CUBIC_SMOOTH_REL; }
        virtual String pathSegTypeAsLetter() const { return "s"; }

    private:
        SVGPathSegCurvetoCubicSmoothRel(float x, float y, float x2, float y2);
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif

// vim:ts=4:noet
