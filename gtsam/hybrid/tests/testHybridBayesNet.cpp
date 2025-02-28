/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file    testHybridBayesNet.cpp
 * @brief   Unit tests for HybridBayesNet
 * @author  Varun Agrawal
 * @author  Fan Jiang
 * @author  Frank Dellaert
 * @date    December 2021
 */

#include <gtsam/base/serializationTestHelpers.h>
#include <gtsam/hybrid/HybridBayesNet.h>
#include <gtsam/hybrid/HybridBayesTree.h>
#include <gtsam/nonlinear/NonlinearFactorGraph.h>

#include "Switching.h"

// Include for test suite
#include <CppUnitLite/TestHarness.h>

using namespace std;
using namespace gtsam;
using namespace gtsam::serializationTestHelpers;

using noiseModel::Isotropic;
using symbol_shorthand::M;
using symbol_shorthand::X;

static const DiscreteKey Asia(0, 2);

/* ****************************************************************************/
// Test creation
TEST(HybridBayesNet, Creation) {
  HybridBayesNet bayesNet;

  bayesNet.add(Asia, "99/1");

  DiscreteConditional expected(Asia, "99/1");

  CHECK(bayesNet.atDiscrete(0));
  auto& df = *bayesNet.atDiscrete(0);
  EXPECT(df.equals(expected));
}

/* ****************************************************************************/
// Test choosing an assignment of conditionals
TEST(HybridBayesNet, Choose) {
  Switching s(4);

  Ordering ordering;
  for (auto&& kvp : s.linearizationPoint) {
    ordering += kvp.key;
  }

  HybridBayesNet::shared_ptr hybridBayesNet;
  HybridGaussianFactorGraph::shared_ptr remainingFactorGraph;
  std::tie(hybridBayesNet, remainingFactorGraph) =
      s.linearizedFactorGraph.eliminatePartialSequential(ordering);

  DiscreteValues assignment;
  assignment[M(1)] = 1;
  assignment[M(2)] = 1;
  assignment[M(3)] = 0;

  GaussianBayesNet gbn = hybridBayesNet->choose(assignment);

  EXPECT_LONGS_EQUAL(4, gbn.size());

  EXPECT(assert_equal(*(*boost::dynamic_pointer_cast<GaussianMixture>(
                          hybridBayesNet->atMixture(0)))(assignment),
                      *gbn.at(0)));
  EXPECT(assert_equal(*(*boost::dynamic_pointer_cast<GaussianMixture>(
                          hybridBayesNet->atMixture(1)))(assignment),
                      *gbn.at(1)));
  EXPECT(assert_equal(*(*boost::dynamic_pointer_cast<GaussianMixture>(
                          hybridBayesNet->atMixture(2)))(assignment),
                      *gbn.at(2)));
  EXPECT(assert_equal(*(*boost::dynamic_pointer_cast<GaussianMixture>(
                          hybridBayesNet->atMixture(3)))(assignment),
                      *gbn.at(3)));
}

/* ****************************************************************************/
// Test bayes net optimize
TEST(HybridBayesNet, OptimizeAssignment) {
  Switching s(4);

  Ordering ordering;
  for (auto&& kvp : s.linearizationPoint) {
    ordering += kvp.key;
  }

  HybridBayesNet::shared_ptr hybridBayesNet;
  HybridGaussianFactorGraph::shared_ptr remainingFactorGraph;
  std::tie(hybridBayesNet, remainingFactorGraph) =
      s.linearizedFactorGraph.eliminatePartialSequential(ordering);

  DiscreteValues assignment;
  assignment[M(1)] = 1;
  assignment[M(2)] = 1;
  assignment[M(3)] = 1;

  VectorValues delta = hybridBayesNet->optimize(assignment);

  // The linearization point has the same value as the key index,
  // e.g. X(1) = 1, X(2) = 2,
  // but the factors specify X(k) = k-1, so delta should be -1.
  VectorValues expected_delta;
  expected_delta.insert(make_pair(X(1), -Vector1::Ones()));
  expected_delta.insert(make_pair(X(2), -Vector1::Ones()));
  expected_delta.insert(make_pair(X(3), -Vector1::Ones()));
  expected_delta.insert(make_pair(X(4), -Vector1::Ones()));

  EXPECT(assert_equal(expected_delta, delta));
}

/* ****************************************************************************/
// Test bayes net optimize
TEST(HybridBayesNet, Optimize) {
  Switching s(4);

  Ordering hybridOrdering = s.linearizedFactorGraph.getHybridOrdering();
  HybridBayesNet::shared_ptr hybridBayesNet =
      s.linearizedFactorGraph.eliminateSequential(hybridOrdering);

  HybridValues delta = hybridBayesNet->optimize();

  DiscreteValues expectedAssignment;
  expectedAssignment[M(1)] = 1;
  expectedAssignment[M(2)] = 0;
  expectedAssignment[M(3)] = 1;
  EXPECT(assert_equal(expectedAssignment, delta.discrete()));

  VectorValues expectedValues;
  expectedValues.insert(X(1), -0.999904 * Vector1::Ones());
  expectedValues.insert(X(2), -0.99029 * Vector1::Ones());
  expectedValues.insert(X(3), -1.00971 * Vector1::Ones());
  expectedValues.insert(X(4), -1.0001 * Vector1::Ones());

  EXPECT(assert_equal(expectedValues, delta.continuous(), 1e-5));
}

/* ****************************************************************************/
// Test bayes net multifrontal optimize
TEST(HybridBayesNet, OptimizeMultifrontal) {
  Switching s(4);

  Ordering hybridOrdering = s.linearizedFactorGraph.getHybridOrdering();
  HybridBayesTree::shared_ptr hybridBayesTree =
      s.linearizedFactorGraph.eliminateMultifrontal(hybridOrdering);
  HybridValues delta = hybridBayesTree->optimize();

  VectorValues expectedValues;
  expectedValues.insert(X(1), -0.999904 * Vector1::Ones());
  expectedValues.insert(X(2), -0.99029 * Vector1::Ones());
  expectedValues.insert(X(3), -1.00971 * Vector1::Ones());
  expectedValues.insert(X(4), -1.0001 * Vector1::Ones());

  EXPECT(assert_equal(expectedValues, delta.continuous(), 1e-5));
}

/* ****************************************************************************/
// Test HybridBayesNet serialization.
TEST(HybridBayesNet, Serialization) {
  Switching s(4);
  Ordering ordering = s.linearizedFactorGraph.getHybridOrdering();
  HybridBayesNet hbn = *(s.linearizedFactorGraph.eliminateSequential(ordering));

  EXPECT(equalsObj<HybridBayesNet>(hbn));
  EXPECT(equalsXML<HybridBayesNet>(hbn));
  EXPECT(equalsBinary<HybridBayesNet>(hbn));
}

/* ************************************************************************* */
int main() {
  TestResult tr;
  return TestRegistry::runAllTests(tr);
}
/* ************************************************************************* */
