#include <gtest/gtest.h>
#include <textract.h>

using namespace imgstr;

TEST(SimilaritySuite, SingleString) {

  using namespace std;

  string a = "intention";
  string b = "execution";

  EXPECT_EQ(levenshteinScore(a, b), 5);
}

TEST(SimilaritySuite, ImageSHA256Equal) {

  using namespace std;
  const string path = "../../../images/";
  string file_a = path + "screenshot.png";
  string file_b = path + "dupescreenshot.png";

  string sha_a = computeSHA256(file_a);
  string sha_b = computeSHA256(file_b);

  EXPECT_EQ(sha_a, sha_b);
}

TEST(SimilaritySuite, ImageSHA256Unequal) {

  using namespace std;
  const string path = "../../../images/";
  string file_a = path + "screenshot.png";
  string file_b = path + "imgtext.jpeg";

  string sha_a = computeSHA256(file_a);
  string sha_b = computeSHA256(file_b);

  EXPECT_NE(sha_a, sha_b);
}