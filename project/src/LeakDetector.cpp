// ---------------------------------------------
// --- GRAPHICS PROGRAMMING COURSE 2025-2026 ---
// ---------------------------------------------
// Authors: Matthieu Delaere
// ---------------------------------------------
#include "LeakDetector.h"
using namespace dae;

#if (defined(_WIN32) || defined(_WIN64)) && defined(_DEBUG)
static const char* g_leaks_report_file = nullptr;
LeakDetector::LeakDetector()
{
	// Enable debug heap allocations and automatic leak check at program exit
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
}

void LeakDetector::BreakOnAllocationId(const int id)
{
	_crtBreakAlloc = id;
}

void LeakDetector::CheckForLeaks()
{
	_CrtDumpMemoryLeaks();
}

#else

LeakDetector::LeakDetector() = default;
void LeakDetector::BreakOnAllocationId(int) {}
void LeakDetector::CheckForLeaks() {}

#endif