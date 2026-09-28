#include <JuceHeader.h>
#include "HubEditor.h"
#include <utility>

namespace
{
class HubWindow final : public juce::DocumentWindow
{
public:
    HubWindow(std::function<juce::String()> getPath,
        std::function<void(juce::String)> setPath)
        : juce::DocumentWindow("Arranger Manager", juce::Colour(0xff1d232b),
              juce::DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new HubEditor(std::move(getPath), std::move(setPath)), true);
        setResizable(true, false);
        setResizeLimits(460, 320, 1600, 1200);
        centreWithSize(980, 620);
        setVisible(true);
    }

    void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
};

class HubApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Arranger Manager"; }
    const juce::String getApplicationVersion() override { return "0.0l"; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise(const juce::String&) override
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "Arranger Manager";
        options.folderName = "Moon River Studio";
        options.filenameSuffix = ".settings";
        settings.setStorageParameters(options);
        if (auto* user = settings.getUserSettings())
            projectPath = user->getValue("lastProjectPath");

        window = std::make_unique<HubWindow>(
            [this] { return projectPath; },
            [this](juce::String path)
            {
                projectPath = std::move(path);
                if (auto* user = settings.getUserSettings())
                {
                    user->setValue("lastProjectPath", projectPath);
                    user->saveIfNeeded();
                }
            });
    }

    void shutdown() override
    {
        window.reset();
        settings.saveIfNeeded();
    }

private:
    juce::ApplicationProperties settings;
    juce::String projectPath;
    std::unique_ptr<HubWindow> window;
};
}

START_JUCE_APPLICATION(HubApplication)
