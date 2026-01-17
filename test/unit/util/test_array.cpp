#include <tuvx/util/array.hpp>

#include <cmath>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

using namespace tuvx::array;

// ============================================================================
// AlmostEqual Tests
// ============================================================================

TEST(AlmostEqualTest, ExactEquality)
{
  EXPECT_TRUE(AlmostEqual(1.0, 1.0));
  EXPECT_TRUE(AlmostEqual(0.0, 0.0));
  EXPECT_TRUE(AlmostEqual(-1.0, -1.0));
}

TEST(AlmostEqualTest, WithinTolerance)
{
  EXPECT_TRUE(AlmostEqual(1.0, 1.0 + 1e-10));
  EXPECT_TRUE(AlmostEqual(1.0, 1.0 - 1e-10));
  EXPECT_TRUE(AlmostEqual(1000.0, 1000.0 + 1e-7));
}

TEST(AlmostEqualTest, OutsideTolerance)
{
  EXPECT_FALSE(AlmostEqual(1.0, 1.1));
  EXPECT_FALSE(AlmostEqual(1.0, 2.0));
  EXPECT_FALSE(AlmostEqual(0.0, 1e-10, 1e-12, 1e-15));  // Tighter tolerance
}

TEST(AlmostEqualTest, ZeroComparisons)
{
  EXPECT_TRUE(AlmostEqual(0.0, 0.0));
  EXPECT_TRUE(AlmostEqual(0.0, 1e-13));           // Within default abs_tol
  EXPECT_FALSE(AlmostEqual(0.0, 1e-10, 1e-12, 1e-15));  // With tighter tolerance
}

TEST(AlmostEqualTest, NaN)
{
  double nan = std::numeric_limits<double>::quiet_NaN();
  EXPECT_FALSE(AlmostEqual(nan, nan));
  EXPECT_FALSE(AlmostEqual(nan, 1.0));
  EXPECT_FALSE(AlmostEqual(1.0, nan));
}

TEST(AlmostEqualTest, Infinity)
{
  double inf = std::numeric_limits<double>::infinity();
  double neg_inf = -std::numeric_limits<double>::infinity();

  EXPECT_TRUE(AlmostEqual(inf, inf));
  EXPECT_TRUE(AlmostEqual(neg_inf, neg_inf));
  EXPECT_FALSE(AlmostEqual(inf, neg_inf));
  EXPECT_FALSE(AlmostEqual(inf, 1.0));
  EXPECT_FALSE(AlmostEqual(1.0, inf));
}

TEST(AlmostEqualTest, CustomTolerances)
{
  EXPECT_TRUE(AlmostEqual(1.0, 1.01, 0.02));  // 2% relative tolerance
  EXPECT_FALSE(AlmostEqual(1.0, 1.01, 0.005));  // 0.5% relative tolerance

  EXPECT_TRUE(AlmostEqual(0.0, 0.001, 1e-9, 0.01));  // Large absolute tolerance
  EXPECT_FALSE(AlmostEqual(0.0, 0.001, 1e-9, 0.0001));  // Small absolute tolerance
}

// ============================================================================
// FindString Tests
// ============================================================================

TEST(FindStringTest, CaseInsensitiveMatch)
{
  std::vector<std::string> array = { "Hello", "World", "Test" };

  auto result = FindString(array, "hello");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 0u);

  result = FindString(array, "WORLD");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 1u);

  result = FindString(array, "TeSt");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 2u);
}

TEST(FindStringTest, CaseSensitiveMatch)
{
  std::vector<std::string> array = { "Hello", "World", "Test" };

  auto result = FindString(array, "Hello", true);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 0u);

  result = FindString(array, "hello", true);
  EXPECT_FALSE(result.has_value());
}

TEST(FindStringTest, NotFound)
{
  std::vector<std::string> array = { "Hello", "World", "Test" };

  auto result = FindString(array, "NotHere");
  EXPECT_FALSE(result.has_value());
}

TEST(FindStringTest, EmptyArray)
{
  std::vector<std::string> array;

  auto result = FindString(array, "test");
  EXPECT_FALSE(result.has_value());
}

TEST(FindStringTest, EmptyTarget)
{
  std::vector<std::string> array = { "Hello", "", "World" };

  auto result = FindString(array, "");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 1u);
}

TEST(FindStringTest, FirstMatchReturned)
{
  std::vector<std::string> array = { "test", "TEST", "Test" };

  auto result = FindString(array, "test");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 0u);  // Should return first match
}

// ============================================================================
// Linspace Tests
// ============================================================================

TEST(LinspaceTest, BasicLinspace)
{
  auto result = Linspace(0.0, 10.0, 11);

  ASSERT_EQ(result.size(), 11u);
  EXPECT_DOUBLE_EQ(result.front(), 0.0);
  EXPECT_DOUBLE_EQ(result.back(), 10.0);

  // Check uniform spacing
  for (std::size_t i = 1; i < result.size(); ++i)
  {
    EXPECT_NEAR(result[i] - result[i - 1], 1.0, 1e-10);
  }
}

TEST(LinspaceTest, TwoElements)
{
  auto result = Linspace(0.0, 1.0, 2);

  ASSERT_EQ(result.size(), 2u);
  EXPECT_DOUBLE_EQ(result[0], 0.0);
  EXPECT_DOUBLE_EQ(result[1], 1.0);
}

TEST(LinspaceTest, SingleElement)
{
  auto result = Linspace(5.0, 10.0, 1);

  ASSERT_EQ(result.size(), 1u);
  EXPECT_DOUBLE_EQ(result[0], 5.0);
}

TEST(LinspaceTest, ZeroElements)
{
  auto result = Linspace(0.0, 10.0, 0);
  EXPECT_TRUE(result.empty());
}

TEST(LinspaceTest, NegativeRange)
{
  auto result = Linspace(-5.0, 5.0, 11);

  ASSERT_EQ(result.size(), 11u);
  EXPECT_DOUBLE_EQ(result.front(), -5.0);
  EXPECT_DOUBLE_EQ(result.back(), 5.0);
  EXPECT_NEAR(result[5], 0.0, 1e-10);
}

TEST(LinspaceTest, ReversedRange)
{
  auto result = Linspace(10.0, 0.0, 11);

  ASSERT_EQ(result.size(), 11u);
  EXPECT_DOUBLE_EQ(result.front(), 10.0);
  EXPECT_DOUBLE_EQ(result.back(), 0.0);

  // Should be decreasing
  for (std::size_t i = 1; i < result.size(); ++i)
  {
    EXPECT_LT(result[i], result[i - 1]);
  }
}

// ============================================================================
// Logspace Tests
// ============================================================================

TEST(LogspaceTest, BasicLogspace)
{
  auto result = Logspace(1.0, 1000.0, 4);

  ASSERT_EQ(result.size(), 4u);
  EXPECT_DOUBLE_EQ(result[0], 1.0);
  EXPECT_NEAR(result[1], 10.0, 1e-10);
  EXPECT_NEAR(result[2], 100.0, 1e-8);
  EXPECT_DOUBLE_EQ(result[3], 1000.0);
}

TEST(LogspaceTest, TwoElements)
{
  auto result = Logspace(1.0, 100.0, 2);

  ASSERT_EQ(result.size(), 2u);
  EXPECT_DOUBLE_EQ(result[0], 1.0);
  EXPECT_DOUBLE_EQ(result[1], 100.0);
}

TEST(LogspaceTest, SingleElement)
{
  auto result = Logspace(10.0, 100.0, 1);

  ASSERT_EQ(result.size(), 1u);
  EXPECT_DOUBLE_EQ(result[0], 10.0);
}

TEST(LogspaceTest, ZeroElements)
{
  auto result = Logspace(1.0, 100.0, 0);
  EXPECT_TRUE(result.empty());
}

TEST(LogspaceTest, InvalidMinZero)
{
  auto result = Logspace(0.0, 100.0, 5);
  EXPECT_TRUE(result.empty());
}

TEST(LogspaceTest, InvalidMinNegative)
{
  auto result = Logspace(-1.0, 100.0, 5);
  EXPECT_TRUE(result.empty());
}

TEST(LogspaceTest, InvalidMaxZero)
{
  auto result = Logspace(1.0, 0.0, 5);
  EXPECT_TRUE(result.empty());
}

TEST(LogspaceTest, SmallValues)
{
  auto result = Logspace(1e-10, 1e-5, 6);

  ASSERT_EQ(result.size(), 6u);
  EXPECT_DOUBLE_EQ(result[0], 1e-10);
  EXPECT_DOUBLE_EQ(result[5], 1e-5);

  // Check logarithmic spacing
  for (std::size_t i = 1; i < result.size(); ++i)
  {
    double ratio = result[i] / result[i - 1];
    EXPECT_NEAR(ratio, 10.0, 1e-6);  // Each step should be factor of 10
  }
}

// ============================================================================
// MergeSorted Tests
// ============================================================================

TEST(MergeSortedTest, NonOverlapping)
{
  std::vector<double> a = { 1.0, 2.0, 3.0 };
  std::vector<double> b = { 4.0, 5.0, 6.0 };

  auto result = MergeSorted(a, b);

  ASSERT_EQ(result.size(), 6u);
  EXPECT_DOUBLE_EQ(result[0], 1.0);
  EXPECT_DOUBLE_EQ(result[1], 2.0);
  EXPECT_DOUBLE_EQ(result[2], 3.0);
  EXPECT_DOUBLE_EQ(result[3], 4.0);
  EXPECT_DOUBLE_EQ(result[4], 5.0);
  EXPECT_DOUBLE_EQ(result[5], 6.0);
}

TEST(MergeSortedTest, Interleaved)
{
  std::vector<double> a = { 1.0, 3.0, 5.0 };
  std::vector<double> b = { 2.0, 4.0, 6.0 };

  auto result = MergeSorted(a, b);

  ASSERT_EQ(result.size(), 6u);
  for (std::size_t i = 0; i < 6; ++i)
  {
    EXPECT_DOUBLE_EQ(result[i], static_cast<double>(i + 1));
  }
}

TEST(MergeSortedTest, WithDuplicates)
{
  std::vector<double> a = { 1.0, 2.0, 3.0 };
  std::vector<double> b = { 2.0, 3.0, 4.0 };

  auto result = MergeSorted(a, b);

  ASSERT_EQ(result.size(), 4u);
  EXPECT_DOUBLE_EQ(result[0], 1.0);
  EXPECT_DOUBLE_EQ(result[1], 2.0);
  EXPECT_DOUBLE_EQ(result[2], 3.0);
  EXPECT_DOUBLE_EQ(result[3], 4.0);
}

TEST(MergeSortedTest, NearDuplicatesWithTolerance)
{
  std::vector<double> a = { 1.0, 2.0, 3.0 };
  std::vector<double> b = { 2.0 + 1e-11, 3.0 - 1e-11 };  // Near duplicates

  auto result = MergeSorted(a, b);

  ASSERT_EQ(result.size(), 3u);  // Duplicates should be merged
}

TEST(MergeSortedTest, EmptyArrays)
{
  std::vector<double> a = { 1.0, 2.0 };
  std::vector<double> b;

  auto result = MergeSorted(a, b);
  EXPECT_EQ(result.size(), 2u);

  result = MergeSorted(b, a);
  EXPECT_EQ(result.size(), 2u);

  result = MergeSorted(b, b);
  EXPECT_TRUE(result.empty());
}

TEST(MergeSortedTest, IdenticalArrays)
{
  std::vector<double> a = { 1.0, 2.0, 3.0 };
  std::vector<double> b = { 1.0, 2.0, 3.0 };

  auto result = MergeSorted(a, b);

  ASSERT_EQ(result.size(), 3u);
}

TEST(MergeSortedTest, SingleElements)
{
  std::vector<double> a = { 1.0 };
  std::vector<double> b = { 2.0 };

  auto result = MergeSorted(a, b);

  ASSERT_EQ(result.size(), 2u);
  EXPECT_DOUBLE_EQ(result[0], 1.0);
  EXPECT_DOUBLE_EQ(result[1], 2.0);
}

TEST(MergeSortedTest, CustomTolerance)
{
  std::vector<double> a = { 1.0, 2.0 };
  std::vector<double> b = { 1.5, 2.1 };

  // With very small tolerance, 2.0 and 2.1 are different
  auto result = MergeSorted(a, b, 1e-10);
  EXPECT_EQ(result.size(), 4u);

  // With larger tolerance, 2.0 and 2.1 might still be different since tolerance is 0.1
  result = MergeSorted(a, b, 0.15);
  EXPECT_EQ(result.size(), 3u);  // 2.0 and 2.1 merged
}
