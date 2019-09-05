#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H


class RxNoncopyable
{
 public:
  RxNoncopyable(const RxNoncopyable&) = delete;
  void operator=(const RxNoncopyable&) = delete;

 protected:
  RxNoncopyable() = default;
  ~RxNoncopyable() = default;
};
#endif // NONCOPYABLE_H
