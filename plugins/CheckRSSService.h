#ifndef markstools_services_CheckRSSService_h
#define markstools_services_CheckRSSService_h

#include <string>

namespace edm
{
	class ActivityRegistry;
	class ParameterSet;
	class ModuleDescription;
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
		class CheckRSSService
		{
		public:
			CheckRSSService( const edm::ParameterSet& parameterSet, edm::ActivityRegistry& activityRegister );
			virtual ~CheckRSSService();

			CheckRSSService( const CheckRSSService& otherCheckRSSService ) = delete;
			CheckRSSService& operator=( const CheckRSSService& otherCheckRSSService ) = delete;
		protected:
			size_t eventNumber_;
			size_t runNumber_;
			size_t lumiNumber_;
		}; // end of class CheckRSSService

	} // end of namespace services
} // end of namespace markstools

#endif // end of #ifndef markstools_services_CheckRSSService_h
