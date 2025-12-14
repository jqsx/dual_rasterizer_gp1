// ---------------------------------------------
// --- GRAPHICS PROGRAMMING COURSE 2025-2026 ---
// ---------------------------------------------
// Authors: Matthieu Delaere
// ---------------------------------------------
#ifndef LEAK_DETECTOR_HEADER
#define LEAK_DETECTOR_HEADER

//--- Windows specific includes for memory leak detection to get additional information ---
#if (defined(_WIN32) || defined(_WIN64)) && defined(_DEBUG)
    #define _CRTDBG_MAP_ALLOC
	#define NOMINMAX
    #include <Windows.h>
    #include <crtdbg.h>

    // Redefine C allocators to debug versions with file/line info.
    #define malloc(s) _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
    #define calloc(c, s) _calloc_dbg(c, s, _NORMAL_BLOCK, __FILE__, __LINE__)
    #define realloc(p, s) _realloc_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
    #define free(p) _free_dbg(p, _NORMAL_BLOCK)
#endif

namespace dae
{
    //--- Class ----
    class LeakDetector final
    {
    public:
        //--- Constructors & Destructor ---
        LeakDetector();
        ~LeakDetector() = default;
        LeakDetector(const LeakDetector&) = default;
        LeakDetector& operator=(const LeakDetector&) = default;
        LeakDetector(LeakDetector&&) = default;
        LeakDetector& operator=(LeakDetector&&) = default;

        //--- Functions ---
        static void BreakOnAllocationId(const int id);
        static void CheckForLeaks();
    };
}
#endif //LEAK_DETECTOR_HEADER