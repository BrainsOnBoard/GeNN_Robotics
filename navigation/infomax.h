#pragma once

// BoB robotics includes
#include "../common/assert.h"
#include "../common/logging.h"
#include "insilico_rotater.h"
#include "visual_navigation_base.h"

// Third-party includes
#include "../third_party/units.h"

// Eigen
#include <Eigen/Core>

// OpenCV
#include <opencv2/opencv.hpp>

// Standard C includes
#include <cmath>

// Standard C++ includes
#include <algorithm>
#include <iostream>
#include <random>
#include <tuple>
#include <utility>
#include <vector>

namespace BoBRobotics {
namespace Navigation {
//------------------------------------------------------------------------
// BoBRobotics::Navigation::InfoMax
//------------------------------------------------------------------------
template<typename FloatType = float>
class InfoMax : public VisualNavigationBase
{
    using MatrixType = Eigen::Matrix<FloatType, Eigen::Dynamic, Eigen::Dynamic>;
    using VectorType = Eigen::Matrix<FloatType, Eigen::Dynamic, 1>;

public:
    InfoMax(const cv::Size &unwrapRes,
            const MatrixType &initialWeights,
            FloatType learningRate = 0.0001,
            size_t numNonRetinatopicInputs = 0,
            size_t nonRetinatopicInputPixels = 1)
      : VisualNavigationBase(unwrapRes),
        m_NumNonRetinatopicInputs(numNonRetinatopicInputs),
        m_NonRetinatopicInputPixels(nonRetinatopicInputPixels),
        m_NumNonRetinatopicRows(ceil((double)(numNonRetinatopicInputs * nonRetinatopicInputPixels) / (double)unwrapRes.width)),
        m_ZeroNonRetinatopicInput(numNonRetinatopicInputs, 0),
        m_LearningRate(learningRate),
        m_Weights(initialWeights)
    {
        // Create 'fused' image to hold unwrapped image AND non-retinatopic inputs
        m_FusedImage.create(unwrapRes.height + m_NumNonRetinatopicRows, unwrapRes.width, CV_8UC1);
    }

    InfoMax(const cv::Size &unwrapRes,
            FloatType learningRate = 0.0001,
            size_t numNonRetinatopicInputs = 0,
            size_t nonRetinatopicInputPixels = 1)
      : VisualNavigationBase(unwrapRes),
        m_NumNonRetinatopicInputs(numNonRetinatopicInputs),
        m_NonRetinatopicInputPixels(nonRetinatopicInputPixels),
        m_NumNonRetinatopicRows(ceil((double)(numNonRetinatopicInputs * nonRetinatopicInputPixels) / (double)unwrapRes.width)),
        m_ZeroNonRetinatopicInput(numNonRetinatopicInputs, 0),
        m_LearningRate(learningRate),
        m_Weights(getInitialWeights((unwrapRes.width * unwrapRes.height) + (numNonRetinatopicInputs * nonRetinatopicInputPixels),
                                    1 + (unwrapRes.width * unwrapRes.height) + (numNonRetinatopicInputs * nonRetinatopicInputPixels)))
    {
        // Create 'fused' image to hold unwrapped image AND non-retinatopic inputs
        m_FusedImage.create(unwrapRes.height + m_NumNonRetinatopicRows, unwrapRes.width, CV_8UC1);
    }

    //------------------------------------------------------------------------
    // VisualNavigationBase virtuals
    //------------------------------------------------------------------------
    virtual void train(const cv::Mat &image) override
    {
        train(image, m_ZeroNonRetinatopicInput);
    }

    virtual float test(const cv::Mat &image) const override
    {
        return test(image, m_ZeroNonRetinatopicInput);
    }

    //! Generates new random weights
    virtual void clearMemory() override
    {
        m_Weights = getInitialWeights(m_Weights.cols(), m_Weights.rows());
    }

    //------------------------------------------------------------------------
    // Public API
    //------------------------------------------------------------------------
    const MatrixType &getWeights() const
    {
        return m_Weights;
    }

    void train(const cv::Mat &image, const std::vector<FloatType> &nonRetinatopicInput)
    {
        setFusedImage(image);
        setFusedNonRetinatopic(nonRetinatopicInput);

        calculateUY(m_FusedImage);
        trainUY();
    }

    float test(const cv::Mat &image, const std::vector<FloatType> &nonRetinatopicInput) const
    {
        setFusedImage(image);
        setFusedNonRetinatopic(nonRetinatopicInput);

        const auto decs = m_Weights * getFloatVector(m_FusedImage);
        return decs.array().abs().sum();
    }

#ifndef EXPOSE_INFOMAX_INTERNALS
    private:
#endif
    void setFusedImage(const cv::Mat &image) const
    {
        const cv::Size &unwrapRes = getUnwrapResolution();
        cv::Mat imageROI = m_FusedImage(cv::Rect(0, 0, unwrapRes.width, unwrapRes.height));

        // Copy image into ROI
        image.copyTo(imageROI);
    }

    void setFusedNonRetinatopic(const std::vector<FloatType> &nonRetinatopicInput) const
    {
        BOB_ASSERT(nonRetinatopicInput.size() == m_NumNonRetinatopicInputs);

        const cv::Size &unwrapRes = getUnwrapResolution();
        cv::Mat nonRetinatopicROI = m_FusedImage(cv::Rect(0, unwrapRes.height,
                                                          unwrapRes.width, unwrapRes.height + m_NumNonRetinatopicRows));

        // Loop through non-retinatopic inputs
        auto nonRetinatopicIter = nonRetinatopicROI.begin<uint8_t>();
        for(FloatType r : nonRetinatopicInput) {
            // Conver component to a byte
            const uint8_t byte = (uint8_t)std::max(0.0f, std::min(255.0f, std::round(r * 255.0f)));

            for(size_t i = 0; i < m_NonRetinatopicInputPixels; i++) {
                *nonRetinatopicIter++ = byte;
            }
        }

        // Fill remainder of ROI with zeros
        std::fill(nonRetinatopicIter, nonRetinatopicROI.end<uint8_t>(), 0);
    }

    void trainUY()
    {
        // weights = weights + lrate/N * (eye(H)-(y+u)*u') * weights;
        const auto id = MatrixType::Identity(m_Weights.rows(), m_Weights.rows());
        const auto sumYU = (m_Y.array() + m_U.array()).matrix();
        const FloatType learnRate = m_LearningRate / (FloatType) m_U.rows();
        m_Weights.array() += (learnRate * (id - sumYU * m_U.transpose()) * m_Weights).array();
    }

    void calculateUY(const cv::Mat &image)
    {
        BOB_ASSERT(image.type() == CV_8UC1);

        const cv::Size &unwrapRes = getUnwrapResolution();
        BOB_ASSERT(image.cols == unwrapRes.width);
        BOB_ASSERT(image.rows == unwrapRes.height);

        // Convert image to vector of floats
        m_U = m_Weights * getFloatVector(image);
        m_Y = tanh(m_U.array());
    }

    std::pair<VectorType, VectorType> getUY() const
    {
        // Copy the vectors
        return std::make_pair<>(m_U, m_Y);
    }

    static MatrixType getInitialWeights(const int numInputs,
                                        const int numHidden,
                                        const unsigned seed = std::random_device()())
    {
        // Note that we transpose this matrix after normalisation
        MatrixType weights(numInputs, numHidden);

        LOG_INFO << "Seed for weights is: " << seed;

        std::default_random_engine generator(seed);
        std::normal_distribution<FloatType> distribution;
        for (int i = 0; i < numInputs; i++) {
            for (int j = 0; j < numHidden; j++) {
                weights(i, j) = distribution(generator);
            }
        }

        LOG_VERBOSE << "Initial weights" << std::endl << weights;

        // Normalise mean and SD for row so mean == 0 and SD == 1
        const auto means = weights.rowwise().mean();
        LOG_VERBOSE << "Means" << std::endl << means;

        weights.colwise() -= means;
        LOG_VERBOSE << "Weights after subtracting means" << std::endl << weights;

        LOG_VERBOSE << "New means" << std::endl << weights.rowwise().mean();

        const auto sd = matrixSD(weights);
        LOG_VERBOSE << "SD" << std::endl << sd;

        weights = weights.array().colwise() / sd;
        LOG_VERBOSE << "Weights after dividing by SD" << std::endl << weights;
        LOG_VERBOSE << "New SD" << std::endl << matrixSD(weights);

        return weights.transpose();
    }

private:
    const size_t m_NumNonRetinatopicInputs;
    const size_t m_NonRetinatopicInputPixels;
    const size_t m_NumNonRetinatopicRows;
    const std::vector<FloatType> m_ZeroNonRetinatopicInput;

    size_t m_SnapshotCount = 0;
    FloatType m_LearningRate;
    MatrixType m_Weights;
    VectorType m_U, m_Y;

    mutable cv::Mat m_FusedImage;

    static auto getFloatVector(const cv::Mat &image)
    {
        Eigen::Map<Eigen::Matrix<uint8_t, Eigen::Dynamic, 1>> map(image.data, image.cols * image.rows);
        return map.cast<FloatType>() / 255.0;
    }

    template<class T>
    static auto matrixSD(const T &mat)
    {
        return (mat.array() * mat.array()).rowwise().mean();
    }
}; // InfoMax

//------------------------------------------------------------------------
// BoBRobotics::Navigation::InfoMaxRotater
//------------------------------------------------------------------------
template<typename Rotater = InSilicoRotater, typename FloatType = float>
class InfoMaxRotater : public InfoMax<FloatType>
{
    using MatrixType = Eigen::Matrix<FloatType, Eigen::Dynamic, Eigen::Dynamic>;

public:
    InfoMaxRotater(const cv::Size &unwrapRes,
                   const MatrixType &initialWeights,
                   FloatType learningRate = 0.0001)
    :   InfoMax<FloatType>(unwrapRes, initialWeights, learningRate)
    {}

    InfoMaxRotater(const cv::Size &unwrapRes, FloatType learningRate = 0.0001)
    :   InfoMax<FloatType>(unwrapRes, learningRate)
    {}

    //------------------------------------------------------------------------
    // Public API
    //------------------------------------------------------------------------
    template<class... Ts>
    const std::vector<FloatType> &getImageDifferences(Ts &&... args) const
    {
        auto rotater = Rotater::create(this->getUnwrapResolution(), this->getMaskImage(), std::forward<Ts>(args)...);
        calcImageDifferences(rotater);
        return m_RotatedDifferences;
    }

    template<class... Ts>
    auto getHeading(Ts &&... args) const
    {
        using radian_t = units::angle::radian_t;

        const cv::Size unwrapRes = this->getUnwrapResolution();
        auto rotater = Rotater::create(unwrapRes, this->getMaskImage(), std::forward<Ts>(args)...);
        calcImageDifferences(rotater);

        // Find index of lowest difference
        const auto el = std::min_element(m_RotatedDifferences.cbegin(), m_RotatedDifferences.cend());
        const size_t bestIndex = std::distance(m_RotatedDifferences.cbegin(), el);

        // Convert this to an angle
        radian_t heading = rotater.columnToHeading(bestIndex);
        while (heading <= -180_deg) {
            heading += 360_deg;
        }
        while (heading > 180_deg) {
            heading -= 360_deg;
        }

        return std::make_tuple(heading, *el, std::cref(m_RotatedDifferences));
    }

private:
    //------------------------------------------------------------------------
    // Private API
    //------------------------------------------------------------------------
    template<typename R>
    void calcImageDifferences(R &rotater) const
    {
        // Ensure there's enough space in rotated differe
        m_RotatedDifferences.reserve(rotater.numRotations());
        m_RotatedDifferences.clear();

        // Populate rotated differences with results
        rotater.rotate([this] (const cv::Mat &image, auto, auto) {
            m_RotatedDifferences.push_back(this->test(image));
        });
    }

    //------------------------------------------------------------------------
    // Members
    //------------------------------------------------------------------------
    mutable std::vector<FloatType> m_RotatedDifferences;
};
} // Navigation
} // BoBRobotics
