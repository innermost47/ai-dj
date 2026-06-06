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

class MainContainer : public ObsidianComponent
{
  public:
	MainContainer(TracksContainer &tracksontainer, MixerPanel &mixerPanel);
	~MainContainer() = default;

	void resized() override;

  private:
	TracksContainer &tracksContainer;
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
		return tracksContainer ? tracksContainer.get() : nullptr;
	}

	LeftContainer *getLeftContainer()
	{
		return leftContainer ? leftContainer.get() : nullptr;
	}

	MainContainer *getMainContainer()
	{
		return mainContainer ? mainContainer.get() : nullptr;
	}

	LeftPanelWrapper *getLeftPanelWrapper()
	{
		return leftPanelWrapper ? leftPanelWrapper.get() : nullptr;
	}

	RightPanelWrapper *getRightPanelWrapper()
	{
		return rightPanelWrapper ? rightPanelWrapper.get() : nullptr;
	}

  private:
	DjIaVstEditor &editor;
	DjIaVstProcessor &audioProcessor;
	MixerPanel &mixerPanel;
	std::unique_ptr<TracksContainer> tracksContainer;
	std::unique_ptr<RightPanelWrapper> rightPanelWrapper;
	std::unique_ptr<LeftPanelWrapper> leftPanelWrapper;
	std::unique_ptr<LeftContainer> leftContainer;
	std::unique_ptr<MainContainer> mainContainer;
};