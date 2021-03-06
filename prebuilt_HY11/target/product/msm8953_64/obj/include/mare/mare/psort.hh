// --~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~--
// Copyright 2013-2015 Qualcomm Technologies, Inc.
// All rights reserved.
// Confidential and Proprietary – Qualcomm Technologies, Inc.
// --~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~--
#pragma once

#include <mare/taskfactory.hh>
#include <mare/tuner.hh>

#include <mare/internal/patterns/psort.hh>

namespace mare {
namespace pattern {

template <typename Compare> class psorter;

template <typename Compare> psorter<Compare> create_psort(Compare&& cmp);

template <typename Compare>
class psorter {
public:
  template <typename RandomAccessIterator>
  void run(RandomAccessIterator first, RandomAccessIterator last, const mare::pattern::tuner& t = mare::pattern::tuner()) {
    internal::psort_internal(first, last, _cmp, t);
  }

  template <typename RandomAccessIterator>
  void operator()(RandomAccessIterator first, RandomAccessIterator last, const mare::pattern::tuner& t = mare::pattern::tuner()) {
    run(first, last, t);
  }

  explicit psorter(Compare&& cmp) : _cmp(cmp) {}

private:
  Compare _cmp;
  friend psorter create_psort<Compare>(Compare&& cmp);
  template<typename Cmp, typename ...Args>
  friend mare::task_ptr<void()> mare::create_task(const psorter<Cmp>& p, Args&&...args);
};

template <typename Compare>
psorter<Compare> create_psort(Compare&& cmp) {
  using traits = internal::function_traits<Compare>;

  static_assert(traits::arity::value == 2, "psort takes a function accepting two arguments");

  return psorter<Compare>(std::forward<Compare>(cmp));
}

};

template <class RandomAccessIterator, class Compare>
void psort(RandomAccessIterator first, RandomAccessIterator last, Compare cmp,
    const mare::pattern::tuner& tuner = mare::pattern::tuner())
{
  auto p = mare::pattern::create_psort(cmp);
  p(first, last, tuner);
}

template <class RandomAccessIterator, class Compare>
mare::task_ptr<void()>
psort_async(RandomAccessIterator first, RandomAccessIterator last, Compare&& cmp,
    const mare::pattern::tuner& tuner = mare::pattern::tuner())
{
  return mare::internal::psort_async
    (std::forward<Compare>(cmp), first, last, tuner);
}

template <class RandomAccessIterator>
void psort(RandomAccessIterator first, RandomAccessIterator last,
    const mare::pattern::tuner& tuner = mare::pattern::tuner())
{
  auto p = mare::pattern::create_psort(std::less<typename std::iterator_traits<RandomAccessIterator>::value_type>());
  p(first, last, tuner);
}

template <class RandomAccessIterator>
mare::task_ptr<void()>
psort_async(RandomAccessIterator first, RandomAccessIterator last,
    const mare::pattern::tuner& tuner = mare::pattern::tuner())
{
  auto g = internal::legacy::create_group();
  auto t = mare::create_task([g, first, last, tuner]{
      internal::psort_internal(first, last, std::less<typename std::iterator_traits<RandomAccessIterator>::value_type>(), tuner);
      internal::legacy::finish_after(g);
  });
  auto gptr = internal::c_ptr(g);
  gptr->set_representative_task(internal::c_ptr(t));
  return t;
}

template <typename Compare, typename ...Args>
mare::task_ptr<void()>
create_task(const mare::pattern::psorter<Compare>& p, Args&&...args)
{
  return mare::internal::psort_async(p._cmp, args...);
}

template <typename Compare, typename ...Args>
mare::task_ptr<void()>
launch(const mare::pattern::psorter<Compare>& p, Args&&...args)
{
  auto t = mare::create_task(p, args...);
  t->launch();
  return t;
}

};
