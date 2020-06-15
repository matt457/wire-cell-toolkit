// Add coherent noise to the waveform
// mailto:ascarpell@bnl.gov

#ifndef WIRECELL_GEN_ADDCOHERENTNOISE
#define WIRECELL_GEN_ADDCOHERENTNOISE

#include "WireCellIface/IFrameFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IRandom.h"
#include "WireCellIface/IChannelSpectrum.h"
#include "WireCellUtil/Waveform.h"
#include "WireCellUtil/Logging.h"

#include <string>

namespace WireCell {
    namespace Gen {

        class AddCoherentNoise : public IFrameFilter, public IConfigurable {
        public:
            AddCoherentNoise(const std::string& model = "",
                             const std::string& rng="Random");

            virtual ~AddCoherentNoise();

            /// IFrameFilter
            virtual bool operator()(const input_pointer& inframe, output_pointer& outframe);

            /// IConfigurable
            virtual void configure(const WireCell::Configuration& config);
            virtual WireCell::Configuration default_configuration() const;

            void gen_elec_resp_default();
            size_t argclosest(std::vector<float> const& vec, float value);

        private:
              typedef std::map<int, std::pair<int, std::vector<float>> > noise_map_t;

              IRandom::pointer m_rng;

              std::string m_spectra_file, m_rng_tn;
	      int m_nsamples;
              double m_fluctuation;
              double m_period;
              double m_normalization;

              int m_fft_length;

              noise_map_t m_group_noise;

              Waveform::realseq_t m_elec_resp_freq;

              Log::logptr_t log;
	     };
    }
}

#endif
