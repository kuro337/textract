#include <folly/SharedMutex.h>
#include <iostream>

struct Image {
    struct WriteMetadata {
        std::string output_path;
        std::string write_timestamp;
        bool        output_written = false;
    };

    std::string   image_sha256;
    std::string   path;
    std::size_t   image_size {};
    std::string   content_fuzzhash;
    std::string   text_content;
    std::size_t   text_size {};
    std::string   time_processed;
    WriteMetadata write_info;

    mutable std::unique_ptr<folly::SharedMutex> mutex;

    Image(std::string path): path(std::move(path)), mutex(nullptr) {}

    // Function to update write metadata
    void updateWriteMetadata(const std::string &output_path,
                             const std::string &write_timestamp,
                             bool               output_written) {
        if (!mutex) {
            mutex = std::make_unique<folly::SharedMutex>();
        }
        std::unique_lock<folly::SharedMutex> writerLock(*mutex);
        write_info.output_path     = output_path;
        write_info.write_timestamp = write_timestamp;
        write_info.output_written  = output_written;
    }

    // Function to read write metadata
    auto readWriteMetadata() const -> WriteMetadata {
        if (!mutex) {
            return write_info; // If mutex is not initialized, no write has
                               // occurred
        }
        std::shared_lock<folly::SharedMutex> readerLock(*mutex);
        return write_info;
    }

    void logAlreadyWritten() const {
        std::cout << "Image already written by " << write_info.output_path << std::endl;
    }
};
#include <gtest/gtest.h>

class ImageTest: public ::testing::Test {
  protected:
    Image img {"test_path"};
};

TEST_F(ImageTest, MutexNullIfNoWrite) {
    EXPECT_EQ(img.mutex, nullptr); // Mutex should be null initially
}

TEST_F(ImageTest, MutexLazyInitialization) {
    EXPECT_EQ(img.mutex, nullptr); // Mutex should be null initially

    img.updateWriteMetadata("path/to/output", "2024-01-20", true);

    EXPECT_NE(img.mutex, nullptr); // now mutex exists - not nullptr
}

// Test for updating write metadata
TEST_F(ImageTest, UpdateWriteMetadata) {
    std::string new_output_path     = "new_path";
    std::string new_write_timestamp = "2024-01-20";
    bool        new_output_written  = true;

    img.updateWriteMetadata(new_output_path, new_write_timestamp, new_output_written);

    auto write_info = img.readWriteMetadata();
    EXPECT_EQ(write_info.output_path, new_output_path);
    EXPECT_EQ(write_info.write_timestamp, new_write_timestamp);
    EXPECT_EQ(write_info.output_written, new_output_written);
}

// Test for reading write metadata before any write
TEST_F(ImageTest, ReadWriteMetadataBeforeWrite) {
    auto write_info = img.readWriteMetadata();

    // Assuming default values before any write
    EXPECT_EQ(write_info.output_path, "");
    EXPECT_EQ(write_info.write_timestamp, "");
    EXPECT_FALSE(write_info.output_written);
}

TEST(ImageConcurrentWriteTest, ConcurrentWriteAttempts) {
    Image img("test_path");

    // Function to be run by threads
    auto writeAttempt = [&img](std::string path) {
        // Reading without a lock
        if (!img.write_info.output_written) {
            img.updateWriteMetadata(path, "2024-01-21", true);
        } else {
            img.logAlreadyWritten();
        }
    };

    std::thread writerThread1(writeAttempt, "thread 1 wrote");
    std::thread writerThread2(writeAttempt, "thread 2 wrote");
    std::thread writerThread3(writeAttempt, "thread 3 wrote");
    std::thread writerThread4(writeAttempt, "thread 4 wrote");

    // Wait for threads to complete
    EXPECT_NO_THROW(writerThread1.join());
    EXPECT_NO_THROW(writerThread2.join());
    EXPECT_NO_THROW(writerThread3.join());
    EXPECT_NO_THROW(writerThread4.join());
}
