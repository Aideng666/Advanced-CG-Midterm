#pragma once

#include "PostEffect.h"

class BloomEffect : public PostEffect
{
public:
	//Initializes framebuffer
	//Overrides post effect Init
	void Init(unsigned width, unsigned height) override;

	//Applies the effect to this buffer
	//passes the previous framebuffer with the texture to apply as parameter
	void ApplyEffect(PostEffect* buffer) override;

	void Reshape(unsigned width, unsigned height) override;

	//Getters
	float GetDownscale() const;
	float GetThreshold() const;
	unsigned GetPasses() const;

	//Setters
	void SetDownscale(float downscale);
	void SetThreshold(float threshold);
	void SetPasses(unsigned passes);
private:
	float _downscale = 2.0f;
	float _threshold = 0.5f;
	unsigned _passes = 10;
	glm::vec2 _pixelSize;
};
