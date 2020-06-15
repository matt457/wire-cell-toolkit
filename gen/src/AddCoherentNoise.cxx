#include "WireCellGen/AddCoherentNoise.h"

#include "WireCellIface/SimpleTrace.h"
#include "WireCellIface/SimpleFrame.h"

#include "WireCellUtil/Persist.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/FFTBestLength.h"

#include "Noise.h"

#include <iostream>

WIRECELL_FACTORY(AddCoherentNoise, WireCell::Gen::AddCoherentNoise,
                 WireCell::IFrameFilter, WireCell::IConfigurable)

using namespace std;
using namespace WireCell;

Gen::AddCoherentNoise::AddCoherentNoise(const std::string& spectra_file, const std::string& rng)
    : m_spectra_file(spectra_file)
    , m_rng_tn(rng)
    , m_nsamples(4096)
    , m_fluctuation(0.1) // Fluctuation randomization
    , m_period(0.4)
    , log(Log::logger("sim"))
{

}

Gen::AddCoherentNoise::~AddCoherentNoise()
{
}


void Gen::AddCoherentNoise::gen_elec_resp_default()
{
  // calculate the frequencies ...
  m_elec_resp_freq.resize(m_fft_length,0);
  for (unsigned int i=0;i!=m_elec_resp_freq.size();i++)
  {
    if (i<=m_elec_resp_freq.size()/2.){
      m_elec_resp_freq.at(i) = i / (m_elec_resp_freq.size() *1.0) * 1./m_period ; // the second half is useless ...
    }else{
      m_elec_resp_freq.at(i) = (m_elec_resp_freq.size()-i) / (m_elec_resp_freq.size() *1.0) * 1./m_period ; // the second half is useless ...
    }
  }
}

WireCell::Configuration Gen::AddCoherentNoise::default_configuration() const
{
    Configuration cfg;

    cfg["spectra_file"] = m_spectra_file;
    cfg["rng"] = m_rng_tn;
    cfg["nsamples"] = m_nsamples;
    cfg["random_fluctuation_amplitude"] = m_fluctuation;
    cfg["period"] = m_period;
    cfg["normalization"] = m_normalization;

    return cfg;
}

void Gen::AddCoherentNoise::configure(const WireCell::Configuration& cfg)
{

    m_rng_tn = get(cfg, "rng", m_rng_tn);
    m_rng = Factory::find_tn<IRandom>(m_rng_tn);
    m_spectra_file = get(cfg, "spectra_file", m_spectra_file);
    m_nsamples = get<int>(cfg,"nsamples",m_nsamples);
    m_period = get<int>(cfg,"period",m_period);
    m_fluctuation = get<double>(cfg,"random_fluctuation_amplitude",m_fluctuation);
    m_normalization = get<int>(cfg,"normalization",m_normalization);

    m_fft_length = fft_best_length(m_nsamples);
    gen_elec_resp_default();

    auto jdat = Persist::load(m_spectra_file);
    const int ngroups = jdat.size();

    for(int igroup=0; igroup<ngroups; igroup++)
    {
      auto jentry = jdat[igroup];
      int wire_deltas = jentry["wire-delta"].asInt();

      auto jfreqs = jentry["freqs"];
      auto jamps = jentry["amps"];
      const int nfreqs = jfreqs.size();

      std::vector<float> spec(nfreqs);
      for (int ind=0; ind<nfreqs; ++ind){
         spec[ind] = jamps[ind].asFloat();
      }

      m_group_noise[igroup] = make_pair(wire_deltas, spec);
    }

    // TODO: [ascarpel] We assume the spectra are well prepared and compatible
    // with a given detector geometry. Make sure this is the case

}

bool Gen::AddCoherentNoise::operator()(const input_pointer& inframe,
                                                       output_pointer& outframe)
{
    if (!inframe) {
        outframe = nullptr;
        return true;
    }

    ITrace::vector outtraces;

    // Set the iterator to the first entry of the map
    noise_map_t::iterator m_group_noise_it = m_group_noise.begin();
    auto group_noise = (*m_group_noise_it).second;
    int wire_delta = group_noise.first;
    std::vector<float> spec = group_noise.second;
    int ch_count = 0;

    std::vector<float> random_phases(spec.size(), 0);
    std::vector<float> random_amplitudes(spec.size(), 0);

    for (const auto& intrace : *inframe->traces())
    {

      int chid = intrace->channel();

      // It should never happen but in case the spec arrays have differnt
      // lengths it might be useful
      if( random_phases.size() != spec.size() ){
        random_phases.resize(spec.size());
      }

      if( random_amplitudes.size() != spec.size() ){
        random_amplitudes.resize(spec.size());
      }

      // Create the ampls vector and multiply for the phase
      WireCell::Waveform::compseq_t noise_freq(spec.size(),0);

      for (size_t i=0;i<spec.size();i++)
      {
        const double amplitude = spec[i];

        // Define theta and radius of the complex number
        float theta = random_phases[i];
        float rad = amplitude*random_amplitudes[i];

        // Define the complex number
        complex<float> tc( rad*cos(theta), rad*sin(theta) );
        noise_freq[i] = tc;
      }

      Waveform::realseq_t wave = WireCell::Waveform::idft(noise_freq);

      // Add signal (be careful to double counting with the incoherent noise)
      Waveform::increase(wave, intrace->charge());
      auto trace = make_shared<SimpleTrace>(chid, intrace->tbin(), wave);
      outtraces.push_back(trace);

      // Move the couter forward and check if the correlation group is over
      ch_count++;
      if( ch_count == wire_delta ){
        for(size_t i=0; i<spec.size(); i++){
          random_amplitudes[i] = 0.9 + 2*m_fluctuation*m_rng->uniform(0, 1);
          random_phases[i] = m_rng->uniform(0, 2*3.1415926);
        }

        // Move the iterator forward of one. If we crawled over all possible
        // groups, then start from the beginning again
        ++m_group_noise_it;
        if( m_group_noise_it == m_group_noise.end() ){
          m_group_noise_it = m_group_noise.begin();
        }

        // This condition resets also the channel count...
        ch_count=0;

        // ... and assigns new values to spec and wire_delta
        auto group_noise = (*m_group_noise_it).second;
        wire_delta = group_noise.first;
        spec = group_noise.second;
      }

  } // end channels


  outframe = make_shared<SimpleFrame>(inframe->ident(), inframe->time(),
                                                    outtraces, inframe->tick());
  return true;

}
