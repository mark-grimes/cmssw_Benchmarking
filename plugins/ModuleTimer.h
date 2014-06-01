#ifndef markstools_services_ModuleTimer_h
#define markstools_services_ModuleTimer_h

namespace edm
{
	class ActivityRegistry;
	class Event;
	class EventSetup;
	class ParameterSet;
	class ModuleDescription;
	class EventID;
	class Timestamp;
}

namespace markstools
{
	namespace services
	{
		/** @brief CMSSW service that times the execution of modules
		 *
		 * @author Mark Grimes (mark.grimes@bristol.ac.uk)
		 * @date 31/May/2014
		 */
		class ModuleTimer
		{
		public:
			ModuleTimer( const edm::ParameterSet& parameterSet, edm::ActivityRegistry& activityRegister );
			virtual ~ModuleTimer();
			ModuleTimer( const ModuleTimer& otherModuleTimer ) = delete;
			ModuleTimer& operator=( const ModuleTimer& otherModuleTimer ) = delete;
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

			void preProcessEvent( const edm::EventID& eventID, const edm::Timestamp& timeStamp );
			void postProcessEvent( const edm::Event& event, const edm::EventSetup& eventSetup );

			void preModuleEndLumi( const edm::ModuleDescription& description );
			void postModuleEndLumi( const edm::ModuleDescription& description );

			void preModuleEndRun( const edm::ModuleDescription& description );
			void postModuleEndRun( const edm::ModuleDescription& description );

			void preModuleEndJob( const edm::ModuleDescription& description );
			void postModuleEndJob( const edm::ModuleDescription& description );
		private:
			/// @brief Hide all the private members in a pimple. Google "pimple idiom" for details.
			class ModuleTimerPimple* pImple_;
		}; // end of class ModuleTimer

	} // end of namespace services
} // end of namespace markstools

#endif // end of #ifndef markstools_services_ModuleTimer_h
