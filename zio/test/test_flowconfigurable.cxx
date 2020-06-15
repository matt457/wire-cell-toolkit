/**
 * IFrame -> ITensorset -> zio::Message -> ITensorset
 */
#include "WireCellUtil/Testing.h"
#include "WireCellZio/FlowConfigurable.h"
#include "WireCellZio/TensUtil.h"
#include "WireCellIface/SimpleTrace.h"
#include "WireCellIface/SimpleFrame.h"
#include "WireCellAux/SimpleTensor.h"
#include "WireCellAux/TaggedFrameTensorSet.h"

using namespace std;
using namespace WireCell;

ITensorSet::pointer make_tensorset() {
    const double tick = 0.5*units::us;
    // "Signal"
    const int signal_frame_ident = 100;
    const double signal_start_time = 6*units::ms;
    vector<float> signal{0.0,1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0,0.0};

    ITrace::vector signal_traces{
        make_shared<SimpleTrace>(1, 10, signal),
        make_shared<SimpleTrace>(1, 20, signal),
        make_shared<SimpleTrace>(2, 30, signal),
        make_shared<SimpleTrace>(2, 33, signal)};
    auto iframe1 = make_shared<SimpleFrame>(signal_frame_ident,
                                       signal_start_time,
                                       signal_traces,
                                       tick);

    ITensorSet::pointer itensorset=nullptr;
    Aux::TaggedFrameTensorSet tfts;
    auto cfg = tfts.default_configuration();
    cfg["tensors"][0]["tag"] = "";
    tfts.configure(cfg);
    bool ok = tfts(iframe1, itensorset);
    Assert(ok);

    return itensorset;
}

void print(const ITensor::pointer tens) {
    float* data = (float*) tens->data();
    for(size_t x1=0; x1<tens->shape()[0]; ++x1) {
        std::cout << "[" << x1 << "]: ";
        for(size_t x2=0; x2<tens->shape()[1]; ++x2) {
            std::cout << data[x2*tens->shape()[0]+x1] << " ";
        }
        std::cout << std::endl;
    }
}

int main() {

    auto input = make_tensorset();
    
    std::cout << "\n======== input ITensorSet ========\n";
    Json::FastWriter jwriter;
    std::cout << jwriter.write(input->metadata());
    print(input->tensors()->front());

    std::cout << "\n======== zio::Message ========\n";
    auto msg = Zio::pack(input);
    std::cout << msg.label() << std::endl;

    std::cout << "\n======== output ITensorSet ========\n";
    auto out = Zio::unpack(msg);
    std::cout << jwriter.write(out->metadata());
    print(out->tensors()->front());

    return 0;
}
