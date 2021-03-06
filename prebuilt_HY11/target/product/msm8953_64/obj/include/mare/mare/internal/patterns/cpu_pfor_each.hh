// --~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~--
// Copyright 2013-2015 Qualcomm Technologies, Inc.
// All rights reserved.
// Confidential and Proprietary – Qualcomm Technologies, Inc.
// --~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~--
#pragma once

#include <mare/range.hh>

#include <mare/internal/patterns/common.hh>

namespace mare {

namespace internal {

template <size_t Dims, typename UnaryFn>
struct pfor_each_range;

template <typename UnaryFn>
struct pfor_each_range<1, UnaryFn> {
  static void pfor_each_range_impl(group_shared_ptr group,
                                   const mare::range<1>& r,
                                   UnaryFn fn,
                                   const mare::pattern::tuner& tuner)
  {
    pfor_each_internal(group, r.begin(0), r.end(0),
                       [&r, fn](size_t i) {
                         index<1> idx(i);
                         fn(idx);
                       }, r.stride(0), tuner);
  }
};

template <typename UnaryFn>
struct pfor_each_range<2, UnaryFn> {
  static void pfor_each_range_impl(group_shared_ptr group,
                                   const mare::range<2>& r,
                                   UnaryFn fn,
                                   const mare::pattern::tuner& tuner)
  {
    pfor_each_internal(group, r.begin(0), r.end(0),
                       [&r, fn](size_t i) {
                          for (size_t j = r.begin(1); j < r.end(1); j += r.stride(1)) {
                            index<2> idx(i, j);
                            fn(idx);
                          }
                        }, r.stride(0), tuner);
  }
};

template <typename UnaryFn>
struct pfor_each_range<3, UnaryFn> {
  static void pfor_each_range_impl(group_shared_ptr group,
                                   const mare::range<3>& r,
                                   UnaryFn fn,
                                   const mare::pattern::tuner& tuner)
  {
    pfor_each_internal(group, r.begin(0), r.end(0),
                       [&r, fn](size_t i) {
                          for (size_t j = r.begin(1); j < r.end(1); j += r.stride(1)) {
                            for (size_t k = r.begin(2); k < r.end(2); k += r.stride(2)) {
                              index<3> idx(i, j, k);
                              fn(idx);
                            }
                          }
                        }, r.stride(0), tuner);
  }
};

MARE_MSC_IGNORE_BEGIN(4702)
template <class InputIterator, typename UnaryFn>
void
pfor_each_static(group_shared_ptr group,
                 InputIterator first,
                 InputIterator last,
                 UnaryFn&& fn, const size_t stride,
                 const mare::pattern::tuner& tuner) {
  if (first >= last)
    return ;

  auto g_internal = legacy::create_group();
  auto g = g_internal;
  if (group != nullptr)
    g = legacy::intersect(g, group);

  typedef typename internal::distance_helper<InputIterator>::_result_type working_type;

  const working_type GRANULARITY_MULTIPLIER = 4;

  const working_type dist = internal::distance(first,last);
  auto num_chunks = GRANULARITY_MULTIPLIER * static_cast<working_type>(tuner.get_doc());
  working_type blksz = std::max(working_type(1), dist / num_chunks);
  num_chunks = dist % blksz == 0 ? dist / blksz : dist / blksz + 1;

  MARE_INTERNAL_ASSERT(blksz > 0, "block size should be greater than zero");

  std::atomic<working_type> work_id(0);

  auto chunk_body = [first, last, blksz, num_chunks, stride, &work_id, fn] {
    while(1) {
      auto prev = work_id.fetch_add(1, mare::mem_order_relaxed);
      if (prev < num_chunks) {
        InputIterator lb = first + prev * blksz;
        working_type safeofs = std::min(working_type(internal::distance(lb, last)), blksz);
        InputIterator rb = lb + safeofs;
        InputIterator it = (lb - first) % stride == 0 ?
                           lb : first + (prev * blksz / stride + 1) * stride;
        while (it < rb) {
          fn(it);
          it += working_type(stride);
        }
      } else {
        break;
      }
    }
  };

  for (size_t i = 0; i < tuner.get_doc() - 1; ++i) {
    legacy::launch(g, chunk_body);
  }
  chunk_body();

  legacy::spin_wait_for(g);
}
MARE_MSC_IGNORE_END(4702)

template<typename Body>
void
pfor_each_dynamic(group_shared_ptr group, size_t first, size_t last,
                  Body&& body, const size_t stride,
                  const mare::pattern::tuner& tuner) {

  if (first >= last)
    return ;

  typedef ::mare::internal::legacy::body_wrapper<Body> pfor_body_wrapper;
  pfor_body_wrapper wrapper(std::forward<Body>(body));

  MARE_INTERNAL_ASSERT(pfor_body_wrapper::get_arity() == 1 ||
                       pfor_body_wrapper::get_arity() == 2,
                       "invalid number of arguments in body");

  auto body_attrs = wrapper.get_attrs();
  body_attrs = legacy::add_attr(body_attrs, attr::pfor);
  body_attrs = legacy::add_attr(body_attrs, attr::inlined);

  auto& fn = wrapper.get_fn();
  typedef typename std::remove_reference<decltype(fn)>::type UnaryFn;

  size_t blk_size = tuner.get_chunk_size();

  auto t = current_task();
  if (t && t->is_pfor()) {

    if (group == nullptr) {
      for (size_t i = first; i < last; i += stride)
        fn(i);
      return ;
    }

MARE_MSC_IGNORE_BEGIN(4702)
  auto inlined = ::mare::internal::legacy::create_task(legacy::with_attrs(body_attrs,
        [first, last, &fn, stride] {
          for (size_t i = first; i < last; i += stride) {
            fn(i);
          }
      }));
MARE_MSC_IGNORE_END(4702)
    c_ptr(inlined)->execute_inline_task(c_ptr(group));
    return;
  };

  auto g_internal = legacy::create_group();
  auto g = g_internal;
  if(group)
    g = legacy::intersect(g, group);

  typedef adaptive_steal_strategy<void, size_t, UnaryFn, UnaryFn, Policy::MAP> strategy_type;

  size_t num_stride = blk_size / stride;
  blk_size = blk_size % stride == 0 ?
    num_stride * stride : (num_stride + 1) * stride;

  mare_shared_ptr<strategy_type> strategy_ptr(new strategy_type(
                                                    g, first, last - 1,
                                                    std::forward<UnaryFn>(fn),
                                                    body_attrs,
                                                    blk_size,
                                                    stride,
                                                    tuner));

  size_t max_tasks = strategy_ptr->get_max_tasks();
  if (max_tasks > 1 && max_tasks < (last - first))
    strategy_ptr->static_split(max_tasks);

  execute_master_task<void, strategy_type, Policy::MAP>(strategy_ptr);

  legacy::spin_wait_for(g);

#ifdef MARE_ADAPTIVE_PFOR_DEBUG
  print_tree<void>(strategy_ptr->get_tree());
#endif
}

};
};
