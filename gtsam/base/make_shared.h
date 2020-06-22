/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2020, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file make_shared.h
 * @brief make_shared trampoline function to ensure proper alignment
 * @author Fan Jiang
 */

#pragma once

#include <gtsam/base/types.h>

#include <Eigen/Core>

#include <boost/make_shared.hpp>
#include <boost/utility/enable_if.hpp>


namespace gtsam {

  /// Add our own `make_shared` as a layer of wrapping on `boost::make_shared`
  template<typename T, typename ... Args>
  boost::enable_if_t<has_custom_allocator<T>::value, boost::shared_ptr<T>> make_shared(Args&&... args)
  {
    return boost::allocate_shared<T>(Eigen::aligned_allocator<T>(), std::forward<Args> (args)...);
  }

  /// Fall back to the boost version if no need for alignment
  template<typename T, typename ... Args>
  boost::enable_if_t<!has_custom_allocator<T>::value, boost::shared_ptr<T>> make_shared(Args&&... args)
  {
    return boost::make_shared<T>(std::forward<Args> (args)...);
  }

}
