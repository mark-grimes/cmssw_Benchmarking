#ifndef markstools_services_MemoryCounter_h
#define markstools_services_MemoryCounter_h

namespace edm
{
	class ActivityRegistry;
	class Event;
	class EventSetup;
	class ParameterSet;
	class ModuleDescription;
}

namespace markstools
{
	namespace services
	{
		/** @brief CMSSW service that counts the size of memory allocations in between module processing.
		 *
		 * For it to work cmsRunGlibC must be invoked with intrusiveMemoryAnalyser, in a similar way to the igprof service.
		 * The code in intrusiveMemoryAnalyser is heavily based on igprof, just with everything except memory counting
		 * stripped out.  I was finding the igprof output to be far too much to store, let alone analyse.
		 *
		 * Note that it doesn't work with jemalloc, so you have to use "cmsRunGlibC" instead of "cmsRun".
		 *
		 * @author Mark Grimes (mark.grimes@bristol.ac.uk)
		 * @date 30/Jul/2011
		 */
		class MemoryCounter
		{
		public:
			MemoryCounter( const edm::ParameterSet& parameterSet, edm::ActivityRegistry& activityRegister );
			virtual ~MemoryCounter();
		private:
			/// @brief Private copy constructor because there should only ever be one instance.
			MemoryCounter( const MemoryCounter& otherMemoryCounter );

			/// @brief Private assignment operator because there should only ever be one instance.
			MemoryCounter& operator=( const MemoryCounter& otherMemoryCounter );
		protected:
			void preModuleConstruction( const edm::ModuleDescription& description );
			void postModuleConstruction( const edm::ModuleDescription& description );

			void preModuleBeginJob( const edm::ModuleDescription& description );
			void postModuleBeginJob( const edm::ModuleDescription& description );

			void preModuleBeginRun( const edm::ModuleDescription& description );
			void postModuleBeginRun( const edm::ModuleDescription& description );

			void preModuleBeginLumi( const edm::ModuleDescription& description );
			void postModuleBeginLumi( const edm::ModuleDescription& description );

			void preModule( const edm::ModuleDescription& description );
			void postModule( const edm::ModuleDescription& description );

			void postProcessEvent( const edm::Event& event, const edm::EventSetup& eventSetup );

			void preModuleEndLumi( const edm::ModuleDescription& description );
			void postModuleEndLumi( const edm::ModuleDescription& description );

			void preModuleEndRun( const edm::ModuleDescription& description );
			void postModuleEndRun( const edm::ModuleDescription& description );

			void preModuleEndJob( const edm::ModuleDescription& description );
			void postModuleEndJob( const edm::ModuleDescription& description );
		private:
			/// @brief Hide all the private members in a pimple. Google "pimple idiom" for details.
			class MemoryCounterPimple* pImple_;
		}; // end of class MemoryCounter

	} // end of namespace services
} // end of namespace markstools

#endif // end of #ifndef markstools_services_MemoryCounter_h
