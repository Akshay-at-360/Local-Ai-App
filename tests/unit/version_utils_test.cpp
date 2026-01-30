#include <gtest/gtest.h>
#include "ondeviceai/version_utils.hpp"

using namespace ondeviceai::version;

// Test semantic version parsing
TEST(VersionUtilsTest, ParseValidSemanticVersion) {
    SemanticVersion ver;
    
    ASSERT_TRUE(SemanticVersion::parse("1.2.3", ver));
    EXPECT_EQ(ver.major, 1);
    EXPECT_EQ(ver.minor, 2);
    EXPECT_EQ(ver.patch, 3);
    
    ASSERT_TRUE(SemanticVersion::parse("0.0.1", ver));
    EXPECT_EQ(ver.major, 0);
    EXPECT_EQ(ver.minor, 0);
    EXPECT_EQ(ver.patch, 1);
    
    ASSERT_TRUE(SemanticVersion::parse("10.20.30", ver));
    EXPECT_EQ(ver.major, 10);
    EXPECT_EQ(ver.minor, 20);
    EXPECT_EQ(ver.patch, 30);
}

TEST(VersionUtilsTest, ParseInvalidSemanticVersion) {
    SemanticVersion ver;
    
    // Missing components
    EXPECT_FALSE(SemanticVersion::parse("1.2", ver));
    EXPECT_FALSE(SemanticVersion::parse("1", ver));
    
    // Extra components
    EXPECT_FALSE(SemanticVersion::parse("1.2.3.4", ver));
    
    // Non-numeric
    EXPECT_FALSE(SemanticVersion::parse("a.b.c", ver));
    EXPECT_FALSE(SemanticVersion::parse("1.2.x", ver));
    
    // Empty
    EXPECT_FALSE(SemanticVersion::parse("", ver));
    
    // With prefix/suffix
    EXPECT_FALSE(SemanticVersion::parse("v1.2.3", ver));
    EXPECT_FALSE(SemanticVersion::parse("1.2.3-beta", ver));
}

TEST(VersionUtilsTest, VersionToString) {
    SemanticVersion ver(1, 2, 3);
    EXPECT_EQ(ver.toString(), "1.2.3");
    
    SemanticVersion ver2(0, 0, 1);
    EXPECT_EQ(ver2.toString(), "0.0.1");
    
    SemanticVersion ver3(10, 20, 30);
    EXPECT_EQ(ver3.toString(), "10.20.30");
}

TEST(VersionUtilsTest, VersionComparison) {
    SemanticVersion v1_0_0(1, 0, 0);
    SemanticVersion v1_0_1(1, 0, 1);
    SemanticVersion v1_1_0(1, 1, 0);
    SemanticVersion v2_0_0(2, 0, 0);
    
    // Test compare method
    EXPECT_EQ(v1_0_0.compare(v1_0_0), 0);
    EXPECT_LT(v1_0_0.compare(v1_0_1), 0);
    EXPECT_GT(v1_0_1.compare(v1_0_0), 0);
    
    EXPECT_LT(v1_0_0.compare(v1_1_0), 0);
    EXPECT_LT(v1_0_0.compare(v2_0_0), 0);
    EXPECT_LT(v1_1_0.compare(v2_0_0), 0);
    
    // Test isNewerThan
    EXPECT_TRUE(v1_0_1.isNewerThan(v1_0_0));
    EXPECT_TRUE(v1_1_0.isNewerThan(v1_0_1));
    EXPECT_TRUE(v2_0_0.isNewerThan(v1_1_0));
    EXPECT_FALSE(v1_0_0.isNewerThan(v1_0_1));
    EXPECT_FALSE(v1_0_0.isNewerThan(v1_0_0));
    
    // Test isOlderThan
    EXPECT_TRUE(v1_0_0.isOlderThan(v1_0_1));
    EXPECT_TRUE(v1_0_1.isOlderThan(v1_1_0));
    EXPECT_TRUE(v1_1_0.isOlderThan(v2_0_0));
    EXPECT_FALSE(v1_0_1.isOlderThan(v1_0_0));
    EXPECT_FALSE(v1_0_0.isOlderThan(v1_0_0));
}

TEST(VersionUtilsTest, VersionOperators) {
    SemanticVersion v1_0_0(1, 0, 0);
    SemanticVersion v1_0_1(1, 0, 1);
    SemanticVersion v1_1_0(1, 1, 0);
    SemanticVersion v2_0_0(2, 0, 0);
    SemanticVersion v1_0_0_copy(1, 0, 0);
    
    // Equality
    EXPECT_TRUE(v1_0_0 == v1_0_0_copy);
    EXPECT_FALSE(v1_0_0 == v1_0_1);
    
    // Inequality
    EXPECT_TRUE(v1_0_0 != v1_0_1);
    EXPECT_FALSE(v1_0_0 != v1_0_0_copy);
    
    // Less than
    EXPECT_TRUE(v1_0_0 < v1_0_1);
    EXPECT_TRUE(v1_0_1 < v1_1_0);
    EXPECT_TRUE(v1_1_0 < v2_0_0);
    EXPECT_FALSE(v1_0_1 < v1_0_0);
    EXPECT_FALSE(v1_0_0 < v1_0_0_copy);
    
    // Less than or equal
    EXPECT_TRUE(v1_0_0 <= v1_0_1);
    EXPECT_TRUE(v1_0_0 <= v1_0_0_copy);
    EXPECT_FALSE(v1_0_1 <= v1_0_0);
    
    // Greater than
    EXPECT_TRUE(v1_0_1 > v1_0_0);
    EXPECT_TRUE(v1_1_0 > v1_0_1);
    EXPECT_TRUE(v2_0_0 > v1_1_0);
    EXPECT_FALSE(v1_0_0 > v1_0_1);
    EXPECT_FALSE(v1_0_0 > v1_0_0_copy);
    
    // Greater than or equal
    EXPECT_TRUE(v1_0_1 >= v1_0_0);
    EXPECT_TRUE(v1_0_0 >= v1_0_0_copy);
    EXPECT_FALSE(v1_0_0 >= v1_0_1);
}

TEST(VersionUtilsTest, IsValidSemanticVersion) {
    EXPECT_TRUE(isValidSemanticVersion("1.2.3"));
    EXPECT_TRUE(isValidSemanticVersion("0.0.1"));
    EXPECT_TRUE(isValidSemanticVersion("10.20.30"));
    
    EXPECT_FALSE(isValidSemanticVersion("1.2"));
    EXPECT_FALSE(isValidSemanticVersion("1"));
    EXPECT_FALSE(isValidSemanticVersion("1.2.3.4"));
    EXPECT_FALSE(isValidSemanticVersion("a.b.c"));
    EXPECT_FALSE(isValidSemanticVersion(""));
    EXPECT_FALSE(isValidSemanticVersion("v1.2.3"));
}

TEST(VersionUtilsTest, CompareVersionStrings) {
    // Equal versions
    EXPECT_EQ(compareVersions("1.2.3", "1.2.3"), 0);
    
    // First version older
    EXPECT_LT(compareVersions("1.2.3", "1.2.4"), 0);
    EXPECT_LT(compareVersions("1.2.3", "1.3.0"), 0);
    EXPECT_LT(compareVersions("1.2.3", "2.0.0"), 0);
    
    // First version newer
    EXPECT_GT(compareVersions("1.2.4", "1.2.3"), 0);
    EXPECT_GT(compareVersions("1.3.0", "1.2.3"), 0);
    EXPECT_GT(compareVersions("2.0.0", "1.2.3"), 0);
    
    // Invalid versions
    EXPECT_EQ(compareVersions("invalid", "1.2.3"), -2);
    EXPECT_EQ(compareVersions("1.2.3", "invalid"), -2);
    EXPECT_EQ(compareVersions("invalid", "invalid"), -2);
}

TEST(VersionUtilsTest, MajorVersionChanges) {
    SemanticVersion v1(1, 0, 0);
    SemanticVersion v2(2, 0, 0);
    SemanticVersion v3(3, 0, 0);
    
    EXPECT_TRUE(v2 > v1);
    EXPECT_TRUE(v3 > v2);
    EXPECT_TRUE(v3 > v1);
    
    EXPECT_EQ(v1.compare(v2), -1);
    EXPECT_EQ(v2.compare(v3), -1);
    EXPECT_EQ(v3.compare(v1), 1);
}

TEST(VersionUtilsTest, MinorVersionChanges) {
    SemanticVersion v1_0(1, 0, 0);
    SemanticVersion v1_1(1, 1, 0);
    SemanticVersion v1_2(1, 2, 0);
    
    EXPECT_TRUE(v1_1 > v1_0);
    EXPECT_TRUE(v1_2 > v1_1);
    EXPECT_TRUE(v1_2 > v1_0);
    
    EXPECT_EQ(v1_0.compare(v1_1), -1);
    EXPECT_EQ(v1_1.compare(v1_2), -1);
    EXPECT_EQ(v1_2.compare(v1_0), 1);
}

TEST(VersionUtilsTest, PatchVersionChanges) {
    SemanticVersion v1_0_0(1, 0, 0);
    SemanticVersion v1_0_1(1, 0, 1);
    SemanticVersion v1_0_2(1, 0, 2);
    
    EXPECT_TRUE(v1_0_1 > v1_0_0);
    EXPECT_TRUE(v1_0_2 > v1_0_1);
    EXPECT_TRUE(v1_0_2 > v1_0_0);
    
    EXPECT_EQ(v1_0_0.compare(v1_0_1), -1);
    EXPECT_EQ(v1_0_1.compare(v1_0_2), -1);
    EXPECT_EQ(v1_0_2.compare(v1_0_0), 1);
}

TEST(VersionUtilsTest, MixedVersionComparisons) {
    SemanticVersion v1_0_0(1, 0, 0);
    SemanticVersion v1_0_1(1, 0, 1);
    SemanticVersion v1_1_0(1, 1, 0);
    SemanticVersion v2_0_0(2, 0, 0);
    
    // Major version takes precedence over minor and patch
    EXPECT_TRUE(v2_0_0 > v1_1_0);
    EXPECT_TRUE(v2_0_0 > v1_0_1);
    EXPECT_TRUE(v2_0_0 > v1_0_0);
    
    // Minor version takes precedence over patch
    EXPECT_TRUE(v1_1_0 > v1_0_1);
    EXPECT_TRUE(v1_1_0 > v1_0_0);
}

TEST(VersionUtilsTest, ZeroVersions) {
    SemanticVersion v0_0_0(0, 0, 0);
    SemanticVersion v0_0_1(0, 0, 1);
    SemanticVersion v0_1_0(0, 1, 0);
    SemanticVersion v1_0_0(1, 0, 0);
    
    EXPECT_TRUE(v0_0_1 > v0_0_0);
    EXPECT_TRUE(v0_1_0 > v0_0_1);
    EXPECT_TRUE(v1_0_0 > v0_1_0);
    
    EXPECT_EQ(v0_0_0.toString(), "0.0.0");
}

TEST(VersionUtilsTest, LargeVersionNumbers) {
    SemanticVersion v1(100, 200, 300);
    SemanticVersion v2(100, 200, 301);
    SemanticVersion v3(100, 201, 0);
    SemanticVersion v4(101, 0, 0);
    
    EXPECT_TRUE(v2 > v1);
    EXPECT_TRUE(v3 > v2);
    EXPECT_TRUE(v4 > v3);
    
    EXPECT_EQ(v1.toString(), "100.200.300");
}
