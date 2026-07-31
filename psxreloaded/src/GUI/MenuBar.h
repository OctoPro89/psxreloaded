#pragma once

class MenuBar
{
public:
	struct FrameStats
	{
		double accumulator = 0.0;
		unsigned int frames = 0;

		double fps = 0.0;
		double frameTimeMS = 0.0;
	};

	static bool Update(bool s_mainMenuBarVisible);
};