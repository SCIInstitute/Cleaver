#include "ConstantField.h"

namespace cleaver{

template <class T>
ConstantField<T>::ConstantField(T value, const BoundingBox &bounds) :
    m_value(value), m_bounds(bounds)
{
}

template <class T>
ConstantField<T>::ConstantField(T value, int w, int h, int d) :
    m_value(value), m_bounds(BoundingBox(vec3::zero, vec3(w,h,d)))
{
}

template <class T>
void ConstantField<T>::setValue(T value)
{
    m_value = value;
}

template <class T>
double ConstantField<T>::valueAt(double x, double y, double z) const
{
    return m_value;
}

template <class T>
double ConstantField<T>::valueAt(const cleaver::vec3 &x) const
{
    return m_value;
}

template <class T>
cleaver::BoundingBox ConstantField<T>::bounds() const
{
    return m_bounds;
}

// explicit instantion of acceptable types
// TODO: find way to hide implementation but allow aribtrary instantion
template class ConstantField<char>;
template class ConstantField<int>;
template class ConstantField<float>;
template class ConstantField<double>;
template class ConstantField<unsigned char>;
template class ConstantField<unsigned int>;

template class ConstantField<long int>;
template class ConstantField<long double>;

}  // namespace Cleaver
