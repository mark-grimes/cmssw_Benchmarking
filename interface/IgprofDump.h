#ifndef markstools_services_IgprofDump_h
#define markstools_services_IgprofDump_h

#include <string>

//
// Forward declarations
//
namespace edm
{
	class ActivityRegistry;
	class ParameterSet;
}

namespace markstools
{
	namespace services
	{
		/** @brief CMSSW service that prompts igprof to dump a profile at certain points in the run
		 *
		 * This tool works by using igprofs "-D" option, where you can tell igprof to watch a file
		 * and dump every time it gets updated. This service takes the filename of this file as a
		 * configuration parameter and uses 'touch' on it to make igprof dump, then moves the dump
		 * file to a unique filename so that the next dump doesn't overwrite it.
		 *
		 * @author Mark Grimes (mark.grimes@bristol.ac.uk)
		 * @date 01/Jun/2015
		 */
		class IgprofDump
		{
		public:
			IgprofDump( const edm::ParameterSet& parameterSet, edm::ActivityRegistry& activityRegister );
			virtual ~IgprofDump();

			/// @brief Instructs Igprof to dump now. Optional dumpNameSuffix is added to the end of the dump filename
			void dumpNow( const std::string dumpNameSuffix="" );

			// Since this is a service, there should be no way to get an instance
			// except through the framework.
			IgprofDump( const IgprofDump& otherIgprofDump ) = delete;
			IgprofDump( const IgprofDump&& otherIgprofDump ) = delete;
			IgprofDump& operator=( const IgprofDump& otherIgprofDump ) = delete;
			IgprofDump& operator=( const IgprofDump&& otherIgprofDump ) = delete;
		private:
			/// @brief Hide all the private members in a pimple. Google "pimple idiom" for details.
			struct IgprofDumpPimple* pImple_;
		}; // end of class IgprofDump

	} // end of namespace services
} // end of namespace markstools

#endif // end of #ifndef markstools_services_IgprofDump_h
