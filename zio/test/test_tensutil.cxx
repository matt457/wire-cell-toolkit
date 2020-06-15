#include "WireCellZio/TensUtil.h"
#include "WireCellUtil/Testing.h"
#include "zio/tens.hpp"
#include <iostream>
using namespace WireCell;

template<typename TYPE>
void by_type()
{
    std::cout << "by_type<" << zio::tens::type_name(typeid(TYPE)) << sizeof(TYPE) << ">\n";

    TYPE tensor[2][3][4] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23};
    const TYPE* tensor1 = (TYPE*) tensor;
    std::vector<size_t> shape={2,3,4};

    zio::Message msg1(zio::tens::form);

    // Add an initial, unrelated message part just to make sure tens
    // respects the rules that tens parts aren't all parts.
    msg1.add(zio::message_t((char*)nullptr,0));
    assert(msg1.payload().size() == 1);
    zio::tens::append(msg1, tensor1, shape);
    assert(msg1.payload().size() == 2);

    {
        auto lo1 = msg1.label_object();
        lo1["TENS"]["metadata"]["key"] = "value";
        lo1["TENS"]["metadata"]["answer"] = 42;
        lo1["TENS"]["tensors"][0]["metadata"]["extra"] = "hello world";
        msg1.set_label_object(lo1);
    }


    auto itens = Zio::unpack(msg1);

    auto msg2 = Zio::pack(itens);

    std::cout << "msg1: " << msg1.label() << std::endl;

    std::cout << "tens set metadata: " << itens->metadata() << std::endl;
    auto iten = itens->tensors()->at(0);
    std::cout << "\tmetadata: " << iten->metadata() << std::endl;
    std::cout << "\tshape: [" << shape.size() << "] = ";
    for (auto s : shape) {
        std::cout << s << " ";
    }
    std::cout << std::endl;
    std::cout << "\tword: " << iten->element_size() << std::endl;
    std::cout << "\tdtype: " << zio::tens::type_name(iten->element_type()) << std::endl;

    std::cout << "msg2: " << msg2.label() << std::endl;

    // The two have different parts because msg1 has that dummy
    // message added and msg2 only holds TENS parts.  The rest should
    // be identical.
    auto lo1 = msg1.label_object();
    auto lo2 = msg2.label_object();
    Assert (lo2["TENS"]["tensors"][0]["part"].get<int>() == 0);
    lo1["TENS"]["tensors"][0].erase("part");
    lo2["TENS"]["tensors"][0].erase("part");
    Assert(lo1 == lo2);

    zio::tens::append(msg1, tensor1, shape);
    lo1 = msg1.label_object();
    Assert(lo1["TENS"]["tensors"][0]["part"].get<int>() == 1);
    Assert(lo1["TENS"]["tensors"][1]["part"].get<int>() == 2);

    std::cout << "--------\n";
}

int main()
{
    by_type<float>();
    by_type<double>();

    by_type<char>();
    by_type<int>();
    by_type<long>();
    by_type<long long>();
    by_type<unsigned char>();
    by_type<unsigned int>();
    by_type<unsigned long>();
    by_type<unsigned long long>();

    by_type<int8_t>();
    by_type<int16_t>();
    by_type<int32_t>();
    by_type<int64_t>();

    by_type<uint8_t>();
    by_type<uint16_t>();
    by_type<uint32_t>();
    by_type<uint64_t>();

    return 0;
}
