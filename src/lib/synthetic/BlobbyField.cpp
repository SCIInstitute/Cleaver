#include "BlobbyField.h"
#include <cmath>
#include <cstdlib>

using namespace cleaver;

namespace blobby{
float randf()
{
    #ifdef RAND_MAX
    return ((float)rand() / RAND_MAX);
    #else
    return ((float)rand() / std::numeric_limits<int>::max());
    #endif
}
}

BlobbyField::BlobbyField(const vec3 &cx, float r, int nspheres, const BoundingBox &bounds) :
    m_cx(cx), m_r(r), m_bounds(bounds)
{
    //m_cx = bounds.center();

    a = 128.0;
    b = 64.0;
    c = 32;

    nspheres = (int)(5 + blobby::randf()*10);
    r = 0.5f*r + blobby::randf()*0.5f*r;

    for (int j=0; j < nspheres; j++){
        cleaver::vec3 pos;
        a = 2*r;
        b = a/2.0f;
        c = b/2.0f;
        pos[0] = m_cx[0] + a*(blobby::randf() - 0.5);
        pos[1] = m_cx[1] + a*(blobby::randf() - 0.5);
        pos[2] = m_cx[2] + a*(blobby::randf() - 0.5);
        spheres.push_back(pos);
    }
}

double BlobbyField::valueAt(double x, double y, double z) const
{
    return valueAt(vec3(x,y,z));
}

double BlobbyField::valueAt(const vec3 &x) const
{
    float d = 0;
    for (size_t i=0; i<spheres.size(); i++){
        float r = (float)std::abs(length(spheres[i] - x));
        d += meta(r);
        //d += 1/((1-r*r)*(1-r*r));
    }
    return d - c;
//    double d = m_r - fabs(length(x - m_cx));
//    return d;
}

void BlobbyField::setBounds(const cleaver::BoundingBox &bounds)
{
    m_bounds = bounds;
}


BoundingBox BlobbyField::bounds() const
{
    return m_bounds;
}

float BlobbyField::meta(float r) const
{
    if (r < b/3.0)
        return a * (1 - 3*r*r/ (b*b));
    else if (b/3.0 <= r && r <= b){
        return 3*a*0.5f * (1 - r/b)*(1 - r/b);
    }
    else
        return 0;

}
