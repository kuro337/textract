#include <crypto.h>
#include <gtest/gtest.h>
#include <util.h>

namespace {
    constinit auto *const a       = "intention";
    constinit auto *const b       = "execution";
    constinit auto *const equal_1 = IMAGE_FOLDER_PATH "/screenshot.png";
    constinit auto *const equal_2 = IMAGE_FOLDER_PATH "/dupescreenshot.png";
    constinit auto *const unequal = IMAGE_FOLDER_PATH "/imgtext.jpeg";
} // namespace

TEST(SimilaritySuite, SingleString) { EXPECT_EQ(levenshteinScore(a, b), 5); }

TEST(SimilaritySuite, ImageSHA256Equal) {
    auto sha_a = computeSHA256(equal_1);
    auto sha_b = computeSHA256(equal_2);

    EXPECT_EQ(sha_a, sha_b);
}

TEST(SimilaritySuite, ImageSHA256Unequal) {
    auto sha_a = computeSHA256(equal_1);
    auto sha_b = computeSHA256(unequal);

    EXPECT_NE(sha_a, sha_b);
}
