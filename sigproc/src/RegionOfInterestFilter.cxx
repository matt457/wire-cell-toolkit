#include "WireCellSigProc/RegionOfInterestFilter.h"

#include "WireCellSigProc/Diagnostics.h"

#include "WireCellUtil/Response.h"

#include "WireCellIface/SimpleFrame.h"
#include "WireCellIface/SimpleTrace.h"

#include "WireCellUtil/NamedFactory.h"
// #include "WireCellUtil/ExecMon.h" // debugging

#include "FrameUtils.h"          // fixme: needs to move to somewhere more useful.

#include <unordered_map>
#define PEAK 20
#define ROI 120
#define ROI_ch 10

// testing
#include <iostream>
using namespace std;

WIRECELL_FACTORY(RegionOfInterestFilter, WireCell::SigProc::RegionOfInterestFilter,
                 WireCell::IFrameFilter, WireCell::IConfigurable)

using namespace WireCell;

using namespace WireCell::SigProc;

RegionOfInterestFilter::RegionOfInterestFilter(const std::string& roi_tag, const std::string& old_tag)
: m_roi_tag(roi_tag)
, m_old_tag(old_tag)
, m_frame_tag("sigproc")
, log(Log::logger("sigproc"))
{
}
RegionOfInterestFilter::~RegionOfInterestFilter()
{
}

void RegionOfInterestFilter::configure(const WireCell::Configuration& cfg)
{
  m_roi_tag = get(cfg,"roi_tag",m_roi_tag);   
  m_old_tag = get(cfg,"old_tag",m_old_tag);   
  m_frame_tag = get(cfg,"frame_tag",m_frame_tag); 
}

WireCell::Configuration RegionOfInterestFilter::default_configuration() const
{
    Configuration cfg;    
    cfg["roi_tag"] = m_roi_tag;
    cfg["old_tag"] = m_old_tag;
    cfg["frame_tag"] = m_frame_tag;
    return cfg;
}

static bool ispeak(float x) { return (x <0 or x>0); }
static bool isZero(float x) { return x == 0.0; }

bool RegionOfInterestFilter::operator()(const input_pointer& inframe, output_pointer& outframe)
{
    log->debug("RegionOfInterestFilter: inside operator roi_tag {}, old_tag {}", m_roi_tag, m_old_tag);

    if (!inframe) {             // eos
        outframe = nullptr;
        return true;
    }
    // Convert to OSP cmm indexed by OSB sequential channels, NOT WCT channel ID.
    m_cmm.clear();
    for (auto cm : inframe->masks())
    {  //in->masks give back a map (ChannelMaskMap) done with a string (in name) and on other map (in m) ChannelMasks
        const std::string name = cm.first;
        for (auto m: cm.second)
        {   //m is the map ChannelMasks done with an int (wct_channel_ident) and a BinRangeList
            const int wct_channel_ident = m.first;
            const OspChan& och = m_channel_map[wct_channel_ident]; //from config
            if (och.plane < 0) continue;               // in case user gives us multi apa frame
            m_cmm[name][och.channel] = m.second;  //m.second è la BinRangeList
        }
    }

    IFrame::trace_list_t roi_traces, old_traces;
    ITrace::vector* newtraces = new ITrace::vector; // will become shared_ptr.

    // outframe = inframe;

    auto traces = inframe->traces();
    auto trace_tags = inframe->trace_tags();

    log->debug("RegionOfInterestFilter: num tags {}", trace_tags.size());

    for (auto ttag : trace_tags)
        log->debug("RegionOfInterestFilter: tag {}", ttag);

    int num_channels = traces->size(); // number of channels
    int ch_ind = 0; // channel indexer

    // auto ex_trace = traces->at(0);
    // auto ex_charges = ex_trace->charge();
    // int charges_size = charges.size();
    int charges_size = traces->at(0)->charge().size();
    int num_store_ch[charges_size] = {}; // number of upper channels to store ROI info per bin (initialized to 0)

    for (auto trace : *traces.get())
    {
        int channel = trace->channel();
        int tbin = trace->tbin();
        auto const& charges = trace->charge();

        ITrace::ChargeSequence newcharge(charges.size(), 0.0);

        auto chargessort = charges;

        std::nth_element(chargessort.begin(), chargessort.begin() + chargessort.size()/2, chargessort.end());
        float median = chargessort[chargessort.size()/2];

        log->debug("RegionOfInterestFilter: channel {}, initial time {}, size {}", channel, tbin, (int)charges.size());  

        // int peak_flag=0;
        // int region_end=-1;

        for (int bin = 0; bin < (int)charges.size(); ++bin)
        {
          //  float left_tail = 0.0;
          //  float right_tail = 0.0;
          //  float tail = 0.0;
          //  float central_value = 0.0;
          //  int ntail = 0;
          //  if (bin>49)
          //  {
          //      for (int i = 0; i < 20; ++i)
          //      {
          //          left_tail += charges[bin-49+i]/20.; 
          //      }
          //      tail += left_tail;
          //      ntail++;
          //  }   
          //  if (bin < ((int)charges.size()-49))
          //  {              
          //      for (int i = 0; i < 20; ++i)
          //      {
          //          right_tail += charges[bin+49-i]/20.; 
          //      }
          //      tail += right_tail;
          //      ntail++;
          // } 
          // tail = tail/ntail;
          // central_value = charges[bin]- tail;
          // log->debug("RegionOfInterestFilter: carica nel bin {} = {}, tail({}. {}, {}), Cvalue {}, median {}", bin, charges[bin], left_tail, right_tail, tail, central_value, median );  

          float central_value = charges[bin]- median;
          // log->debug("RegionOfInterestFilter: carica nel bin {} = {}, median {}, Cvalue {}", bin, charges[bin], median, central_value );  

          if(central_value<-PEAK or central_value>PEAK)
          {

            log->debug("RegionOfInterestFilter: peak in the bin {} = {}, median {}, Cvalue {}, ispeak {}", bin, charges[bin], median, central_value, ispeak(charges[bin]) );
          	for(int delta = -ROI; delta < ROI; ++delta)
          	{
          	    int newbin = bin+delta;
          	    if(newbin>-1 and newbin<(int)charges.size())
          		    newcharge.at(newbin) = charges[newbin]- median;

          	}

            // ITrace::ChargeSequence reducedcharge;
            // if(peak_flag == 0 and region_end < bin)
            // {
            //   reduced_bin=0;
            //   for(int delta = -30; delta < 1; ++delta)
            //   {
            //     int newbin = bin+delta;
            //     if(newbin>-1 and newbin > region_end)
            //       reducedcharge.push_back(charges[newbin]- median);
            //   }
            //   peak_flag=1;
            // }
            // else
            // {
            //   reducedcharge.push_back(charges[bin]- median);
            //   peak_flag=1;
            // }
          }
          // else
          // {
          //   if(peak_flag==1)
          //   {
          //     region_end = bin + 30;
          //     peak_flag = 0;
          //   }

          //   if(region_end > bin) reducedcharge.push_back(charges[bin]- median);

          // }

        }

        // Channel ROI
        if(ch_ind>0)
        {
          auto prev_newtrace = newtraces->at(ch_ind-1); // newtrace at channel-1 (after ROI in time)
          auto prev_newcharges = prev_newtrace->charge();
          auto prev_trace = traces->at(ch_ind-1); // trace at channel-1 (before ROI in time)
          auto prev_charges = prev_trace->charge();

          for(int bin=0; bin<(int)newcharge.size(); bin++)
          {
            // Lower channel ROI
            // non-zero in channel and zero in channel-1 (start of track)
            if(ispeak(newcharge.at(bin)) and isZero(prev_newcharges.at(bin)))
            {
              // fill lower channel ROI
              for(int j=-ROI_ch; j<0; j++)
              {
                int update_channel = ch_ind+j;
                if(update_channel>-1)
                {
                  // fill ROI
                  auto update_trace = newtraces->at(update_channel);
                  auto update_charges = update_trace->charge();
                  update_charges.at(bin) = prev_charges[bin] - median; // note this is the median from curr_channel, not update_channel
                } 
              }
            }

            // Upper channel ROI
            // zero in channel and non-zero in channel-1 (end of track)
            if(num_store_ch[bin]==0 and isZero(newcharge.at(bin)) and ispeak(prev_newcharges.at(bin)))
            {
              // fill upper channel ROI
              newcharge.at(bin) = prev_charges[bin] - median;
              // set num_store_ch
              num_store_ch[bin] = ROI_ch;
            }
            else if(num_store_ch>0 and isZero(newcharge.at(bin)) and ispeak(prev_newcharges.at(bin)))
            {
              // fill upper channel ROI
              newcharge.at(bin) = prev_charges[bin] - median;
            }

            // iterate num_store_ch
            if(num_store_ch[bin]>1)
              num_store_ch[bin] -= 1; // >0 means store that many more channels in ROI
            else if(num_store_ch[bin]==1)
              num_store_ch[bin] -= -1; // -1 means end of upper ROI
            else if(num_store_ch[bin]==-1)
              num_store_ch[bin] -= 0; // 0 means ready to find next end of track 

          }
        }
        

        // std::vector<float>::const_iterator beg=newcharge.begin(), end=newcharge.end();
        // auto i1 = std::find_if(beg, end, ispeak); // first start

        // log->debug("RegionOfInterestFilter: inizio striscia bin {} = {}, inizio vettore {}, fine {}, size {}", i1, *i1, beg, end, newcharge.size() );
        // log->debug("RegionOfInterestFilter: begin of the stripe bin {}, size {}", *i1, newcharge.size() );

        ///////////////TEMPORARY COMMENTED////////////////////
        // while (i1 != end)
        // {
        //   // stop at next zero or end and make little temp vector
        //   auto i2 = std::find_if(i1, end, isZero);

        //   const std::vector<float> q(i1,i2);


        //   for (int pbin = 0; pbin < (int)q.size(); ++pbin)
        //   {
        //     log->debug("RegionOfInterestFilter: q bin = {}, newtbin = {}", pbin, q[pbin]);
        //   }
        //   // save out
        //   const int newtbin = i1 - beg;
        //   SimpleTrace *tracetemp = new SimpleTrace(channel, newtbin, q);

        //   log->debug("RegionOfInterestFilter: end stripe bin = {}, newtbin = {}", *i2, newtbin);

        //   const size_t roi_trace_index = newtraces->size();
        //   roi_traces.push_back(roi_trace_index);
        //   newtraces->push_back(ITrace::pointer(tracetemp));
        //   // find start for next loop
        //   i1 = std::find_if(i2, end, ispeak);

        //   log->debug("RegionOfInterestFilter: begin new stripe bin = {}", *i1 );
        // }
        //////////////////////////////////////
        //////////TO comment if is uncommented the part over this
        SimpleTrace *tracetemp = new SimpleTrace(channel, tbin, newcharge);
        const size_t roi_trace_index = newtraces->size();
        roi_traces.push_back(roi_trace_index);
        newtraces->push_back(ITrace::pointer(tracetemp));

        
        const size_t old_trace_index = newtraces->size();
        old_traces.push_back(old_trace_index);
        newtraces->push_back(ITrace::pointer(trace));

        ch_ind = ch_ind+1; // iterate channel index
    }


    /*for (auto trace : *traces.get())
    {

    }*/

    SimpleFrame* sframe = new SimpleFrame(inframe->ident(), inframe->time(),
                                        ITrace::shared_vector(newtraces),
                                        inframe->tick(), inframe->masks());
    sframe->tag_frame(m_frame_tag);
    sframe->tag_traces(m_roi_tag, roi_traces);
    sframe->tag_traces(m_old_tag, old_traces);

    outframe = IFrame::pointer(sframe);

    auto checktraces = outframe->traces();

    for (auto checktrace : *checktraces.get())
    {
        int channel = checktrace->channel();
        int tbin = checktrace->tbin();
        auto const& charges = checktrace->charge();

        log->debug("RegionOfInterestFilter: newtraces channel {}, start time {}, size {}", channel, tbin,(int)charges.size());

        for (int bin = 0; bin < (int)charges.size(); ++bin)
        {
          log->debug("RegionOfInterestFilter: newtrace bin {} value = {}", bin, charges[bin] );
        }
    }

    // outframe = IFrame::pointer(sframe);
    log->debug("RegionOfInterestFilter: end operator");

    return true;
}


// Local Variables:
// mode: c++
// c-basic-offset: 4
// End:
