#pragma once

#include <memory>
#include "mmu.h"

class PPU
{
public:
	PPU(MMU& mmu);

private:
	MMU& mmu;
};