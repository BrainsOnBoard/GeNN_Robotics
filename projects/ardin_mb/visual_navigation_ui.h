#pragma once

// Standard C++ includes
#include <string>
#include <vector>

// Ardin MB includes
#include "opencv_texture.h"

// Forward declarations
class MBMemory;
class MBMemoryArdin;
class MBMemoryHOG;

//----------------------------------------------------------------------------
// VisualNavigationUI
//----------------------------------------------------------------------------
class VisualNavigationUI
{
public:
    //----------------------------------------------------------------------------
    // Declared virtuals
    //----------------------------------------------------------------------------
    virtual void handleUI(){}
    virtual void handleUITraining(){}
    virtual void handleUITesting(){}

    virtual void saveLogs(const std::string &){};
};

//----------------------------------------------------------------------------
// MBUI
//----------------------------------------------------------------------------
#ifndef NO_GENN
class MBUI : public VisualNavigationUI
{
public:
    MBUI(MBMemory &memory, const std::string &filename,
         unsigned int numPN, unsigned int numKC);

    //----------------------------------------------------------------------------
    // VisualNavigationUI virtuals
    //----------------------------------------------------------------------------
    virtual void handleUI() override;
    virtual void handleUITraining() override;
    virtual void handleUITesting() override;

    virtual void saveLogs(const std::string &filename) override;

protected:
    virtual void handleUIMBProperties(){}
    virtual void handleUIClear(){}

    MBMemory &getMemory(){ return m_Memory; }

private:
    //----------------------------------------------------------------------------
    // Members
    //----------------------------------------------------------------------------
    MBMemory &m_Memory;

    const std::string m_Filename;
    const unsigned int m_NumPN;
    const unsigned int m_NumKC;

    // Data for plotting
    std::vector<float> m_UnusedWeightsData;
    std::vector<float> m_ActivePNData;
    std::vector<float> m_ActiveKCData;
    std::vector<float> m_NumENData;
};

//----------------------------------------------------------------------------
// MBArdinUI
//----------------------------------------------------------------------------
class MBArdinUI : public MBUI
{
public:
    MBArdinUI(MBMemoryArdin &memory);
};
#endif  // NO_GENN
