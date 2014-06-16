#ifndef RASTER_PROFILER_H
#define RASTER_PROFILER_H

#ifndef RASTER_DO_PROFILE
#define RASTER_DO_PROFILE 1
#endif


namespace Profiler {
#if RASTER_DO_PROFILE
	void start(const char *msg);
	void stop(const char *msg);
	void print();
	class Profiler {
		public:
			Profiler(const char *msg) : msg(msg) { start(msg); };
			~Profiler() { stop(msg); };
			const char *msg;
	};
#else
	inline void start(const char *) {};
	inline void stop(const char *) {};
	inline void print() {};
	class Profiler {
		public:
			Profiler(const char *msg) {};
	};
#endif
}

#endif