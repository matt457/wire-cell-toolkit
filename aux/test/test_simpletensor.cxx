#include "WireCellAux/SimpleTensor.h"
#include "WireCellUtil/Testing.h"
#include "WireCellUtil/Array.h"
#include <boost/multi_array.hpp>
#include <iostream>

using namespace WireCell;


void test_small()
{
    const std::vector<size_t> shape = {3,4,5};
    size_t nele = 1;
    for (auto s : shape) { nele *= s; }
    Aux::SimpleTensor<float> st(shape);
    Assert(st.size() == sizeof(float)*nele);
    auto& d = st.store();
    Assert(d.size() == st.size());
    
    boost::multi_array_ref<float, 3> mar((float*)d.data(), shape);
    Assert(mar.num_elements() == nele);

    for (int i=0; i<3; ++i) {
        for (int j=0; j<4; ++j) {
            for (int k=0; k<5; ++k) {
                mar[i][j][k] = (i+1)*100 + (j+1)*10 + k+1;
            }
        }
    }

    for (size_t ind=0; ind<mar.num_elements(); ++ind) {
        std::cout << ind << " " << mar.data()[ind] << std::endl;
    }
}

void test_large()
{
    const size_t nchans=15360;
    const size_t nticks=6000;

    Aux::SimpleTensor<int>* cht = new Aux::SimpleTensor<int>({nchans});
    Eigen::Map<Eigen::ArrayXi> charr((int*)cht->data(), nchans);
    assert((size_t)charr.size() == nchans);
    charr.setZero();

    Aux::SimpleTensor<double>* sumt = new Aux::SimpleTensor<double>({nchans});
    Eigen::Map<Eigen::ArrayXd> sumarr((double*)sumt->data(), nchans);
    assert((size_t)sumarr.size() == nchans);
    sumarr.setZero();

    const std::vector<size_t> shape = {nchans, nticks};
    size_t nele = 1;
    for (auto s : shape) { nele *= s; }
    Aux::SimpleTensor<float>* st = new Aux::SimpleTensor<float>(shape);
    Eigen::Map<Eigen::ArrayXXf> arr((float*)st->data(), nchans, nticks);
    const double pad = 0.0;
    arr.setConstant(pad);
    Assert(st->size() == sizeof(float)*nele);
    delete st;
    delete sumt;
    delete cht;
}
int main()
{
    test_small();
    test_large();
    return 0;
}
