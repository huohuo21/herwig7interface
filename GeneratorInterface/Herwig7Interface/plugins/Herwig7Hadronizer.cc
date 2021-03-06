#include <memory>
#include <sstream>

#include <HepMC/GenEvent.h>
#include <HepMC/IO_BaseClass.h>

#include <ThePEG/Repository/Repository.h>
#include <ThePEG/EventRecord/Event.h>
#include <ThePEG/Config/ThePEG.h>
#include <ThePEG/LesHouches/LesHouchesReader.h>

#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "SimDataFormats/GeneratorProducts/interface/HepMCProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/GenEventInfoProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/GenRunInfoProduct.h"

#include "GeneratorInterface/Core/interface/BaseHadronizer.h"
#include "GeneratorInterface/Core/interface/GeneratorFilter.h"
#include "GeneratorInterface/Core/interface/HadronizerFilter.h"

#include "GeneratorInterface/LHEInterface/interface/LHEProxy.h"

#include "GeneratorInterface/Herwig7Interface/interface/Herwig7Interface.h"

namespace CLHEP {
  class HepRandomEngine;
}

class Herwig7Hadronizer : public Herwig7Interface, public gen::BaseHadronizer {
    public:
	Herwig7Hadronizer(const edm::ParameterSet &params);
	virtual ~Herwig7Hadronizer();

	bool readSettings( int ) { return true; }
	bool initializeForInternalPartons();
	bool initializeForExternalPartons();
	bool declareStableParticles(const std::vector<int> &pdgIds);
	bool declareSpecialSettings( const std::vector<std::string> ) { return true; }

	void statistics();

	bool generatePartonsAndHadronize();
	bool hadronize();
	bool decay();
	bool residualDecay();
	void finalizeEvent();

	const char *classname() const { return "Herwig7Hadronizer"; }

    private:

        virtual void doSetRandomEngine(CLHEP::HepRandomEngine* v) override { setPEGRandomEngine(v); }

	unsigned int			eventsToPrint;

	ThePEG::EventPtr		thepegEvent;
	
	boost::shared_ptr<lhef::LHEProxy> proxy_;
	const std::string		handlerDirectory_;
};

Herwig7Hadronizer::Herwig7Hadronizer(const edm::ParameterSet &pset) :
	Herwig7Interface(pset),
	BaseHadronizer(pset),
	eventsToPrint(pset.getUntrackedParameter<unsigned int>("eventsToPrint", 0)),
	handlerDirectory_(pset.getParameter<std::string>("eventHandlers"))
{  
	initRepository(pset);

}

Herwig7Hadronizer::~Herwig7Hadronizer()
{
}

bool Herwig7Hadronizer::initializeForInternalPartons()
{
	if (!initGenerator())
	{
		edm::LogInfo("Generator|Herwig7Hadronizer") << "No run step for Herwig chosen. Program will be aborted.";
		exit(0);
	}
	return true;
}

bool Herwig7Hadronizer::initializeForExternalPartons()
{
	edm::LogError("Herwig7 interface") << "Read in of LHE files is not supported in this way. You can read them manually if necessary.";
	return false;
}

bool Herwig7Hadronizer::declareStableParticles(const std::vector<int> &pdgIds)
{
	return false;
}

void Herwig7Hadronizer::statistics()
{
	runInfo().setInternalXSec(GenRunInfoProduct::XSec(
		eg_->integratedXSec() / ThePEG::picobarn,
		eg_->integratedXSecErr() / ThePEG::picobarn));
}

bool Herwig7Hadronizer::generatePartonsAndHadronize()
{
	edm::LogInfo("Generator|Herwig7Hadronizer") << "Start production";

	flushRandomNumberGenerator();

        try {
                thepegEvent = eg_->shoot();
        } catch (std::exception& exc) {
                edm::LogWarning("Generator|Herwig7Hadronizer") << "EGPtr::shoot() thrown an exception, event skipped: " << exc.what();
                return false;
        } catch (...) {
                edm::LogWarning("Generator|Herwig7Hadronizer") << "EGPtr::shoot() thrown an unknown exception, event skipped";
                return false;
        }        
        
	if (!thepegEvent) {
		edm::LogWarning("Generator|Herwig7Hadronizer") << "thepegEvent not initialized";
		return false;
	}

	event() = convert(thepegEvent);
	if (!event().get()) {
		edm::LogWarning("Generator|Herwig7Hadronizer") << "genEvent not initialized";
		return false;
	}

	return true;
}

bool Herwig7Hadronizer::hadronize()
{

	edm::LogError("Herwig7 interface") << "Read in of LHE files is not supported in this way. You can read them manually if necessary.";
	return false;
}

void Herwig7Hadronizer::finalizeEvent()
{
	eventInfo().reset(new GenEventInfoProduct(event().get()));
	eventInfo()->setBinningValues(
			std::vector<double>(1, pthat(thepegEvent)));

	if (eventsToPrint) {
		eventsToPrint--;
		event()->print();
	}

	if (iobc_.get())
		iobc_->write_event(event().get());

	edm::LogInfo("Generator|Herwig7Hadronizer") << "Event produced";
}

bool Herwig7Hadronizer::decay()
{
	return true;
}

bool Herwig7Hadronizer::residualDecay()
{
	return true;
}

#include "GeneratorInterface/ExternalDecays/interface/ExternalDecayDriver.h"

typedef edm::GeneratorFilter<Herwig7Hadronizer, gen::ExternalDecayDriver> Herwig7GeneratorFilter;
DEFINE_FWK_MODULE(Herwig7GeneratorFilter);

typedef edm::HadronizerFilter<Herwig7Hadronizer, gen::ExternalDecayDriver> Herwig7HadronizerFilter;
DEFINE_FWK_MODULE(Herwig7HadronizerFilter);
