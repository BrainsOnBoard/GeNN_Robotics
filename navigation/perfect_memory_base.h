#pragma once

// Standard C includes
#include <cassert>
#include <cstdint>

// Standard C++ includes
#include <iostream>
#include <limits>
#include <tuple>

// OpenCV
#include <opencv2/opencv.hpp>

// BoB robotics includes
#include "common/image_database.h"

// Local includes
//#include "config.h"

namespace BoBRobotics {
namespace Navigation {
//------------------------------------------------------------------------
// BoBRobotics::Navigation::PerfectMemoryBase
//------------------------------------------------------------------------
class PerfectMemoryBase
{
public:
    PerfectMemoryBase(const cv::Size unwrapRes, const unsigned int scanStep,
                      const filesystem::path outputPath = "snapshots")
      : m_UnwrapRes(unwrapRes)
      , m_ScanStep(scanStep)
      , m_OutputPath(outputPath)
      , m_ScratchMaskImage(unwrapRes, CV_8UC1)
      , m_ScratchRollImage(unwrapRes, CV_8UC1)
    {}

    //------------------------------------------------------------------------
    // Declared virtuals
    //------------------------------------------------------------------------
    virtual size_t getNumSnapshots() const = 0;
    virtual const cv::Mat &getSnapshot(size_t index) const = 0;

    //------------------------------------------------------------------------
    // Public API
    //------------------------------------------------------------------------
    void loadSnapshots()
    {
        loadSnapshots(m_OutputPath);
    }

    void loadSnapshots(const filesystem::path &routePath)
    {
        for(size_t i = 0;;i++) {
            const auto filename = routePath / getRouteDatabaseFilename(i);
            if(!filename.exists()) {
                break;
            }

            // Load image
            cv::Mat image = cv::imread(filename.str(), cv::IMREAD_GRAYSCALE);
            assert(image.cols == m_UnwrapRes.width);
            assert(image.rows == m_UnwrapRes.height);
            assert(image.type() == CV_8UC1);

            // Add snapshot
            addSnapshot(image);
        }
    }

    void setMaskImage(const std::string path)
    {
        m_MaskImage = cv::imread(path, cv::IMREAD_GRAYSCALE);
        assert(m_MaskImage.cols == m_UnwrapRes.width);
        assert(m_MaskImage.rows == m_UnwrapRes.height);
        assert(m_MaskImage.type() == CV_8UC1);
    }

    size_t train(const cv::Mat &image)
    {
        assert(image.cols == m_UnwrapRes.width);
        assert(image.rows == m_UnwrapRes.height);
        assert(image.type() == CV_8UC1);

        // Add snapshot
        const size_t index = addSnapshot(image);
        
        // Save snapshot
        cv::imwrite(getSnapshotPath(index).str(), image);
        
        // Return index to snapshot
        return index;
    }

    std::tuple<float, size_t, float> findSnapshot(const cv::Mat &image) const
    {
        assert(image.cols == m_UnwrapRes.width);
        assert(image.rows == m_UnwrapRes.height);
        assert(image.type() == CV_8UC1);

        // Clone mask and image so they can be rolled inplace
        m_MaskImage.copyTo(m_ScratchMaskImage);
        image.copyTo(m_ScratchRollImage);

        // Scan across image columns
        float minDifferenceSquared = std::numeric_limits<float>::max();
        int bestCol = 0;
        size_t bestSnapshot = std::numeric_limits<size_t>::max();
        const size_t numSnapshots = getNumSnapshots();
        for(int i = 0; i < m_ScratchRollImage.cols; i += m_ScanStep) {
            // Loop through snapshots
            for(size_t s = 0; s < numSnapshots; s++) {
                // Calculate difference
                const float differenceSquared = calcSnapshotDifferenceSquared(m_ScratchRollImage, m_ScratchMaskImage, s);

                // If this is an improvement - update
                if(differenceSquared < minDifferenceSquared) {
                    minDifferenceSquared = differenceSquared;
                    bestCol = i;
                    bestSnapshot = s;
                }
            }

            // Roll image and corresponding mask left by m_ScanStep
            rollImage(m_ScratchRollImage, m_ScanStep);
            if(!m_ScratchMaskImage.empty()) {
                rollImage(m_ScratchMaskImage, m_ScanStep);
            }
        }

        // If best column is more than 180 degrees away, flip
        if(bestCol > (m_UnwrapRes.width / 2)) {
            bestCol -= m_UnwrapRes.width;
        }

        // Convert column into angle
        constexpr float pi = 3.141592653589793238462643383279502884f;
        const float bestAngle = ((float)bestCol / (float)m_UnwrapRes.width) * (2.0 * pi);

        // Return result
        return std::make_tuple(bestAngle, bestSnapshot, minDifferenceSquared);
    }

protected:
    //------------------------------------------------------------------------
    // Declared virtuals
    //------------------------------------------------------------------------
    // Add a snapshot to memory and return its index
    virtual size_t addSnapshot(const cv::Mat &image) = 0;

    // Calculate difference between memory and snapshot with index
    virtual float calcSnapshotDifferenceSquared(const cv::Mat &image, const cv::Mat &imageMask, size_t snapshot) const = 0;

    //------------------------------------------------------------------------
    // Protected methods
    //------------------------------------------------------------------------
    // Gets mask image
    const cv::Mat &getMaskImage() const{ return m_MaskImage; }

private:
    //------------------------------------------------------------------------
    // Private methods
    //------------------------------------------------------------------------
    filesystem::path getSnapshotPath(size_t index) const
    {
        return m_OutputPath / getRouteDatabaseFilename(index);
    }

    //------------------------------------------------------------------------
    // Private static methods
    //------------------------------------------------------------------------
    // 'Rolls' an image scanStep to the left
    static void rollImage(cv::Mat &image, unsigned int scanStep)
    {
        // Buffer to hold scanStep of pixels
        uint8_t *rollBuffer = new uint8_t[scanStep];

        // Loop through rows
        for(int y = 0; y < image.rows; y++) {
            // Get pointer to start of row
            uint8_t *rowPtr = image.ptr(y);

            // Copy scanStep pixels at left hand size of row into buffer
            std::copy_n(rowPtr, scanStep, rollBuffer);

            // Copy rest of row back over pixels we've copied to buffer
            std::copy_n(rowPtr + scanStep, image.cols - scanStep, rowPtr);

            // Copy buffer back into row
            std::copy(rollBuffer, &rollBuffer[scanStep], rowPtr + (image.cols - scanStep));
        }

        // ** YUCK **
        delete rollBuffer;
    }

    //------------------------------------------------------------------------
    // Members
    //------------------------------------------------------------------------
    const cv::Size m_UnwrapRes;
    const unsigned int m_ScanStep;
    const filesystem::path m_OutputPath;
    cv::Mat m_MaskImage;

    mutable cv::Mat m_ScratchMaskImage;
    mutable cv::Mat m_ScratchRollImage;
}; // PerfectMemoryBase
} // Navigation
} // BoBRobotics