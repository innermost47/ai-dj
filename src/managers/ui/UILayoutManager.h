#pragma once
#include "ObsidianBase.h"
#include <JuceHeader.h>

class DjIaVstProcessor;
class DjIaVstEditor;
class LeftPanelWrapper;
class RightPanelWrapper;
class MixerPanel;

class TracksContainer : public ObsidianComponent
{
  public:
	TracksContainer(DjIaVstEditor &editor);
	~TracksContainer() = default;

	void resized() override;

  private:
	DjIaVstEditor &editor;
};

class TracksAndFXContainer : public ObsidianComponent
{
  public:
	TracksAndFXContainer(TracksContainer &tracksContainer, RightPanelWrapper &rightPanelWrapper);
	~TracksAndFXContainer() = default;

	void resized() override;

  private:
	TracksContainer &tracksContainer;
	RightPanelWrapper &rightPanelWrapper;
};

class MainContainer : public ObsidianComponent
{
  public:
	MainContainer(TracksAndFXContainer &tracksAndFXContainer, MixerPanel &mixerPanel);
	~MainContainer() = default;

	void resized() override;

  private:
	TracksAndFXContainer &tracksAndFXContainer;
	MixerPanel &mixerPanel;
};

class ConfigContainer : public ObsidianComponent
{
  public:
	ConfigContainer(DjIaVstEditor &editor);
	~ConfigContainer() = default;

	void resized() override;

  private:
	DjIaVstEditor &editor;
};

class LeftContainer : public ObsidianComponent
{
  public:
	LeftContainer(DjIaVstEditor &editor, LeftPanelWrapper &leftPanelWrapper);
	~LeftContainer() = default;

	void resized() override;

  private:
	DjIaVstEditor &editor;
	LeftPanelWrapper &leftPanelWrapper;
	std::unique_ptr<ConfigContainer> configContainer;
};

class UILayoutManager
{
  public:
	explicit UILayoutManager(DjIaVstProcessor &processor, DjIaVstEditor &editor, MixerPanel &mixerPanel);
	~UILayoutManager() = default;

	void resized();

	TracksContainer *getTracksContainer()
	{
		return tracksContainer.get();
	}

	LeftContainer *getLeftContainer()
	{
		return leftContainer.get();
	}

	MainContainer *getMainContainer()
	{
		return mainContainer.get();
	}

	LeftPanelWrapper *getLeftPanelWrapper()
	{
		return leftPanelWrapper.get();
	}

	RightPanelWrapper *getRightPanelWrapper()
	{
		return rightPanelWrapper.get();
	}

  private:
	DjIaVstEditor &editor;
	DjIaVstProcessor &audioProcessor;
	MixerPanel &mixerPanel;
	std::unique_ptr<TracksContainer> tracksContainer;
	std::unique_ptr<RightPanelWrapper> rightPanelWrapper;
	std::unique_ptr<TracksAndFXContainer> tracksAndFXContainer;
	std::unique_ptr<LeftPanelWrapper> leftPanelWrapper;
	std::unique_ptr<LeftContainer> leftContainer;
	std::unique_ptr<MainContainer> mainContainer;
};