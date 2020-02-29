#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

namespace Rinx {
class RxNoncopyable
{
public:
    RxNoncopyable(const RxNoncopyable&) = delete;
    void operator=(const RxNoncopyable&) = delete;

protected:
    RxNoncopyable() = default;
    ~RxNoncopyable() = default;
};

} //namespace Rinx
#endif // NONCOPYABLE_H
