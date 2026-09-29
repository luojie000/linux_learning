#pragma once

#include<vector>
#include"MemoryRegion.h"
#include"MemoryStatistics.h"

class MemoryParser
{
private:
std::vector<MemoryRegion> regions;
MemoryStatistics statistics;

public:

void savememory();
void showmemory();
void caulatemessages();

};